#include "find_replace.h"
#include <ctype.h>

static int is_word_character(char character)
{
        return isalnum((unsigned char)character) || character == '_';
}


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

int find_replace(const char *file_name, const char *target_word, const char *replacement_word)
{
        printf("STATUS : Opening file '%s' in read-binary mode\n", file_name);
        FILE *file_pointer = fopen(file_name, "rb");
        if (file_pointer == NULL)
        {
                fprintf(stderr, "STATUS : Error opening file: %s\n", strerror(errno));
                return ERROR_STATUS;
        }

        if (fseek(file_pointer, FILE_SEEK_START, SEEK_END) != 0)
        {
                fprintf(stderr, "STATUS : Error seeking to end of file: %s\n", strerror(errno));
                fclose(file_pointer);
                return ERROR_STATUS;
        }

        long file_size = ftell(file_pointer);
        if (file_size < 0)
        {
                fprintf(stderr, "STATUS : Error getting file size: %s\n", strerror(errno));
                fclose(file_pointer);
                return ERROR_STATUS;
        }

        rewind(file_pointer);
        printf("STATUS : File size determined: %ld bytes\n", file_size);

        // Allocate single dynamic buffer (+1 extra byte for null terminator)
        char *file_buffer = (char *)malloc(file_size + EXTRA_NULL_TERMINATOR_BYTE);
        if (file_buffer == NULL)
        {
                fprintf(stderr, "STATUS : Memory allocation failed for file buffer\n");
                fclose(file_pointer);
                return ERROR_STATUS;
        }
        printf("STATUS : Dynamically allocated %ld bytes on the heap\n", file_size + EXTRA_NULL_TERMINATOR_BYTE);

        // Read file contents into the buffer
        size_t bytes_read = fread(file_buffer, 1, file_size, file_pointer);
        if (bytes_read != (size_t)file_size)
        {
                fprintf(stderr, "STATUS : Warning: Read %zu bytes, expected %ld bytes\n", bytes_read, file_size);
        }

        // Null-terminate the buffer to treat it as a valid C string
        file_buffer[file_size] = '\0';
        printf("STATUS : Successfully loaded file content into single buffer and null-terminated\n");

        // Close read file handle
        fclose(file_pointer);
        printf("STATUS : Closed read file handle safely\n");

        // Get string lengths
        size_t target_length = strlen(target_word);
        size_t replacement_length = strlen(replacement_word);

        if (target_length == 0)
        {
                fprintf(stderr, "STATUS : Error: Target word cannot be empty\n");
                free(file_buffer);
                return ERROR_STATUS;
        }

        int total_occurrences = 0;
        char *current_pointer = file_buffer;

        printf("STATUS : Scanning buffer for occurrences of target word: '%s'\n", target_word);
        while ((current_pointer = strstr(current_pointer, target_word)) != NULL)
        {
                total_occurrences++;
                current_pointer += target_length;
        }

        printf("STATUS : Initial scan complete. Total found: %d\n", total_occurrences);

        if (total_occurrences == 0)
        {
                printf("STATUS : No occurrences found. Exiting without modifying file.\n");
                free(file_buffer);
                return SUCCESS_STATUS;
        }

        // Allocate match index array with the EXACT required size (zero memory waste)
        size_t *match_indices = (size_t *)malloc(total_occurrences * sizeof(size_t));
        if (match_indices == NULL)
        {
                fprintf(stderr, "STATUS : Memory allocation failed for match tracking\n");
                free(file_buffer);
                return ERROR_STATUS;
        }

        int confirmed_occurrences = 0;
        current_pointer = file_buffer;

        while ((current_pointer = strstr(current_pointer, target_word)) != NULL)
        {
                size_t match_offset = (size_t)(current_pointer - file_buffer);
                int is_partial = check_is_partial_match(file_buffer, current_pointer, target_length);

			if (is_partial)
                	{
                        // Isolate and extract the exact compound/embedded word containing the target
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
                        char embedded_word_buffer[256];
                        
                        size_t copy_length = (extracted_word_len < sizeof(embedded_word_buffer) - 1) 
                                             ? extracted_word_len : sizeof(embedded_word_buffer) - 1;

                        memcpy(embedded_word_buffer, word_start, copy_length);
                        embedded_word_buffer[copy_length] = '\0';

                        printf("\n------------------------------------------------------------\n");
                        printf("STATUS : Partial match detected!\n");
                        printf("         Target '%s' is embedded inside the word: '%s'\n", target_word, embedded_word_buffer);
                        printf("PROMPT : Do you want to replace this occurrence? [y/n]: ");
                        
                        int user_response = getchar();
                        int character_cleaner;
                        while ((character_cleaner = getchar()) != '\n' && character_cleaner != EOF);

                        if (user_response == 'y' || user_response == 'Y')
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
                        // Clean whole-word match, accept automatically
                        match_indices[confirmed_occurrences++] = match_offset;
                }

                current_pointer += target_length;
        }

        printf("STATUS : Evaluation complete. Confirmed for replacement: %d of %d\n", 
               confirmed_occurrences, total_occurrences);

        if (confirmed_occurrences == 0)
        {
                printf("STATUS : No replacements confirmed. Exiting without modifying file.\n");
                free(match_indices);
                free(file_buffer);
                return SUCCESS_STATUS;
        }

        // Reallocation & In-Place Memory Shifting
        long length_difference = (long)replacement_length - (long)target_length;
        long new_file_size;
        if(replacement_length >= target_length){
        	new_file_size = file_size + (confirmed_occurrences * length_difference);
	}
	else{
		new_file_size = file_size;
	}
		
        printf("STATUS : Calculating new file size: %ld + (%d * %ld) = %ld bytes\n", file_size, confirmed_occurrences, length_difference, new_file_size);

        // Reallocate the single buffer to accommodate the new size (+1 for null terminator)
        char *resized_buffer = (char *)realloc(file_buffer, new_file_size + EXTRA_NULL_TERMINATOR_BYTE);
        if (resized_buffer == NULL)
        {
                fprintf(stderr, "STATUS : Memory reallocation failed\n");
                free(match_indices);
                free(file_buffer);
                return ERROR_STATUS;
        }
        file_buffer = resized_buffer;
        printf("STATUS : Single buffer successfully reallocated to %ld bytes\n", new_file_size + EXTRA_NULL_TERMINATOR_BYTE);

        printf("STATUS : Performing right-to-left in-place memory shifting and replacement\n");

        size_t current_tail_end = file_size;

        for (int i = confirmed_occurrences - 1; i >= 0; i--)
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
        printf("STATUS : In-place replacement completed successfully\n");

        // Write Modified Buffer Back to File
        printf("STATUS : Reopening file '%s' in write-binary mode for truncation and saving\n", file_name);
        file_pointer = fopen(file_name, "wb");
        if (file_pointer == NULL)
        {
                fprintf(stderr, "STATUS : Error opening file for writing: %s\n", strerror(errno));
                free(file_buffer);
                return ERROR_STATUS;
        }

        size_t bytes_written = fwrite(file_buffer, 1, new_file_size, file_pointer);
        if (bytes_written != (size_t)new_file_size)
        {
                fprintf(stderr, "STATUS : Error writing to file: expected %ld bytes, wrote %zu bytes\n", new_file_size, bytes_written);
                fclose(file_pointer);
                free(file_buffer);
                return ERROR_STATUS;
        }

        printf("STATUS : Successfully wrote %zu bytes back to '%s'\n", bytes_written, file_name);

        // Final cleanup
        fclose(file_pointer);
        free(file_buffer);
        printf("STATUS : File handle closed and heap buffer freed successfully.\n");

        return SUCCESS_STATUS;
}
