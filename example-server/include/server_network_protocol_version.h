#pragma once

#include <cstdint>

// Version protocole réseau client -> serveur
static constexpr uint16_t SERVER_NETWORK_PROTOCOL_VERSION_MAJOR = 1;
static constexpr uint16_t SERVER_NETWORK_PROTOCOL_VERSION_MINOR = 2;
static constexpr uint16_t SERVER_NETWORK_PROTOCOL_VERSION_PATCH = 3;

static constexpr uint32_t SERVER_NETWORK_PROTOCOL_VERSION = (
    (static_cast<uint32_t>(SERVER_NETWORK_PROTOCOL_VERSION_MAJOR) << 16) |
    (static_cast<uint32_t>(SERVER_NETWORK_PROTOCOL_VERSION_MINOR) << 8) |
    (static_cast<uint32_t>(SERVER_NETWORK_PROTOCOL_VERSION_PATCH))
);