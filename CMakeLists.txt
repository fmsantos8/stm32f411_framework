cmake_minimum_required(VERSION 3.22)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/target.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/gcc-arm-none-eabi.cmake")

project(stm32f411_framework LANGUAGES C ASM)

add_executable(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    SUFFIX ".elf"
)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/flags.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/files.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/stm32f4.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/boards.cmake")
add_subdirectory(src)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary "$<TARGET_FILE:${PROJECT_NAME}>" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin"
    COMMAND ${CMAKE_OBJCOPY} -O ihex "$<TARGET_FILE:${PROJECT_NAME}>" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex"
    COMMAND ${CMAKE_SIZE} "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMENT "Generating STM32F411 binary and Intel HEX images"
    VERBATIM
)
