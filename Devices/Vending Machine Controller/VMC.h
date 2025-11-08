/**
 * @file VMC.h
 * @brief Vending Machine Controller (VMC) - MDB Bus Master
 *
 * The VMC is the master device on the MDB bus. It initiates all
 * communications and coordinates peripheral devices (coin changer,
 * bill validator, cashless devices, etc.).
 *
 * Implements MDB Protocol Sections 1-4 (Master Side).
 */

#ifndef VMC_HEADER
#define VMC_HEADER

#include <stdint.h>
#include "Communication_Format.h"
#include "PreProcessors.h"

/**
 * @brief VMC Session State
 */
typedef enum {
    VMC_STATE_IDLE = 0,           /* No session active */
    VMC_STATE_DISABLED,           /* VMC disabled */
    VMC_STATE_ENABLED,            /* Waiting for user interaction */
    VMC_STATE_SESSION_IDLE,       /* Session started, no vend yet */
    VMC_STATE_VEND,               /* Vend in progress */
    VMC_STATE_REVALUE,            /* Revalue in progress */
    VMC_STATE_NEGATIVE_VEND       /* Negative vend (refund) */
} VMCState;

/**
 * @brief Peripheral device status
 */
typedef struct {
    uint8_t online_f;             /* Device is online and responding */
    uint8_t enabled_f;            /* Device is enabled */
    uint8_t busy_f;               /* Device is busy */
    uint8_t error_f;              /* Device has error */
    uint8_t justReset_f;          /* Device just reset */
    uint8_t setupComplete_f;      /* SETUP command completed */
    uint32_t lastPollTime;        /* Last successful poll (microseconds) */
    uint32_t pollInterval;        /* Poll interval in microseconds */
} PeripheralStatus;

/**
 * @brief VMC Master Controller State
 */
typedef struct {
    /* VMC State */
    VMCState state;               /* Current VMC state */
    uint8_t sessionActive_f;      /* Vend session active */
    uint16_t creditAvailable;     /* Total credit available */
    uint16_t vendPrice;           /* Current vend price */
    uint16_t itemNumber;          /* Selected item number */

    /* MDB 4.3 Features */
    uint8_t basketMode_f;         /* Basket mode active */
    uint8_t remoteVend_f;         /* Remote vend in progress */
    uint8_t basketItemCount;      /* Items in basket */
    uint16_t basketTotal;         /* Total basket value */

    /* Peripheral Status */
    PeripheralStatus coinChanger;
    PeripheralStatus billValidator;
    PeripheralStatus cashless1;
    PeripheralStatus cashless2;
    PeripheralStatus commGateway;
    PeripheralStatus display;

    /* Poll Management */
    uint8_t pollQueueIndex;       /* Current device being polled */
    uint32_t lastBusActivity;     /* Last bus activity time */

    /* Configuration */
    uint8_t maxPollInterval_C;    /* Maximum poll interval (ms) */
    uint8_t maxResponseTime_C;    /* Maximum response time (ms) */
    uint8_t autoPollEnabled_s;    /* Auto-polling enabled */

} VMCController;

/* Global VMC instance */
extern VMCController vmc;

/**
 * @brief Initialize VMC controller
 *
 * @return 0 on success, error code otherwise
 */
extern uint8_t vmc_init(void);

/**
 * @brief Main VMC processing loop
 *
 * Call this periodically to manage all MDB bus operations.
 *
 * @return Processing status
 */
extern uint8_t vmc_process(void);

/**
 * @brief Reset a peripheral device
 *
 * @param deviceAddress Device address to reset
 * @return Response status
 */
extern uint8_t vmc_resetDevice(uint8_t deviceAddress);

/**
 * @brief Setup a peripheral device
 *
 * @param deviceAddress Device address to setup
 * @return Response status
 */
extern uint8_t vmc_setupDevice(uint8_t deviceAddress);

/**
 * @brief Poll a peripheral device
 *
 * @param deviceAddress Device address to poll
 * @return Response status
 */
extern uint8_t vmc_pollDevice(uint8_t deviceAddress);

/**
 * @brief Poll all enabled devices in round-robin
 *
 * @return Response status
 */
extern uint8_t vmc_pollAll(void);

/**
 * @brief Start a vend session
 *
 * @param itemNumber Item number to vend
 * @param price Item price
 * @return 0 on success, error code otherwise
 */
extern uint8_t vmc_beginVend(uint16_t itemNumber, uint16_t price);

/**
 * @brief Approve a vend (after payment confirmed)
 *
 * @return Response status
 */
extern uint8_t vmc_approveVend(void);

/**
 * @brief Cancel current vend
 *
 * @return Response status
 */
extern uint8_t vmc_cancelVend(void);

/**
 * @brief Report vend success to payment devices
 *
 * @param itemDispensed Item number actually dispensed
 * @return Response status
 */
extern uint8_t vmc_vendSuccess(uint16_t itemDispensed);

/**
 * @brief Report vend failure
 *
 * @return Response status
 */
extern uint8_t vmc_vendFailure(void);

/**
 * @brief End vend session
 *
 * @return Response status
 */
extern uint8_t vmc_sessionComplete(void);

/**
 * @brief Add credit from coin/bill acceptance
 *
 * @param amount Amount to add
 * @param source Source device (CHANGER_ADDRESS, BILL_VALIDATOR_ADDRESS, etc.)
 */
extern void vmc_addCredit(uint16_t amount, uint8_t source);

/**
 * @brief Get total available credit
 *
 * @return Total credit from all sources
 */
extern uint16_t vmc_getTotalCredit(void);

/**
 * @brief Enable/disable a peripheral device
 *
 * @param deviceAddress Device address
 * @param enable 1 to enable, 0 to disable
 * @return Response status
 */
extern uint8_t vmc_enableDevice(uint8_t deviceAddress, uint8_t enable);

/**
 * @brief Check if device is online and responsive
 *
 * @param deviceAddress Device address
 * @return 1 if online, 0 if offline
 */
extern uint8_t vmc_isDeviceOnline(uint8_t deviceAddress);

/**
 * @brief Get peripheral status
 *
 * @param deviceAddress Device address
 * @return Pointer to device status, or NULL if invalid
 */
extern PeripheralStatus* vmc_getDeviceStatus(uint8_t deviceAddress);

/* MDB 4.3 Cashless Coordinator Functions */

/**
 * @brief Start basket transaction (MDB 4.3)
 *
 * @return 0 on success, error code otherwise
 */
extern uint8_t vmc_beginBasket(void);

/**
 * @brief Add item to basket (MDB 4.3)
 *
 * @param itemNumber Item to add
 * @param price Item price
 * @return 0 on success, error code otherwise
 */
extern uint8_t vmc_addToBasket(uint16_t itemNumber, uint16_t price);

/**
 * @brief Complete basket transaction (MDB 4.3)
 *
 * @return Response status
 */
extern uint8_t vmc_completeBasket(void);

/**
 * @brief Calculate and issue partial refund (MDB 4.3)
 *
 * @param itemsFailed Number of items that failed to dispense
 * @return Refund amount issued
 */
extern uint16_t vmc_partialRefund(uint8_t itemsFailed);

#endif /* VMC_HEADER */
