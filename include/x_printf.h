#include <stdarg.h>

extern int (*pr_out)(const char *, va_list);

void set_printf(BOOL buffer);
int x_printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
