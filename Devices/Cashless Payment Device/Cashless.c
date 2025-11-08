/**
 * @file Cashless.c
 * @brief MDB Cashless Payment Device Implementation (MDB 4.3)
 *
 * Implements Section 7 of the MDB 4.3 specification with support for:
 * - Remote Vend Request (Expansion 0x10)
 * - Basket/Multi-Vend (Expansion 0x12)
 * - Coupon Support (Expansion 0x11)
 * - Partial Refund (Expansion 0x13)
 * - PA101 Item Numbers (Enhanced numbering)
 * - Mixed Vend Flags
 * - Card Information (Expansion 0x00 subcommand 0x05)
 * - Enhanced Item Numbers in VEND
 */

#include "Cashless.h"
#include "Communication_Format.h"
#include "PreProcessors.h"
#include <string.h>

/* Helper function prototypes */
static uint8_t cashless_handleExpansion(CashlessState *state);
static uint8_t cashless_handleVendRequest(CashlessState *state);
static uint8_t cashless_handleVendApproved(CashlessState *state);
static uint8_t cashless_encodePA101ItemNumber(uint16_t itemNumber, uint8_t *buffer);
static uint16_t cashless_decodePA101ItemNumber(const uint8_t *buffer);

/**
 * @brief Initialize cashless device state
 */
void cashless_init(CashlessState *state, uint8_t address) {
    memset(state, 0, sizeof(CashlessState));

    state->address_C = address;
    state->featureLevel_C = 0x03;  /* Level 03 = MDB 4.3 */
    state->currencyCode_C = 0x1840;  /* USD */
    state->scaleFactor_C = 1;
    state->decimalPlaces_C = 2;
    state->maxResponseTime_C = 5;  /* 5 seconds */

    /* MDB 4.3 Features - Enable all */
    state->remoteVendEnabled_s = 1;
    state->basketEnabled_s = 1;
    state->couponEnabled_s = 1;
    state->partialRefundEnabled_s = 1;
    state->enhancedItemNumberEnabled_s = 1;
    state->pa101ItemNumbersEnabled_s = 1;
    state->mixedVendFlagsEnabled_s = 1;
    state->cardInfoEnabled_s = 1;

    state->state = CASHLESS_STATE_INACTIVE;
    state->vendState = CASHLESS_VEND_IDLE;
}

/**
 * @brief Main processing function
 */
uint8_t cashless_process(CashlessState *state) {
    if (!rxBitsAvailable()) {
        return 0;
    }

    uint16_t rx_data = rX9Bits();
    if (rx_data == 0xFFFF) {
        return 0;  /* Invalid data */
    }

    uint8_t mode = (rx_data >> 8) & 0x01;
    uint8_t data = rx_data & 0xFF;

    /* Check if addressed to us */
    if (mode == 1 && (data & 0xF8) == state->address_C) {
        uint8_t cmd = data & 0x07;

        switch (cmd) {
            case CASHLESS1_CMD_RESET:
                return cashless_reset(state);

            case CASHLESS1_CMD_SETUP:
                return cashless_setup(state);

            case CASHLESS1_CMD_POLL:
                return cashless_poll(state);

            case CASHLESS1_CMD_VEND:
                return cashless_vend(state);

            case CASHLESS1_CMD_READER:
                return cashless_reader(state);

            case CASHLESS1_CMD_REVALUE:
                return cashless_revalue(state);

            case CASHLESS1_CMD_EXPANSION:
                return cashless_expansion(state);

            default:
                tX9Bits(0, NAK);
                return 0;
        }
    }

    return 0;
}

/**
 * @brief RESET command handler
 */
uint8_t cashless_reset(CashlessState *state) {
    uint8_t address_backup = state->address_C;
    uint8_t featureLevel_backup = state->featureLevel_C;

    /* Clear state but preserve configuration */
    cashless_init(state, address_backup);
    state->featureLevel_C = featureLevel_backup;

    state->state = CASHLESS_STATE_INACTIVE;
    state->justReset_f = 1;

    tX9Bits(0, ACK);
    return 1;
}

/**
 * @brief SETUP command handler
 */
uint8_t cashless_setup(CashlessState *state) {
    uint8_t response[36];
    uint8_t len = 0;

    /* Reader Config Data (7 bytes) */
    response[len++] = state->featureLevel_C;
    response[len++] = (state->currencyCode_C >> 8) & 0xFF;  /* Country/Currency MSB */
    response[len++] = state->currencyCode_C & 0xFF;         /* Country/Currency LSB */
    response[len++] = state->scaleFactor_C;
    response[len++] = state->decimalPlaces_C;
    response[len++] = state->maxResponseTime_C;
    response[len++] = 0x00;  /* Miscellaneous Options */

    /* MDB 4.3 Feature Bits (if Level 03) */
    if (state->featureLevel_C >= 0x03) {
        uint32_t features = 0;

        if (state->remoteVendEnabled_s) features |= (1 << 0);
        if (state->basketEnabled_s) features |= (1 << 1);
        if (state->couponEnabled_s) features |= (1 << 2);
        if (state->partialRefundEnabled_s) features |= (1 << 3);
        if (state->enhancedItemNumberEnabled_s) features |= (1 << 4);
        if (state->pa101ItemNumbersEnabled_s) features |= (1 << 5);
        if (state->mixedVendFlagsEnabled_s) features |= (1 << 6);
        if (state->cardInfoEnabled_s) features |= (1 << 7);

        response[len++] = (features >> 24) & 0xFF;
        response[len++] = (features >> 16) & 0xFF;
        response[len++] = (features >> 8) & 0xFF;
        response[len++] = features & 0xFF;
    }

    /* Send response */
    for (uint8_t i = 0; i < len; i++) {
        tX9Bits(0, response[i]);
    }

    /* Calculate and send checksum */
    uint8_t chk = 0;
    chk += state->address_C | CASHLESS1_CMD_SETUP;
    for (uint8_t i = 0; i < len; i++) {
        chk += response[i];
    }
    tX9Bits(0, chk);

    state->state = CASHLESS_STATE_DISABLED;

    return 1;
}

/**
 * @brief POLL command handler
 */
uint8_t cashless_poll(CashlessState *state) {
    uint8_t response[16];
    uint8_t len = 0;

    /* Check for pending events */
    if (state->justReset_f) {
        response[len++] = CASHLESS1_POLL_JUSTRESET;
        state->justReset_f = 0;
        state->state = CASHLESS_STATE_DISABLED;
    }
    else if (state->readerConfigDataChanged_f) {
        response[len++] = CASHLESS1_POLL_READERCONFIGDATACHANGED;
        state->readerConfigDataChanged_f = 0;
    }
    else if (state->displayRequest_f) {
        response[len++] = CASHLESS1_POLL_DISPLAYREQUEST;
        response[len++] = state->displayRequestTime;
        state->displayRequest_f = 0;
    }
    else if (state->beginSession_f) {
        response[len++] = CASHLESS1_POLL_BEGINSESSION;
        response[len++] = (state->userFunds >> 8) & 0xFF;
        response[len++] = state->userFunds & 0xFF;
        state->beginSession_f = 0;
        state->state = CASHLESS_STATE_IDLE;
        state->sessionActive_f = 1;
    }
    else if (state->sessionCancelRequest_f) {
        response[len++] = CASHLESS1_POLL_SESSIONCANCELREQUEST;
        state->sessionCancelRequest_f = 0;
    }
    else if (state->vendApproved_f) {
        response[len++] = CASHLESS1_POLL_VENDAPPROVED;
        response[len++] = (state->vendAmount >> 8) & 0xFF;
        response[len++] = state->vendAmount & 0xFF;
        state->vendApproved_f = 0;
        state->vendState = CASHLESS_VEND_APPROVED;
    }
    else if (state->vendDenied_f) {
        response[len++] = CASHLESS1_POLL_VENDDENIED;
        state->vendDenied_f = 0;
        state->vendState = CASHLESS_VEND_IDLE;
    }
    else if (state->endSession_f) {
        response[len++] = CASHLESS1_POLL_ENDSESSION;
        state->endSession_f = 0;
        state->state = CASHLESS_STATE_DISABLED;
        state->sessionActive_f = 0;
    }
    else if (state->cancelled_f) {
        response[len++] = CASHLESS1_POLL_CANCELLED;
        state->cancelled_f = 0;
        state->state = CASHLESS_STATE_IDLE;
    }

    /* MDB 4.3: Remote Vend Notify */
    else if (state->remoteVendActive_f) {
        response[len++] = CASHLESS1_POLL_REMOTEVENDNOTIFY;

        /* Encode item number */
        if (state->pa101ItemNumbersEnabled_s) {
            len += cashless_encodePA101ItemNumber(state->remoteVendItemNumber, &response[len]);
        } else {
            response[len++] = (state->remoteVendItemNumber >> 8) & 0xFF;
            response[len++] = state->remoteVendItemNumber & 0xFF;
        }

        response[len++] = (state->remoteVendPrice >> 8) & 0xFF;
        response[len++] = state->remoteVendPrice & 0xFF;

        state->remoteVendActive_f = 0;
    }

    /* MDB 4.3: Partial Refund Notify */
    else if (state->partialRefundPending_f) {
        response[len++] = CASHLESS1_POLL_PARTIALREFUNDNOTIFY;
        response[len++] = (state->partialRefundAmount >> 8) & 0xFF;
        response[len++] = state->partialRefundAmount & 0xFF;
        state->partialRefundPending_f = 0;
    }

    /* No events */
    if (len == 0) {
        tX9Bits(0, ACK);
        return 0;
    }

    /* Send response */
    for (uint8_t i = 0; i < len; i++) {
        tX9Bits(0, response[i]);
    }

    /* Calculate and send checksum */
    uint8_t chk = 0;
    chk += state->address_C | CASHLESS1_CMD_POLL;
    for (uint8_t i = 0; i < len; i++) {
        chk += response[i];
    }
    tX9Bits(0, chk);

    return 1;
}

/**
 * @brief VEND command handler with MDB 4.3 enhancements
 */
uint8_t cashless_vend(CashlessState *state) {
    /* Read subcommand */
    uint16_t rx = rX9Bits();
    if (rx == 0xFFFF) {
        tX9Bits(0, NAK);
        return 0;
    }

    uint8_t subcmd = rx & 0xFF;

    switch (subcmd) {
        case CASHLESS1_VEND_VENDREQUEST: {
            uint16_t itemPrice = (rX9Bits() & 0xFF) << 8;
            itemPrice |= (rX9Bits() & 0xFF);

            uint16_t itemNumber = 0;
            uint8_t vendFlags = 0;

            /* MDB 4.3: Enhanced Item Number */
            if (state->enhancedItemNumberEnabled_s) {
                if (state->pa101ItemNumbersEnabled_s) {
                    /* PA101 format: variable length */
                    uint8_t buf[4];
                    for (uint8_t i = 0; i < 4; i++) {
                        uint16_t d = rX9Bits();
                        if (d == 0xFFFF) break;
                        buf[i] = d & 0xFF;
                        if (!(buf[i] & 0x80)) {  /* MSB=0 means last byte */
                            itemNumber = cashless_decodePA101ItemNumber(buf);
                            break;
                        }
                    }
                } else {
                    /* Standard 16-bit item number */
                    itemNumber = (rX9Bits() & 0xFF) << 8;
                    itemNumber |= (rX9Bits() & 0xFF);
                }

                /* MDB 4.3: Mixed Vend Flags (optional) */
                if (state->mixedVendFlagsEnabled_s) {
                    uint16_t flags_rx = rX9Bits();
                    if (flags_rx != 0xFFFF) {
                        vendFlags = flags_rx & 0xFF;
                    }
                }
            }

            state->vendAmount = itemPrice;
            state->vendItemNumber = itemNumber;
            state->vendFlags = vendFlags;

            return cashless_handleVendRequest(state);
        }

        case CASHLESS1_VEND_VENDSUCCESS:
            state->vendState = CASHLESS_VEND_IDLE;

            /* MDB 4.3: If in basket mode, add to basket */
            if (state->basketActive_f) {
                if (state->basketItemCount < 16) {
                    state->basketItems[state->basketItemCount] = state->vendItemNumber;
                    state->basketPrices[state->basketItemCount] = state->vendAmount;
                    state->basketTotalValue += state->vendAmount;
                    state->basketItemCount++;
                }
            } else {
                /* Single vend complete - deduct from user funds */
                if (state->userFunds >= state->vendAmount) {
                    state->userFunds -= state->vendAmount;
                }
            }

            tX9Bits(0, ACK);
            return 1;

        case CASHLESS1_VEND_VENDFAILURE:
            state->vendState = CASHLESS_VEND_IDLE;
            /* Don't deduct funds on failure */
            tX9Bits(0, ACK);
            return 1;

        case CASHLESS1_VEND_SESSIONCOMPLETE:
            state->endSession_f = 1;
            state->sessionActive_f = 0;
            state->vendState = CASHLESS_VEND_IDLE;

            /* MDB 4.3: Complete basket if active */
            if (state->basketActive_f) {
                cashless_completeBasket(state);
            }

            tX9Bits(0, ACK);
            return 1;

        case CASHLESS1_VEND_CASHSALE: {
            /* Cash sale notification */
            uint16_t cashAmount = (rX9Bits() & 0xFF) << 8;
            cashAmount |= (rX9Bits() & 0xFF);

            uint16_t itemNumber = 0;
            if (state->enhancedItemNumberEnabled_s) {
                itemNumber = (rX9Bits() & 0xFF) << 8;
                itemNumber |= (rX9Bits() & 0xFF);
            }

            /* Read checksum */
            rX9Bits();

            tX9Bits(0, ACK);
            return 1;
        }

        default:
            tX9Bits(0, NAK);
            return 0;
    }
}

/**
 * @brief READER command handler
 */
uint8_t cashless_reader(CashlessState *state) {
    uint16_t rx = rX9Bits();
    if (rx == 0xFFFF) {
        tX9Bits(0, NAK);
        return 0;
    }

    uint8_t subcmd = rx & 0xFF;

    switch (subcmd) {
        case CASHLESS1_READER_READERDISABLE:
            state->state = CASHLESS_STATE_DISABLED;
            state->readerEnabled_s = 0;
            tX9Bits(0, ACK);
            return 1;

        case CASHLESS1_READER_READERENABLE:
            state->state = CASHLESS_STATE_ENABLED;
            state->readerEnabled_s = 1;
            tX9Bits(0, ACK);
            return 1;

        case CASHLESS1_READER_READERCANCEL:
            state->cancelled_f = 1;
            state->sessionActive_f = 0;
            state->vendState = CASHLESS_VEND_IDLE;

            /* MDB 4.3: Cancel basket if active */
            if (state->basketActive_f) {
                cashless_cancelBasket(state);
            }

            tX9Bits(0, ACK);
            return 1;

        default:
            tX9Bits(0, NAK);
            return 0;
    }
}

/**
 * @brief REVALUE command handler
 */
uint8_t cashless_revalue(CashlessState *state) {
    uint16_t rx = rX9Bits();
    if (rx == 0xFFFF) {
        tX9Bits(0, NAK);
        return 0;
    }

    uint8_t subcmd = rx & 0xFF;

    switch (subcmd) {
        case CASHLESS1_REVALUE_REVALUEREQUEST: {
            uint16_t revalueAmount = (rX9Bits() & 0xFF) << 8;
            revalueAmount |= (rX9Bits() & 0xFF);

            /* Read checksum */
            rX9Bits();

            /* Add to user funds */
            state->userFunds += revalueAmount;

            tX9Bits(0, ACK);
            return 1;
        }

        case CASHLESS1_REVALUE_REVALUELIMITREQUEST:
            /* Return max revalue amount */
            tX9Bits(0, 0xFF);  /* Max MSB */
            tX9Bits(0, 0xFF);  /* Max LSB */

            /* Calculate checksum */
            uint8_t chk = (state->address_C | CASHLESS1_CMD_REVALUE);
            chk += subcmd + 0xFF + 0xFF;
            tX9Bits(0, chk);
            return 1;

        default:
            tX9Bits(0, NAK);
            return 0;
    }
}

/**
 * @brief EXPANSION command handler - MDB 4.3 features
 */
uint8_t cashless_expansion(CashlessState *state) {
    uint16_t rx = rX9Bits();
    if (rx == 0xFFFF) {
        tX9Bits(0, NAK);
        return 0;
    }

    uint8_t subcmd = rx & 0xFF;

    switch (subcmd) {
        case 0x00: {  /* Expansion Request Identification */
            uint16_t subsubcmd_rx = rX9Bits();
            if (subsubcmd_rx == 0xFFFF) {
                tX9Bits(0, NAK);
                return 0;
            }
            uint8_t subsubcmd = subsubcmd_rx & 0xFF;

            if (subsubcmd == 0x05 && state->cardInfoEnabled_s) {
                /* Card Information Request - MDB 4.3 */
                uint8_t response[32];
                uint8_t len = 0;

                /* Return card data (example: PAN, expiry, etc.) */
                memcpy(&response[len], state->cardData, state->cardDataLength);
                len += state->cardDataLength;

                for (uint8_t i = 0; i < len; i++) {
                    tX9Bits(0, response[i]);
                }

                uint8_t chk = (state->address_C | CASHLESS1_CMD_EXPANSION);
                chk += subcmd + subsubcmd;
                for (uint8_t i = 0; i < len; i++) {
                    chk += response[i];
                }
                tX9Bits(0, chk);

                return 1;
            }

            tX9Bits(0, NAK);
            return 0;
        }

        case 0x10:  /* Remote Vend Request - MDB 4.3 */
            if (!state->remoteVendEnabled_s) {
                tX9Bits(0, NAK);
                return 0;
            }

            /* Read item number */
            if (state->pa101ItemNumbersEnabled_s) {
                uint8_t buf[4];
                for (uint8_t i = 0; i < 4; i++) {
                    uint16_t d = rX9Bits();
                    if (d == 0xFFFF) break;
                    buf[i] = d & 0xFF;
                    if (!(buf[i] & 0x80)) {
                        state->remoteVendItemNumber = cashless_decodePA101ItemNumber(buf);
                        break;
                    }
                }
            } else {
                state->remoteVendItemNumber = (rX9Bits() & 0xFF) << 8;
                state->remoteVendItemNumber |= (rX9Bits() & 0xFF);
            }

            /* Read price */
            state->remoteVendPrice = (rX9Bits() & 0xFF) << 8;
            state->remoteVendPrice |= (rX9Bits() & 0xFF);

            /* Read checksum */
            rX9Bits();

            /* Trigger remote vend */
            state->remoteVendActive_f = 1;

            tX9Bits(0, ACK);
            return 1;

        case 0x11:  /* Coupon Data - MDB 4.3 */
            if (!state->couponEnabled_s) {
                tX9Bits(0, NAK);
                return 0;
            }

            /* Read coupon type */
            state->couponType = rX9Bits() & 0xFF;

            /* Read coupon value */
            state->couponValue = (rX9Bits() & 0xFF) << 8;
            state->couponValue |= (rX9Bits() & 0xFF);

            /* Read coupon ID length */
            uint8_t idLen = rX9Bits() & 0xFF;
            if (idLen > 32) idLen = 32;

            /* Read coupon ID */
            for (uint8_t i = 0; i < idLen; i++) {
                state->couponID[i] = rX9Bits() & 0xFF;
            }
            state->couponIDLength = idLen;

            /* Read checksum */
            rX9Bits();

            /* Apply coupon to user funds */
            state->userFunds += state->couponValue;

            tX9Bits(0, ACK);
            return 1;

        case 0x12:  /* Basket Control - MDB 4.3 */
            if (!state->basketEnabled_s) {
                tX9Bits(0, NAK);
                return 0;
            }

            /* Read basket action */
            uint8_t basketAction = rX9Bits() & 0xFF;

            /* Read checksum */
            rX9Bits();

            if (basketAction == 0x01) {
                /* Begin basket */
                cashless_beginBasket(state);
            } else if (basketAction == 0x02) {
                /* Complete basket */
                cashless_completeBasket(state);
            } else if (basketAction == 0x03) {
                /* Cancel basket */
                cashless_cancelBasket(state);
            }

            tX9Bits(0, ACK);
            return 1;

        case 0x13:  /* Partial Refund Notify - MDB 4.3 */
            if (!state->partialRefundEnabled_s) {
                tX9Bits(0, NAK);
                return 0;
            }

            /* Read number of items that failed */
            uint8_t itemsFailed = rX9Bits() & 0xFF;

            /* Read checksum */
            rX9Bits();

            /* Calculate refund amount */
            state->partialRefundAmount = cashless_calculatePartialRefund(state, itemsFailed);
            state->partialRefundPending_f = 1;

            /* Add refund back to user funds */
            state->userFunds += state->partialRefundAmount;

            tX9Bits(0, ACK);
            return 1;

        default:
            tX9Bits(0, NAK);
            return 0;
    }
}

/**
 * @brief Begin basket transaction - MDB 4.3
 */
uint8_t cashless_beginBasket(CashlessState *state) {
    if (!state->basketEnabled_s) {
        return 0;
    }

    state->basketActive_f = 1;
    state->basketItemCount = 0;
    state->basketTotalValue = 0;
    memset(state->basketItems, 0, sizeof(state->basketItems));
    memset(state->basketPrices, 0, sizeof(state->basketPrices));

    return 1;
}

/**
 * @brief Complete basket transaction - MDB 4.3
 */
uint8_t cashless_completeBasket(CashlessState *state) {
    if (!state->basketActive_f) {
        return 0;
    }

    /* Deduct total basket value from user funds */
    if (state->userFunds >= state->basketTotalValue) {
        state->userFunds -= state->basketTotalValue;
    }

    /* Clear basket */
    state->basketActive_f = 0;
    state->basketItemCount = 0;
    state->basketTotalValue = 0;

    return 1;
}

/**
 * @brief Cancel basket transaction - MDB 4.3
 */
uint8_t cashless_cancelBasket(CashlessState *state) {
    if (!state->basketActive_f) {
        return 0;
    }

    /* Clear basket without deducting funds */
    state->basketActive_f = 0;
    state->basketItemCount = 0;
    state->basketTotalValue = 0;
    memset(state->basketItems, 0, sizeof(state->basketItems));
    memset(state->basketPrices, 0, sizeof(state->basketPrices));

    return 1;
}

/**
 * @brief Calculate partial refund - MDB 4.3
 */
uint16_t cashless_calculatePartialRefund(CashlessState *state, uint8_t itemsFailed) {
    if (!state->basketActive_f || itemsFailed == 0 || itemsFailed > state->basketItemCount) {
        return 0;
    }

    /* Refund the last N items that failed */
    uint16_t refundAmount = 0;
    for (uint8_t i = 0; i < itemsFailed; i++) {
        uint8_t idx = state->basketItemCount - 1 - i;
        refundAmount += state->basketPrices[idx];
    }

    return refundAmount;
}

/**
 * @brief Handle vend request
 */
static uint8_t cashless_handleVendRequest(CashlessState *state) {
    /* Check if user has sufficient funds */
    if (state->userFunds >= state->vendAmount) {
        state->vendApproved_f = 1;
        state->vendState = CASHLESS_VEND_APPROVED;

        /* Don't deduct yet - wait for VEND SUCCESS */
        tX9Bits(0, ACK);
        return 1;
    } else {
        state->vendDenied_f = 1;
        state->vendState = CASHLESS_VEND_IDLE;
        tX9Bits(0, ACK);
        return 0;
    }
}

/**
 * @brief Encode item number in PA101 format - MDB 4.3
 *
 * PA101 format: Variable-length encoding
 * - MSB = 1 indicates more bytes follow
 * - MSB = 0 indicates last byte
 */
static uint8_t cashless_encodePA101ItemNumber(uint16_t itemNumber, uint8_t *buffer) {
    uint8_t len = 0;

    if (itemNumber <= 0x7F) {
        /* Single byte: 0xxxxxxx */
        buffer[len++] = itemNumber & 0x7F;
    } else if (itemNumber <= 0x3FFF) {
        /* Two bytes: 1xxxxxxx 0xxxxxxx */
        buffer[len++] = 0x80 | ((itemNumber >> 7) & 0x7F);
        buffer[len++] = itemNumber & 0x7F;
    } else {
        /* Three bytes: 1xxxxxxx 1xxxxxxx 0xxxxxxx */
        buffer[len++] = 0x80 | ((itemNumber >> 14) & 0x7F);
        buffer[len++] = 0x80 | ((itemNumber >> 7) & 0x7F);
        buffer[len++] = itemNumber & 0x7F;
    }

    return len;
}

/**
 * @brief Decode item number from PA101 format - MDB 4.3
 */
static uint16_t cashless_decodePA101ItemNumber(const uint8_t *buffer) {
    uint16_t itemNumber = 0;
    uint8_t shift = 0;

    for (uint8_t i = 0; i < 4; i++) {
        uint8_t byte = buffer[i];

        if (byte & 0x80) {
            /* More bytes follow */
            itemNumber |= ((uint16_t)(byte & 0x7F) << shift);
            shift += 7;
        } else {
            /* Last byte */
            itemNumber |= ((uint16_t)(byte & 0x7F) << shift);
            break;
        }
    }

    return itemNumber;
}
