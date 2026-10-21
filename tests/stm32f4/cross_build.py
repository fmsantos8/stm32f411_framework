#!/usr/bin/env python3
"""Compile/link every public port function for both MCUs using ARM GCC.

These are link checks without startup, not flashable images. F411 includes the
firmware entry point and BSP; F407 checks the port with a minimal entry point.
The root project's normal CMake presets build the actual F411 image.
"""
from pathlib import Path
import os
import subprocess
import tempfile
from sdk import HAL_MODULES, drivers_directory

root = Path(__file__).resolve().parents[2]
sdk = drivers_directory()
port = root / "hal/ports/stm32f4"
sources = sorted(port.rglob("*.c"))
sources += [sdk / f"STM32F4xx_HAL_Driver/Src/stm32f4xx_{module}.c" for module in HAL_MODULES]
sources += [root / "hal/memory/hal_memory.c", root / "Src/syscall.c", root / "Src/sysmem.c",
            sdk / "CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c"]
symbols = ("hal_system_init", "hal_system_seal", "hal_gpio_config", "hal_gpio_read", "hal_gpio_write",
           "hal_i2c_config", "hal_i2c_write", "hal_i2c_read", "hal_uart_config", "hal_uart_write",
           "hal_timer_config", "hal_timer_start", "TIM2_IRQHandler", "TIM3_IRQHandler", "TIM4_IRQHandler", "TIM5_IRQHandler")
compiler = os.environ.get("ARM_CC", "arm-none-eabi-gcc")
with tempfile.TemporaryDirectory(prefix="stm32f4-cross-") as directory:
    port_entry = Path(directory) / "port_main.c"
    port_entry.write_text("int main(void) { for (;;) {} }\n")
    for device in ("STM32F411xE", "STM32F407xx"):
        device_sources = list(sources)
        if device == "STM32F411xE":
            device_sources += [root / "Src/main.c", root / "boards/black_pill_stm32f411/bsp.c"]
        else:
            device_sources.append(port_entry)
        command = [compiler, "-std=c11", "-Os", "-g", "-mcpu=cortex-m4", "-mthumb", "-mfpu=fpv4-sp-d16",
                   "-mfloat-abi=hard", "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter",
                   "-ffunction-sections", "-fdata-sections", f"-D{device}", "-DUSE_HAL_DRIVER"]
        for include in (root / "config", root / "hal/interface", root / "hal/memory", root / "boards", port):
            command += ["-I", str(include)]
        for include in (sdk / "STM32F4xx_HAL_Driver/Inc", sdk / "CMSIS/Include", sdk / "CMSIS/Device/ST/STM32F4xx/Include"):
            command += ["-isystem", str(include)]
        command += [str(source) for source in device_sources]
        command += ["-nostartfiles", "--specs=nosys.specs", "-Wl,--gc-sections", "-Wl,-e,main",
                    "-T", str(root / "stm32f411xe_flash.ld")]
        command += [f"-Wl,-u,{symbol}" for symbol in symbols]
        command += ["-o", str(Path(directory) / f"{device}.elf")]
        subprocess.run(command, check=True)
        print(f"{device}: all public functions compiled and linked", flush=True)
