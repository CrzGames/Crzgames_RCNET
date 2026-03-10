#pragma once

#include <rcenet/RCENET_enet.h> // ENetHost

// Installe (si necessaire) l'encryptor ENet sur le host serveur.
//
// Cette fonction est idempotente:
// - si l'encryptor est deja attache a ce host, elle ne fait rien.
// - sinon, elle appelle enet_host_encrypt avec les callbacks serveur.
void ServerNetworkEncryption_EnsureHostEncryptorInstalled(ENetHost* host);
