#include "log.h"
#include <stdarg.h>
#include <stdio.h>

void
log_debug(const char *format, ...)
{
	va_list ap;

	fprintf(stderr, "DEBUG: ");

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);

	fprintf(stderr, "\n");
	fflush(stderr);
}

void
log_error(const char *format, ...)
{
	va_list ap;

	fprintf(stderr, "ERROR: ");

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);

	fprintf(stderr, "\n");
	fflush(stderr);
}
