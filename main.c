#include "find_replace.h"
#include "utility.h"

// Print the program banner
static void print_banner(void)
{
        printf("============================================================\n");
        printf(" %s (v%s)\n", PROGRAM_NAME, PROGRAM_VERSION);
        printf("============================================================\n");
}

// Print usage instructions
static void print_help(const char *program_name)
{
        print_banner();
        printf("Usage:\n");
        printf("  %s -f <file_name> -t <target_word> -r <replacement_word>\n", program_name);
        printf("  (Arguments can be provided via flags or prompted interactively)\n");
        printf("  %s --help\n", program_name);
        printf("  %s --version\n", program_name);
        printf("\nFlags:\n");
        printf("  -f, --file        Path to the target file\n");
        printf("  -t, --target      Word to search for\n");
        printf("  -r, --replace     Word to replace with\n");
}

// Print version information
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

        char *file_name = NULL;
        char *target_word = NULL;
        char *replacement_word = NULL;

        // Parse command-line arguments 
        for (int i = 1; i < argc; i++)
        {
                if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--file") == 0) && (i + 1 < argc))
                {
                        file_name = safe_string_copy(argv[++i]);
                }
                else if ((strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--target") == 0) && (i + 1 < argc))
                {
                        target_word = safe_string_copy(argv[++i]);
                }
                else if ((strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--replace") == 0) && (i + 1 < argc))
                {
                        replacement_word = safe_string_copy(argv[++i]);
                }
                else
                {
                        fprintf(stderr, "ERROR : Invalid or unknown argument/flag -> '%s'\n", argv[i]);
                        fprintf(stderr, "Try '%s --help' for usage information.\n", argv[0]);
                        return ERROR_STATUS;
                }
        }

        // Prompt interactively for any missing parameters at runtime
        if (file_name == NULL)
        {
                printf("STATUS : File path not provided. Please enter target file path:\n");
                file_name = read_dynamic_input("  -> ");
        }

        if (target_word == NULL)
        {
                printf("STATUS : Target word not provided. Please enter target word:\n");
                target_word = read_dynamic_input("  -> ");
        }

        if (replacement_word == NULL)
        {
                printf("STATUS : Replacement word not provided. Please enter replacement word:\n");
                replacement_word = read_dynamic_input("  -> ");
        }

        // Validate that all required parameters are successfully resolved
        if (file_name == NULL || target_word == NULL || replacement_word == NULL)
        {
                fprintf(stderr, "ERROR : Failed to resolve required arguments.\n");
                
                free(file_name);
                free(target_word);
                free(replacement_word);

                return ERROR_STATUS;
        }

        printf("STATUS : Arguments successfully configured:\n");
        printf("STATUS :   File       -> %s\n", file_name);
        printf("STATUS :   Target     -> %s\n", target_word);
        printf("STATUS :   Replacement-> %s\n", replacement_word);

        int result = find_replace(file_name, target_word, replacement_word);

        // Free dynamically allocated parameters
        free(file_name);
        free(target_word);
        free(replacement_word);

        return result;
}
