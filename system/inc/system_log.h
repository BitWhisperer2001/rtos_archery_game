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

extern volatile bool IsDMALogBusy;

#define TX_BUF_LOG_SIZE     128

// extern void sys_log_debug(const char* fmt, ...);
extern void sys_log_send_string(const char* s);
extern void sys_log_send_char(char c);
#define DBG_LOG(fmt, ...)   sys_log_debug("[DBG]" fmt "\r\n", ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif  // __cplusplus
#endif  // _SYSTEM_LOG_H