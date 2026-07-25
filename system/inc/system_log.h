#ifndef _SYSTEM_LOG_H
#define _SYSTEM_LOG_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define TX_BUF_LOG_SIZE     128

void sys_log_debug(const char* fmt, ...);
void sys_log_send_string(const char* s);
void sys_log_send_char(char c);

/* Allocate the mutex/completion objects used once the scheduler is running. */
bool sys_log_create_resources(void);

/*
 * Keep DMA ownership private to the logger. The ISR reports completion through
 * this narrow API instead of modifying a driver global from another module.
 */
void sys_log_dma_complete_from_isr(void);
#define DBG_LOG(fmt, ...)   sys_log_debug("[DBG]" fmt "\r\n", ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif  // __cplusplus
#endif  // _SYSTEM_LOG_H
