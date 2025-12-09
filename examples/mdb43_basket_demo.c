/**
 * @file mdb43_basket_demo.c
 * @brief MDB 4.3 Basket/Multi-Vend Feature Demonstration
 *
 * This example demonstrates the MDB 4.3 basket shopping feature where
 * a user can select multiple items before completing payment.
 *
 * Features demonstrated:
 * - Basket transaction initiation
 * - Adding multiple items to basket
 * - Partial refund on dispensing failure
 * - Enhanced item numbers with PA101 encoding
 * - Remote vend requests
 */

/* Define POSIX features for clock_gettime, nanosleep, usleep, etc. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "MDB_Linux.h"
#include "VMC.h"
#include "Cashless.h"
#include "PreProcessors.h"

static volatile int running = 1;

/**
 * @brief Signal handler for graceful shutdown
 */
void signal_handler(int signum) {
    (void)signum;
    printf("\nShutting down...\n");
    running = 0;
}

/**
 * @brief Simulate dispensing a product
 * @return 1 if dispensed successfully, 0 if failed
 */
uint8_t dispense_product(uint16_t itemNumber) {
    printf("  [DISPENSE] Item #%u... ", itemNumber);
    fflush(stdout);

    /* Simulate dispensing time */
    usleep(500000);  /* 500ms */

    /* Simulate 90% success rate */
    uint8_t success = (rand() % 10) < 9;

    if (success) {
        printf("OK\n");
    } else {
        printf("FAILED\n");
    }

    return success;
}

/**
 * @brief MDB 4.3 Basket Shopping Demo
 */
void demo_basket_shopping(void) {
    printf("\n");
    printf("==============================================\n");
    printf("MDB 4.3 Basket Shopping Demo\n");
    printf("==============================================\n\n");

    CashlessState cashless;
    cashless_init(&cashless, CASHLESS1_ADDRESS);

    printf("Step 1: User taps card - Begin Session\n");
    printf("----------------------------------------\n");

    /* Simulate card with $10.00 */
    cashless.userFunds = 1000;  /* $10.00 in cents */
    cashless.sessionActive_f = 1;
    cashless.state = CASHLESS_STATE_IDLE;

    printf("Session started with $%.2f credit\n\n", cashless.userFunds / 100.0);

    printf("Step 2: VMC Initiates Basket Mode (MDB 4.3)\n");
    printf("----------------------------------------\n");

    vmc_beginBasket();
    cashless_beginBasket(&cashless);

    printf("Basket mode activated\n\n");

    /* Items user wants to purchase */
    struct {
        uint16_t itemNumber;
        uint16_t price;
        const char *name;
    } items[] = {
        {101, 150, "Coca-Cola"},
        {205, 175, "Snickers Bar"},
        {310, 200, "Doritos"}
    };

    printf("Step 3: User Selects Multiple Items\n");
    printf("----------------------------------------\n");

    for (size_t i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
        printf("[SELECT] %s (#%u) - $%.2f\n",
               items[i].name,
               items[i].itemNumber,
               items[i].price / 100.0);

        /* Add to basket */
        vmc_addToBasket(items[i].itemNumber, items[i].price);

        /* Simulate cashless approval */
        cashless.vendItemNumber = items[i].itemNumber;
        cashless.vendAmount = items[i].price;

        if (cashless.basketItemCount < 16) {
            cashless.basketItems[cashless.basketItemCount] = items[i].itemNumber;
            cashless.basketPrices[cashless.basketItemCount] = items[i].price;
            cashless.basketTotalValue += items[i].price;
            cashless.basketItemCount++;
        }
    }

    printf("\nBasket contains %u items, total: $%.2f\n\n",
           cashless.basketItemCount,
           cashless.basketTotalValue / 100.0);

    printf("Step 4: User Confirms Purchase\n");
    printf("----------------------------------------\n");

    printf("VMC dispenses items...\n\n");

    uint8_t successCount = 0;
    uint8_t failCount = 0;

    for (uint8_t i = 0; i < cashless.basketItemCount; i++) {
        if (dispense_product(cashless.basketItems[i])) {
            successCount++;
        } else {
            failCount++;
        }
    }

    printf("\nDispensing complete: %u success, %u failed\n\n", successCount, failCount);

    if (failCount > 0) {
        printf("Step 5: MDB 4.3 Partial Refund\n");
        printf("----------------------------------------\n");

        uint16_t refundAmount = cashless_calculatePartialRefund(&cashless, failCount);

        printf("Partial refund for %u failed items: $%.2f\n",
               failCount,
               refundAmount / 100.0);

        /* Issue refund */
        cashless.partialRefundAmount = refundAmount;
        cashless.partialRefundPending_f = 1;
        cashless.userFunds += refundAmount;

        printf("Refund issued - updated balance: $%.2f\n\n",
               cashless.userFunds / 100.0);

        /* VMC notifies cashless device */
        vmc_partialRefund(failCount);
    }

    printf("Step 6: Complete Basket Transaction\n");
    printf("----------------------------------------\n");

    /* Complete basket */
    cashless_completeBasket(&cashless);
    vmc_completeBasket();

    printf("Basket completed\n");
    printf("Final charge: $%.2f\n",
           (cashless.basketTotalValue - (failCount > 0 ? cashless.partialRefundAmount : 0)) / 100.0);
    printf("Remaining credit: $%.2f\n\n",
           cashless.userFunds / 100.0);
}

/**
 * @brief MDB 4.3 Remote Vend Demo
 */
void demo_remote_vend(void) {
    printf("\n");
    printf("==============================================\n");
    printf("MDB 4.3 Remote Vend Demo\n");
    printf("==============================================\n\n");

    printf("Scenario: User orders via mobile app\n\n");

    CashlessState cashless;
    cashless_init(&cashless, CASHLESS1_ADDRESS);

    printf("Step 1: Mobile App Payment\n");
    printf("----------------------------------------\n");

    cashless.userFunds = 250;  /* $2.50 pre-paid */
    cashless.sessionActive_f = 1;
    cashless.state = CASHLESS_STATE_IDLE;

    printf("Mobile payment: $%.2f\n", cashless.userFunds / 100.0);
    printf("Item ordered: Pepsi (#102)\n\n");

    printf("Step 2: Cashless Device Initiates Remote Vend (MDB 4.3)\n");
    printf("--------------------------------------------------------\n");

    cashless.remoteVendItemNumber = 102;
    cashless.remoteVendPrice = 150;  /* $1.50 */
    cashless.remoteVendActive_f = 1;

    printf("Remote vend request: Item #%u @ $%.2f\n\n",
           cashless.remoteVendItemNumber,
           cashless.remoteVendPrice / 100.0);

    printf("Step 3: VMC Processes Remote Vend\n");
    printf("----------------------------------------\n");

    if (dispense_product(cashless.remoteVendItemNumber)) {
        printf("Remote vend successful\n");
        cashless.userFunds -= cashless.remoteVendPrice;
    } else {
        printf("Remote vend failed - no charge\n");
    }

    printf("Remaining credit: $%.2f\n\n", cashless.userFunds / 100.0);

    cashless.remoteVendActive_f = 0;
}

/**
 * @brief MDB 4.3 Coupon Demo
 */
void demo_coupon_support(void) {
    printf("\n");
    printf("==============================================\n");
    printf("MDB 4.3 Coupon Support Demo\n");
    printf("==============================================\n\n");

    CashlessState cashless;
    cashless_init(&cashless, CASHLESS1_ADDRESS);

    printf("Step 1: User Has Digital Coupon\n");
    printf("----------------------------------------\n");

    printf("Coupon: $1.00 off any purchase\n");
    printf("Coupon ID: SUMMER2024\n\n");

    printf("Step 2: Cashless Device Applies Coupon (MDB 4.3)\n");
    printf("--------------------------------------------------\n");

    cashless.couponType = 0x01;  /* Discount coupon */
    cashless.couponValue = 100;  /* $1.00 off */
    const char *couponID = "SUMMER2024";
    memcpy(cashless.couponID, couponID, strlen(couponID));
    cashless.couponIDLength = strlen(couponID);

    printf("Coupon applied: +$%.2f credit\n", cashless.couponValue / 100.0);

    cashless.userFunds += cashless.couponValue;
    cashless.sessionActive_f = 1;

    printf("Total credit: $%.2f\n\n", cashless.userFunds / 100.0);

    printf("Step 3: User Makes Purchase\n");
    printf("----------------------------------------\n");

    uint16_t itemPrice = 150;  /* $1.50 */
    printf("Item price: $%.2f\n", itemPrice / 100.0);

    if (dispense_product(301)) {
        cashless.userFunds -= itemPrice;
        printf("Final cost with coupon: $%.2f\n", (itemPrice - cashless.couponValue) / 100.0);
        printf("Remaining credit: $%.2f\n\n", cashless.userFunds / 100.0);
    }
}

/**
 * @brief Main function
 */
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Seed random for demo */
    srand(time(NULL));

    printf("\n");
    printf("##############################################\n");
    printf("#                                            #\n");
    printf("#   MDB 4.3 Feature Demonstration           #\n");
    printf("#                                            #\n");
    printf("##############################################\n");

    printf("\nThis demo showcases MDB 4.3 features:\n");
    printf("  - Basket/Multi-Vend transactions\n");
    printf("  - Partial refunds on failed dispensing\n");
    printf("  - Remote vend requests\n");
    printf("  - Coupon support\n");
    printf("\nNOTE: This is a simulation. No actual hardware required.\n");

    /* Run demos */
    demo_basket_shopping();

    printf("\nPress Enter to continue to Remote Vend demo...\n");
    getchar();

    demo_remote_vend();

    printf("\nPress Enter to continue to Coupon demo...\n");
    getchar();

    demo_coupon_support();

    printf("\n");
    printf("==============================================\n");
    printf("Demo Complete!\n");
    printf("==============================================\n\n");

    printf("Key MDB 4.3 Features Demonstrated:\n");
    printf("  ✓ Basket transactions with multiple items\n");
    printf("  ✓ Partial refund calculation and processing\n");
    printf("  ✓ Remote vend initiation from cashless device\n");
    printf("  ✓ Digital coupon integration\n");
    printf("  ✓ Enhanced error handling\n\n");

    printf("For more information, see:\n");
    printf("  - MDB 4.3 Specification\n");
    printf("  - CLAUDE.md documentation\n");
    printf("  - Cashless.h/Cashless.c implementation\n\n");

    return 0;
}
