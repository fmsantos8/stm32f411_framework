# Framework port and its SDK dependency. Keep this outside Cube-managed files.
set(STM32F4_SDK_DIR "$ENV{STM32F4_SDK_DIR}" CACHE PATH "External STM32CubeF4 package root")
if(NOT STM32F4_SDK_DIR)
    message(FATAL_ERROR
        "Set STM32F4_SDK_DIR to an external STM32CubeF4 installation. "
        "Use the devcontainer or tools/install_stm32f4_sdk.py; see README.md.")
endif()
get_filename_component(STM32F4_SDK_DIR "${STM32F4_SDK_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(STM32F4_DRIVERS_DIR "${STM32F4_SDK_DIR}/Drivers")
set(STM32F4_HAL_DIR "${STM32F4_DRIVERS_DIR}/STM32F4xx_HAL_Driver")
set(STM32F4_DEVICE_DIR "${STM32F4_DRIVERS_DIR}/CMSIS/Device/ST/STM32F4xx")
foreach(required_file
        "${STM32F4_HAL_DIR}/Inc/stm32f4xx_hal.h"
        "${STM32F4_DRIVERS_DIR}/CMSIS/Include/core_cm4.h"
        "${STM32F4_DEVICE_DIR}/Include/stm32f411xe.h"
        "${STM32F4_DEVICE_DIR}/Include/stm32f407xx.h"
        "${STM32F4_DEVICE_DIR}/Source/Templates/system_stm32f4xx.c")
    if(NOT EXISTS "${required_file}")
        message(FATAL_ERROR
            "Incomplete STM32CubeF4 SDK: ${required_file} is missing. "
            "Initialize the CMSIS device and HAL submodules, or use tools/install_stm32f4_sdk.py.")
    endif()
endforeach()
set(STM32F4_PORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/hal/ports/stm32f4")

target_compile_features(${PROJECT_NAME} PRIVATE c_std_11)
target_compile_definitions(${PROJECT_NAME} PRIVATE STM32F411xE USE_HAL_DRIVER)
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/hal/interface"
    "${CMAKE_CURRENT_SOURCE_DIR}/hal/memory"
    "${STM32F4_PORT_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/config"
)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    "${STM32F4_HAL_DIR}/Inc"
    "${STM32F4_DRIVERS_DIR}/CMSIS/Include"
    "${STM32F4_DEVICE_DIR}/Include"
)
target_sources(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/hal/memory/hal_memory.c"
    "${STM32F4_PORT_DIR}/hal_stm32f4.c"
    "${STM32F4_PORT_DIR}/system/hal_system.c"
    "${STM32F4_PORT_DIR}/gpio/hal_gpio.c"
    "${STM32F4_PORT_DIR}/uart/hal_uart.c"
    "${STM32F4_PORT_DIR}/i2c/hal_i2c.c"
    "${STM32F4_PORT_DIR}/timer/hal_timer.c"
    "${STM32F4_DEVICE_DIR}/Source/Templates/system_stm32f4xx.c"
)
foreach(module hal hal_cortex hal_rcc hal_rcc_ex hal_pwr hal_pwr_ex hal_flash
               hal_flash_ex hal_flash_ramfunc hal_gpio hal_dma hal_dma_ex
               hal_tim hal_tim_ex hal_uart hal_i2c hal_i2c_ex)
    target_sources(${PROJECT_NAME} PRIVATE "${STM32F4_HAL_DIR}/Src/stm32f4xx_${module}.c")
endforeach()
