#include "simulation/process/incoming/secure_session_hello_response_message.h"

#include "crypto/kx.h"
#include "network/protocol/secure_session.h"
#include "network/protocol/secure_session_attestation.h"

#include <RC2D/RC2D.h>

#include <array>
#include <cstring>
#include <ctime>
#include <mutex>

void ClientSimulation_ProcessNetworkIncomingDispatcher_HandleSecureSessionHelloResponseMessage(
    NetworkState& networkState,
    const NetworkINToSimulationMessage& networkInToSimMessage)
{
    // Raccourci vers le packet de reponse secure-session recu du serveur.
    const ServerSecureSessionHelloResponsePacketReliable& packet =
        networkInToSimMessage.secureSessionHelloResponsePacket;

    // Verrouiller l'etat partage (simulation + reseau).
    std::lock_guard<std::mutex> lock(networkState.cryptoMutex);

    // Si le serveur annonce un echec de handshake, on reset l'etat de session.
    if (packet.status != ServerSecureSessionHelloResponseStatus::SUCCESS)
    {
        networkState.secureSessionEstablished = false;
        networkState.authValidated = false;
        networkState.authTokenValidated = false;
        networkState.encryptionEnabled = false;
        networkState.hasPendingSecureSessionHello = false;

        RC2D_log(
            RC2D_LOG_WARN,
            "[CLIENT] [SIMULATION] [SECURE_SESSION] Rejected by server (status=%u).",
            static_cast<unsigned>(packet.status));
        return;
    }

    // Le client doit avoir un hello en attente pour accepter cette reponse.
    if (!networkState.hasPendingSecureSessionHello)
    {
        networkState.secureSessionEstablished = false;
        networkState.encryptionEnabled = false;
        RC2D_log(
            RC2D_LOG_WARN,
            "[CLIENT] [SIMULATION] [SECURE_SESSION] Received response without pending hello.");
        return;
    }

    // Verifier que le nonce echo renvoye par le serveur correspond au nonce
    // envoye dans le hello secure-session.
    if (std::memcmp(
            packet.clientNonceEcho.data(),
            networkState.pendingClientNonce.data(),
            networkState.pendingClientNonce.size()) != 0)
    {
        networkState.secureSessionEstablished = false;
        networkState.encryptionEnabled = false;
        networkState.hasPendingSecureSessionHello = false;
        RC2D_log(
            RC2D_LOG_WARN,
            "[CLIENT] [SIMULATION] [SECURE_SESSION] Nonce mismatch in hello response.");
        return;
    }

    // Verifier la signature Ed25519 de l'attestation secure-session.
    if (!ClientSecureSession_VerifyServerHelloResponseAttestation(packet))
    {
        networkState.secureSessionEstablished = false;
        networkState.encryptionEnabled = false;
        networkState.hasPendingSecureSessionHello = false;
        RC2D_log(
            RC2D_LOG_ERROR,
            "[CLIENT] [SIMULATION] [SECURE_SESSION] Invalid server attestation signature.");
        return;
    }

    // Verifier la coherence de la fenetre temporelle de l'attestation.
    if (packet.expiresAtUnixSeconds < packet.issuedAtUnixSeconds)
    {
        networkState.secureSessionEstablished = false;
        networkState.encryptionEnabled = false;
        networkState.hasPendingSecureSessionHello = false;
        RC2D_log(
            RC2D_LOG_ERROR,
            "[CLIENT] [SIMULATION] [SECURE_SESSION] Invalid attestation time window.");
        return;
    }

    const uint64_t validityWindow = packet.expiresAtUnixSeconds - packet.issuedAtUnixSeconds;
    if (validityWindow > CLIENT_SECURE_SESSION_SIGNATURE_TTL_SECONDS)
    {
        networkState.secureSessionEstablished = false;
        networkState.encryptionEnabled = false;
        networkState.hasPendingSecureSessionHello = false;
        RC2D_log(
            RC2D_LOG_ERROR,
            "[CLIENT] [SIMULATION] [SECURE_SESSION] Attestation TTL too large (window=%llu).",
            static_cast<unsigned long long>(validityWindow));
        return;
    }

    // Refuser une attestation expiree.
    const uint64_t nowUnixSeconds = static_cast<uint64_t>(std::time(nullptr));
    if (nowUnixSeconds > packet.expiresAtUnixSeconds)
    {
        networkState.secureSessionEstablished = false;
        networkState.encryptionEnabled = false;
        networkState.hasPendingSecureSessionHello = false;
        RC2D_log(
            RC2D_LOG_WARN,
            "[CLIENT] [SIMULATION] [SECURE_SESSION] Attestation expired (now=%llu, exp=%llu).",
            static_cast<unsigned long long>(nowUnixSeconds),
            static_cast<unsigned long long>(packet.expiresAtUnixSeconds));
        return;
    }

    // Deriver les cles de session client (rx/tx) avec la cle publique serveur.
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> rxKey{};
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> txKey{};
    if (!ClientCryptoKx_ComputeSessionKeys(
            networkState.cryptoKxState,
            packet.serverPublicKey,
            rxKey,
            txKey))
    {
        networkState.secureSessionEstablished = false;
        networkState.encryptionEnabled = false;
        networkState.hasPendingSecureSessionHello = false;
        RC2D_log(
            RC2D_LOG_ERROR,
            "[CLIENT] [SIMULATION] [SECURE_SESSION] Failed to derive session keys.");
        return;
    }

    // Enregistrer les cles de session pour le transport chiffre.
    networkState.clientRxKey = rxKey;
    networkState.clientTxKey = txKey;

    // Marquer la secure-session comme etablie.
    networkState.secureSessionEstablished = true;

    // Activer le chiffrement ENet pour le peer serveur.
    networkState.encryptionEnabled = true;

    // Le hello en attente est maintenant resolu.
    networkState.hasPendingSecureSessionHello = false;

    RC2D_log(
        RC2D_LOG_INFO,
        "[CLIENT] [SIMULATION] [SECURE_SESSION] Established and packet encryption enabled.");
}
