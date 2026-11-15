#ifndef FIND_REPLACE_H
#define FIND_REPLACE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SUCCESS_STATUS                  0
#define ERROR_STATUS                    1
#define EXPECTED_ARG_COUNT              4
#define EXTRA_NULL_TERMINATOR_BYTE      1
#define FILE_SEEK_START                 0


#define PROGRAM_NAME                    "Resource-Optimized In-Memory C Word Replacer"
#define PROGRAM_VERSION                 "1"


int find_replace(const char *file_name, const char *target_word, const char *replacement_word);

#endif 
