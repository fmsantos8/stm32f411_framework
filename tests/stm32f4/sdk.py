"""Locate the external STM32CubeF4 installation shared with the CMake build."""
from pathlib import Path
import os

HAL_MODULES = ("hal", "hal_cortex", "hal_rcc", "hal_rcc_ex", "hal_pwr", "hal_pwr_ex",
               "hal_flash", "hal_flash_ex", "hal_flash_ramfunc", "hal_gpio", "hal_dma",
               "hal_dma_ex", "hal_tim", "hal_tim_ex", "hal_uart", "hal_i2c", "hal_i2c_ex")


def drivers_directory():
    directory = os.environ.get("STM32F4_SDK_DIR")
    if not directory:
        raise SystemExit("Set STM32F4_SDK_DIR to the external STM32CubeF4 package root; see README.md")
    drivers = Path(directory).expanduser().resolve() / "Drivers"
    for required in ("STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h", "CMSIS/Include/core_cm4.h",
                     "CMSIS/Device/ST/STM32F4xx/Include/stm32f411xe.h",
                     "CMSIS/Device/ST/STM32F4xx/Include/stm32f407xx.h",
                     "CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c"):
        if not (drivers / required).is_file():
            raise SystemExit(f"Incomplete STM32CubeF4 SDK: {drivers / required} is missing; see README.md")
    return drivers
