#pragma once

#include <cstddef> // std::size_t
#include <cstdint> // uint16_t, uint32_t, etc.
#include <vector>  // std::vector

#include <rcenet/RCENET_enet.h> // ENetPeer

// Taille cible maximale de payload UDP "ultra-safe" a respecter, lors de l'envoi de paquets réseau, 
// afin d'éviter tout risque de fragmentation IP, qui pourrait entraîner des problèmes de performance 
// et de fiabilité dans la transmission des données. Cette limite est basée sur les recommandations générales 
// pour les réseaux modernes, en tenant compte des en-têtes IP et UDP, ainsi que d'une marge de sécurité pour 
// éviter les problèmes liés à la fragmentation.
static constexpr std::size_t kServerNetworkOutgoingPayloadMaxBytes = 508u;

bool ServerNetworkOutgoing_SendMatchInitPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes);
bool ServerNetworkOutgoing_SendWorldStaticStateInitPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes);
bool ServerNetworkOutgoing_SendMatchStartPacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes);
bool ServerNetworkOutgoing_SendSnapshotFullPacketUnreliable(ENetPeer* peer, const std::vector<uint8_t>& bytes);
bool ServerNetworkOutgoing_SendSecureSessionHelloResponsePacketReliable(
    ENetPeer* peer,
    const std::vector<uint8_t>& bytes,
    bool disconnectAfterAck,
    bool enableEncryptionAfterAck);
bool ServerNetworkOutgoing_SendAuthResponsePacketReliable(ENetPeer* peer, const std::vector<uint8_t>& bytes, bool disconnectAfterAck);
