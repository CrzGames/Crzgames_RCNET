#include "simulation/process/incoming/secure_session_hello_message.h"

#include "crypto/kx.h"
#include "network/packets/server/reliable.h"
#include "network/serialization/serialize_packets_server.h"

#include <array>         // std::array
#include <unordered_map> // std::unordered_map
#include <cstdint>       // uint32_t, etc.

#include <sodium.h>

#include <RCNET/RCNET.h>

void ServerSimulation_ProcessNetworkIncomingDispatcher_HandleSecureSessionHelloMessage(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    const NetworkINToSimulationMessage& msg)
{
    // Rechercher la session correspondant à cette connexion.
    std::unordered_map<uint32_t, ClientSession>::iterator sit = networkState.sessions.find(msg.connectionId);

    // Si la session n'existe pas, on ne peut pas traiter le hello sécurisé.
    if (sit == networkState.sessions.end())
    {
        // Log d'avertissement pour signaler une connexion inconnue.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [SIMULATION] [SECURE_SESSION] - Unknown connectionId=%u\n",
                  msg.connectionId);

        // Abandon du traitement.
        return;
    }

    // Référence directe vers la session du client.
    ClientSession& session = sit->second;

    // Sauvegarder la clé publique client dans la session.
    session.clientPublicKey = msg.secureSessionHelloPacket.clientPublicKey;

    // Préparer le buffer de clé RX serveur.
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> serverRxKey{};

    // Préparer le buffer de clé TX serveur.
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> serverTxKey{};

    // Calculer les clés de session serveur à partir
    // de l'état crypto global serveur et de la clé publique client.
    const bool ok = ServerCryptoKx_ComputeSessionKeys(
        networkState.cryptoKxState,
        msg.secureSessionHelloPacket.clientPublicKey,
        serverRxKey,
        serverTxKey);

    // Préparer le packet de réponse envoyé au client.
    ServerSecureSessionHelloResponsePacketReliable responsePacket{};

    // Renseigner le type du packet de réponse.
    responsePacket.header.type = ServerReliablePacketType::SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE;

    // Si le calcul crypto a échoué...
    if (!ok)
    {
        // ... indiquer une clé client invalide.
        responsePacket.status = ServerSecureSessionHelloResponseStatus::INVALID_CLIENT_KEY;
    }
    else
    {
        // Stocker la clé RX serveur dans la session.
        session.serverRxKey = serverRxKey;

        // Stocker la clé TX serveur dans la session.
        session.serverTxKey = serverTxKey;

        // Marquer la session comme sécurisée.
        session.isSecureSessionEstablished = true;

        // Indiquer le succès dans la réponse.
        responsePacket.status = ServerSecureSessionHelloResponseStatus::SUCCESS;

        // Fournir la clé publique serveur au client.
        responsePacket.serverPublicKey = networkState.cryptoKxState.serverPublicKey;
    }

    // Construire le message sortant simulation -> réseau.
    SimulationToNetworkOUTMessage outMsg{};

    // Renseigner le type logique de message sortant.
    outMsg.type = SimulationToNetworkOUTMessageType::SERVER_SECURE_SESSION_HELLO_RESPONSE_PACKET_RELIABLE;

    // Renseigner l'identifiant de connexion cible.
    outMsg.connectionId = msg.connectionId;

    // Sérialiser le packet prêt à être envoyé.
    outMsg.serializedPacket = serializeServerSecureSessionHelloResponsePacketReliable(responsePacket);

    // Pousser le message vers le thread réseau sortant.
    simToNetQueue.push(outMsg);

    // Log du résultat final.
    RCNET_log(RCNET_LOG_INFO,
              "[SERVER] [SIMULATION] [SECURE_SESSION] - connectionId=%u secureSessionEstablished=%u\n",
              msg.connectionId,
              session.isSecureSessionEstablished ? 1u : 0u);
}