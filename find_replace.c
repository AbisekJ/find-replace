#include "find_replace.h"
#include "utility.h"

// Validate core function arguments
static int validate_arguments(const char *file_name, const char *target_word, const char *replacement_word)
{
        if (file_name == NULL || target_word == NULL || replacement_word == NULL)
        {
                fprintf(stderr, "ERROR : Invalid or NULL arguments provided\n");
                return ERROR_STATUS;
        }

        if (target_word[0] == '\0')
        {
                fprintf(stderr, "ERROR : Target word cannot be empty\n");
                return ERROR_STATUS;
        }

        return SUCCESS_STATUS;
}

// Verify file write accessibility in beginning itself
static int check_file_accessibility(const char *file_name)
{
        if (access(file_name, F_OK) != 0)
        {
                perror("ERROR : File does not exist");
                return ERROR_STATUS;
        }

        if (access(file_name, W_OK) != 0)
        {
                perror("ERROR : File is not writable or access denied");
                return ERROR_STATUS;
        }

        return SUCCESS_STATUS;
}

// Open file, verify size against target word length, allocate buffer, and read contents
static char *load_and_validate_file(const char *file_name, const char *target_word, size_t *out_size)
{
        printf("STATUS : Opening file '%s' for reading\n", file_name);

        FILE *file_pointer = fopen(file_name, "rb");
        if (file_pointer == NULL)
        {
                perror("ERROR : File opening failed");
                return NULL;
        }

        if (fseek(file_pointer, 0, SEEK_END) != 0)
        {
                perror("ERROR : File seeking failed");
                fclose(file_pointer);
                return NULL;
        }

        long file_size = ftell(file_pointer);
        if (file_size < 0 || (uintmax_t)file_size > (uintmax_t)SIZE_MAX - 1)
        {
                fprintf(stderr, "ERROR : Invalid file size or file too large\n");
                fclose(file_pointer);
                return NULL;
        }

        // Handle explicitly empty files
        if (file_size == 0)
        {
                printf("STATUS : File is empty (0 bytes). No matches possible.\n");
                fclose(file_pointer);
                *out_size = 0;
                return NULL;
        }

        size_t target_length = strlen(target_word);

        // Check if file size is smaller than the target word length
        if ((size_t)file_size < target_length)
        {
                printf("STATUS : File size (%ld bytes) is smaller than target word length (%zu bytes). No matches possible.\n", file_size, target_length);
                fclose(file_pointer);
                *out_size = 0;
                return NULL;
        }

        if (fseek(file_pointer, 0, SEEK_SET) != 0)
        {
                perror("ERROR : File rewind failed");
                fclose(file_pointer);
                return NULL;
        }

        char *file_buffer = malloc((size_t)file_size + 1);
        if (file_buffer == NULL)
        {
                fprintf(stderr, "ERROR : Memory allocation failed for file buffer\n");
                fclose(file_pointer);
                return NULL;
        }

        size_t bytes_read = fread(file_buffer, 1, (size_t)file_size, file_pointer);
        if (bytes_read != (size_t)file_size)
        {
                fprintf(stderr, "ERROR : File reading failed. Expected %ld bytes, read %zu bytes\n", file_size, bytes_read);
                fclose(file_pointer);
                free(file_buffer);
                return NULL;
        }

        file_buffer[bytes_read] = '\0';
        fclose(file_pointer);

        printf("STATUS : File loaded successfully (%zu bytes)\n", bytes_read);
        *out_size = bytes_read;
        return file_buffer;
}

// Write modified buffer content back to file with strict validation
static int write_and_validate_file(const char *file_name, const char *file_buffer, size_t new_file_size)
{
        printf("STATUS : Opening file '%s' for writing\n", file_name);

        FILE *file_pointer = fopen(file_name, "wb");
        if (file_pointer == NULL)
        {
                perror("ERROR : File writing open failed");
                return ERROR_STATUS;
        }

        size_t bytes_written = fwrite(file_buffer, 1, new_file_size, file_pointer);
        if (bytes_written != new_file_size)
        {
                fprintf(stderr, "ERROR : Write failed. Expected %zu, wrote %zu\n", new_file_size, bytes_written);
                fclose(file_pointer);
                return ERROR_STATUS;
        }

        if (fclose(file_pointer) != 0)
        {
                perror("ERROR : File closing failed");
                return ERROR_STATUS;
        }

        printf("STATUS : File written and closed successfully\n");
        return SUCCESS_STATUS;
}

// Perform a single find and replace operation on the in-memory buffer
static char *perform_single_replacement(char *file_buffer, size_t current_size, const char *target_word, const char *replacement_word, size_t *out_new_size)
{
        // Check if target and replacement words are identical
        if (strcmp(target_word, replacement_word) == 0)
        {
                printf("STATUS : Target word and replacement word are identical. No changes required for this iteration.\n");
                *out_new_size = current_size;
                return file_buffer;
        }

        size_t target_length = strlen(target_word);
        size_t replacement_length = strlen(replacement_word);

        if (current_size < target_length)
        {
                printf("STATUS : Buffer size is smaller than target word length. No matches possible.\n");
                *out_new_size = current_size;
                return file_buffer;
        }

        size_t max_occurrences = current_size / target_length;

        if (max_occurrences > SIZE_MAX / sizeof(size_t))
        {
                fprintf(stderr, "ERROR : Index array size overflow\n");
                return NULL;
        }

        size_t *occurrence_indices = NULL;

        if (max_occurrences > 0)
        {
                occurrence_indices = malloc(max_occurrences * sizeof(size_t));

                if (occurrence_indices == NULL)
                {
                        fprintf(stderr, "ERROR : Index allocation failed\n");
                        return NULL;
                }
        }

        printf("STATUS : Finding occurrences and storing indices\n");

        size_t occurrence_count = 0;
        char *search_pointer = file_buffer;
        char *match_pointer;

        while ((match_pointer = strstr(search_pointer, target_word)) != NULL)
        {
                occurrence_indices[occurrence_count++] = (size_t)(match_pointer - file_buffer);
                search_pointer = match_pointer + target_length;
        }

        printf("STATUS : Total occurrences found: %zu\n", occurrence_count);

        if (occurrence_count == 0)
        {
                printf("STATUS : No occurrences found for target '%s'\n", target_word);
                free(occurrence_indices);
                *out_new_size = current_size;
                return file_buffer;
        }

        size_t new_file_size;

        if (replacement_length > target_length)
        {
                size_t difference = replacement_length - target_length;

                if (occurrence_count > (SIZE_MAX - 1 - current_size) / difference)
                {
                        fprintf(stderr, "ERROR : File size overflow\n");
                        free(occurrence_indices);
                        return NULL;
                }

                new_file_size = current_size + (occurrence_count * difference);
        }
        else
        {
                size_t difference = target_length - replacement_length;

                if (difference != 0 && occurrence_count > current_size / difference)
                {
                        fprintf(stderr, "ERROR : Invalid resulting size\n");
                        free(occurrence_indices);
                        return NULL;
                }

                new_file_size = current_size - (occurrence_count * difference);
        }

        printf("STATUS : Current size: %zu bytes\n", current_size);
        printf("STATUS : Projected size: %zu bytes\n", new_file_size);

        if (replacement_length > target_length)
        {
                printf("STATUS : Expanding existing buffer\n");

                char *resized_buffer = realloc(file_buffer, new_file_size + 1);
                if (resized_buffer == NULL)
                {
                        fprintf(stderr, "ERROR : Buffer expansion failed\n");
                        free(occurrence_indices);
                        return NULL;
                }

                file_buffer = resized_buffer;
        }
        else
        {
                printf("STATUS : Existing buffer capacity is sufficient\n");
        }

        size_t read_pointer = current_size;
        size_t write_pointer;

        if (replacement_length < target_length)
        {
                write_pointer = current_size;
        }
        else
        {
                write_pointer = new_file_size;
        }

        printf("STATUS : Starting reverse block-level replacement\n");

        for (size_t i = occurrence_count; i > 0; i--)
        {
                size_t match_index = occurrence_indices[i - 1];
                size_t block_start = match_index + target_length;
                size_t block_length = read_pointer - block_start;

                write_pointer -= block_length;
                memmove(file_buffer + write_pointer, file_buffer + block_start, block_length);

                write_pointer -= replacement_length;
                memcpy(file_buffer + write_pointer, replacement_word, replacement_length);

                read_pointer = match_index;
        }

        size_t remaining_length = read_pointer;
        write_pointer -= remaining_length;
        memmove(file_buffer + write_pointer, file_buffer, remaining_length);

        size_t expected_offset = 0;
        if (replacement_length < target_length)
        {
                expected_offset = current_size - new_file_size;
        }

        if (write_pointer != expected_offset)
        {
                fprintf(stderr, "ERROR : Pointer calculation error\n");
                free(occurrence_indices);
                return NULL;
        }

        if (write_pointer > 0)
        {
                memmove(file_buffer, file_buffer + write_pointer, new_file_size);
        }

        file_buffer[new_file_size] = '\0';
        free(occurrence_indices);

        printf("STATUS : Reverse block replacement iteration completed successfully\n");
        *out_new_size = new_file_size;
        return file_buffer;
}

int find_replace(const char *file_name, const char *target_word, const char *replacement_word)
{
        // Validate input arguments
        if (validate_arguments(file_name, target_word, replacement_word) != SUCCESS_STATUS)
        {
                return ERROR_STATUS;
        }

        // Verify file accessibility and write permissions upfront
        if (check_file_accessibility(file_name) != SUCCESS_STATUS)
        {
                return ERROR_STATUS;
        }

        size_t bytes_read = 0;
        char *file_buffer = load_and_validate_file(file_name, target_word, &bytes_read);
        if (file_buffer == NULL)
        {
                return ERROR_STATUS;
        }

        // If file size was smaller than target word or empty, handle gracefully
        if (bytes_read == 0)
        {
                printf("STATUS : No occurrences found (file smaller than target or empty)\n");
                free(file_buffer);
                return SUCCESS_STATUS;
        }

        size_t current_size = bytes_read;

        // Perform the first find and replace iteration with initial parameters
        file_buffer = perform_single_replacement(file_buffer, current_size, target_word, replacement_word, &current_size);
        if (file_buffer == NULL)
        {
                return ERROR_STATUS;
        }
        
	// Multiple find and replace interactive loop using dynamic input
        while (1)
        {
                printf("STATUS : Do you want to perform another find and replace on this file? (y/n): ");
                char *choice = read_dynamic_input(NULL);
                if (choice == NULL)
                {
                        break;
                }

                // Validate choice input
                if (strcmp(choice, "y") == 0 || strcmp(choice, "Y") == 0)
                {
                        free(choice);
                }
                else if (strcmp(choice, "n") == 0 || strcmp(choice, "N") == 0)
                {
                        free(choice);
                        break;
                }
                else
                {
                        fprintf(stderr, "ERROR : Invalid input. Please enter 'y' for yes or 'n' for no.\n");
                        free(choice);
                        continue;
                }

                printf("STATUS : Enter new target word:\n");
                char *next_target = read_dynamic_input("  -> ");
                if (next_target == NULL)
                {
                        break;
                }

                if (next_target[0] == '\0')
                {
                        fprintf(stderr, "ERROR : Target word cannot be empty\n");
                        free(next_target);
                        continue;
                }

                printf("STATUS : Enter new replacement word:\n");
                char *next_replacement = read_dynamic_input("  -> ");
                if (next_replacement == NULL)
                {
                        free(next_target);
                        break;
                }

                file_buffer = perform_single_replacement(file_buffer, current_size, next_target, next_replacement, &current_size);
                
                free(next_target);
                free(next_replacement);

                if (file_buffer == NULL)
                {
                        return ERROR_STATUS;
                }
        }

        // Write modified content back to original file once after all iterations are done
        if (write_and_validate_file(file_name, file_buffer, current_size) != SUCCESS_STATUS)
        {
                free(file_buffer);
                return ERROR_STATUS;
        }

        free(file_buffer);
        printf("STATUS : All find and replace operations completed successfully\n");

        return SUCCESS_STATUS;
}
