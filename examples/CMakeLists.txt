# Example Applications

# MDB Bus Sniffer Application
add_executable(mdb_sniffer_app sniffer.c)
target_link_libraries(mdb_sniffer_app PRIVATE mdb_core)
set_target_properties(mdb_sniffer_app PROPERTIES OUTPUT_NAME mdb_sniffer)

# MDB Protocol Test
add_executable(mdb_test test_protocol.c)
target_link_libraries(mdb_test PRIVATE mdb_core)

# Threaded Example (uses callback interface + direct HAL for serial thread)
add_executable(mdb_threaded threaded_example.c)
# Links with both callback (for MDB protocol) and HAL (for serial thread's mdb_init/close)
# Note: Uses --allow-multiple-definition to resolve symbol conflicts between HAL implementations
target_link_libraries(mdb_threaded PRIVATE mdb_protocol mdb_hal_callback mdb_hal pthread)
target_link_options(mdb_threaded PRIVATE "LINKER:--allow-multiple-definition")
message(STATUS "  - mdb_threaded (callback-based with pthread)")

# MDB 4.3 Basket Shopping Demo
# Note: Requires full implementation of Cashless.c and VMC.c (currently header-only)
# add_executable(mdb43_basket_demo mdb43_basket_demo.c)
# target_include_directories(mdb43_basket_demo PRIVATE
#     "${CMAKE_SOURCE_DIR}/Devices/Vending Machine Controller"
#     "${CMAKE_SOURCE_DIR}/Devices/Cashless Payment Device"
# )
# target_link_libraries(mdb43_basket_demo PRIVATE mdb_core)
# message(STATUS "  - mdb43_basket_demo (MDB 4.3 features demonstration)")

# Installation
install(TARGETS mdb_sniffer_app mdb_test mdb_threaded
    RUNTIME DESTINATION bin
)
