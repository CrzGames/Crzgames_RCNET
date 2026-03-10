#include "network/transport/encryption/enet_host_encryptor.h"

#include "core/context.h"
#include "network/state.h"
#include "simulation/session/client.h"

#include <array>         // std::array
#include <cstdint>       // uint32_t, uintptr_t
#include <cstring>       // std::memcmp, std::memcpy
#include <mutex>         // std::lock_guard
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

#include <sodium.h>

#include <RCNET/RCNET.h> // RCNET_log

namespace
{
    // Version de format pour les datagrammes applicatifs chiffres.
    // Permet de rejeter proprement un paquet si le format evolue.
    constexpr enet_uint8 kEncryptedPacketMagic[4] = {'R', 'C', 'N', '1'};

    constexpr size_t kEncryptedPacketMagicSize = sizeof(kEncryptedPacketMagic);
    constexpr size_t kEncryptedPacketNonceSize = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
    constexpr size_t kEncryptedPacketTagSize = crypto_aead_xchacha20poly1305_ietf_ABYTES;
    constexpr size_t kEncryptedPacketHeaderSize = kEncryptedPacketMagicSize + kEncryptedPacketNonceSize;

    struct ServerNetworkHostEncryptorContext
    {
        // Aucun etat mutable pour l'instant.
    };

    static bool ServerNetworkEncryption_TryGetConnectionIdFromPeer(ENetPeer* peer, uint32_t& outConnectionId)
    {
        if (peer == nullptr || peer->data == nullptr)
        {
            return false;
        }

        const uint32_t connectionId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(peer->data));
        if (connectionId == 0)
        {
            return false;
        }

        outConnectionId = connectionId;
        return true;
    }

    static bool ServerNetworkEncryption_TryLoadSessionKeyIfEnabled(
        NetworkState& networkState,
        uint32_t connectionId,
        bool forOutgoingEncryption,
        std::array<uint8_t, crypto_kx_SESSIONKEYBYTES>& outKey)
    {
        std::lock_guard<std::mutex> lock(networkState.sessionsMutex);

        std::unordered_map<uint32_t, ClientSession>::const_iterator sit =
            networkState.sessions.find(connectionId);
        if (sit == networkState.sessions.end())
        {
            return false;
        }

        const ClientSession& session = sit->second;
        if (!session.isPacketEncryptionEnabled)
        {
            return false;
        }

        outKey = forOutgoingEncryption ? session.serverTxKey : session.serverRxKey;
        return true;
    }

    static size_t ServerNetworkEncryption_CopyInBuffersToOutData(
        const ENetBuffer* inBuffers,
        size_t inBufferCount,
        size_t inLimit,
        enet_uint8* outData,
        size_t outLimit)
    {
        if (inBuffers == nullptr || outData == nullptr || inLimit > outLimit)
        {
            return 0;
        }

        size_t copied = 0;
        for (size_t i = 0; i < inBufferCount && copied < inLimit; ++i)
        {
            const size_t remaining = inLimit - copied;
            const size_t chunk =
                (inBuffers[i].dataLength < remaining) ? inBuffers[i].dataLength : remaining;

            if (chunk == 0)
            {
                continue;
            }

            if (inBuffers[i].data == nullptr)
            {
                return 0;
            }

            std::memcpy(outData + copied, inBuffers[i].data, chunk);
            copied += chunk;
        }

        return copied == inLimit ? copied : 0;
    }

    static size_t ServerNetworkEncryption_CopyInDataToOutData(
        const enet_uint8* inData,
        size_t inLimit,
        enet_uint8* outData,
        size_t outLimit)
    {
        if (inData == nullptr || outData == nullptr || inLimit > outLimit)
        {
            return 0;
        }

        if (inLimit > 0)
        {
            std::memcpy(outData, inData, inLimit);
        }

        return inLimit;
    }

    static size_t ENET_CALLBACK ServerNetworkEncryption_EncryptCallback(
        void* context,
        ENetPeer* peer,
        const ENetBuffer* inBuffers,
        size_t inBufferCount,
        size_t inLimit,
        enet_uint8* outData,
        size_t outLimit)
    {
        (void)context;

        // Pas de peer associe (ex: handshake bas niveau): passthrough.
        uint32_t connectionId = 0;
        if (!ServerNetworkEncryption_TryGetConnectionIdFromPeer(peer, connectionId))
        {
            return ServerNetworkEncryption_CopyInBuffersToOutData(
                inBuffers,
                inBufferCount,
                inLimit,
                outData,
                outLimit);
        }

        NetworkState& networkState = GetNetworkState();

        std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> txKey{};
        if (!ServerNetworkEncryption_TryLoadSessionKeyIfEnabled(networkState, connectionId, true, txKey))
        {
            return ServerNetworkEncryption_CopyInBuffersToOutData(
                inBuffers,
                inBufferCount,
                inLimit,
                outData,
                outLimit);
        }

        const size_t requiredSize = kEncryptedPacketHeaderSize + inLimit + kEncryptedPacketTagSize;
        if (requiredSize > outLimit)
        {
            sodium_memzero(txKey.data(), txKey.size());
            return 0;
        }

        std::vector<enet_uint8> plaintext(inLimit);
        if (ServerNetworkEncryption_CopyInBuffersToOutData(
                inBuffers,
                inBufferCount,
                inLimit,
                plaintext.data(),
                plaintext.size()) != inLimit)
        {
            sodium_memzero(txKey.data(), txKey.size());
            sodium_memzero(plaintext.data(), plaintext.size());
            return 0;
        }

        std::memcpy(outData, kEncryptedPacketMagic, kEncryptedPacketMagicSize);

        enet_uint8* nonce = outData + kEncryptedPacketMagicSize;
        randombytes_buf(nonce, kEncryptedPacketNonceSize);

        unsigned long long ciphertextLen = 0;
        const int encryptResult = crypto_aead_xchacha20poly1305_ietf_encrypt(
            outData + kEncryptedPacketHeaderSize,
            &ciphertextLen,
            plaintext.data(),
            plaintext.size(),
            nullptr,
            0,
            nullptr,
            nonce,
            txKey.data());

        sodium_memzero(txKey.data(), txKey.size());
        sodium_memzero(plaintext.data(), plaintext.size());

        if (encryptResult != 0)
        {
            return 0;
        }

        return kEncryptedPacketHeaderSize + static_cast<size_t>(ciphertextLen);
    }

    static size_t ENET_CALLBACK ServerNetworkEncryption_DecryptCallback(
        void* context,
        ENetPeer* peer,
        const enet_uint8* inData,
        size_t inLimit,
        enet_uint8* outData,
        size_t outLimit)
    {
        (void)context;

        // Pas de peer associe (ex: handshake bas niveau): passthrough.
        uint32_t connectionId = 0;
        if (!ServerNetworkEncryption_TryGetConnectionIdFromPeer(peer, connectionId))
        {
            return ServerNetworkEncryption_CopyInDataToOutData(inData, inLimit, outData, outLimit);
        }

        NetworkState& networkState = GetNetworkState();

        std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> rxKey{};
        if (!ServerNetworkEncryption_TryLoadSessionKeyIfEnabled(networkState, connectionId, false, rxKey))
        {
            return ServerNetworkEncryption_CopyInDataToOutData(inData, inLimit, outData, outLimit);
        }

        if (inData == nullptr || inLimit < (kEncryptedPacketHeaderSize + kEncryptedPacketTagSize))
        {
            sodium_memzero(rxKey.data(), rxKey.size());
            return 0;
        }

        if (std::memcmp(inData, kEncryptedPacketMagic, kEncryptedPacketMagicSize) != 0)
        {
            sodium_memzero(rxKey.data(), rxKey.size());
            RCNET_log(
                RCNET_LOG_WARN,
                "[SERVER] [NETWORK_ENCRYPTION] [DECRYPT] - Invalid encrypted packet magic for connectionId=%u",
                connectionId);
            return 0;
        }

        const enet_uint8* nonce = inData + kEncryptedPacketMagicSize;
        const enet_uint8* ciphertext = inData + kEncryptedPacketHeaderSize;
        const size_t ciphertextLen = inLimit - kEncryptedPacketHeaderSize;

        const size_t maximumPlaintextLen = ciphertextLen - kEncryptedPacketTagSize;
        if (maximumPlaintextLen > outLimit)
        {
            sodium_memzero(rxKey.data(), rxKey.size());
            return 0;
        }

        unsigned long long plaintextLen = 0;
        const int decryptResult = crypto_aead_xchacha20poly1305_ietf_decrypt(
            outData,
            &plaintextLen,
            nullptr,
            ciphertext,
            ciphertextLen,
            nullptr,
            0,
            nonce,
            rxKey.data());

        sodium_memzero(rxKey.data(), rxKey.size());

        if (decryptResult != 0)
        {
            RCNET_log(
                RCNET_LOG_WARN,
                "[SERVER] [NETWORK_ENCRYPTION] [DECRYPT] - Authentication failed for connectionId=%u",
                connectionId);
            return 0;
        }

        return static_cast<size_t>(plaintextLen);
    }
}

void ServerNetworkEncryption_EnsureHostEncryptorInstalled(ENetHost* host)
{
    if (host == nullptr)
    {
        return;
    }

    static ServerNetworkHostEncryptorContext encryptorContext{};
    static ENetEncryptor encryptor = {
        &encryptorContext,
        ServerNetworkEncryption_EncryptCallback,
        ServerNetworkEncryption_DecryptCallback,
        nullptr
    };

    // Evite de reconfigurer le host a chaque tick.
    if (host->encryptor.context == encryptor.context &&
        host->encryptor.encrypt == encryptor.encrypt &&
        host->encryptor.decrypt == encryptor.decrypt)
    {
        return;
    }

    enet_host_encrypt(host, &encryptor);
    RCNET_log(RCNET_LOG_INFO, "[SERVER] [NETWORK_ENCRYPTION] - ENet host encryptor installed");
}
