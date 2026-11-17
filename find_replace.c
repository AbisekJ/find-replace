#include "find_replace.h"
#include <stdint.h>

int find_replace(const char *file_name, const char *target_word, const char *replacement_word)
{
        printf("STATUS : Opening file '%s' in read-binary mode\n", file_name);

        FILE *file_pointer = fopen(file_name, "rb");

        if (file_pointer == NULL)
        {
                fprintf(stderr, "STATUS : Error opening file: %s\n", strerror(errno));
                return ERROR_STATUS;
        }

        // Determine file size
        if (fseek(file_pointer, FILE_SEEK_START, SEEK_END) != 0)
        {
                fprintf(stderr, "STATUS : Error seeking to end of file: %s\n", strerror(errno));
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

        rewind(file_pointer);

        printf("STATUS : File size determined: %ld bytes\n", file_size);

        // Allocate single dynamic buffer
        char *file_buffer = (char *)malloc((size_t)file_size + EXTRA_NULL_TERMINATOR_BYTE);

        if (file_buffer == NULL)
        {
                fprintf(stderr, "STATUS : Memory allocation failed for file buffer\n");
                fclose(file_pointer);
                return ERROR_STATUS;
        }

        printf("STATUS : Dynamically allocated %ld bytes on the heap\n", file_size + EXTRA_NULL_TERMINATOR_BYTE);

        // Read file contents into buffer
        size_t bytes_read = fread(file_buffer, 1, (size_t)file_size, file_pointer);

        if (bytes_read != (size_t)file_size)
        {
                fprintf(stderr, "STATUS : Error reading file contents\n");
                fclose(file_pointer);
                free(file_buffer);
                return ERROR_STATUS;
        }

        file_buffer[bytes_read] = '\0';

        printf("STATUS : Successfully loaded file content into single buffer\n");

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

        // count occurrences

        printf("STATUS : Counting occurrences\n");

        size_t occurrence_count = 0;
        size_t read_pointer = 0;

        while (read_pointer <= bytes_read &&
               target_length <= bytes_read - read_pointer)
        {
                if (memcmp(file_buffer + read_pointer,
                           target_word,
                           target_length) == 0)
                {
                        occurrence_count++;
                        read_pointer += target_length;
                }
                else
                {
                        read_pointer++;
                }
        }
        
        printf("STATUS : Initial scan complete. Total found: %zu\n", occurrence_count);

        if (occurrence_count == 0)
        {
                printf("STATUS : No occurrences found. Exiting without modifying file.\n");
                free(file_buffer);
                return SUCCESS_STATUS;
        }


        size_t new_file_size;

        if (replacement_length > target_length)
        {
                size_t length_difference = replacement_length - target_length;

                if (occurrence_count > (SIZE_MAX - bytes_read) / length_difference)
                {
                        fprintf(stderr, "STATUS : File size overflow\n");
                        free(file_buffer);
                        return ERROR_STATUS;
                }

                new_file_size = bytes_read + (occurrence_count * length_difference);
        }
        else
        {
                size_t length_difference = target_length - replacement_length;

                new_file_size = bytes_read - (occurrence_count * length_difference);
        }

        if (new_file_size == SIZE_MAX)
        {
                fprintf(stderr, "STATUS : File size overflow\n");
                free(file_buffer);
                return ERROR_STATUS;
        }

        printf("STATUS : Original file size: %zu bytes\n", bytes_read);
        printf("STATUS : New file size: %zu bytes\n", new_file_size);

        char *resized_buffer = (char *)realloc(file_buffer, new_file_size + EXTRA_NULL_TERMINATOR_BYTE);

        if (resized_buffer == NULL)
        {
                fprintf(stderr, "STATUS : Memory reallocation failed\n");
                free(file_buffer);
                return ERROR_STATUS;
        }

        file_buffer = resized_buffer;

        printf("STATUS : Single buffer successfully resized\n");

        if (replacement_length <= target_length)
        {
                printf("STATUS : Performing forward in-place replacement\n");

                size_t read_pointer = 0;
                size_t write_pointer = 0;

                while (read_pointer < bytes_read)
                {
                        if (target_length <= bytes_read - read_pointer &&
                            memcmp(file_buffer + read_pointer, target_word, target_length) == 0)
                        {

                                memcpy(file_buffer + write_pointer, replacement_word, replacement_length);
                                read_pointer += target_length;
                                write_pointer += replacement_length;
                        }
                        else
                        {

                                file_buffer[write_pointer] = file_buffer[read_pointer];
                                read_pointer++;
                                write_pointer++;
                        }
                }

                file_buffer[write_pointer] = '\0';

        }
        else
        {

                printf("STATUS : Performing backward in-place replacement\n");

                size_t read_pointer = bytes_read;
                size_t write_pointer = new_file_size;

                file_buffer[new_file_size] = '\0';

                while (read_pointer > 0)
                {
                        if (read_pointer >= target_length &&
                            memcmp(file_buffer + read_pointer - target_length, target_word, target_length) == 0)
                        {

                                read_pointer -= target_length;
                                write_pointer -= replacement_length;
                                memcpy(file_buffer + write_pointer, replacement_word, replacement_length);
                        }
                        else
                        {

                                file_buffer[--write_pointer] = file_buffer[--read_pointer];
                        }
                }
        }

        printf("STATUS : In-place replacement completed successfully\n");

        printf("STATUS : Reopening file '%s' in write-binary mode\n", file_name);

        file_pointer = fopen(file_name, "wb");

        if (file_pointer == NULL)
        {
                fprintf(stderr, "STATUS : Error opening file for writing: %s\n",
                        strerror(errno));

                free(file_buffer);
                return ERROR_STATUS;
        }

        size_t bytes_written = fwrite(file_buffer, 1, new_file_size, file_pointer);

        if (bytes_written != new_file_size)
        {
                fprintf(stderr,
                        "STATUS : Error writing file. Expected %zu bytes, wrote %zu bytes\n",
                        new_file_size,
                        bytes_written);

                fclose(file_pointer);
                free(file_buffer);

                return ERROR_STATUS;
        }

        printf("STATUS : Successfully wrote %zu bytes back to '%s'\n",
               bytes_written,
               file_name);

        // Final cleanup
        fclose(file_pointer);

        free(file_buffer);

        printf("STATUS : File handle closed and heap buffer freed successfully.\n");
        printf("STATUS : Find and replace completed successfully!\n");

        return SUCCESS_STATUS;
}
