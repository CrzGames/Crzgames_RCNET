#include "crypto/kx.h"

bool ClientCryptoKx_Initialize(ClientCryptoKxState& state)
{
    crypto_kx_keypair(
        state.clientPublicKey.data(),
        state.clientSecretKey.data());

    return true;
}

const std::array<uint8_t, crypto_kx_PUBLICKEYBYTES>& ClientCryptoKx_GetClientPublicKey(const ClientCryptoKxState& state)
{
    return state.clientPublicKey;
}

bool ClientCryptoKx_ComputeSessionKeys(
    const ClientCryptoKxState& state,
    const std::array<uint8_t, crypto_kx_PUBLICKEYBYTES>& clientPublicKey,
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES>& outClientRxKey,
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES>& outClientTxKey)
{
    const int result = crypto_kx_client_session_keys(
        outClientRxKey.data(),
        outClientTxKey.data(),
        state.clientPublicKey.data(),
        state.clientSecretKey.data(),
        clientPublicKey.data());

    return result == 0;
}