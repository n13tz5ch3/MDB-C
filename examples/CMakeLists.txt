# Example Applications

# MDB Bus Sniffer Application
add_executable(mdb_sniffer_app sniffer.c)
target_link_libraries(mdb_sniffer_app PRIVATE mdb_core)
set_target_properties(mdb_sniffer_app PROPERTIES OUTPUT_NAME mdb_sniffer)

# MDB Protocol Test
add_executable(mdb_test test_protocol.c)
target_link_libraries(mdb_test PRIVATE mdb_core)

# Threaded Example (uses callback interface)
add_executable(mdb_threaded threaded_example.c)
target_link_libraries(mdb_threaded PRIVATE mdb_core_callback mdb_hal pthread)
message(STATUS "  - mdb_threaded (callback-based with pthread)")

# MDB 4.3 Basket Shopping Demo
add_executable(mdb43_basket_demo mdb43_basket_demo.c)
target_link_libraries(mdb43_basket_demo PRIVATE mdb_core)
message(STATUS "  - mdb43_basket_demo (MDB 4.3 features demonstration)")

# Installation
install(TARGETS mdb_sniffer_app mdb_test mdb_threaded mdb43_basket_demo
    RUNTIME DESTINATION bin
)
