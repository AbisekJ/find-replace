#include "find_replace.h"

int main(int argc, char *argv[])
{
        if (argc != EXPECTED_ARG_COUNT)
        {
                fprintf(stderr, "Usage: %s <file_name> <target_word> <replacement_word>\n", argv[0]);
                return ERROR_STATUS;
        }

        char *file_name = argv[1];
        char *target_word = argv[2];
        char *replacement_word = argv[3];

        int result = find_replace(file_name, target_word, replacement_word);
        return result;
}
