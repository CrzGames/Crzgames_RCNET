# Secure-Session: Ce Que Le Client Doit Pinner

## Cote Server variable d'environnement obligatoire pour staging/prod : 
SERVER_ED25519_PRIVATE_SEED_HEX
NATS_NKEY_PUBLIC_KEY
NATS_NKEY_PRIVATE_KEY

## Cote client variable d'environnement obligatoire pour staging/prod :
CLIENT_SECURE_SESSION_SERVER_ED25519_PUBLIC_KEY_HEX (clé public : SERVER_ED25519_PUBLIC_KEY_HEX)

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