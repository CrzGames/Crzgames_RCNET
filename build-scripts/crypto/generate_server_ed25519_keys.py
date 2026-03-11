#!/usr/bin/env python3
"""Generate server Ed25519 signing values for secure-session."""

from __future__ import annotations

import os
import sys

def _derive_public_key(seed: bytes) -> bytes:
    try:
        from nacl.bindings import crypto_sign_seed_keypair

        public_key, _secret_key = crypto_sign_seed_keypair(seed)
        return public_key
    except Exception:
        pass

    try:
        from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
        from cryptography.hazmat.primitives.serialization import Encoding, PublicFormat

        private_key = Ed25519PrivateKey.from_private_bytes(seed)
        return private_key.public_key().public_bytes(Encoding.Raw, PublicFormat.Raw)
    except Exception as exc:  # pragma: no cover - runtime dependency check
        print("[ERROR] Missing dependency: PyNaCl or cryptography.", file=sys.stderr)
        print("Install one of:", file=sys.stderr)
        print("  pip install pynacl", file=sys.stderr)
        print("  pip install cryptography", file=sys.stderr)
        print(f"Details: {exc}", file=sys.stderr)
        raise SystemExit(1)


def main() -> int:
    seed = os.urandom(32)
    public_key = _derive_public_key(seed)

    print(f"SERVER_ED25519_PRIVATE_SEED_HEX={seed.hex()}")
    print(f"SERVER_ED25519_PUBLIC_KEY_HEX={public_key.hex()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
