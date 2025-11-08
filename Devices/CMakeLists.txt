# Device Libraries
# Each device type is built as a separate library

message(STATUS "Building MDB device libraries...")

# VMC - Vending Machine Controller (Master Device)
# Note: Implementation stub - full implementation pending
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Vending Machine Controller/VMC.h")
    add_library(mdb_vmc INTERFACE)
    target_include_directories(mdb_vmc INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/Vending Machine Controller"
    )
    target_link_libraries(mdb_vmc INTERFACE mdb_core)
    message(STATUS "  - mdb_vmc (headers only - MDB Master)")
endif()

# Sniffer Device (passive bus monitor)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Sniffer/Sniffer.c")
    add_library(mdb_sniffer STATIC
        Sniffer/Sniffer.c
    )
    target_include_directories(mdb_sniffer PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Sniffer
    )
    target_link_libraries(mdb_sniffer PUBLIC mdb_core)
    message(STATUS "  - mdb_sniffer")
endif()

# Bill Validator Device (Section 6)
# Note: Implementation stub - full implementation pending
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Bill Validator/Bill_Validator.h")
    # Header-only for now, implementation can be added later
    add_library(mdb_bill_validator INTERFACE)
    target_include_directories(mdb_bill_validator INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/Bill Validator"
    )
    target_link_libraries(mdb_bill_validator INTERFACE mdb_core)
    message(STATUS "  - mdb_bill_validator (headers only)")
endif()

# Coin Changer Device (Section 5)
# Note: Implementation stub - full implementation pending
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Coin Changer/Coin_Changer.h")
    add_library(mdb_coin_changer INTERFACE)
    target_include_directories(mdb_coin_changer INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/Coin Changer"
    )
    target_link_libraries(mdb_coin_changer INTERFACE mdb_core)
    message(STATUS "  - mdb_coin_changer (headers only)")
endif()

# Cashless Payment Device (Section 7 - MDB 4.3)
# Note: Implementation stub - full implementation pending
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Cashless Payment Device/Cashless.h")
    add_library(mdb_cashless INTERFACE)
    target_include_directories(mdb_cashless INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/Cashless Payment Device"
    )
    target_link_libraries(mdb_cashless INTERFACE mdb_core)
    message(STATUS "  - mdb_cashless (headers only - MDB 4.3 ready)")
endif()

# Installation
if(TARGET mdb_sniffer)
    install(TARGETS mdb_sniffer
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
    )
    install(FILES Sniffer/Sniffer.h
        DESTINATION include/mdb/devices
    )
endif()

# Install device headers
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/"
    DESTINATION include/mdb/devices
    FILES_MATCHING PATTERN "*.h"
)