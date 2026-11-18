#include "find_replace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int find_replace(const char *file_name, const char *target_word, const char *replacement_word)
{
        if (file_name == NULL || target_word == NULL || replacement_word == NULL)
        {
                fprintf(stderr, "STATUS : Invalid arguments\n");
                return ERROR_STATUS;
        }

        if (target_word[0] == '\0')
        {
                fprintf(stderr, "STATUS : Target word cannot be empty\n");
                return ERROR_STATUS;
        }

        printf("STATUS : Opening file '%s'\n", file_name);

        FILE *file_pointer = fopen(file_name, "rb");

        if (file_pointer == NULL)
        {
                perror("STATUS : File opening failed");
                return ERROR_STATUS;
        }

        if (fseek(file_pointer, 0, SEEK_END) != 0)
        {
                perror("STATUS : File seeking failed");
                fclose(file_pointer);
                return ERROR_STATUS;
        }

        long file_size = ftell(file_pointer);

        if (file_size < 0 || (uintmax_t)file_size > (uintmax_t)SIZE_MAX - 1)
        {
                fprintf(stderr, "STATUS : Invalid file size\n");
                fclose(file_pointer);
                return ERROR_STATUS;
        }

        if (fseek(file_pointer, 0, SEEK_SET) != 0)
        {
                perror("STATUS : File rewind failed");
                fclose(file_pointer);
                return ERROR_STATUS;
        }

        char *file_buffer = malloc((size_t)file_size + 1);

        if (file_buffer == NULL)
        {
                fprintf(stderr, "STATUS : Memory allocation failed\n");
                fclose(file_pointer);
                return ERROR_STATUS;
        }

        size_t bytes_read = fread(file_buffer, 1, (size_t)file_size, file_pointer);

        if (bytes_read != (size_t)file_size)
        {
                fprintf(stderr, "STATUS : File reading failed\n");
                fclose(file_pointer);
                free(file_buffer);
                return ERROR_STATUS;
        }

        file_buffer[bytes_read] = '\0';

        fclose(file_pointer);

        printf("STATUS : File loaded successfully (%zu bytes)\n", bytes_read);

        size_t target_length = strlen(target_word);
        size_t replacement_length = strlen(replacement_word);

        size_t max_occurrences = bytes_read / target_length;

        if (max_occurrences > SIZE_MAX / sizeof(size_t))
        {
                fprintf(stderr, "STATUS : Index array size overflow\n");
                free(file_buffer);
                return ERROR_STATUS;
        }

        size_t *occurrence_indices = NULL;

        if (max_occurrences > 0)
        {
                occurrence_indices =
                    malloc(max_occurrences * sizeof(size_t));

                if (occurrence_indices == NULL)
                {
                        fprintf(stderr, "STATUS : Index allocation failed\n");
                        free(file_buffer);
                        return ERROR_STATUS;
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
                printf("STATUS : No occurrences found\n");

                free(occurrence_indices);
                free(file_buffer);

                return SUCCESS_STATUS;
        }

        size_t new_file_size;
        size_t total_growth = 0;

        if (replacement_length > target_length)
        {
                size_t difference = replacement_length - target_length;

                if (occurrence_count > (SIZE_MAX - 1 - bytes_read) / difference)
                {
                        fprintf(stderr, "STATUS : File size overflow\n");

                        free(occurrence_indices);
                        free(file_buffer);

                        return ERROR_STATUS;
                }

                total_growth = occurrence_count * difference;

                new_file_size = bytes_read + total_growth;
        }
        else
        {
                size_t difference = target_length - replacement_length;

                if (difference != 0 && occurrence_count > bytes_read / difference)
                {
                        fprintf(stderr, "STATUS : Invalid resulting size\n");

                        free(occurrence_indices);
                        free(file_buffer);

                        return ERROR_STATUS;
                }

                size_t total_reduction = occurrence_count * difference;

                new_file_size = bytes_read - total_reduction;
        }

        printf("STATUS : Original size: %zu bytes\n", bytes_read);
        printf("STATUS : New size: %zu bytes\n", new_file_size);
        

        if (replacement_length > target_length)
        {
                printf("STATUS : Expanding existing buffer\n");

                char *resized_buffer = realloc(file_buffer, new_file_size + 1);

                if (resized_buffer == NULL)
                {
                        fprintf(stderr, "STATUS : Buffer expansion failed\n");

                        free(occurrence_indices);
                        free(file_buffer);

                        return ERROR_STATUS;
                }

                file_buffer = resized_buffer;
        }
        else
        {
                printf("STATUS : Existing buffer capacity is sufficient\n");
        }

        size_t read_pointer = bytes_read;

        size_t write_pointer;

        if (replacement_length < target_length)
        {
                write_pointer = bytes_read;
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

                memcpy(file_buffer + write_pointer, replacement_word,replacement_length);

                read_pointer = match_index;
        }

        size_t remaining_length = read_pointer;

        write_pointer -= remaining_length;

        memmove(file_buffer + write_pointer, file_buffer, remaining_length);

        size_t expected_offset = 0;

	if (replacement_length < target_length)
	{
        	expected_offset = bytes_read - new_file_size;
	}

	if (write_pointer != expected_offset)
	{
        	fprintf(stderr, "STATUS : Pointer calculation error\n");

        	free(occurrence_indices);
        	free(file_buffer);

        	return ERROR_STATUS;
	}

        if (write_pointer > 0)
        {
                memmove(file_buffer, file_buffer + write_pointer, new_file_size);
        }

        file_buffer[new_file_size] = '\0';

        printf("STATUS : Reverse block replacement completed\n");

        // Write modified content to original file

        printf("STATUS : Opening file for writing\n");

        file_pointer = fopen(file_name, "wb");

        if (file_pointer == NULL)
        {
                perror("STATUS : File writing open failed");

                free(occurrence_indices);
                free(file_buffer);

                return ERROR_STATUS;
        }

        size_t bytes_written = fwrite(file_buffer, 1, new_file_size, file_pointer);

        if (bytes_written != new_file_size)
        {
                fprintf(stderr, "STATUS : Write failed. Expected %zu, wrote %zu\n", new_file_size, bytes_written);

                fclose(file_pointer);

                free(occurrence_indices);
                free(file_buffer);

                return ERROR_STATUS;
        }

        if (fclose(file_pointer) != 0)
        {
                perror("STATUS : File closing failed");

                free(occurrence_indices);
                free(file_buffer);

                return ERROR_STATUS;
        }

        free(occurrence_indices);
        free(file_buffer);

        printf("STATUS : Find and replace completed successfully\n");

        return SUCCESS_STATUS;
}

