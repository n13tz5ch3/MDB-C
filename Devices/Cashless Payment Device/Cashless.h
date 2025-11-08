/**
 * @file Cashless.h
 * @brief MDB Cashless Payment Device Interface
 *
 * Implements MDB Protocol Section 7 - Cashless Device specifications.
 * Supports card readers, NFC, mobile payments, and MDB 4.3 features.
 */

#ifndef CASHLESS_HEADER
#define CASHLESS_HEADER

#include <stdint.h>
#include "Communication_Format.h"
#include "PreProcessors.h"

/**
 * @brief Cashless device state structure
 *
 * This structure follows MDB 4.3 specification for cashless devices.
 */
typedef struct {
    /* Device Configuration */
    uint8_t address_C;                    /* 0x10 (Cashless1) or 0x60 (Cashless2) */
    uint8_t featureLevel_C;               /* Level: 01, 02, or 03 (MDB 4.3) */
    uint8_t countryCurrencyCode_C[2];     /* ISO 4217 currency code */
    uint8_t scaleFactor_C[2];             /* Scaling factor */
    uint8_t decimalPlaces_C;              /* Decimal places */
    uint8_t maxResponseTime_C;            /* Max response time in units */
    uint8_t miscOptions_C;                /* Optional features bitmask */

    /* Session State */
    uint8_t sessionActive_f;              /* Session in progress */
    uint8_t vendApproved_f;               /* Vend approved by cashless */
    uint8_t vendDenied_f;                 /* Vend denied */
    uint16_t fundsAvailable;              /* Funds on card/account */
    uint16_t vendAmount;                  /* Current vend amount */
    uint16_t vendItemPrice;               /* Item price */

    /* MDB 4.3 Features */
    uint8_t remoteVendEnabled_s;          /* Remote vend support */
    uint8_t basketEnabled_s;              /* Basket/multi-vend support */
    uint8_t couponEnabled_s;              /* Coupon support */
    uint8_t partialRefundEnabled_s;       /* Partial refund support */
    uint8_t enhancedItemNumberEnabled_s;  /* PA101 item numbers */

    /* Basket Transaction (MDB 4.3) */
    uint8_t basketItemCount;              /* Items in basket */
    uint16_t basketItems[16];             /* Item numbers in basket */
    uint16_t basketPrices[16];            /* Price per item */
    uint16_t basketTotalValue;            /* Total basket value */
    uint8_t basketItemsDispensed;         /* Items successfully dispensed */

    /* Coupon Data (MDB 4.3) */
    uint8_t couponType;                   /* Coupon type code */
    uint16_t couponValue;                 /* Coupon value */
    uint8_t couponID[32];                 /* Coupon identifier */
    uint8_t couponIDLength;               /* Length of coupon ID */

    /* Remote Vend (MDB 4.3) */
    uint8_t remoteVendActive_f;           /* Remote vend in progress */
    uint16_t remoteVendItemNumber;        /* Item number for remote vend */
    uint16_t remoteVendPrice;             /* Price for remote vend */

    /* Status Flags */
    uint8_t disabled_f;                   /* Device disabled */
    uint8_t busy_f;                       /* Device busy */
    uint8_t defective_f;                  /* Hardware defect */
    uint8_t wasReset_f;                   /* Just reset */

    /* Revalue Support (Level 02+) */
    uint8_t revalueSupported_s;           /* Revalue capability */
    uint16_t revalueAmount;               /* Amount to add to card */
    uint8_t revalueLimitExceeded_f;       /* Revalue limit reached */

    /* FTL Support */
    uint8_t ftlSupported_s;               /* File Transfer Layer enabled */

    /* Display Data */
    uint8_t displayData[32];              /* Data to display to user */
    uint8_t displayDataLength;            /* Length of display data */

} CashlessState;

/* Global cashless device instance */
extern CashlessState cashless1;
extern CashlessState cashless2;

/**
 * @brief Initialize cashless device
 */
extern uint8_t cashless_init(CashlessState *dev, uint8_t address);

/**
 * @brief Handle RESET command (0x10 / 0x60)
 */
extern uint8_t cashless_reset(CashlessState *dev);

/**
 * @brief Handle SETUP command (0x11 / 0x61)
 */
extern uint8_t cashless_setup(CashlessState *dev);

/**
 * @brief Handle POLL command (0x12 / 0x62)
 */
extern uint8_t cashless_poll(CashlessState *dev);

/**
 * @brief Handle VEND command (0x13 / 0x63)
 */
extern uint8_t cashless_vend(CashlessState *dev);

/**
 * @brief Handle READER command (0x14 / 0x64)
 */
extern uint8_t cashless_reader(CashlessState *dev);

/**
 * @brief Handle REVALUE command (0x15 / 0x65)
 */
extern uint8_t cashless_revalue(CashlessState *dev);

/**
 * @brief Handle EXPANSION command (0x17 / 0x67)
 */
extern uint8_t cashless_expansion(CashlessState *dev);

/**
 * @brief Main processing loop
 */
extern uint8_t cashless_process(CashlessState *dev);

/**
 * @brief Begin vend session
 */
extern uint8_t cashless_beginSession(CashlessState *dev);

/**
 * @brief End vend session
 */
extern uint8_t cashless_endSession(CashlessState *dev);

/**
 * @brief Calculate partial refund (MDB 4.3)
 */
extern uint16_t cashless_calculatePartialRefund(CashlessState *dev);

#endif /* CASHLESS_HEADER */
