
## Secure-Session: Ce Que Le Client Doit Pinner

### Cote client (valeurs fixes / hardcodees)
1. `SERVER_ED25519_PUBLIC_KEY_HEX`
   - Cle publique Ed25519 du serveur.
   - Utilisee pour `crypto_sign_verify_detached(...)` sur
     `ServerSecureSessionHelloResponsePacketReliable::signature`.
2. Domaine de signature
   - Valeur actuelle: `SERVER_ED25519_DOMAIN_SECURE_SESSION_ATTESTATION_V1`
   - Doit etre strictement identique entre client et serveur pour verifier la signature.
3. Format secure-session attendu
   - Le client envoie `ClientSecureSessionHelloPacketReliable::clientNonce`.
   - Le serveur renvoie exactement cette valeur dans
     `ServerSecureSessionHelloResponsePacketReliable::clientNonceEcho`.
   - Taille nonce: `SERVER_SECURE_SESSION_CLIENT_NONCE_BYTES` (actuellement `16`).

### Cote serveur (secret)
1. `SERVER_ED25519_PRIVATE_SEED_HEX`
   - Seed privee Ed25519 (32 bytes, 64 caracteres hex).
   - A stocker dans un secret manager (ex: 1Password), jamais cote client.

### Lien entre les deux
- `SERVER_ED25519_PUBLIC_KEY_HEX` est derivee de `SERVER_ED25519_PRIVATE_SEED_HEX`.
- Le client ne doit connaitre que la cle publique (pinning), jamais la seed privee.

### Generer `SERVER_ED25519_PRIVATE_SEED_HEX` et `SERVER_ED25519_PUBLIC_KEY_HEX`
```bash
# Windows (PowerShell)
.\build-scripts\crypto\generate-server-ed25519-signing-keys.bat

# Linux/macOS (Bash)
chmod +x ./build-scripts/crypto/generate-server-ed25519-signing-keys.sh
./build-scripts/crypto/generate-server-ed25519-signing-keys.sh
```

Le script affiche directement dans le terminal:
```bash
SERVER_ED25519_PRIVATE_SEED_HEX=<hex_64_chars>
SERVER_ED25519_PUBLIC_KEY_HEX=<hex_64_chars>
```