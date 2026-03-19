from __future__ import annotations

import argparse
from pathlib import Path


def escape_cpp_bytes(data: bytes) -> str:
    out: list[str] = []
    for b in data:
        if b == 0x5C:       # backslash
            out.append(r'\\')
        elif b == 0x22:     # double quote
            out.append(r'\"')
        elif b == 0x0A:     # LF
            out.append(r'\n')
        elif b == 0x0D:     # CR
            out.append(r'\r')
        elif 0x20 <= b <= 0x7E:
            out.append(chr(b))
        else:
            out.append(f'\\x{b:02X}')
    return ''.join(out)


def generate_header(
    input_pem: Path,
    output_header: Path,
    var_name: str,
    size_name: str,
    namespace: str | None,
) -> tuple[int, int]:
    pem_bytes = input_pem.read_bytes()
    lines = pem_bytes.splitlines(keepends=True)

    with output_header.open('w', encoding='utf-8', newline='\n') as f:
        f.write('#pragma once\n\n')
        f.write('#include <cstddef> // std::size_t\n\n')
        f.write('// Bundle CA embarqué pour la vérification TLS libcurl en mode cross-platform.\n')
        f.write('// Source: https://curl.se/ca/cacert.pem\n')
        f.write('// Fichier généré automatiquement depuis un fichier PEM.\n')
        f.write('// Compatible MSVC: contenu découpé en plusieurs petits littéraux.\n\n')

        if namespace:
            for ns in namespace.split('::'):
                f.write(f'namespace {ns} {{\n')
            f.write('\n')

        f.write(f'inline constexpr char {var_name}[] =\n')
        for line in lines:
            f.write(f'    "{escape_cpp_bytes(line)}"\n')
        f.write('    ;\n\n')
        f.write(f'inline constexpr std::size_t {size_name} = sizeof({var_name}) - 1;\n')

        if namespace:
            f.write('\n')
            for ns in reversed(namespace.split('::')):
                f.write(f'}} // namespace {ns}\n')

    return len(pem_bytes), len(lines)


def main() -> None:
    parser = argparse.ArgumentParser(
        description='Génère un header C++ contenant un bundle PEM, compatible MSVC.'
    )
    parser.add_argument('input_pem', nargs='?', default='cacert.pem', help='Fichier PEM source')
    parser.add_argument('output_header', nargs='?', default='ca_bundle_pem.h', help='Header .h de sortie')
    parser.add_argument('--var-name', default='kClientHttpTlsCaBundlePem', help='Nom de la variable C++')
    parser.add_argument('--size-name', default='kClientHttpTlsCaBundlePemSize', help='Nom de la taille C++')
    parser.add_argument('--namespace', default=None, help='Namespace optionnel, ex: rcnet::http::tls')
    args = parser.parse_args()

    input_pem = Path(args.input_pem)
    output_header = Path(args.output_header)

    byte_count, literal_count = generate_header(
        input_pem=input_pem,
        output_header=output_header,
        var_name=args.var_name,
        size_name=args.size_name,
        namespace=args.namespace,
    )

    print(f'Generated: {output_header.resolve()}')
    print(f'Input bytes: {byte_count}')
    print(f'Output literals: {literal_count}')


if __name__ == '__main__':
    main()
