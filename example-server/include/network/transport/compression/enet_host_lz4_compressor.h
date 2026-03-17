#pragma once

#include <rcenet/RCENET_enet.h> // ENetHost

// Installe (si nécessaire) le compresseur LZ4 ENet sur le host serveur.
//
// Rôle :
// - brancher des callbacks de compression/décompression au niveau ENetHost.
// - laisser ENet compresser/décompresser les datagrammes UDP quand c'est pertinent.
//
// Propriété importante :
// - la fonction est idempotente.
// - si le même compresseur est déjà attaché au host, elle ne fait rien.
// - sinon, elle appelle enet_host_compress avec les callbacks serveur.
void ServerNetworkCompression_EnsureHostCompressorInstalled(ENetHost* host);
