/**
 * @file test_protocol.c
 * @brief MDB Protocol Test - Basic functionality test
 *
 * Tests basic MDB protocol functions without requiring hardware.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "MDB_Linux.h"
#include "Communication_Format.h"
#include "PreProcessors.h"

/**
 * @brief Test parity calculation
 */
void test_parity(void) {
    printf("Testing parity calculation...\n");

    /* Test cases: byte -> expected odd parity */
    struct {
        uint8_t byte;
        uint8_t expected_parity;
    } tests[] = {
        {0x00, 1},  /* 0 bits set -> odd parity = 1 */
        {0x01, 0},  /* 1 bit set  -> odd parity = 0 */
        {0x03, 1},  /* 2 bits set -> odd parity = 1 */
        {0x07, 0},  /* 3 bits set -> odd parity = 0 */
        {0xFF, 1},  /* 8 bits set -> odd parity = 1 */
        {0xAA, 1},  /* 4 bits set -> odd parity = 1 */
        {0x55, 1},  /* 4 bits set -> odd parity = 1 */
    };

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        uint8_t result = parityBitCalc(tests[i].byte);
        if (result == tests[i].expected_parity) {
            printf("  [PASS] 0x%02X -> parity %d\n",
                   tests[i].byte, result);
        } else {
            printf("  [FAIL] 0x%02X -> expected %d, got %d\n",
                   tests[i].byte, tests[i].expected_parity, result);
        }
    }
}

/**
 * @brief Test block clear function
 */
void test_clear_block(void) {
    printf("\nTesting block clear...\n");

    /* Fill block with data */
    for (int i = 0; i < 35; i++) {
        block[i].whole = 0x1FF;  /* All bits set */
    }

    /* Clear block */
    clearBlock();

    /* Verify all cleared */
    int passed = 1;
    for (int i = 0; i < 35; i++) {
        if (block[i].whole != 0) {
            printf("  [FAIL] block[%d] = 0x%04X (expected 0x0000)\n",
                   i, block[i].whole);
            passed = 0;
        }
    }

    if (passed) {
        printf("  [PASS] Block cleared successfully\n");
    }
}

/**
 * @brief Test 9-bit data structure
 */
void test_nine_bit_union(void) {
    printf("\nTesting 9-bit data union...\n");

    union nineBit test;

    /* Test 1: Set data and mode separately */
    test.part.data = 0x55;
    test.part.mode = 0;
    if (test.whole == 0x0055) {
        printf("  [PASS] Data 0x55, mode 0 -> 0x%04X\n", test.whole);
    } else {
        printf("  [FAIL] Expected 0x0055, got 0x%04X\n", test.whole);
    }

    /* Test 2: Set mode bit */
    test.part.data = 0xAA;
    test.part.mode = 1;
    if (test.whole == 0x01AA) {
        printf("  [PASS] Data 0xAA, mode 1 -> 0x%04X\n", test.whole);
    } else {
        printf("  [FAIL] Expected 0x01AA, got 0x%04X\n", test.whole);
    }

    /* Test 3: Set whole value */
    test.whole = 0x0142;
    if (test.part.data == 0x42 && test.part.mode == 1) {
        printf("  [PASS] Whole 0x0142 -> data 0x%02X, mode %d\n",
               test.part.data, test.part.mode);
    } else {
        printf("  [FAIL] Expected data 0x42 mode 1, got data 0x%02X mode %d\n",
               test.part.data, test.part.mode);
    }
}

/**
 * @brief Test address decoding
 */
void test_address_decode(void) {
    printf("\nTesting address decoding...\n");

    struct {
        uint8_t addr;
        const char *name;
    } tests[] = {
        {VMC_ADDRESS, "VMC"},
        {CHANGER_ADDRESS, "Changer"},
        {CASHLESS1_ADDRESS, "Cashless1"},
        {BILL_VALIDATOR_ADDRESS, "Bill Validator"},
        {CASHLESS2_ADDRESS, "Cashless2"},
    };

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        printf("  [INFO] Address 0x%02X = %s\n", tests[i].addr, tests[i].name);
    }
}

/**
 * @brief Main function
 */
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("MDB Protocol Test Suite\n");
    printf("=======================\n\n");

    /* Run tests */
    test_parity();
    test_clear_block();
    test_nine_bit_union();
    test_address_decode();

    printf("\nAll tests completed.\n");
    printf("\nNote: This is a basic test that doesn't require hardware.\n");
    printf("For full testing, use 'mdb_sniffer' with actual MDB hardware.\n");

    return 0;
}
