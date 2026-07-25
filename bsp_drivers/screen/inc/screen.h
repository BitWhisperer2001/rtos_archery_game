#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_ADDRESS            0x78U
#define SCREEN_I2C_TIMEOUT_MS     10U

typedef enum {
    SCREEN_STATUS_OK = 0,
    SCREEN_STATUS_INVALID_ARGUMENT,
    SCREEN_STATUS_TIMEOUT,
    SCREEN_STATUS_NACK,
    SCREEN_STATUS_BUS_ERROR,
    SCREEN_STATUS_ARBITRATION_LOST,
    SCREEN_STATUS_OVERRUN,
    SCREEN_STATUS_RECOVERY_FAILED
} screen_status_t;

void screen_init(void);

screen_status_t screen_data_write(uint8_t address, const uint8_t *data, uint16_t length);

#ifdef __cplusplus
}
#endif
#endif
