#pragma once

#include <cstdint> // uint16_t, uint32_t, etc.
#include <string>  // std::string

// ======================================================================================
// ClientReliablePacketType
//
// Type de packet envoyé sur le channel reliable client -> serveur.
//
// Tous les packets reliable doivent commencer par ClientReliablePacketHeader
// pour permettre au serveur de dispatcher correctement.
// ======================================================================================
enum class ClientReliablePacketType : uint8_t
{
    CLIENT_HANDSHAKE_PACKET_RELIABLE = 0,
    CLIENT_READY_FOR_MATCH_PACKET_RELIABLE = 1,
};

struct ClientReliablePacketHeader
{
    ClientReliablePacketType type;
};

// ======================================================================================
// Liste des packets reliable envoyés par le client au serveur.
// ======================================================================================

struct ClientHandshakePacketReliable
{
    // Header commun à tous les packets reliable client -> serveur
    ClientReliablePacketHeader header;

    // Seulement pour le packet de handshake initial envoyé par le client lors de la connexion.
    // Permet au serveur de vérifier la compatibilité du protocole réseau avant d'accepter la connexion..etc
    uint32_t networkProtocolVersion;

    // Token d’authentification (Bearer token oat)
    std::string authToken;
};

struct ClientReadyForMatchPacketReliable
{
    // Header commun à tous les packets reliable client -> serveur.
    ClientReliablePacketHeader header;
};