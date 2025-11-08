/**
 * @file Coin_Changer.h
 * @brief MDB Coin Changer Device Interface
 *
 * Implements MDB Protocol Section 5 - Coin Changer specifications.
 * Supports coin acceptance, tube management, dispensing, and payout.
 */

#ifndef COIN_CHANGER_HEADER
#define COIN_CHANGER_HEADER

#include <stdint.h>
#include "Communication_Format.h"
#include "PreProcessors.h"

/**
 * @brief Coin Changer state structure
 */
typedef struct {
    /* Device Configuration */
    uint8_t address_C;                    /* 0x08 for coin changer */
    uint8_t featureLevel_C;               /* Feature level */
    uint8_t countryCurrencyCode_C[2];     /* ISO 4217 currency code */
    uint8_t coinScalingFactor_C;          /* Scaling factor */
    uint8_t decimalPlaces_C;              /* Decimal places */
    uint8_t coinTypeRouting_C[2];         /* Routing configuration */
    uint8_t coinTypeCredit_C[16];         /* Credit for each coin type */

    /* Tube Status */
    uint8_t tubeFullStatus[2];            /* Tube full indicators */
    uint8_t tubeStatus[16];               /* Coins in each tube */

    /* Runtime Settings */
    uint8_t coinEnable_s[2];              /* Enabled coin types */
    uint8_t manualDispenseEnable_s[2];    /* Manual dispense enable */

    /* Status Flags */
    uint8_t busy_f;
    uint8_t disabled_f;
    uint8_t noCredit_f;
    uint8_t defectiveTubeSensor_f;
    uint8_t doubleArrival_f;
    uint8_t acceptorUnplugged_f;
    uint8_t tubeJam_f;
    uint8_t romChecksumError_f;
    uint8_t coinRoutingError_f;
    uint8_t coinJam_f;
    uint8_t possibleCreditedCoinRemoval_f;
    uint8_t wasReset_f;

    /* Activity Data */
    uint8_t coinsDeposited[16];           /* Coins deposited queue */
    uint8_t coinsDispensedManually[16];   /* Manual dispense queue */
    uint8_t coinsFilledManually[16];      /* Manual fill data */
    uint8_t coinsPaid[16];                /* Payout data */
    uint8_t slugCount;                    /* Slug counter */

    /* Payout State */
    uint8_t payoutActive_f;
    uint16_t valueOfCoinsToBePaidOut;
    uint16_t payoutValue;

    /* Optional Features */
    uint8_t alternativePayoutMethod_s;
    uint8_t extendedDiagnosticCommand_s;
    uint8_t controlledManualFillPayout_s;
    uint8_t ftlSupported_s;

} CoinChangerState;

/* Global coin changer instance */
extern CoinChangerState coinChanger;

/**
 * @brief Initialize coin changer
 */
extern uint8_t coinChanger_init(void);

/**
 * @brief Handle RESET command (0x08)
 */
extern uint8_t coinChanger_reset(void);

/**
 * @brief Handle SETUP command (0x09)
 */
extern uint8_t coinChanger_setup(void);

/**
 * @brief Handle TUBE STATUS command (0x0A)
 */
extern uint8_t coinChanger_tubeStatus(void);

/**
 * @brief Handle POLL command (0x0B)
 */
extern uint8_t coinChanger_poll(void);

/**
 * @brief Handle COIN TYPE command (0x0C)
 */
extern uint8_t coinChanger_coinType(void);

/**
 * @brief Handle DISPENSE command (0x0D)
 */
extern uint8_t coinChanger_dispense(void);

/**
 * @brief Handle EXPANSION command (0x0F)
 */
extern uint8_t coinChanger_expansion(void);

/**
 * @brief Main processing loop
 */
extern uint8_t coinChanger_process(void);

#endif /* COIN_CHANGER_HEADER */
