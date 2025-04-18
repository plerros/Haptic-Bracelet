#define NANOPRINTF_IMPLEMENTATION

#define NANOPRINTF_USE_FIELD_WIDTH_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_PRECISION_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_FLOAT_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_LARGE_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_BINARY_FORMAT_SPECIFIERS 1
#define NANOPRINTF_USE_WRITEBACK_FORMAT_SPECIFIERS 0

#include "../../lib/nanoprintf.h"

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

char printf_buffer[1024];

int npf_vsnprintf(char *buffer, size_t bufsz, char const *format, va_list vlist);

int nanoprintf(const char *str, ...) {
	va_list argptr;
	va_start(argptr,str);

	int len = npf_vsnprintf(printf_buffer, sizeof(printf_buffer), str, argptr);
	fflush(stdout);
	write(1, printf_buffer, len);
	return len;
}