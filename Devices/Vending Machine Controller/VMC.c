/**
 * @file VMC.c
 * @brief Vending Machine Controller (VMC) - MDB Bus Master Implementation
 *
 * The VMC acts as the master device on the MDB bus, coordinating all
 * peripheral devices including coin changers, bill validators, and
 * cashless payment systems. Supports MDB 4.3 features.
 */

#include "VMC.h"
#include "Communication_Format.h"
#include "PreProcessors.h"
#include "Bus_Timing.h"
#include <string.h>

/* Global VMC instance */
VMCController vmc;

/* Basket tracking (MDB 4.3) */
static uint16_t basketItems[16];
static uint16_t basketPrices[16];

/* Device poll queue */
static const uint8_t pollQueue[] = {
    CASHLESS1_ADDRESS,
    CASHLESS2_ADDRESS,
    CHANGER_ADDRESS,
    BILL_VALIDATOR_ADDRESS
};
static const uint8_t pollQueueSize = sizeof(pollQueue) / sizeof(pollQueue[0]);

/* Helper function prototypes */
static uint8_t vmc_sendCommand(uint8_t address, uint8_t command);
static uint8_t vmc_sendCommandData(uint8_t address, uint8_t command, const uint8_t *data, uint8_t length);
static uint8_t vmc_waitForResponse(uint32_t timeoutUs);
static PeripheralStatus* vmc_getStatusByAddress(uint8_t address);
static void vmc_updateDeviceStatus(uint8_t address, uint8_t online);

/**
 * @brief Initialize VMC controller
 */
uint8_t vmc_init(void) {
    memset(&vmc, 0, sizeof(VMCController));

    /* Initialize state */
    vmc.state = VMC_STATE_IDLE;
    vmc.sessionActive_f = 0;
    vmc.creditAvailable = 0;

    /* Configuration defaults */
    vmc.maxPollInterval_C = 100;  /* 100ms */
    vmc.maxResponseTime_C = 5;    /* 5ms */
    vmc.autoPollEnabled_s = 1;    /* Auto-polling enabled */

    /* Initialize peripheral status */
    vmc.coinChanger.pollInterval = 200000;      /* 200ms */
    vmc.billValidator.pollInterval = 200000;    /* 200ms */
    vmc.cashless1.pollInterval = 100000;        /* 100ms */
    vmc.cashless2.pollInterval = 100000;        /* 100ms */

    vmc.pollQueueIndex = 0;
    vmc.lastBusActivity = micros();

    return 0;
}

/**
 * @brief Main VMC processing loop
 */
uint8_t vmc_process(void) {
    if (!vmc.autoPollEnabled_s) {
        return 0;
    }

    /* Auto-poll devices in round-robin */
    return vmc_pollAll();
}

/**
 * @brief Reset a peripheral device
 */
uint8_t vmc_resetDevice(uint8_t deviceAddress) {
    uint8_t command = deviceAddress | 0x00;  /* RESET is command 0 */

    /* Send RESET command */
    clearBlock();
    tX9Bits(1, command);  /* Address/Command with mode bit */
    tXBitsFlush();

    /* Wait for ACK */
    uint32_t timeout = micros() + (vmc.maxResponseTime_C * 1000);
    while (!rxBitsAvailable()) {
        if (micros() > timeout) {
            vmc_updateDeviceStatus(deviceAddress, 0);
            return 0;  /* Timeout */
        }
    }

    uint16_t response = rX9Bits();
    if ((response & 0xFF) == ACK) {
        PeripheralStatus *status = vmc_getStatusByAddress(deviceAddress);
        if (status) {
            status->justReset_f = 1;
            status->setupComplete_f = 0;
        }
        vmc_updateDeviceStatus(deviceAddress, 1);
        return 1;
    }

    return 0;
}

/**
 * @brief Setup a peripheral device
 */
uint8_t vmc_setupDevice(uint8_t deviceAddress) {
    uint8_t setupData[12];
    uint8_t len = 0;

    /* Determine device-specific setup data */
    if (deviceAddress == CASHLESS1_ADDRESS || deviceAddress == CASHLESS2_ADDRESS) {
        /* Cashless SETUP - Config Data */
        setupData[len++] = 0x01;  /* Subcommand: Config Data */
        setupData[len++] = 0x03;  /* VMC Feature Level 3 (MDB 4.3) */
        setupData[len++] = 0x01;  /* Columns on display */
        setupData[len++] = 0x10;  /* Rows on display */
        setupData[len++] = 0x01;  /* Display type (ASCII) */

    } else if (deviceAddress == CHANGER_ADDRESS) {
        /* Coin Changer SETUP - Config Data */
        setupData[len++] = 0x01;  /* Subcommand: Config Data */
        setupData[len++] = 0x03;  /* Feature Level 3 */
        setupData[len++] = 0x18;  /* Country code MSB */
        setupData[len++] = 0x40;  /* Country code LSB (USD) */
        setupData[len++] = 0x01;  /* Coin scaling factor */
        setupData[len++] = 0x02;  /* Decimal places */

    } else if (deviceAddress == BILL_VALIDATOR_ADDRESS) {
        /* Bill Validator SETUP - Security/Type */
        setupData[len++] = 0x00;  /* Subcommand: Security */
        setupData[len++] = 0x03;  /* Feature Level 3 */
        setupData[len++] = 0x18;  /* Country code MSB */
        setupData[len++] = 0x40;  /* Country code LSB */
        setupData[len++] = 0x64;  /* Bill scaling factor (100) */
        setupData[len++] = 0x02;  /* Decimal places */
        setupData[len++] = 0x00;  /* Stacker capacity MSB */
        setupData[len++] = 0xFF;  /* Stacker capacity LSB */
    }

    /* Send SETUP command with data */
    clearBlock();
    tX9Bits(1, deviceAddress | 0x01);  /* SETUP is command 1 */

    for (uint8_t i = 0; i < len; i++) {
        tX9Bits(0, setupData[i]);
    }

    /* Send checksum */
    uint8_t chk = deviceAddress | 0x01;
    for (uint8_t i = 0; i < len; i++) {
        chk += setupData[i];
    }
    tX9Bits(0, chk);
    tXBitsFlush();

    /* Wait for response */
    if (vmc_waitForResponse(vmc.maxResponseTime_C * 1000)) {
        PeripheralStatus *status = vmc_getStatusByAddress(deviceAddress);
        if (status) {
            status->setupComplete_f = 1;
        }
        vmc_updateDeviceStatus(deviceAddress, 1);

        /* Clear receive buffer */
        clearBlock();

        return 1;
    }

    return 0;
}

/**
 * @brief Poll a peripheral device
 */
uint8_t vmc_pollDevice(uint8_t deviceAddress) {
    /* Send POLL command */
    clearBlock();
    tX9Bits(1, deviceAddress | 0x03);  /* POLL is command 3 */
    tXBitsFlush();

    /* Wait for response */
    uint32_t timeout = micros() + (vmc.maxResponseTime_C * 1000);

    while (!rxBitsAvailable()) {
        if (micros() > timeout) {
            vmc_updateDeviceStatus(deviceAddress, 0);
            return 0;  /* Timeout */
        }
    }

    /* Read first byte of response */
    uint16_t firstByte = rX9Bits();

    if ((firstByte & 0xFF) == ACK) {
        /* No data to report */
        vmc_updateDeviceStatus(deviceAddress, 1);
        return 1;
    }

    /* Device has data - read response */
    uint8_t responseData = firstByte & 0xFF;

    /* Process device-specific responses */
    if (deviceAddress == CASHLESS1_ADDRESS || deviceAddress == CASHLESS2_ADDRESS) {
        /* Handle cashless poll responses */
        switch (responseData) {
            case CASHLESS1_POLL_JUSTRESET:
                vmc_setupDevice(deviceAddress);
                break;

            case CASHLESS1_POLL_BEGINSESSION: {
                /* Read funds available */
                uint16_t funds = (rX9Bits() & 0xFF) << 8;
                funds |= (rX9Bits() & 0xFF);
                vmc_addCredit(funds, deviceAddress);
                vmc.sessionActive_f = 1;
                vmc.state = VMC_STATE_SESSION_IDLE;
                break;
            }

            case CASHLESS1_POLL_VENDAPPROVED: {
                /* Vend approved - dispense product */
                uint16_t approvedAmount = (rX9Bits() & 0xFF) << 8;
                approvedAmount |= (rX9Bits() & 0xFF);
                /* Application should handle product dispensing */
                break;
            }

            case CASHLESS1_POLL_VENDDENIED:
                /* Vend denied - insufficient funds */
                vmc.state = VMC_STATE_SESSION_IDLE;
                break;

            case CASHLESS1_POLL_ENDSESSION:
                vmc.sessionActive_f = 0;
                vmc.state = VMC_STATE_IDLE;
                vmc.creditAvailable = 0;
                break;
        }
    }

    /* Clear remaining response data */
    clearBlock();

    vmc_updateDeviceStatus(deviceAddress, 1);
    return 1;
}

/**
 * @brief Poll all enabled devices in round-robin
 */
uint8_t vmc_pollAll(void) {
    /* Get current device from poll queue */
    uint8_t deviceAddress = pollQueue[vmc.pollQueueIndex];
    PeripheralStatus *status = vmc_getStatusByAddress(deviceAddress);

    /* Check if it's time to poll this device */
    if (status && status->online_f) {
        uint32_t now = micros();
        if ((now - status->lastPollTime) >= status->pollInterval) {
            vmc_pollDevice(deviceAddress);
            status->lastPollTime = now;
        }
    }

    /* Move to next device */
    vmc.pollQueueIndex = (vmc.pollQueueIndex + 1) % pollQueueSize;

    return 1;
}

/**
 * @brief Start a vend session
 */
uint8_t vmc_beginVend(uint16_t itemNumber, uint16_t price) {
    if (!vmc.sessionActive_f) {
        return 0;  /* No session active */
    }

    if (vmc.creditAvailable < price) {
        return 0;  /* Insufficient credit */
    }

    vmc.vendPrice = price;
    vmc.itemNumber = itemNumber;
    vmc.state = VMC_STATE_VEND;

    /* Send VEND REQUEST to cashless devices */
    uint8_t deviceAddress = CASHLESS1_ADDRESS;  /* Or CASHLESS2 */

    uint8_t vendData[8];
    uint8_t len = 0;

    vendData[len++] = CASHLESS1_VEND_VENDREQUEST;
    vendData[len++] = (price >> 8) & 0xFF;
    vendData[len++] = price & 0xFF;
    vendData[len++] = (itemNumber >> 8) & 0xFF;
    vendData[len++] = itemNumber & 0xFF;

    return vmc_sendCommandData(deviceAddress, 0x04, vendData, len);  /* VEND is command 4 */
}

/**
 * @brief Report vend success
 */
uint8_t vmc_vendSuccess(uint16_t itemDispensed) {
    uint8_t deviceAddress = CASHLESS1_ADDRESS;

    uint8_t vendData[8];
    uint8_t len = 0;

    vendData[len++] = CASHLESS1_VEND_VENDSUCCESS;
    vendData[len++] = (itemDispensed >> 8) & 0xFF;
    vendData[len++] = itemDispensed & 0xFF;

    vmc.state = VMC_STATE_SESSION_IDLE;

    return vmc_sendCommandData(deviceAddress, 0x04, vendData, len);
}

/**
 * @brief Report vend failure
 */
uint8_t vmc_vendFailure(void) {
    uint8_t deviceAddress = CASHLESS1_ADDRESS;

    uint8_t vendData[1];
    vendData[0] = CASHLESS1_VEND_VENDFAILURE;

    vmc.state = VMC_STATE_SESSION_IDLE;

    return vmc_sendCommandData(deviceAddress, 0x04, vendData, 1);
}

/**
 * @brief End vend session
 */
uint8_t vmc_sessionComplete(void) {
    uint8_t deviceAddress = CASHLESS1_ADDRESS;

    uint8_t vendData[1];
    vendData[0] = CASHLESS1_VEND_SESSIONCOMPLETE;

    vmc.sessionActive_f = 0;
    vmc.state = VMC_STATE_IDLE;
    vmc.creditAvailable = 0;

    return vmc_sendCommandData(deviceAddress, 0x04, vendData, 1);
}

/**
 * @brief Add credit from peripheral device
 */
void vmc_addCredit(uint16_t amount, uint8_t source) {
    (void)source;  /* Unused for now */
    vmc.creditAvailable += amount;
}

/**
 * @brief Get total available credit
 */
uint16_t vmc_getTotalCredit(void) {
    return vmc.creditAvailable;
}

/**
 * @brief Enable/disable a peripheral device
 */
uint8_t vmc_enableDevice(uint8_t deviceAddress, uint8_t enable) {
    uint8_t command = deviceAddress | 0x05;  /* READER command */
    uint8_t subcommand = enable ? CASHLESS1_READER_READERENABLE : CASHLESS1_READER_READERDISABLE;

    uint8_t data[1];
    data[0] = subcommand;

    PeripheralStatus *status = vmc_getStatusByAddress(deviceAddress);
    if (status) {
        status->enabled_f = enable;
    }

    return vmc_sendCommandData(deviceAddress, 0x05, data, 1);
}

/**
 * @brief Check if device is online
 */
uint8_t vmc_isDeviceOnline(uint8_t deviceAddress) {
    PeripheralStatus *status = vmc_getStatusByAddress(deviceAddress);
    return status ? status->online_f : 0;
}

/**
 * @brief Get peripheral device status
 */
PeripheralStatus* vmc_getDeviceStatus(uint8_t deviceAddress) {
    return vmc_getStatusByAddress(deviceAddress);
}

/* MDB 4.3 Basket Functions */

/**
 * @brief Start basket transaction (MDB 4.3)
 */
uint8_t vmc_beginBasket(void) {
    if (!vmc.sessionActive_f) {
        return 0;
    }

    vmc.basketMode_f = 1;
    vmc.basketItemCount = 0;
    vmc.basketTotal = 0;
    memset(basketItems, 0, sizeof(basketItems));
    memset(basketPrices, 0, sizeof(basketPrices));

    /* Send EXPANSION BASKET CONTROL - Begin */
    uint8_t deviceAddress = CASHLESS1_ADDRESS;
    uint8_t expansionData[2];
    expansionData[0] = 0x12;  /* Basket Control */
    expansionData[1] = 0x01;  /* Begin basket */

    return vmc_sendCommandData(deviceAddress, 0x07, expansionData, 2);  /* EXPANSION is command 7 */
}

/**
 * @brief Add item to basket (MDB 4.3)
 */
uint8_t vmc_addToBasket(uint16_t itemNumber, uint16_t price) {
    if (!vmc.basketMode_f || vmc.basketItemCount >= 16) {
        return 0;
    }

    /* Initiate vend request for this item */
    uint8_t result = vmc_beginVend(itemNumber, price);

    if (result) {
        /* Track in basket */
        basketItems[vmc.basketItemCount] = itemNumber;
        basketPrices[vmc.basketItemCount] = price;
        vmc.basketItemCount++;
        vmc.basketTotal += price;
    }

    return result;
}

/**
 * @brief Complete basket transaction (MDB 4.3)
 */
uint8_t vmc_completeBasket(void) {
    if (!vmc.basketMode_f) {
        return 0;
    }

    /* Send EXPANSION BASKET CONTROL - Complete */
    uint8_t deviceAddress = CASHLESS1_ADDRESS;
    uint8_t expansionData[2];
    expansionData[0] = 0x12;  /* Basket Control */
    expansionData[1] = 0x02;  /* Complete basket */

    uint8_t result = vmc_sendCommandData(deviceAddress, 0x07, expansionData, 2);

    if (result) {
        /* Clear basket state */
        vmc.basketMode_f = 0;
        vmc.basketItemCount = 0;
        vmc.basketTotal = 0;
    }

    return result;
}

/**
 * @brief Calculate and issue partial refund (MDB 4.3)
 */
uint16_t vmc_partialRefund(uint8_t itemsFailed) {
    if (!vmc.basketMode_f || itemsFailed == 0 || itemsFailed > vmc.basketItemCount) {
        return 0;
    }

    /* Calculate refund amount */
    uint16_t refundAmount = 0;
    for (uint8_t i = 0; i < itemsFailed; i++) {
        uint8_t idx = vmc.basketItemCount - 1 - i;
        refundAmount += basketPrices[idx];
    }

    /* Send EXPANSION PARTIAL REFUND NOTIFY */
    uint8_t deviceAddress = CASHLESS1_ADDRESS;
    uint8_t expansionData[2];
    expansionData[0] = 0x13;  /* Partial Refund Notify */
    expansionData[1] = itemsFailed;

    vmc_sendCommandData(deviceAddress, 0x07, expansionData, 2);

    return refundAmount;
}

/* Helper Functions */

/**
 * @brief Send command to device
 */
static uint8_t vmc_sendCommand(uint8_t address, uint8_t command) {
    clearBlock();
    tX9Bits(1, address | command);
    tXBitsFlush();

    return vmc_waitForResponse(vmc.maxResponseTime_C * 1000);
}

/**
 * @brief Send command with data to device
 */
static uint8_t vmc_sendCommandData(uint8_t address, uint8_t command, const uint8_t *data, uint8_t length) {
    clearBlock();
    tX9Bits(1, address | command);

    for (uint8_t i = 0; i < length; i++) {
        tX9Bits(0, data[i]);
    }

    /* Calculate and send checksum */
    uint8_t chk = address | command;
    for (uint8_t i = 0; i < length; i++) {
        chk += data[i];
    }
    tX9Bits(0, chk);
    tXBitsFlush();

    return vmc_waitForResponse(vmc.maxResponseTime_C * 1000);
}

/**
 * @brief Wait for device response
 */
static uint8_t vmc_waitForResponse(uint32_t timeoutUs) {
    uint32_t deadline = micros() + timeoutUs;

    while (!rxBitsAvailable()) {
        if (micros() > deadline) {
            return 0;  /* Timeout */
        }
    }

    return 1;  /* Response received */
}

/**
 * @brief Get peripheral status by address
 */
static PeripheralStatus* vmc_getStatusByAddress(uint8_t address) {
    switch (address) {
        case CHANGER_ADDRESS:
            return &vmc.coinChanger;
        case BILL_VALIDATOR_ADDRESS:
            return &vmc.billValidator;
        case CASHLESS1_ADDRESS:
            return &vmc.cashless1;
        case CASHLESS2_ADDRESS:
            return &vmc.cashless2;
        default:
            return NULL;
    }
}

/**
 * @brief Update device online status
 */
static void vmc_updateDeviceStatus(uint8_t address, uint8_t online) {
    PeripheralStatus *status = vmc_getStatusByAddress(address);
    if (status) {
        status->online_f = online;
        if (online) {
            status->lastPollTime = micros();
        }
    }
}
