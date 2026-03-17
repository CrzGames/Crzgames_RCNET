#pragma once

#include <cstddef> // size_t
#include <cstdint> // uint64_t

// Taille (en bytes) du nonce transporte dans
// ClientSecureSessionHelloPacketReliable::clientNonce.
// Le serveur doit recopier exactement cette valeur dans
// ServerSecureSessionHelloResponsePacketReliable::clientNonceEcho
// (valeur incluse dans la signature) pour lier la reponse a CE handshake.
static constexpr size_t SERVER_SECURE_SESSION_CLIENT_NONCE_BYTES = 16;

// Duree de validite (en secondes) de l'attestation signee envoyee dans
// ServerSecureSessionHelloResponsePacketReliable.
// Cote serveur: expiresAtUnixSeconds = issuedAtUnixSeconds + TTL.
// Cote client: refuser la reponse si now > expiresAtUnixSeconds.
static constexpr uint64_t SERVER_SECURE_SESSION_SIGNATURE_TTL_SECONDS = 10;
