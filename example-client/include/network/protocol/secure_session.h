#pragma once

#include <array>   // std::array
#include <cstddef> // size_t
#include <cstdint> // uint8_t, uint64_t

#include <sodium.h> // crypto_sign_PUBLICKEYBYTES

// ============================================================================
// Secure-session metadata (client side)
// ============================================================================

// Taille (en bytes) du nonce transporte dans
// ClientSecureSessionHelloPacketReliable::clientNonce.
// Le serveur doit recopier exactement cette valeur dans
// ServerSecureSessionHelloResponsePacketReliable::clientNonceEcho.
static constexpr size_t CLIENT_SECURE_SESSION_CLIENT_NONCE_BYTES = 16;

// Domaine de signature Ed25519 applique par le serveur au payload secure-session.
// Le client doit reconstruire le message signe avec EXACTEMENT cette valeur.
static constexpr char CLIENT_SECURE_SESSION_SIGNING_DOMAIN[] =
    "SERVER_ED25519_DOMAIN_SECURE_SESSION_ATTESTATION_V1";

// Cle publique Ed25519 pinnee cote client pour l'environnement DEV.
// Cette valeur est derivee de la seed serveur (DEV) et sert a verifier
// ServerSecureSessionHelloResponsePacketReliable::signature.
// La seed privee serveur NE DOIT JAMAIS etre presente cote client.
static constexpr char CLIENT_SECURE_SESSION_SERVER_ED25519_PUBLIC_KEY_HEX[] =
    "af110dde7833e9aa3784e6b44af5754a67b659a09dd85877bdc0330c79e7e02f";

// Duree maximale attendue pour une attestation secure-session (en secondes).
// Le client peut refuser une attestation dont la fenetre de validite est
// anormalement longue.
static constexpr uint64_t CLIENT_SECURE_SESSION_SIGNATURE_TTL_SECONDS = 10;

// Initialise (une seule fois) la cle publique pinnee binaire a partir de
// CLIENT_SECURE_SESSION_SERVER_ED25519_PUBLIC_KEY_HEX.
// Retourne false si la valeur HEX est invalide.
bool ClientSecureSession_InitializePinnedServerSigningPublicKey();

// Copie la cle publique pinnee binaire (32 bytes) dans outPublicKey.
// Retourne false si l'initialisation n'a pas encore ete faite.
bool ClientSecureSession_GetPinnedServerSigningPublicKey(
    std::array<uint8_t, crypto_sign_PUBLICKEYBYTES>& outPublicKey);
