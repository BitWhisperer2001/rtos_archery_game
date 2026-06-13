#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif  // _cplusplus

#include "ssd1306.h"

#define SCREEN_ADDREES      (0x78)

// extern volatile bool IsDMAScreenBusy;
extern void screen_init(void);
extern void screen_data_write(uint8_t address, uint8_t *data, uint16_t len);

#ifdef _cplusplus
}
#endif  // _cplusplus
#endif  // SCREEN_H

