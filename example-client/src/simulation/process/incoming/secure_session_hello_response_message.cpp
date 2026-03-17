#include "simulation/process/incoming/secure_session_hello_response_message.h"

#include "crypto/kx.h"
#include "network/protocol/secure_session.h"

#include <RC2D/RC2D.h>

#include <array>
#include <cstring>
#include <ctime>
#include <mutex>
#include <vector>

// Encode un uint64 en big-endian pour reconstruire le payload signe.
static void ClientSecureSession_WriteU64Be(std::vector<uint8_t>& out, uint64_t value)
{
    out.push_back(static_cast<uint8_t>((value >> 56) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 48) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 40) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 32) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(value & 0xFF));
}

// Reconstruit le message canonique signe cote serveur:
// [domain][server_kx_pub][issued_at_be64][expires_at_be64][client_nonce_echo]
static void ClientSecureSession_BuildSignedMessage(
    const ServerSecureSessionHelloResponsePacketReliable& packet,
    std::vector<uint8_t>& outMessage)
{
    outMessage.clear();
    outMessage.reserve(
        (sizeof(CLIENT_SECURE_SESSION_SIGNING_DOMAIN) - 1) +
        packet.serverPublicKey.size() +
        sizeof(uint64_t) +
        sizeof(uint64_t) +
        packet.clientNonceEcho.size());

    outMessage.insert(
        outMessage.end(),
        CLIENT_SECURE_SESSION_SIGNING_DOMAIN,
        CLIENT_SECURE_SESSION_SIGNING_DOMAIN + (sizeof(CLIENT_SECURE_SESSION_SIGNING_DOMAIN) - 1));

    outMessage.insert(
        outMessage.end(),
        packet.serverPublicKey.begin(),
        packet.serverPublicKey.end());

    ClientSecureSession_WriteU64Be(outMessage, packet.issuedAtUnixSeconds);
    ClientSecureSession_WriteU64Be(outMessage, packet.expiresAtUnixSeconds);

    outMessage.insert(
        outMessage.end(),
        packet.clientNonceEcho.begin(),
        packet.clientNonceEcho.end());
}

// Verifie la signature Ed25519 de l'attestation secure-session avec la cle publique pinnee.
static bool ClientSecureSession_VerifyAttestationSignature(
    const ServerSecureSessionHelloResponsePacketReliable& packet)
{
    std::vector<uint8_t> message;
    ClientSecureSession_BuildSignedMessage(packet, message);

    const int verifyResult = crypto_sign_verify_detached(
        packet.signature.data(),
        message.data(),
        static_cast<unsigned long long>(message.size()),
        CLIENT_SECURE_SESSION_SERVER_ED25519_PUBLIC_KEY.data());

    return verifyResult == 0;
}

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
    if (!ClientSecureSession_VerifyAttestationSignature(packet))
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
