#!/usr/bin/env python3
"""Run the port against real ST register layouts/drivers on Linux; no board needed."""
from pathlib import Path
import os
import subprocess
import tempfile
from sdk import drivers_directory

root = Path(__file__).resolve().parents[2]
sdk = drivers_directory()
port = root / "hal/ports/stm32f4"
test = Path(__file__).resolve().parent
sources = sorted(port.rglob("*.c")) + [root / "hal/memory/hal_memory.c", test / "test_port.c"]
sources += [sdk / f"STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_{module}.c"
            for module in ("gpio", "uart", "i2c", "i2c_ex", "tim")]
with tempfile.TemporaryDirectory(prefix="stm32f4-tests-") as directory:
    for device in ("STM32F411xE", "STM32F407xx"):
        binary = Path(directory) / device
        device_sources = list(sources)
        scenarios = ["normal", "no_memory", "clock_failure", "i2c_failure", "start_failure", "timpre"]
        if device == "STM32F411xE":
            device_sources.append(root / "boards/black_pill_stm32f411/bsp.c")
            scenarios.append("bsp")
        command = [os.environ.get("CC", "cc"), "-std=c11", "-g", "-O1", "-Wall", "-Wextra", "-Werror",
                   "-Wno-unused-parameter", "-Wno-pointer-to-int-cast", "-Wno-int-to-pointer-cast", "-Wno-overflow",
                   "-ffunction-sections", "-fdata-sections", "-fsanitize=undefined", f"-D{device}", "-DUSE_HAL_DRIVER"]
        for include in (test / "mocks", root / "config", root / "hal/interface", root / "hal/memory", root / "boards", port):
            command += ["-I", str(include)]
        for include in (sdk / "STM32F4xx_HAL_Driver/Inc", sdk / "CMSIS/Device/ST/STM32F4xx/Include"):
            command += ["-isystem", str(include)]
        command += [str(path) for path in device_sources]
        command += ["-Wl,--gc-sections", "-Wl,--wrap=HAL_I2C_Init", "-Wl,--wrap=HAL_TIM_Base_Start_IT",
                    "-Wl,--wrap=HAL_UART_Transmit", "-o", str(binary)]
        subprocess.run(command, check=True)
        for scenario in scenarios:
            subprocess.run([str(binary), scenario], check=True, timeout=10)
        print(f"{device}: {len(scenarios)} scenarios passed", flush=True)
