#pragma once

#include <array>   // std::array
#include <cstdint> // uint8_t, uint32_t, etc.

#include <sodium.h> // crypto_kx_PUBLICKEYBYTES, crypto_kx_SECRETKEYBYTES, crypto_kx_SESSIONKEYBYTES

// Structure pour stocker les clés publiques et secrètes du serveur utilisées pour les échanges de clés de session sécurisée avec les clients.
struct ServerCryptoKxState
{
    std::array<uint8_t, crypto_kx_PUBLICKEYBYTES> serverPublicKey{};
    std::array<uint8_t, crypto_kx_SECRETKEYBYTES> serverSecretKey{};
};

// Initialise l’état crypto pour les échanges de clés de session sécurisée du serveur.
bool ServerCryptoKx_Initialize(ServerCryptoKxState& state);

// Récupère la clé publique du serveur utilisée pour les échanges de clés de session sécurisée.
const std::array<uint8_t, crypto_kx_PUBLICKEYBYTES>& ServerCryptoKx_GetServerPublicKey(const ServerCryptoKxState& state);

// Tente de calculer les clés de session sécurisée serveur à partir de la clé publique client reçue dans le packet hello.
bool ServerCryptoKx_ComputeSessionKeys(
    const ServerCryptoKxState& state,
    const std::array<uint8_t, crypto_kx_PUBLICKEYBYTES>& clientPublicKey,
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES>& outServerRxKey,
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES>& outServerTxKey);