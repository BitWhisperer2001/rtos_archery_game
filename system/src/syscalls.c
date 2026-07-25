
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <stdint.h>

#include "system_fault.h"
#include "system_log.h"

/* Variables */
/* errno is supplied by newlib; redeclaring it breaks re-entrant configurations. */
/* Linker-owned bounds keep libc allocations away from the reserved main stack. */
extern char __heap_start__;
extern char __heap_end__;

char *__env[1] = { 0 };
char **environ = __env;



/* Functions */
/* Explicit void prototype keeps this semihosting compatibility hook warning-free. */
void initialise_monitor_handles(void)
{
}

int _getpid(void)
{
	return 1;
}

int _kill(int pid, int sig)
{
	(void)pid;
	(void)sig;
	errno = EINVAL;
	return -1;
}

void _exit (int status)
{
	/* libc termination is a firmware fault, not a valid process lifecycle. */
	(void)status;
	system_panic(SYSTEM_PANIC_LIBC_EXIT);
}

__attribute__((weak)) int _read(int file, char *ptr, int len)
{
	/*
	 * UART RX is not part of this project's console contract. Return a defined
	 * error instead of calling an unresolved weak function at address zero.
	 */
	(void)file;
	(void)ptr;
	(void)len;
	errno = ENOSYS;
	return -1;
}

__attribute__((weak)) int _write(int file, char *ptr, int len)
{
	(void)file;
	int data_index;

	/* Route stdio through the board's concrete UART implementation. */
	if ((ptr == NULL) || (len < 0)) {
		errno = EINVAL;
		return -1;
	}
	for (data_index = 0; data_index < len; data_index++) {
		sys_log_send_char(ptr[data_index]);
	}
	return len;
}

int _close(int file)
{
	(void)file;
	return -1;
}


int _fstat(int file, struct stat *st)
{
	(void)file;
	st->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int file)
{
	(void)file;
	return 1;
}

int _lseek(int file, int ptr, int dir)
{
	(void)file;
	(void)ptr;
	(void)dir;
	return 0;
}

int _open(char *path, int flags, ...)
{
	(void)path;
	(void)flags;
	/* Pretend like we always fail */
	return -1;
}

int _wait(int *status)
{
	(void)status;
	errno = ECHILD;
	return -1;
}

int _unlink(char *name)
{
	(void)name;
	errno = ENOENT;
	return -1;
}

int _times(struct tms *buf)
{
	(void)buf;
	return -1;
}

int _stat(char *file, struct stat *st)
{
	(void)file;
	st->st_mode = S_IFCHR;
	return 0;
}

int _link(char *old, char *new)
{
	(void)old;
	(void)new;
	errno = EMLINK;
	return -1;
}

int _fork(void)
{
	errno = EAGAIN;
	return -1;
}

int _execve(char *name, char **argv, char **env)
{
	(void)name;
	(void)argv;
	(void)env;
	errno = ENOMEM;
	return -1;
}

/**
 _sbrk
 Increase program data space. Malloc and related functions depend on this
**/
caddr_t _sbrk(int incr)
{
	static uintptr_t heap_current;
	uintptr_t heap_start = (uintptr_t)&__heap_start__;
	uintptr_t heap_limit = (uintptr_t)&__heap_end__;
	uintptr_t previous;

	/* Initialize lazily because the symbols are resolved only by the linker. */
	if (heap_current == 0U) {
		heap_current = heap_start;
	}
	previous = heap_current;

	/*
	 * Check both growth and shrink operations without signed pointer overflow.
	 * Newlib receives ENOMEM instead of silently corrupting RTOS or stack RAM.
	 */
	if ((incr >= 0 && (uintptr_t)incr > (heap_limit - heap_current)) ||
	    (incr < 0 && (uintptr_t)(-(int64_t)incr) > (heap_current - heap_start))) {
		errno = ENOMEM;
		return (caddr_t)-1;
	}

	heap_current = (uintptr_t)((int64_t)heap_current + (int64_t)incr);
	return (caddr_t)previous;
}
