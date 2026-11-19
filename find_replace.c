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

// Check if a character is part of a word 
static int is_word_character(char character)
{
        return isalnum((unsigned char)character) || character == '_';
}

// Check if a match location is a partial match inside a larger word
static int check_is_partial_match(const char *buffer_start, const char *match_location, size_t target_length)
{
        int has_left_boundary = 0;
        int has_right_boundary = 0;

        // Check character before the match
        if (match_location > buffer_start)
        {
                if (is_word_character(*(match_location - 1)))
                {
                        has_left_boundary = 1;
                }
        }

        // Check character after the match
        char next_character = *(match_location + target_length);
        if (next_character != '\0' && is_word_character(next_character))
        {
                has_right_boundary = 1;
        }

        return (has_left_boundary || has_right_boundary);
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
        if (file_size < 0 || (size_t)file_size > SIZE_MAX - 1)
        {
                fprintf(stderr, "ERROR : Invalid file size or file too large\n");
                fclose(file_pointer);
                return NULL;
        }

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

// Perform a single find and replace operation 
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

        size_t *match_indices = NULL;

        if (max_occurrences > 0)
        {
                match_indices = malloc(max_occurrences * sizeof(size_t));

                if (match_indices == NULL)
                {
                        fprintf(stderr, "ERROR : Index allocation failed\n");
                        return NULL;
                }
        }

        printf("STATUS : Finding occurrences and evaluating boundaries\n");

        size_t total_occurrences = 0;
        char *current_pointer = file_buffer;

        while ((current_pointer = strstr(current_pointer, target_word)) != NULL)
        {
                total_occurrences++;
                current_pointer += target_length;
        }

        if (total_occurrences == 0)
        {
                printf("STATUS : No occurrences found for target '%s'\n", target_word);
                free(match_indices);
                *out_new_size = current_size;
                return file_buffer;
        }

        size_t confirmed_occurrences = 0;
        current_pointer = file_buffer;

        while ((current_pointer = strstr(current_pointer, target_word)) != NULL)
        {
                size_t match_offset = (size_t)(current_pointer - file_buffer);
                int is_partial = check_is_partial_match(file_buffer, current_pointer, target_length);

                if (is_partial)
                {
                        const char *word_start = current_pointer;
                        while (word_start > file_buffer && is_word_character(*(word_start - 1)))
                        {
                                word_start--;
                        }

                        const char *word_end = current_pointer + target_length;
                        while (*word_end != '\0' && is_word_character(*word_end))
                        {
                                word_end++;
                        }

                        size_t extracted_word_len = (size_t)(word_end - word_start);
                        char *embedded_word_buffer = malloc(extracted_word_len + 1);
                        if (embedded_word_buffer == NULL)
                        {
                                perror("ERROR : Memory allocation failed for embedded word");
                                free(match_indices);
                                return NULL;
                        }

                        memcpy(embedded_word_buffer, word_start, extracted_word_len);
                        embedded_word_buffer[extracted_word_len] = '\0';

                        printf("\n------------------------------------------------------------\n");
                        printf("STATUS : Partial match detected!\n");
                        printf("         Target '%s' is embedded inside the word: '%s'\n", target_word, embedded_word_buffer);
                        printf("PROMPT : Do you want to replace this occurrence? [y/n]: ");

                        free(embedded_word_buffer);
                        
                        char *user_response = read_dynamic_input(NULL);
                        int accept_partial = 0;
                        if (user_response != NULL)
                        {
                                if (user_response[0] == 'y' || user_response[0] == 'Y')
                                {
                                        accept_partial = 1;
                                }
                                free(user_response);
                        }

                        if (accept_partial)
                        {
                                printf("STATUS : User chose to REPLACE partial match.\n");
                                match_indices[confirmed_occurrences++] = match_offset;
                        }
                        else
                        {
                                printf("STATUS : User chose to SKIP partial match.\n");
                        }
                        printf("------------------------------------------------------------\n");
                }
                else
                {
                        match_indices[confirmed_occurrences++] = match_offset;
                }

                current_pointer += target_length;
        }

        if (confirmed_occurrences == 0)
        {
                printf("STATUS : No replacements confirmed after partial match checks.\n");
                free(match_indices);
                *out_new_size = current_size;
                return file_buffer;
        }

        long length_difference = (long)replacement_length - (long)target_length;
        long new_file_size;

        if (replacement_length >= target_length)
        {
                new_file_size = (long)current_size + (confirmed_occurrences * length_difference);
        }
        else
        {
                new_file_size = (long)current_size;
        }

        if (new_file_size < 0 || (uintmax_t)new_file_size > (uintmax_t)SIZE_MAX - 1)
        {
                fprintf(stderr, "ERROR : Resulting file size overflow\n");
                free(match_indices);
                return NULL;
        }

        printf("STATUS : Expanding or adjusting buffer for %zu confirmed replacements\n", confirmed_occurrences);

        char *resized_buffer = realloc(file_buffer, (size_t)new_file_size + 1);
        if (resized_buffer == NULL)
        {
                fprintf(stderr, "ERROR : Buffer reallocation failed\n");
                free(match_indices);
                return NULL;
        }
        file_buffer = resized_buffer;

        printf("STATUS : Starting reverse block-level replacement\n");

        size_t current_tail_end = current_size;

        for (int i = (int)confirmed_occurrences - 1; i >= 0; i--)
        {
                size_t current_index = match_indices[i];
                char *target_location = file_buffer + current_index;

                char *source_address = target_location + target_length;
                char *destination_address = target_location + replacement_length;
                size_t bytes_to_move = current_tail_end - (current_index + target_length);

                memmove(destination_address, source_address, bytes_to_move);
                memcpy(target_location, replacement_word, replacement_length);

                current_tail_end += length_difference;
        }

        free(match_indices);

        file_buffer[new_file_size] = '\0';
        printf("STATUS : Reverse block replacement iteration completed successfully\n");

        *out_new_size = (size_t)new_file_size;
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

                int continue_loop = 0;
                if (strcmp(choice, "y") == 0 || strcmp(choice, "Y") == 0)
                {
                        continue_loop = 1;
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

                if (!continue_loop)
                {
                        break;
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
