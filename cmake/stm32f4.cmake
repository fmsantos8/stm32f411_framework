# Framework port and its SDK dependency. Keep this outside Cube-managed files.
set(STM32F4_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lld/stm32f4" CACHE PATH "STM32F4 CMSIS/HAL root")
set(STM32F4_HAL_DIR "${STM32F4_SDK_DIR}/STM32F4xx_HAL_Driver")
set(STM32F4_DEVICE_DIR "${STM32F4_SDK_DIR}/CMSIS/Device/ST/STM32F4xx")
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
    "${STM32F4_SDK_DIR}/CMSIS/Include"
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
