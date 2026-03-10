#pragma once

#include <rcenet/RCENET_enet.h> // ENetHost

// Installe (si nécessaire) l'encryptor ENet XChaCha20-Poly1305 sur le host serveur.
//
// Rôle :
// - brancher les callbacks de chiffrement/déchiffrement au niveau ENetHost.
// - laisser ENet appeler ces callbacks automatiquement sur les datagrammes UDP.
//
// Propriété importante :
// - la fonction est idempotente.
// - si le même encryptor est déjà attaché au host, elle ne fait rien.
// - sinon, elle appelle enet_host_encrypt avec les callbacks serveur.
void ServerNetworkEncryption_EnsureHostEncryptorInstalled(ENetHost* host);
