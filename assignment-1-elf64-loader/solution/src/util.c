#include <stdarg.h>
#include <stdint.h>
#include <util.h>

size_t str_len(char* str) {
	size_t len = 0;
	while (str[len] != '\0') len++;
	return len;
}

void error_exit(int32_t err_code, char* message) {
	write(2, message, str_len(message));
	exit(err_code);
}
