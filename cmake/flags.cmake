# Keep assertions enabled in every configuration: initialization uses assert().
# The toolchain clears CMake's default Release flags, including NDEBUG.
target_compile_options(${PROJECT_NAME} PRIVATE
    ${CPU_FLAGS}
    -Wall -Wextra -Wpedantic -Wno-unused-parameter
    -fstack-usage
    $<$<CONFIG:Debug>:-Og;-g3>
    $<$<CONFIG:Release>:-Os>
    $<$<COMPILE_LANGUAGE:ASM>:-x;assembler-with-cpp>
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    RTE_DEVICE_STARTUP_STM32F4XX
    $<$<CONFIG:Debug>:DEBUG>
)
target_link_options(${PROJECT_NAME} PRIVATE
    ${CPU_FLAGS}
    "-T${CMAKE_CURRENT_BINARY_DIR}/stm32f411xe_flash.ld"
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.map"
    --specs=nano.specs
    --specs=nosys.specs
    -Wl,--gc-sections
    -Wl,-z,max-page-size=8
    -Wl,--print-memory-usage
)
