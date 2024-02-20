#ifndef ERR_H
#define ERR_H
#include <stdarg.h>
void err(int exitcode, const char *format, ...);
void errx(int exitcode, const char *format, ...);
void warn(const char *format, ...);
void warnx(const char *format, ...);
void verr(int exitcode, const char *format, va_list args);
void verrx(int exitcode, const char *format, va_list args);
void vwarn(const char *format, va_list args);
void vwarnx(const char *format, va_list args);
#endif //ERR_H
