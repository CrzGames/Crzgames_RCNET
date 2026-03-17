#pragma once

#include <array>   // std::array
#include <cstdint> // uint8_t, uint32_t, etc.

#include <sodium.h> // crypto_kx_PUBLICKEYBYTES, crypto_kx_SECRETKEYBYTES, crypto_kx_SESSIONKEYBYTES

// Structure pour stocker la clés publique et secrète du client utilisées pour 
// les échanges de clés de session sécurisée avec le serveur.
struct ClientCryptoKxState
{
    std::array<uint8_t, crypto_kx_PUBLICKEYBYTES> clientPublicKey{};
    std::array<uint8_t, crypto_kx_SECRETKEYBYTES> clientSecretKey{};
};

// Initialise l’état crypto pour les échanges de clés de session sécurisée avec le serveur. 
// Génère une paire de clés publique/privée pour le client.
bool ClientCryptoKx_Initialize(ClientCryptoKxState& state);

// Récupère la clé publique du client utilisée pour les échanges de clés de session sécurisée.
const std::array<uint8_t, crypto_kx_PUBLICKEYBYTES>& ClientCryptoKx_GetClientPublicKey(const ClientCryptoKxState& state);

// Tente de calculer les clés de session sécurisée client à partir de la clé publique serveur reçue dans le packet hello.
bool ClientCryptoKx_ComputeSessionKeys(
    const ClientCryptoKxState& state,
    const std::array<uint8_t, crypto_kx_PUBLICKEYBYTES>& serverPublicKey,
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES>& outClientRxKey,
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES>& outClientTxKey);