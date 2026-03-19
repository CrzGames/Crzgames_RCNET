# Secure-Session: ce que le client doit pinner

## Objectif
Le client pin la cle publique Ed25519 du serveur pour verifier les attestations secure-session.

## Cote serveur - DEV
- Aucune variable d'environnement obligatoire pour la cle Ed25519 (seed hardcodee en DEV).
- Pour NATS en DEV, les cles NKey sont aussi hardcodees dans la config serveur.

## Cote client - DEV
La cle publique Ed25519 du serveur est hardcodee dans:
`example-client/include/network/protocol/secure_session.h`
- `CLIENT_SECURE_SESSION_SERVER_ED25519_PUBLIC_KEY_HEX_DEV`

## Cote serveur - STAGING / PRODUCTION
Variables d'environnement obligatoires:
- `SERVER_ED25519_PRIVATE_SEED_HEX`
- `NATS_NKEY_PUBLIC_KEY`
- `NATS_NKEY_PRIVATE_KEY`

## Cote client - STAGING / PRODUCTION
La cle publique Ed25519 du serveur est hardcodee selon l'environnement de build dans:
`example-client/include/network/protocol/secure_session.h`
- `CLIENT_SECURE_SESSION_SERVER_ED25519_PUBLIC_KEY_HEX_STAGING`
- `CLIENT_SECURE_SESSION_SERVER_ED25519_PUBLIC_KEY_HEX_PRODUCTION`

## Rotation des cles Ed25519

### Generer un nouveau SERVER_ED25519_PRIVATE_SEED_HEX et SERVER_ED25519_PUBLIC_KEY_HEX
```bash
# Windows (PowerShell)
.\build-scripts\crypto\generate-server-ed25519-signing-keys.bat

# Linux/macOS (Bash)
chmod +x ./build-scripts/crypto/generate-server-ed25519-signing-keys.sh
./build-scripts/crypto/generate-server-ed25519-signing-keys.sh
```

Le script affiche:
```bash
SERVER_ED25519_PRIVATE_SEED_HEX=<hex_64_chars>
SERVER_ED25519_PUBLIC_KEY_HEX=<hex_64_chars>
```

### Mise a jour a faire apres rotation
1. Cote serveur:
STAGING/PROD: mettre a jour `SERVER_ED25519_PRIVATE_SEED_HEX` dans les variables d'environnement.
2. Cote client:
mettre a jour la cle publique hardcodee dans `secure_session.h` pour l'environnement cible.

## Mise a jour du bundle CA embarque (TLS libcurl)

Si besoin, regenerer `ca_bundle_pem.h` depuis un `cacert.pem` a jour:

```bash
cd build-scripts/curl-cacertpem/
python generate_ca_bundle_header.py cacert.pem ../../example-client/include/services/http/tls/ca_bundle_pem.h
```

Source recommandee du PEM:
- https://curl.se/ca/cacert.pem