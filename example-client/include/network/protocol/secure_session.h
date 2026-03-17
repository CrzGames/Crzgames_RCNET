#pragma once

#include <cstddef> // size_t

// Taille (en bytes) du nonce transporte dans
// ClientSecureSessionHelloPacketReliable::clientNonce.
// Le serveur doit recopier exactement cette valeur dans
// ServerSecureSessionHelloResponsePacketReliable::clientNonceEcho
// (valeur incluse dans la signature) pour lier la reponse a CE handshake.
static constexpr size_t SERVER_SECURE_SESSION_CLIENT_NONCE_BYTES = 16;