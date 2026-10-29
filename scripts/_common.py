"""Shared paths, arguments and process helpers for the project scripts."""

import argparse
import os
from pathlib import Path
import subprocess
import sys


PROJECT_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_BUILD_DIR = PROJECT_ROOT / "build" / "Debug"


def positive_integer(value):
    try:
        number = int(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("Use um número inteiro positivo.") from error
    if number <= 0:
        raise argparse.ArgumentTypeError("Use um número inteiro positivo.")
    return number


def project_path(value):
    path = Path(value).expanduser()
    if not path.is_absolute():
        path = PROJECT_ROOT / path
    return Path(os.path.abspath(path))


def add_build_argument(parser):
    parser.add_argument(
        "-B", "--build-dir", type=project_path, default=DEFAULT_BUILD_DIR,
        help="Diretório de build; caminhos relativos partem da raiz do projeto (padrão: build/Debug).",
    )


def add_port_argument(parser):
    parser.add_argument("port", nargs="?", help="Porta serial, por exemplo /dev/ttyUSB0 ou COM5.")
    parser.add_argument("-p", "--port", dest="port_option", help="Alternativa ao argumento posicional PORT.")


def serial_port(args):
    if args.port and args.port_option:
        raise ValueError("Informe a porta apenas uma vez: PORT ou --port.")
    configured = args.port_option or args.port or os.environ.get("FW_SERIAL_PORT")
    if configured:
        return configured
    from serial.tools import list_ports
    ports = sorted(port.device for port in list_ports.comports() if port.vid is not None)
    if len(ports) == 1:
        return ports[0]
    if ports:
        raise ValueError("Mais de uma porta USB disponível: " + ", ".join(ports) + ". Informe --port.")
    raise ValueError("Nenhuma porta USB detectada. Informe --port ou defina FW_SERIAL_PORT.")


def read_build_description(build_dir):
    values = {}
    for line in (build_dir / "CMakeCache.txt").read_text(encoding="utf-8").splitlines():
        if not line or line.startswith(("#", "//")) or "=" not in line:
            continue
        key, value = line.split("=", 1)
        values[key.split(":", 1)[0]] = value
    home = values.get("CMAKE_HOME_DIRECTORY")
    if not home or Path(home).resolve() != PROJECT_ROOT:
        raise ValueError("O diretório de build pertence a outro projeto.")
    if values.get("CMAKE_PROJECT_NAME") != "stm32f411_framework":
        raise ValueError("O diretório de build não foi gerado para stm32f411_framework.")
    return values


def run_command(command, cwd=PROJECT_ROOT):
    try:
        result = subprocess.run([str(part) for part in command], cwd=cwd, check=False)
        return result.returncode if result.returncode >= 0 else 128 - result.returncode
    except KeyboardInterrupt:
        print("\nOperação interrompida.", file=sys.stderr)
        return 130


def entrypoint(main):
    try:
        return main()
    except KeyboardInterrupt:
        print("\nOperação interrompida.", file=sys.stderr)
        return 130
    except (OSError, ValueError, ImportError) as error:
        print(f"Erro: {error}", file=sys.stderr)
        if isinstance(error, ImportError):
            print("Instale pyserial para o monitor: python3 -m pip install pyserial.", file=sys.stderr)
        return 1
