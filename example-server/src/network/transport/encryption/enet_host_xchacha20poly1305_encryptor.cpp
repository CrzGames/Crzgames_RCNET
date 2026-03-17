#include "network/transport/encryption/enet_host_xchacha20poly1305_encryptor.h"

#include "core/context.h"
#include "network/state.h"
#include "simulation/session/client.h"

#include <array>         // std::array
#include <cstdint>       // uint32_t, uintptr_t
#include <cstring>       // std::memcmp, std::memcpy
#include <mutex>         // std::lock_guard
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

#include <sodium.h> // XChaCha20-Poly1305

#include <RCNET/RCNET.h> // RCNET_log

// Signature de format placee en tete des paquets applicatifs chiffres.
// Permet de detecter rapidement qu'on lit bien un datagramme attendu.
static constexpr enet_uint8 kEncryptedPacketMagic[4] = {'R', 'C', 'N', '1'};

// Tailles fixes utilisees dans le format de paquet chiffre.
static constexpr size_t kEncryptedPacketMagicSize = sizeof(kEncryptedPacketMagic);
static constexpr size_t kEncryptedPacketNonceSize = crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
static constexpr size_t kEncryptedPacketTagSize = crypto_aead_xchacha20poly1305_ietf_ABYTES;
static constexpr size_t kEncryptedPacketHeaderSize = kEncryptedPacketMagicSize + kEncryptedPacketNonceSize;

// Contexte ENet encryptor.
// Pour l'instant, aucun etat mutable n'est requis.
struct ServerNetworkHostEncryptorContext
{
};

// Extrait le connectionId depuis peer->data.
// Retourne false si le peer est invalide, non initialise, ou si connectionId == 0.
static bool ServerNetworkEncryption_TryGetConnectionIdFromPeer(const ENetPeer* peer, uint32_t& outConnectionId)
{
    // Validation des pointeurs.
    if (peer == nullptr || peer->data == nullptr)
    {
        return false;
    }

    // Conversion du user data ENet vers l'identifiant de connexion serveur.
    const uint32_t connectionId = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(peer->data));
    if (connectionId == 0)
    {
        return false;
    }

    // Ecriture du resultat.
    outConnectionId = connectionId;
    return true;
}

// Charge la cle de session (TX ou RX) si le chiffrement de paquets est actif pour cette session.
// - forOutgoingEncryption == true  -> charge serverTxKey
// - forOutgoingEncryption == false -> charge serverRxKey
static bool ServerNetworkEncryption_TryLoadSessionKeyIfEnabled(
    const NetworkState& networkState,
    uint32_t connectionId,
    bool forOutgoingEncryption,
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES>& outKey)
{
    // Proteger l'acces concurrent a networkState.sessions.
    std::lock_guard<std::mutex> lock(networkState.sessionsMutex);

    // Chercher la session correspondant au connectionId.
    std::unordered_map<uint32_t, ClientSession>::const_iterator sit =
        networkState.sessions.find(connectionId);
    if (sit == networkState.sessions.end())
    {
        return false;
    }

    // Verifier que le chiffrement applicatif est effectivement active.
    const ClientSession& session = sit->second;
    if (!session.isPacketEncryptionEnabled)
    {
        return false;
    }

    // Copier la cle adaptee au sens du flux.
    outKey = forOutgoingEncryption ? session.serverTxKey : session.serverRxKey;
    return true;
}

// Copie une liste de fragments ENet (inBuffers) vers un buffer contigu outData.
// Retourne 0 en cas d'erreur, sinon retourne exactement inLimit.
static size_t ServerNetworkEncryption_CopyInBuffersToOutData(
    const ENetBuffer* inBuffers,
    size_t inBufferCount,
    size_t inLimit,
    enet_uint8* outData,
    size_t outLimit)
{
    // Validation de base.
    if (inBuffers == nullptr || outData == nullptr || inLimit > outLimit)
    {
        RCNET_log(
            RCNET_LOG_ERROR,
            "[SERVER] [NETWORK_ENCRYPTION] [COPY_BUFFERS] - invalid input (inBuffers=%p outData=%p inLimit=%zu outLimit=%zu)",
            inBuffers,
            outData,
            inLimit,
            outLimit);
        return 0;
    }

    // Nombre total d'octets copies vers outData.
    size_t copied = 0;

    // Parcourir chaque fragment.
    for (size_t i = 0; i < inBufferCount && copied < inLimit; ++i)
    {
        // Octets restants a copier.
        const size_t remaining = inLimit - copied;

        // Taille utile du fragment courant.
        const size_t chunk =
            (inBuffers[i].dataLength < remaining) ? inBuffers[i].dataLength : remaining;

        // Fragment vide -> suivant.
        if (chunk == 0)
        {
            continue;
        }

        // Protection contre un pointeur source invalide.
        if (inBuffers[i].data == nullptr)
        {
            RCNET_log(
                RCNET_LOG_ERROR,
                "[SERVER] [NETWORK_ENCRYPTION] [COPY_BUFFERS] - inBuffers[%u].data is null",
                static_cast<unsigned>(i));
            return 0;
        }

        // Copie du fragment dans outData.
        std::memcpy(outData + copied, inBuffers[i].data, chunk);
        copied += chunk;
    }

    // On exige une copie complete de inLimit octets.
    return copied == inLimit ? copied : 0;
}

// Callback ENet appele pour chiffrer un datagramme sortant.
static size_t ENET_CALLBACK ServerNetworkEncryption_EncryptCallback(
    void* context,
    ENetPeer* peer,
    const ENetBuffer* inBuffers,
    size_t inBufferCount,
    size_t inLimit,
    enet_uint8* outData,
    size_t outLimit)
{
    // Contexte non utilise pour le moment.
    (void)context;

    // ENet6: peer peut etre NULL pendant la phase de connexion.
    // Dans ce cas (ou sans connectionId valide), ne pas chiffrer.
    uint32_t connectionId = 0;
    if (!ServerNetworkEncryption_TryGetConnectionIdFromPeer(peer, connectionId))
    {
        return 0;
    }

    // Recuperer l'etat reseau global.
    const NetworkState& networkState = GetNetworkState();

    // Tant que le chiffrement n'est pas active pour ce peer:
    // - on ne chiffre pas ce paquet (return 0),
    // - ENet enverra le paquet en clair (sans flag encrypted).
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> txKey{};
    if (!ServerNetworkEncryption_TryLoadSessionKeyIfEnabled(networkState, connectionId, true, txKey))
    {
        return 0;
    }

    // Calcul de la taille minimale requise :
    // [magic + nonce] + [ciphertext] + [tag AEAD].
    const size_t requiredSize = kEncryptedPacketHeaderSize + inLimit + kEncryptedPacketTagSize;
    if (requiredSize > outLimit)
    {
        RCNET_log(
            RCNET_LOG_ERROR,
            "[SERVER] [NETWORK_ENCRYPTION] [ENCRYPT] - output buffer too small for connectionId=%u (required=%zu outLimit=%zu)",
            connectionId,
            requiredSize,
            outLimit);
        sodium_memzero(txKey.data(), txKey.size());
        return 0;
    }

    // Construire un plaintext contigu depuis les fragments ENet.
    std::vector<enet_uint8> plaintext(inLimit);
    if (ServerNetworkEncryption_CopyInBuffersToOutData(
            inBuffers,
            inBufferCount,
            inLimit,
            plaintext.data(),
            plaintext.size()) != inLimit)
    {
        RCNET_log(
            RCNET_LOG_ERROR,
            "[SERVER] [NETWORK_ENCRYPTION] [ENCRYPT] - failed to linearize input buffers for connectionId=%u",
            connectionId);
        sodium_memzero(txKey.data(), txKey.size());
        sodium_memzero(plaintext.data(), plaintext.size());
        return 0;
    }

    // Ecrire la signature de format en tete.
    std::memcpy(outData, kEncryptedPacketMagic, kEncryptedPacketMagicSize);

    // Ecrire un nonce aleatoire unique pour ce paquet.
    enet_uint8* nonce = outData + kEncryptedPacketMagicSize;
    randombytes_buf(nonce, kEncryptedPacketNonceSize);

    // Chiffrer le plaintext avec XChaCha20-Poly1305.
    unsigned long long ciphertextLen = 0;
    const int encryptResult = crypto_aead_xchacha20poly1305_ietf_encrypt(
        outData + kEncryptedPacketHeaderSize,
        &ciphertextLen,
        plaintext.data(),
        plaintext.size(),
        nullptr, // AD non utilisee
        0,
        nullptr, // nsec non utilise
        nonce,
        txKey.data());

    // Effacer en memoire les donnees sensibles.
    sodium_memzero(txKey.data(), txKey.size());
    sodium_memzero(plaintext.data(), plaintext.size());

    // Si le chiffrement a echoue, signaler un echec au moteur ENet.
    if (encryptResult != 0)
    {
        RCNET_log(
            RCNET_LOG_ERROR,
            "[SERVER] [NETWORK_ENCRYPTION] [ENCRYPT] - crypto_aead_xchacha20poly1305_ietf_encrypt failed for connectionId=%u",
            connectionId);
        return 0;
    }

    // Retourner la taille finale ecrite : header + ciphertext(+tag).
    return kEncryptedPacketHeaderSize + static_cast<size_t>(ciphertextLen);
}

// Callback ENet appele pour dechiffrer un datagramme entrant.
static size_t ENET_CALLBACK ServerNetworkEncryption_DecryptCallback(
    void* context,
    ENetPeer* peer,
    const enet_uint8* inData,
    size_t inLimit,
    enet_uint8* outData,
    size_t outLimit)
{
    // Contexte non utilise pour le moment.
    (void)context;

    // Si peer est NULL / invalide, on ne peut pas resoudre la cle de ce paquet.
    // En mode "encrypted packet", cela doit etre rejete.
    uint32_t connectionId = 0;
    if (!ServerNetworkEncryption_TryGetConnectionIdFromPeer(peer, connectionId))
    {
        return 0;
    }

    // Recuperer l'etat reseau global.
    const NetworkState& networkState = GetNetworkState();

    // Si le chiffrement n'est pas actif pour ce peer, on rejette ce paquet
    // marque "encrypted" (pas de passthrough aveugle).
    std::array<uint8_t, crypto_kx_SESSIONKEYBYTES> rxKey{};
    if (!ServerNetworkEncryption_TryLoadSessionKeyIfEnabled(networkState, connectionId, false, rxKey))
    {
        return 0;
    }

    // Validation minimale de format : header + tag obligatoires.
    if (inData == nullptr || inLimit < (kEncryptedPacketHeaderSize + kEncryptedPacketTagSize))
    {
        RCNET_log(
            RCNET_LOG_WARN,
            "[SERVER] [NETWORK_ENCRYPTION] [DECRYPT] - invalid packet size/pointer for connectionId=%u (inData=%p inLimit=%zu min=%zu)",
            connectionId,
            inData,
            inLimit,
            (kEncryptedPacketHeaderSize + kEncryptedPacketTagSize));
        sodium_memzero(rxKey.data(), rxKey.size());
        return 0;
    }

    // Validation de la signature de format.
    if (std::memcmp(inData, kEncryptedPacketMagic, kEncryptedPacketMagicSize) != 0)
    {
        sodium_memzero(rxKey.data(), rxKey.size());
        RCNET_log(
            RCNET_LOG_WARN,
            "[SERVER] [NETWORK_ENCRYPTION] [DECRYPT] - Invalid encrypted packet magic for connectionId=%u",
            connectionId);
        return 0;
    }

    // Extraire nonce + ciphertext.
    const enet_uint8* nonce = inData + kEncryptedPacketMagicSize;
    const enet_uint8* ciphertext = inData + kEncryptedPacketHeaderSize;
    const size_t ciphertextLen = inLimit - kEncryptedPacketHeaderSize;

    // Taille plaintext maximale theorique (ciphertext - tag).
    const size_t maximumPlaintextLen = ciphertextLen - kEncryptedPacketTagSize;
    if (maximumPlaintextLen > outLimit)
    {
        RCNET_log(
            RCNET_LOG_WARN,
            "[SERVER] [NETWORK_ENCRYPTION] [DECRYPT] - output buffer too small for connectionId=%u (maxPlaintext=%zu outLimit=%zu)",
            connectionId,
            maximumPlaintextLen,
            outLimit);
        sodium_memzero(rxKey.data(), rxKey.size());
        return 0;
    }

    // Dechiffrement + verification d'authenticite (AEAD).
    unsigned long long plaintextLen = 0;
    const int decryptResult = crypto_aead_xchacha20poly1305_ietf_decrypt(
        outData,
        &plaintextLen,
        nullptr, // nsec non utilise
        ciphertext,
        ciphertextLen,
        nullptr, // AD non utilisee
        0,
        nonce,
        rxKey.data());

    // Effacer la cle en memoire.
    sodium_memzero(rxKey.data(), rxKey.size());

    // Si verification/authentification echoue, on rejette le paquet.
    if (decryptResult != 0)
    {
        RCNET_log(
            RCNET_LOG_WARN,
            "[SERVER] [NETWORK_ENCRYPTION] [DECRYPT] - Authentication failed for connectionId=%u",
            connectionId);
        return 0;
    }

    // Retourner la taille plaintext produite.
    return static_cast<size_t>(plaintextLen);
}
void ServerNetworkEncryption_EnsureHostEncryptorInstalled(ENetHost* host)
{
    // Guard : host invalide => rien a faire.
    if (host == nullptr)
    {
        RCNET_log(
            RCNET_LOG_ERROR,
            "[SERVER] [NETWORK_ENCRYPTION] - host is null, encryptor install skipped");
        return;
    }

    // Instances statiques partagees pour eviter des reallocations/reinstallations inutiles.
    static ServerNetworkHostEncryptorContext encryptorContext{};
    static ENetEncryptor encryptor = {
        &encryptorContext,
        ServerNetworkEncryption_EncryptCallback,
        ServerNetworkEncryption_DecryptCallback,
        nullptr // pas de callback destroy necessaire ici
    };

    // Idempotence : si c'est deja cet encryptor qui est branche, sortir.
    if (host->encryptor.context == encryptor.context &&
        host->encryptor.encrypt == encryptor.encrypt &&
        host->encryptor.decrypt == encryptor.decrypt)
    {
        return;
    }

    // Brancher l'encryptor au host ENet.
    enet_host_encrypt(host, &encryptor);

    // Log d'information pour faciliter le diagnostic runtime.
    RCNET_log(RCNET_LOG_INFO, "[SERVER] [NETWORK_ENCRYPTION] - ENet host encryptor installed");
}
