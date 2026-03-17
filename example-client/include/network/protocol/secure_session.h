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

// Cle publique Ed25519 pinnee cote client.
// Cette valeur est derivee de la seed serveur (DEV) et sert a verifier
// ServerSecureSessionHelloResponsePacketReliable::signature.
// La seed privee serveur NE DOIT JAMAIS etre presente cote client.
static constexpr char CLIENT_SECURE_SESSION_SERVER_ED25519_PUBLIC_KEY_HEX[] =
    "af110dde7833e9aa3784e6b44af5754a67b659a09dd85877bdc0330c79e7e02f";

static constexpr std::array<uint8_t, crypto_sign_PUBLICKEYBYTES>
    CLIENT_SECURE_SESSION_SERVER_ED25519_PUBLIC_KEY = {
        0xaf, 0x11, 0x0d, 0xde, 0x78, 0x33, 0xe9, 0xaa,
        0x37, 0x84, 0xe6, 0xb4, 0x4a, 0xf5, 0x75, 0x4a,
        0x67, 0xb6, 0x59, 0xa0, 0x9d, 0xd8, 0x58, 0x77,
        0xbd, 0xc0, 0x33, 0x0c, 0x79, 0xe7, 0xe0, 0x2f};

// Duree maximale attendue pour une attestation secure-session (en secondes).
// Le client peut refuser une attestation dont la fenetre de validite est
// anormalement longue.
static constexpr uint64_t CLIENT_SECURE_SESSION_SIGNATURE_TTL_SECONDS = 10;
