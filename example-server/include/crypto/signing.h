#pragma once

#include <array>   // std::array
#include <cstdint> // uint8_t, uint64_t

#include <sodium.h> // crypto_kx_PUBLICKEYBYTES, crypto_sign_*

#include "network/protocol/secure_session.h"

// Etat de l'identite de signature serveur.
// Cette identite sert uniquement a signer l'attestation de secure-session.
struct ServerCryptoSigningState
{
    // Cle publique Ed25519 d'identite serveur.
    // Cette cle DOIT etre hardcodee (pinnee) cote client pour verifier
    // crypto_sign_verify_detached() sur la signature envoyee dans
    // ServerSecureSessionHelloResponsePacketReliable::signature.
    std::array<uint8_t, crypto_sign_PUBLICKEYBYTES> identityPublicKey{};

    // Cle secrete Ed25519 conservee uniquement cote serveur.
    std::array<uint8_t, crypto_sign_SECRETKEYBYTES> identitySecretKey{};
};

// Contenu logique signe dans la reponse secure-session.
// La signature couvre TOUJOURS exactement ces champs (dans cet ordre):
// 1) cle publique KX ephemere du serveur
// 2) timestamp d'emission
// 3) timestamp d'expiration
// 4) nonce client renvoye (echo)
struct ServerCryptoSigningSecureSessionPayload
{
    // Cle publique KX (X25519) annoncee pour derivation des cles de session.
    std::array<uint8_t, crypto_kx_PUBLICKEYBYTES> serverKxPublicKey{};

    // Date d'emission de l'attestation (epoch unix, secondes).
    uint64_t issuedAtUnixSeconds = 0;

    // Date d'expiration de l'attestation (epoch unix, secondes).
    uint64_t expiresAtUnixSeconds = 0;

    // Copie exacte du champ
    // ClientSecureSessionHelloPacketReliable::clientNonce recu du client.
    // Le serveur ne regenere pas cette valeur: il la renvoie telle quelle.
    std::array<uint8_t, SERVER_SECURE_SESSION_CLIENT_NONCE_BYTES> clientNonceEcho{};
};

// Initialise l'identite Ed25519 serveur.
//
// Politique de chargement:
// - SERVER_ENV_DEV       : seed hardcodee dans signing.cpp
// - SERVER_ENV_STAGING   : seed via variable d'environnement
// - SERVER_ENV_PRODUCTION: seed via variable d'environnement
//
// Retour:
// - true  : initialisation OK
// - false : sodium_init() a echoue ou seed absente/invalide
bool ServerCryptoSigning_Initialize(ServerCryptoSigningState& state);

// Signe le payload secure-session avec la cle privee Ed25519 du serveur.
//
// Retour:
// - true  : signature calculee
// - false : echec libsodium
bool ServerCryptoSigning_SignSecureSessionPayload(
    const ServerCryptoSigningState& state,
    const ServerCryptoSigningSecureSessionPayload& payload,
    std::array<uint8_t, crypto_sign_BYTES>& outSignature);

// Retourne la cle publique Ed25519 d'identite serveur.
const std::array<uint8_t, crypto_sign_PUBLICKEYBYTES>& ServerCryptoSigning_GetIdentityPublicKey(
    const ServerCryptoSigningState& state);
