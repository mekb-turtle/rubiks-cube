#include "err.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// separate err library for use on some Windows environments

extern char *__progname;

static void __vwarnx_internal(const char *format, va_list args) {
	fprintf(stderr, "%s", __progname);
	if (format != NULL) {
		fprintf(stderr, ": ");
		vfprintf(stderr, format, args);
	}
}

#define VA(call)                \
	{                           \
		va_list args;           \
		va_start(args, format); \
		call;                   \
		va_end(args);           \
	}

void err(int exitcode, const char *format, ...) {
	VA(vwarn(format, args))
	exit(exitcode);
}

void errx(int exitcode, const char *format, ...) {
	VA(vwarnx(format, args))
	exit(exitcode);
}

void warn(const char *format, ...) {
	VA(vwarn(format, args))
}

void warnx(const char *format, ...) {
	VA(vwarnx(format, args))
}

void verr(int exitcode, const char *format, va_list args) {
	vwarn(format, args);
	exit(exitcode);
}

void verrx(int exitcode, const char *format, va_list args) {
	vwarnx(format, args);
	exit(exitcode);
}

void vwarn(const char *format, va_list args) {
	__vwarnx_internal(format, args);
	fprintf(stderr, ": %m\n");
}

void vwarnx(const char *format, va_list args) {
	__vwarnx_internal(format, args);
	fprintf(stderr, "\n");
}
