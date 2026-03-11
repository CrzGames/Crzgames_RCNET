#include "crypto/signing.h"

#include <array>   // std::array
#include <cstddef> // size_t
#include <cstdlib> // std::getenv
#include <cstring> // std::strlen
#include <vector>  // std::vector

// Prefixe texte fixe ajoute au message avant la signature.
// Role: lier la signature a CE format de message secure-session uniquement.
// Le client doit utiliser exactement la meme valeur pour verifier.
static constexpr char kSecureSessionSigningDomain[] = "SERVER_ED25519_DOMAIN_SECURE_SESSION_ATTESTATION_V1";

// Ecrit un uint64 en big-endian pour obtenir une representation binaire
// stable et identique sur toutes les plateformes.
static void ServerCryptoSigning_WriteU64Be(std::vector<uint8_t>& out, uint64_t value)
{
    out.push_back(static_cast<uint8_t>((value >> 56) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 48) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 40) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 32) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(value & 0xFF));
}

static void ServerCryptoSigning_BuildSecureSessionMessage(
    const ServerCryptoSigningSecureSessionPayload& payload,
    std::vector<uint8_t>& outMessage)
{
    // Layout exact du message signe (ordre strict):
    // [domain][server_kx_pub][issued_at_be64][expires_at_be64][client_nonce_echo]
    //
    // Important: le client doit reconstruire EXACTEMENT ce layout
    // avant crypto_sign_verify_detached().
    outMessage.clear();
    outMessage.reserve(
        sizeof(kSecureSessionSigningDomain) - 1 +
        payload.serverKxPublicKey.size() +
        8 +
        8 +
        payload.clientNonceEcho.size());

    outMessage.insert(
        outMessage.end(),
        kSecureSessionSigningDomain,
        kSecureSessionSigningDomain + (sizeof(kSecureSessionSigningDomain) - 1));

    outMessage.insert(
        outMessage.end(),
        payload.serverKxPublicKey.begin(),
        payload.serverKxPublicKey.end());
    ServerCryptoSigning_WriteU64Be(outMessage, payload.issuedAtUnixSeconds);
    ServerCryptoSigning_WriteU64Be(outMessage, payload.expiresAtUnixSeconds);
    outMessage.insert(
        outMessage.end(),
        payload.clientNonceEcho.begin(),
        payload.clientNonceEcho.end());
}

// Decode une seed hex -> binaire.
// Retourne false si format invalide ou taille incorrecte.
static bool ServerCryptoSigning_TryDecodeSeedHex(
    const char* seedHex,
    size_t seedHexLength,
    std::array<uint8_t, crypto_sign_SEEDBYTES>& outSeed)
{
    if (seedHex == nullptr || seedHexLength == 0)
    {
        return false;
    }

    size_t decodedLength = 0;
    // Autorise quelques separateurs pour faciliter l'ops:
    // espace, ':', '-', tab, CR, LF.
    const int decodeResult = sodium_hex2bin(
        outSeed.data(),
        outSeed.size(),
        seedHex,
        seedHexLength,
        " :-\t\r\n",
        &decodedLength,
        nullptr);

    if (decodeResult != 0 || decodedLength != outSeed.size())
    {
        // Nettoyage defensif en cas de parse invalide.
        sodium_memzero(outSeed.data(), outSeed.size());
        return false;
    }

    return true;
}

// Charge la seed de signature selon l'environnement de build.
// DEV: seed hardcodee dans ce .cpp.
// STAGING/PROD: seed depuis variable d'environnement.
static bool ServerCryptoSigning_LoadSeed(
    std::array<uint8_t, crypto_sign_SEEDBYTES>& outSeed)
{
    const char* seedHex = nullptr;

#if SERVER_ENV_DEV
    // En dev: seed visible et hardcodee dans le code.
    seedHex = "51511323d3c288c32dd39d3e995cfc6e5fc679df48e5c3f04a3375d353e1d856";
#elif SERVER_ENV_STAGING
    // En staging: seed lue via variable d'environnement
    // SERVER_ED25519_PRIVATE_SEED_HEX.
    seedHex = std::getenv("SERVER_ED25519_PRIVATE_SEED_HEX");
#elif SERVER_ENV_PRODUCTION
    // En production: seed lue via variable d'environnement
    // SERVER_ED25519_PRIVATE_SEED_HEX.
    seedHex = std::getenv("SERVER_ED25519_PRIVATE_SEED_HEX");
#else
#error "Define one of SERVER_ENV_DEV, SERVER_ENV_STAGING or SERVER_ENV_PRODUCTION"
#endif

    // Si seed absente, vide ou invalide, on refuse l'initialisation.
    if (seedHex == nullptr || seedHex[0] == '\0')
    {
        return false;
    }

    return ServerCryptoSigning_TryDecodeSeedHex(seedHex, std::strlen(seedHex), outSeed);
}

bool ServerCryptoSigning_Initialize(ServerCryptoSigningState& state)
{
    // sodium_init() peut etre appele plusieurs fois sans souci.
    if (sodium_init() < 0)
    {
        return false;
    }

    std::array<uint8_t, crypto_sign_SEEDBYTES> seed{};
    if (!ServerCryptoSigning_LoadSeed(seed))
    {
        sodium_memzero(seed.data(), seed.size());
        return false;
    }

    // Identite deterministe/stable -> compatible client qui hardcode
    // la cle publique Ed25519 (pinning).
    crypto_sign_seed_keypair(
        state.identityPublicKey.data(),
        state.identitySecretKey.data(),
        seed.data());
    // Le seed ne doit pas rester en memoire.
    sodium_memzero(seed.data(), seed.size());

    return true;
}

bool ServerCryptoSigning_SignSecureSessionPayload(
    const ServerCryptoSigningState& state,
    const ServerCryptoSigningSecureSessionPayload& payload,
    std::array<uint8_t, crypto_sign_BYTES>& outSignature)
{
    // 1) Serialiser le message canonique a signer.
    std::vector<uint8_t> message;
    ServerCryptoSigning_BuildSecureSessionMessage(payload, message);

    // 2) Signer en detached (signature seule, message envoye separement).
    unsigned long long signatureLength = 0;
    const int signResult = crypto_sign_detached(
        outSignature.data(),
        &signatureLength,
        message.data(),
        static_cast<unsigned long long>(message.size()),
        state.identitySecretKey.data());

    // libsodium renvoie la taille effective; on verifie qu'elle est attendue.
    return signResult == 0 && signatureLength == outSignature.size();
}

const std::array<uint8_t, crypto_sign_PUBLICKEYBYTES>& ServerCryptoSigning_GetIdentityPublicKey(
    const ServerCryptoSigningState& state)
{
    return state.identityPublicKey;
}
