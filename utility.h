#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Safely duplicate a string without using strdup
char *safe_string_copy(const char *source);

// Read dynamic input from standard input with no size limitations
char *read_dynamic_input(const char *prompt_message);

#endif
