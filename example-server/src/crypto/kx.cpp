#include "crypto/kx.h"

bool ServerCryptoKx_Initialize(ServerCryptoKxState& state)
{
    // Genere une paire de cles X25519 serveur.
    crypto_kx_keypair(
        state.serverPublicKey.data(),
        state.serverSecretKey.data());

    return true;
}

const std::array<uint8_t, crypto_kx_PUBLICKEYBYTES>& ServerCryptoKx_GetServerPublicKey(const ServerCryptoKxState& state)
{
    return state.serverPublicKey;
}

bool ServerCryptoKx_ComputeSessionKeys(
    const ServerCryptoKxState& state,
    const std::array<uint8_t, crypto_kx_PUBLICKEYBYTES>& clientPublicKey,
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES>& outServerRxKey,
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES>& outServerTxKey)
{
    // Cote serveur, libsodium expose crypto_kx_server_session_keys():
    // - rx: trafic entrant client -> serveur
    // - tx: trafic sortant serveur -> client
    const int result = crypto_kx_server_session_keys(
        outServerRxKey.data(),
        outServerTxKey.data(),
        state.serverPublicKey.data(),
        state.serverSecretKey.data(),
        clientPublicKey.data());

    return result == 0;
}