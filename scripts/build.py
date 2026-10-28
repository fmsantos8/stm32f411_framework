#!/usr/bin/env python3
"""Build the STM32F411 firmware using CMake and ARM GCC."""

import argparse
import os
import time

from _common import PROJECT_ROOT, entrypoint, project_path, run_command


def main():
    parser = argparse.ArgumentParser(description="Compila o firmware STM32F411 com CMake e ARM GCC.")
    parser.add_argument("--config", choices=("Debug", "Release"), default="Debug")
    parser.add_argument("-B", "--build-dir", type=project_path, help="Diretório de build (padrão: build/<config>).")
    parser.add_argument("-D", "--define", action="append", default=[], metavar="NAME=VALUE")
    parser.add_argument("--fresh", action="store_true", help="Recria o cache do CMake; requer CMake >= 3.24.")
    args = parser.parse_args()
    build_dir = args.build_dir or PROJECT_ROOT / "build" / args.config
    cmake = os.environ.get("CMAKE", "cmake")
    configure = [cmake, "-S", PROJECT_ROOT, "-B", build_dir, "-G", "Ninja", f"-DCMAKE_BUILD_TYPE={args.config}"]
    if args.fresh:
        configure.append("--fresh")
    for definition in args.define:
        if "=" not in definition or not definition.split("=", 1)[0]:
            parser.error("Cada definição deve usar NAME=VALUE.")
        configure.append("-D" + definition)

    print(f"Compilando em {build_dir}...", flush=True)
    start = time.monotonic()
    result = run_command(configure)
    if result == 0:
        result = run_command([cmake, "--build", build_dir, "--parallel"])
    status = "concluída" if result == 0 else "falhou"
    print(f"Compilação {status} ({time.monotonic() - start:.2f} s).", flush=True)
    return result


if __name__ == "__main__":
    raise SystemExit(entrypoint(main))
