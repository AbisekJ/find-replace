#include "find_replace.h"
#include <stdint.h>

int find_replace(const char *file_name,
                 const char *target_word,
                 const char *replacement_word)
{
        printf("STATUS : Opening file '%s'\n", file_name);

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

        if (file_size < 0 ||
            (uintmax_t)file_size > (uintmax_t)SIZE_MAX - 1)
        {
                fprintf(stderr, "STATUS : Invalid file size\n");
                fclose(file_pointer);
                return ERROR_STATUS;
        }

        rewind(file_pointer);

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

        printf("STATUS : Counting occurrences using strstr()\n");

        size_t occurrence_count = 0;

        const char *search_pointer = file_buffer;
        const char *match_pointer;

        while ((match_pointer = strstr(search_pointer, target_word)) != NULL)
        {
                occurrence_count++;
                search_pointer = match_pointer + target_length;
        }

        printf("STATUS : Total occurrences found: %zu\n", occurrence_count);

        if (occurrence_count == 0)
        {
                printf("STATUS : No occurrences found\n");
                free(file_buffer);
                return SUCCESS_STATUS;
        }

        size_t new_file_size;
        size_t total_growth = 0;

        if (replacement_length > target_length)
        {
                size_t length_difference = replacement_length - target_length;

                if (occurrence_count > (SIZE_MAX - 1 - bytes_read) / length_difference)
                {
                        fprintf(stderr, "STATUS : File size overflow\n");
                        free(file_buffer);
                        return ERROR_STATUS;
                }

                total_growth = occurrence_count * length_difference;
                new_file_size = bytes_read + total_growth;
        }
        else
        {
                size_t length_difference = target_length - replacement_length;

                size_t total_reduction = occurrence_count * length_difference;

                if (total_reduction > bytes_read)
                {
                        fprintf(stderr, "STATUS : Invalid resulting file size\n");
                        free(file_buffer);
                        return ERROR_STATUS;
                }

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
                        free(file_buffer);
                        return ERROR_STATUS;
                }

                file_buffer = resized_buffer;
		
		//strncpy(file_buffer + total_growth, file_buffer, bytes_read + 1);
                memmove(file_buffer + total_growth, file_buffer, bytes_read + 1);

                printf("STATUS : Original content shifted once\n");
        }
        else
        {
                printf("STATUS : Existing buffer capacity is sufficient\n");
        }

        char *read_pointer;
        char *write_pointer = file_buffer;

        if (replacement_length > target_length)
        {
                read_pointer = file_buffer + total_growth;
        }
        else
        {
                read_pointer = file_buffer;
        }

        printf("STATUS : Starting block-level replacement\n");

        while ((match_pointer = strstr(read_pointer, target_word)) != NULL)
        {
                size_t block_length = (size_t)(match_pointer - read_pointer);
                char *next_read_pointer = (char *)match_pointer + target_length;

                memmove(write_pointer, read_pointer, block_length);
		//strncpy(write_pointer, read_pointer, block_length);
		
                write_pointer += block_length;

                //strncpy(write_pointer, replacement_word, replacement_length);
		memmove(write_pointer, replacement_word, replacement_length);
		
                write_pointer += replacement_length;
                
                read_pointer = next_read_pointer;
        }

        size_t remaining_length = strlen(read_pointer) + 1;

        memmove(write_pointer, read_pointer, remaining_length);
	//strncpy(write_pointer, read_pointer, remaining_length);
	
        printf("STATUS : All occurrences replaced successfully\n");

        printf("STATUS : Opening file for writing\n");

        file_pointer = fopen(file_name, "wb");

        if (file_pointer == NULL)
        {
                perror("STATUS : File writing open failed");
                free(file_buffer);
                return ERROR_STATUS;
        }

        size_t bytes_written = fwrite(file_buffer, 1, new_file_size, file_pointer);

        if (bytes_written != new_file_size)
        {
                fprintf(stderr,
                        "STATUS : Write failed. Expected %zu, wrote %zu\n",
                        new_file_size,
                        bytes_written);

                fclose(file_pointer);
                free(file_buffer);

                return ERROR_STATUS;
        }

        if (fclose(file_pointer) != 0)
        {
                perror("STATUS : File closing failed");
                free(file_buffer);
                return ERROR_STATUS;
        }

        free(file_buffer);

        printf("STATUS : Find and replace completed successfully\n");

        return SUCCESS_STATUS;
}
