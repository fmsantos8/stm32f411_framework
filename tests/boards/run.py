#!/usr/bin/env python3
"""Check BSP construction order, resource publication and failure lifecycle."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
test = Path(__file__).resolve().parent
boards = sorted((root / "boards").glob("*/bsp.c"))
if not boards:
    raise SystemExit("No board implementations found under boards/.")
with tempfile.TemporaryDirectory(prefix="stm32f4-bsp-tests-") as directory:
    for source in boards:
        binary = Path(directory) / source.parent.name
        subprocess.run([
            os.environ.get("CC", "cc"), "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
            "-fsanitize=undefined", "-I", str(root / "boards"), "-I", str(root / "hal/interface"),
            str(source), str(test / "test_bsp.c"), "-o", str(binary),
        ], check=True)
        for failed_step in range(6):
            subprocess.run([str(binary), str(failed_step)], check=True, timeout=10)
        print(f"{source.parent.name}: 6 construction/failure scenarios passed", flush=True)
