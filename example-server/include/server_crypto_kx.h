#pragma once

#include <array>   // std::array
#include <cstdint> // uint8_t, uint32_t, etc.

#include <sodium.h>

struct ServerCryptoKxState
{
    std::array<uint8_t, crypto_kx_PUBLICKEYBYTES> serverPublicKey{};
    std::array<uint8_t, crypto_kx_SECRETKEYBYTES> serverSecretKey{};
};

bool ServerCryptoKx_Initialize(ServerCryptoKxState& state);

const std::array<uint8_t, crypto_kx_PUBLICKEYBYTES>& ServerCryptoKx_GetServerPublicKey(const ServerCryptoKxState& state);

bool ServerCryptoKx_ComputeSessionKeys(
    const ServerCryptoKxState& state,
    const std::array<uint8_t, crypto_kx_PUBLICKEYBYTES>& clientPublicKey,
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES>& outServerRxKey,
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES>& outServerTxKey);