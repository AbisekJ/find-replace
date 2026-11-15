#include "find_replace.h"

static void print_banner(void)
{
        printf("============================================================\n");
        printf(" %s (v%s)\n", PROGRAM_NAME, PROGRAM_VERSION);
        printf("============================================================\n");
}

static void print_help(const char *program_name)
{
        print_banner();
        printf("Usage:\n");
        printf("  %s <file_name> <target_word> <replacement_word>\n", program_name);
        printf("  %s --help\n", program_name);
        printf("  %s --version\n", program_name);
        printf("\nDescription:\n");
        printf("  Performs resource-optimized in-place word replacement using a single\n");
        printf("  heap buffer and memmove shifting, avoiding temporary files.\n");
}

static void print_version(void)
{
        printf("%s Version %s\n", PROGRAM_NAME, PROGRAM_VERSION);
}

int main(int argc, char *argv[])
{
        // Handle help flag
        if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0))
        {
                print_help(argv[0]);
                return SUCCESS_STATUS;
        }

        // Handle version flag
        if (argc == 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0))
        {
                print_version();
                return SUCCESS_STATUS;
        }

        // Print banner on execution start
        print_banner();

        if (argc != EXPECTED_ARG_COUNT)
        {
                fprintf(stderr, "[STATUS]: Error: Invalid argument count.\n");
                fprintf(stderr, "Usage: %s <file_name> <target_word> <replacement_word>\n", argv[0]);
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                return ERROR_STATUS;
        }

        char *file_name = argv[1];
        char *target_word = argv[2];
        char *replacement_word = argv[3];

        int result = find_replace(file_name, target_word, replacement_word);
        return result;
}
