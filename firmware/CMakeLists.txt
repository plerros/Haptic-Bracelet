# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(firmware C CXX ASM)

# Link Time Optimization
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_SUPPORT_OUTPUT)
if (IPO_SUPPORTED)
    #set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else ()
    message(WARNING "LTO is not supported: ${IPO_SUPPORT_OUTPUT}")
endif ()

add_compile_options(-Wall -Wextra)

# re-run cmake if config files change
function(watch)
    set_property(
        DIRECTORY
        APPEND
        PROPERTY CMAKE_CONFIGURE_DEPENDS ${ARGV}
    )
endfunction()
watch("${CMAKE_CURRENT_SOURCE_DIR}/config.h")
watch("${CMAKE_CURRENT_SOURCE_DIR}/config_adv.h")

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(firmware
    src/main.c
    src/analog/analog.c
    src/bluetooth/btstack_main.c
    src/digital/digital.c
    src/led/led.c
    src/motor/motor.c
    src/npf_interface/npf_interface.c)

pico_set_program_name(firmware "firmware")
pico_set_program_version(firmware "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(firmware 0)
pico_enable_stdio_usb(firmware 1)

# Add the standard library to the build
target_link_libraries(firmware
    hardware_adc
    hardware_gpio
    hardware_pwm
    pico_btstack_classic
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    pico_stdlib)

# Add the standard include files to the build
target_include_directories(firmware PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    .
    src
    src/analog
    src/bluetooth
    src/digital
    src/led
    src/motor
    src/npf_interface
    lib)

pico_add_extra_outputs(firmware)

target_compile_definitions(firmware
    PRIVATE PICO_PANIC_FUNCTION=0
    PRIVATE CYW43_ENABLE_BLUETOOTH=1
)
