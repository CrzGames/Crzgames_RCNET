#include "simulation/process/incoming/secure_session_hello_message.h"

#include "crypto/kx.h"
#include "crypto/signing.h"
#include "network/packets/server/reliable.h"
#include "network/serialization/serialize_packets_server.h"

#include <array>         // std::array
#include <chrono>        // std::chrono
#include <cstdint>       // uint32_t, etc.
#include <mutex>         // std::lock_guard
#include <unordered_map> // std::unordered_map

#include <sodium.h> // sodium_memzero

#include <RCNET/RCNET.h>

void ServerSimulation_ProcessNetworkIncomingDispatcher_HandleSecureSessionHelloMessage(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    const NetworkINToSimulationMessage& msg)
{
    // Etape 1: buffers temporaires pour les cles de session derivees cote serveur.
    // serverRxKey: cle qui servira a dechiffrer ce que le client envoie.
    // serverTxKey: cle qui servira a chiffrer ce que le serveur envoie.
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> serverRxKey{};
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> serverTxKey{};

    bool keyExchangeOk = false;
    bool secureSessionAccepted = false;
    bool secureSessionEstablishedForLog = false;

    // Etape 2: construire la reponse (status par defaut = erreur).
    ServerSecureSessionHelloResponsePacketReliable secureSessionHelloResponsePacket{};
    secureSessionHelloResponsePacket.header.type = ServerReliablePacketType::SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE;
    secureSessionHelloResponsePacket.status = ServerSecureSessionHelloResponseStatus::INVALID_CLIENT_KEY;

    {
        // Lock le mutex de sessions pour acceder a networkState.sessions de facon thread-safe.
        std::lock_guard<std::mutex> lock(networkState.sessionsMutex);

        // Recuperer la session client associee a ce connectionId.
        std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);
        if (sit == networkState.sessions.end())
        {
            RCNET_log(
                RCNET_LOG_WARN,
                "[SERVER] [SIMULATION] [SECURE_SESSION] - connectionId: %u - Session not found",
                msg.connectionId);
            return;
        }

        // On a trouve la session client correspondante, on recupere la reference pour la suite.
        ClientSession& session = sit->second;

        // Etape 3: memoriser la cle publique KX du client pour cette session.
        session.clientPublicKey = msg.secureSessionHelloPacket.clientPublicKey;

        // Etape 4: deriver les cles de session via crypto_kx.
        keyExchangeOk = ServerCryptoKx_ComputeSessionKeys(
            networkState.cryptoKxState,
            msg.secureSessionHelloPacket.clientPublicKey,
            serverRxKey,
            serverTxKey);

        if (keyExchangeOk)
        {
            // Etape 5: construire l'attestation signee de la cle KX serveur.
            // issuedAt/expiresAt servent a limiter la fenetre de validite.
            const uint64_t issuedAtUnixSeconds = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count());
            const uint64_t expiresAtUnixSeconds = issuedAtUnixSeconds + SERVER_SECURE_SESSION_SIGNATURE_TTL_SECONDS;

            secureSessionHelloResponsePacket.status = ServerSecureSessionHelloResponseStatus::SUCCESS;
            secureSessionHelloResponsePacket.serverPublicKey = networkState.cryptoKxState.serverPublicKey;
            secureSessionHelloResponsePacket.issuedAtUnixSeconds = issuedAtUnixSeconds;
            secureSessionHelloResponsePacket.expiresAtUnixSeconds = expiresAtUnixSeconds;

            // Reprendre EXACTEMENT la valeur du champ clientNonce recu:
            // ClientSecureSessionHelloPacketReliable::clientNonce.
            secureSessionHelloResponsePacket.clientNonceEcho = msg.secureSessionHelloPacket.clientNonce;

            // Ce payload est le contenu exact protege par la signature Ed25519.
            ServerCryptoSigningSecureSessionPayload signedPayload{};
            signedPayload.serverKxPublicKey = secureSessionHelloResponsePacket.serverPublicKey;
            signedPayload.issuedAtUnixSeconds = secureSessionHelloResponsePacket.issuedAtUnixSeconds;
            signedPayload.expiresAtUnixSeconds = secureSessionHelloResponsePacket.expiresAtUnixSeconds;
            signedPayload.clientNonceEcho = secureSessionHelloResponsePacket.clientNonceEcho;

            // Etape 6: signer l'attestation.
            const bool signingOk = ServerCryptoSigning_SignSecureSessionPayload(
                networkState.cryptoSigningState,
                signedPayload,
                secureSessionHelloResponsePacket.signature);

            if (signingOk)
            {
                // Etape 7a (succes): stocker les cles derivees dans la session.
                session.serverRxKey = serverRxKey;
                session.serverTxKey = serverTxKey;
                session.isSecureSessionEstablished = true;
                secureSessionAccepted = true;
            }
            else
            {
                // Etape 7b (echec): ne pas activer de session securisee.
                session.isSecureSessionEstablished = false;
                session.serverRxKey.fill(0);
                session.serverTxKey.fill(0);

                secureSessionHelloResponsePacket.status = ServerSecureSessionHelloResponseStatus::SERVER_ATTESTATION_FAILED;

                RCNET_log(
                    RCNET_LOG_ERROR,
                    "[SERVER] [SIMULATION] [SECURE_SESSION] - connectionId: %u - Failed to sign secure-session attestation",
                    msg.connectionId);
            }
        }

        secureSessionEstablishedForLog = session.isSecureSessionEstablished;
    }

    // Nettoyage defensif: effacer les buffers temporaires de cles.
    sodium_memzero(serverRxKey.data(), serverRxKey.size());
    sodium_memzero(serverTxKey.data(), serverTxKey.size());

    // Etape 8: preparer l'envoi reseau de la reponse secure-session.
    SimulationToNetworkOUTMessage outMsg{};

    // Le type de message determine le type de packet reseau a envoyer, et donc la facon de le serializer.
    outMsg.type = SimulationToNetworkOUTMessageType::SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE;

    // Associer la reponse au client via connectionId.
    outMsg.connectionId = msg.connectionId;

    // En cas d'echec (KX ou signature), deconnecter apres ACK de la reponse
    // pour garantir que le client recoit le status explicite.
    outMsg.disconnectAfterAck = !secureSessionAccepted;

    // Activer le chiffrement uniquement apres ACK d'une reponse valide:
    // on evite tout decalage d'etat entre simulation et transport reseau.
    outMsg.enableEncryptionAfterAck = secureSessionAccepted;

    // Serializer la reponse en vue de son envoi reseau.
    outMsg.serializedPacket = serializeServerSecureSessionHelloResponsePacketReliable(secureSessionHelloResponsePacket);

    // Etape 9: envoyer la reponse via la queue de communication simulation -> reseau.
    simToNetQueue.push(outMsg);

    RCNET_log(
        RCNET_LOG_INFO,
        "[SERVER] [SIMULATION] [SECURE_SESSION] - connectionId: %u - Secure-session response queued (secureSessionEstablished: %u, responseStatus: %u)",
        msg.connectionId,
        secureSessionEstablishedForLog ? 1u : 0u,
        static_cast<unsigned>(secureSessionHelloResponsePacket.status));
}
