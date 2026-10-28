#!/usr/bin/env python3
"""Flash the existing STM32F411 ELF via ST-Link and optionally monitor UART."""

import argparse
import os
from pathlib import Path
import sys

from _common import (
    add_build_argument, add_port_argument, entrypoint, positive_integer,
    read_build_description, run_command, serial_port,
)


def main():
    parser = argparse.ArgumentParser(description="Grava via ST-Link/SWD e abre o monitor da USART2.")
    add_build_argument(parser)
    add_port_argument(parser)
    parser.add_argument("--programmer", default=os.environ.get("STM32_PROGRAMMER_CLI", "STM32_Programmer_CLI"))
    parser.add_argument("--stlink-serial", help="Número de série do ST-Link, quando houver mais de um.")
    parser.add_argument("--mode", choices=("NORMAL", "UR", "HOTPLUG"), default="NORMAL")
    parser.add_argument(
        "--monitor-baud", type=positive_integer, default=os.environ.get("FW_MONITOR_BAUD", "115200"),
        help="Velocidade do monitor (padrão: FW_MONITOR_BAUD ou 115200).",
    )
    parser.add_argument("--no-monitor", action="store_true", help="Encerra após gravar o firmware.")
    args = parser.parse_args()
    read_build_description(args.build_dir)
    firmware = args.build_dir / "stm32f411_framework.elf"
    if not firmware.is_file():
        raise ValueError("ELF não encontrado. Execute scripts/build.py primeiro.")
    port = None if args.no_monitor else serial_port(args)
    command = [args.programmer, "-c", "port=SWD", f"mode={args.mode}"]
    if args.stlink_serial:
        command.append(f"sn={args.stlink_serial}")
    command.extend(["-w", firmware, "-v", "-rst"])
    print(f"Gravando {firmware} via ST-Link...", flush=True)
    result = run_command(command)
    if result != 0:
        return result
    print("Gravação concluída.", flush=True)
    if args.no_monitor:
        return 0
    monitor = Path(__file__).with_name("monitor_serial.py")
    return run_command([sys.executable, monitor, "--port", port, "--baud", str(args.monitor_baud)])


if __name__ == "__main__":
    raise SystemExit(entrypoint(main))
