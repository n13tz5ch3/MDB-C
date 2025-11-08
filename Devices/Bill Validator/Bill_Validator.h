/**
 * @file Bill_Validator.h
 * @brief MDB Bill Validator Device Interface
 *
 * Implements MDB Protocol Section 6 - Bill Validator specifications.
 * Supports bill acceptance, escrow, stacking, and security features.
 */

#ifndef BILL_VALIDATOR_HEADER
#define BILL_VALIDATOR_HEADER

#include <stdint.h>
#include "Communication_Format.h"
#include "PreProcessors.h"

/**
 * @brief Bill Validator state structure
 *
 * Maintains the complete state of a bill validator device including
 * configuration, status flags, and runtime data.
 */
typedef struct {
    /* Device Configuration (_C suffix = set once at compile/init time) */
    uint8_t address_C;                    /* Device address (0x30 for primary) */
    uint8_t featureLevel_C;               /* Feature level (1, 2, or 3) */
    uint8_t countryCurrencyCode_C[2];     /* ISO 4217 currency code */
    uint8_t billScalingFactor_C[2];       /* Scaling factor for bill values */
    uint8_t decimalPlaces_C;              /* Number of decimal places */
    uint8_t stackerCapacity_C[2];         /* Maximum bills in stacker */
    uint8_t billSecurityLevels_C[2];      /* Security level bitmask */
    uint8_t escrowCapability_C;           /* Escrow supported flag */
    uint8_t billTypeCredit_C[16];         /* Credit value for each bill type */

    /* Runtime Settings (_s suffix = changed during operation) */
    uint8_t billTypeEnable_s[2];          /* Enabled bill types bitmask */
    uint8_t billEscrowEnable_s[2];        /* Escrow enabled bitmask */

    /* Status Flags (_f suffix) */
    uint8_t busy_f;                       /* Validator busy */
    uint8_t validating_f;                 /* Bill validation in progress */
    uint8_t escrow_f;                     /* Bill in escrow */
    uint8_t disabled_f;                   /* Validator disabled */
    uint8_t jammed_f;                     /* Validator jammed */
    uint8_t stackerFull_f;                /* Stacker full */
    uint8_t cashBoxRemoved_f;             /* Cash box out of position */
    uint8_t motorProblem_f;               /* Motor failure */
    uint8_t sensorProblem_f;              /* Sensor failure */
    uint8_t romChecksumError_f;           /* ROM checksum failed */
    uint8_t wasReset_f;                   /* Just reset status */

    /* Runtime Values (no suffix) */
    uint8_t billInEscrow;                 /* Bill type currently in escrow */
    uint8_t lastBillStacked;              /* Last bill type stacked */
    uint16_t billsStacked;                /* Total bills in stacker */
    uint8_t disabledBillRejectCount;      /* Bills rejected while disabled */

    /* Activity Queue for POLL responses */
    uint8_t activityQueue[16];            /* Queue of events to report */
    uint8_t activityQueueCount;           /* Number of events queued */

    /* FTL Support */
    uint8_t ftlSupported_s;               /* File Transfer Layer enabled */

} BillValidatorState;

/* Global bill validator instance */
extern BillValidatorState billValidator;

/**
 * @brief Initialize bill validator
 *
 * @return 0 on success, error code otherwise
 */
extern uint8_t billValidator_init(void);

/**
 * @brief Handle RESET command (0x30)
 *
 * @return Response status code
 */
extern uint8_t billValidator_reset(void);

/**
 * @brief Handle SETUP command (0x31)
 *
 * @return Response status code
 */
extern uint8_t billValidator_setup(void);

/**
 * @brief Handle SECURITY command (0x32)
 *
 * @return Response status code
 */
extern uint8_t billValidator_security(void);

/**
 * @brief Handle POLL command (0x33)
 *
 * @return Response status code
 */
extern uint8_t billValidator_poll(void);

/**
 * @brief Handle BILL TYPE command (0x34)
 *
 * @return Response status code
 */
extern uint8_t billValidator_billType(void);

/**
 * @brief Handle ESCROW command (0x35)
 *
 * @return Response status code
 */
extern uint8_t billValidator_escrow(void);

/**
 * @brief Handle STACKER command (0x36)
 *
 * @return Response status code
 */
extern uint8_t billValidator_stacker(void);

/**
 * @brief Handle EXPANSION command (0x37)
 *
 * @return Response status code
 */
extern uint8_t billValidator_expansion(void);

/**
 * @brief Main bill validator processing loop
 *
 * Call this periodically to process MDB commands and handle hardware.
 *
 * @return Processing status
 */
extern uint8_t billValidator_process(void);

/**
 * @brief Add activity event to queue for next POLL
 *
 * @param activity Activity code to queue
 */
extern void billValidator_queueActivity(uint8_t activity);

#endif /* BILL_VALIDATOR_HEADER */
