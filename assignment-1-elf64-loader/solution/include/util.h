#ifndef UTIL
#define UTIL

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

size_t str_len(char* str);
void error_exit(int32_t err_code, char* message);

#endif
