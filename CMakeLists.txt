cmake_minimum_required(VERSION 3.10)

project(MDB-C VERSION 4.3.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DMDB_DEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# Options
option(MDB_VERSION_43 "Enable MDB 4.3 features" ON)
option(MDB_VERBOSE_LOGGING "Enable verbose logging" OFF)
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_TESTS "Build test suite" OFF)

# Default TTY device
if(NOT DEFINED MDB_TTY_DEVICE)
    set(MDB_TTY_DEVICE "/dev/ttyUSB0" CACHE STRING "Default MDB serial device path")
endif()

# Add definitions based on options
if(MDB_VERSION_43)
    add_definitions(-DMDB_VERSION_43)
endif()

if(MDB_VERBOSE_LOGGING)
    add_definitions(-DMDB_VERBOSE_LOGGING)
endif()

add_definitions(-DMDB_TTY_DEVICE="${MDB_TTY_DEVICE}")

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Protocol_Files
    ${CMAKE_SOURCE_DIR}/Devices
)

# Protocol Files Library
set(PROTOCOL_SOURCES
    Protocol_Files/Communication_Format.c
    Protocol_Files/Bus_Timing.c
)

add_library(mdb_protocol STATIC ${PROTOCOL_SOURCES})

# Linux Hardware Abstraction Layer
set(HAL_SOURCES
    MDB_Linux.c
)

add_library(mdb_hal STATIC ${HAL_SOURCES})

# Link rt library on Linux (for clock_gettime), not needed on macOS
if(UNIX AND NOT APPLE)
    target_link_libraries(mdb_hal PRIVATE rt)
endif()

# Callback-Based Hardware Abstraction Layer (Thread-Safe)
set(CALLBACK_HAL_SOURCES
    MDB_Callbacks.c
)

add_library(mdb_hal_callback STATIC ${CALLBACK_HAL_SOURCES})
target_include_directories(mdb_hal_callback PUBLIC ${CMAKE_SOURCE_DIR})

# Core MDB Library (combines protocol + HAL)
add_library(mdb_core INTERFACE)
target_link_libraries(mdb_core INTERFACE mdb_protocol mdb_hal)

# Core MDB Library with Callback HAL (for threaded applications)
add_library(mdb_core_callback INTERFACE)
target_link_libraries(mdb_core_callback INTERFACE mdb_protocol mdb_hal_callback)

# Device Libraries (subdirectories)
add_subdirectory(Devices)

# Example applications
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS mdb_protocol mdb_hal mdb_hal_callback
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY Protocol_Files/
    DESTINATION include/mdb
    FILES_MATCHING PATTERN "*.h"
)

install(FILES
    MDB_Linux.h
    MDB_Callbacks.h
    DESTINATION include/mdb
)

# Print configuration summary
message(STATUS "")
message(STATUS "MDB-C Configuration Summary:")
message(STATUS "  Version:              ${PROJECT_VERSION}")
message(STATUS "  Build type:           ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Compiler:           ${CMAKE_C_COMPILER}")
message(STATUS "  MDB 4.3 features:     ${MDB_VERSION_43}")
message(STATUS "  Verbose logging:      ${MDB_VERBOSE_LOGGING}")
message(STATUS "  Default TTY device:   ${MDB_TTY_DEVICE}")
message(STATUS "  Build examples:       ${BUILD_EXAMPLES}")
message(STATUS "  Build tests:          ${BUILD_TESTS}")
message(STATUS "")