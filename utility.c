#include "utility.h"

// Safely duplicate a string without using strdup
char *safe_string_copy(const char *source)
{
        if (source == NULL)
        {
                return NULL;
        }

        size_t length = strlen(source);
        char *destination = malloc(length + 1);
        if (destination == NULL)
        {
                perror("STATUS : Memory allocation failed for string copy");
                return NULL;
        }

        memcpy(destination, source, length + 1);
        return destination;
}

// Read dynamic input from standard input with no size limitations
char *read_dynamic_input(const char *prompt_message)
{
        if (prompt_message != NULL)
        {
                printf("%s", prompt_message);
        }

        size_t buffer_size = 128;
        size_t current_length = 0;
        char *buffer = malloc(buffer_size);

        if (buffer == NULL)
        {
                perror("STATUS : Memory allocation failed for input");
                return NULL;
        }

        int character;
        while ((character = getchar()) != '\n' && character != EOF)
        {
                if (current_length + 1 >= buffer_size)
                {
                        size_t new_size = buffer_size * 2;
                        char *temp = realloc(buffer, new_size);
                        if (temp == NULL)
                        {
                                perror("STATUS : Memory reallocation failed for input");
                                free(buffer);
                                return NULL;
                        }

                        buffer = temp;
                        buffer_size = new_size;
                }

                buffer[current_length++] = (char)character;
        }

        if (current_length == 0 && character == EOF)
        {
                free(buffer);
                return NULL;
        }

        buffer[current_length] = '\0';
        return buffer;
}
