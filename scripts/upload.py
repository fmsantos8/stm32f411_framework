#!/usr/bin/env python3
"""Flash the existing STM32F411 firmware via ST-LINK/V2."""

import argparse
import os
from pathlib import Path
import shutil
import sys

from _common import (
    add_build_argument, add_port_argument, entrypoint, positive_integer,
    read_build_description, run_command, serial_port,
)


def main():
    parser = argparse.ArgumentParser(description="Grava via ST-LINK/SWD; monitor serial opcional.")
    add_build_argument(parser)
    add_port_argument(parser)
    parser.add_argument("--tool", choices=("st-flash", "cube"), default="st-flash",
                        help="Ferramenta de gravação (padrão: st-flash).")
    parser.add_argument("--programmer", help="Caminho do executável da ferramenta selecionada.")
    parser.add_argument("--stlink-serial", help="Número de série do ST-Link, quando houver mais de um.")
    parser.add_argument("--mode", choices=("NORMAL", "UR", "HOTPLUG"), default="NORMAL")
    parser.add_argument(
        "--monitor-baud", type=positive_integer, default=os.environ.get("FW_MONITOR_BAUD", "115200"),
        help="Velocidade do monitor (padrão: FW_MONITOR_BAUD ou 115200).",
    )
    monitor_options = parser.add_mutually_exclusive_group()
    monitor_options.add_argument("--monitor", action="store_true", help="Abre o monitor da USART2 após gravar.")
    monitor_options.add_argument("--no-monitor", action="store_true", help="Encerra após gravar (padrão).")
    args = parser.parse_args()
    read_build_description(args.build_dir)
    firmware = args.build_dir / ("stm32f411_framework.bin" if args.tool == "st-flash"
                                 else "stm32f411_framework.elf")
    if not firmware.is_file():
        raise ValueError(f"{firmware.name} não encontrado. Execute scripts/build.py primeiro.")
    port = serial_port(args) if args.monitor else None
    if args.tool == "st-flash":
        if args.mode != "NORMAL":
            parser.error("--mode só é aceito com --tool cube.")
        programmer = args.programmer or os.environ.get("ST_FLASH", "st-flash")
        command = [programmer, "--reset"]
        if args.stlink_serial:
            command.extend(["--serial", args.stlink_serial])
        command.extend(["write", firmware, "0x08000000"])
    else:
        programmer = args.programmer or os.environ.get("STM32_PROGRAMMER_CLI", "STM32_Programmer_CLI")
        command = [programmer, "-c", "port=SWD", f"mode={args.mode}"]
        if args.stlink_serial:
            command.append(f"sn={args.stlink_serial}")
        command.extend(["-w", firmware, "-v", "-rst"])
    if shutil.which(programmer) is None:
        if args.tool == "st-flash":
            raise ValueError("st-flash não encontrado. No host Ubuntu, instale com "
                             "'sudo apt install stlink-tools' ou use --tool cube "
                             "se tiver STM32CubeProgrammer instalado.")
        raise ValueError(f"{programmer} não encontrado. Instale STM32CubeProgrammer "
                         "ou use --tool st-flash após instalar stlink-tools.")
    print(f"Gravando {firmware} via ST-Link...", flush=True)
    result = run_command(command)
    if result != 0:
        return result
    print("Gravação concluída.", flush=True)
    if not args.monitor:
        return 0
    monitor = Path(__file__).with_name("monitor_serial.py")
    return run_command([sys.executable, monitor, "--port", port, "--baud", str(args.monitor_baud)])


if __name__ == "__main__":
    raise SystemExit(entrypoint(main))
