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
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(usb-mdio-adapter C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

if (TARGET tinyusb_device)
    add_executable(usb-mdio-adapter
        main.c
        usb_mvmdio.c
        mdio.c
    )

    # pull in common dependencies
    target_link_libraries(usb-mdio-adapter pico_stdlib)

    # enable usb output, disable uart output
    #pico_enable_stdio_usb(usb-mdio-adapter 1)
    #pico_enable_stdio_uart(usb-mdio-adapter 0)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(usb-mdio-adapter)

    # add url via pico_set_program_url
elseif(PICO_ON_DEVICE)
    message("Skipping usb-mdio-adapter because TinyUSB submodule is not initialized in the SDK")
endif()
