#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
        // Validate command line arguments
        if (argc != 4)
        {
                fprintf(stderr, "Usage: %s <file_name> <target_word> <replacement_word>\n", argv[0]);
                return 1;
        }

        char *file_name = argv[1];
        char *target_word = argv[2];
        char *replacement_word = argv[3];

        printf("[STATUS]: Opening file '%s' in read-binary mode\n", file_name);
        FILE *file_pointer = fopen(file_name, "rb");
        if (file_pointer == NULL)
        {
                fprintf(stderr, "[STATUS]: Error opening file: %s\n", strerror(errno));
                return 1;
        }

        if (fseek(file_pointer, 0, SEEK_END) != 0)
        {
                fprintf(stderr, "[STATUS]: Error seeking to end of file: %s\n", strerror(errno));
                fclose(file_pointer);
                return 1;
        }

        long file_size = ftell(file_pointer);
        if (file_size < 0)
        {
                fprintf(stderr, "[STATUS]: Error getting file size: %s\n", strerror(errno));
                fclose(file_pointer);
                return 1;
        }

        rewind(file_pointer);
        printf("[STATUS]: File size determined: %ld bytes\n", file_size);

        // Allocate single dynamic buffer (+1 extra byte for null terminator)
        char *file_buffer = (char *)malloc(file_size + 1);
        if (file_buffer == NULL)
        {
                fprintf(stderr, "[STATUS]: Memory allocation failed for file buffer\n");
                fclose(file_pointer);
                return 1;
        }
        printf("[STATUS]: Dynamically allocated %ld bytes on the heap\n", file_size + 1);

        // Read file contents into the buffer
        size_t bytes_read = fread(file_buffer, 1, file_size, file_pointer);
        if (bytes_read != (size_t)file_size)
        {
                fprintf(stderr, "[STATUS]: Warning: Read %zu bytes, expected %ld bytes\n", bytes_read, file_size);
        }

        // Null-terminate the buffer to treat it as a valid C string
        file_buffer[file_size] = '\0';
        printf("[STATUS]: Successfully loaded file content into single buffer and null-terminated\n");

        // Close read file handle
        fclose(file_pointer);
        printf("[STATUS]: Closed read file handle safely\n");

        // Scan and count target occurrences
        size_t target_length = strlen(target_word);
        size_t replacement_length = strlen(replacement_word);

        if (target_length == 0)
        {
                fprintf(stderr, "[STATUS]: Error: Target word cannot be empty\n");
                free(file_buffer);
                return 1;
        }

        int occurrence_count = 0;
        char *current_pointer = file_buffer;

        printf("[STATUS]: Scanning buffer for occurrences of target word: '%s'\n", target_word);
        while ((current_pointer = strstr(current_pointer, target_word)) != NULL)
        {
                occurrence_count++;
                current_pointer += target_length;
        }

        printf("[STATUS]: Scan complete. Found %d occurrence(s)\n", occurrence_count);

        // Reallocation & In-Place Memory Shifting
        long length_difference = (long)replacement_length - (long)target_length;
        long new_file_size = file_size + (occurrence_count * length_difference);

        printf("[STATUS]: Calculating new file size: %ld + (%d * %ld) = %ld bytes\n", 
               file_size, occurrence_count, length_difference, new_file_size);

        // Reallocate the single buffer to accommodate the new size (+1 for null terminator)
        char *resized_buffer = (char *)realloc(file_buffer, new_file_size + 1);
        if (resized_buffer == NULL)
        {
                fprintf(stderr, "[STATUS]: Memory reallocation failed\n");
                free(file_buffer);
                return 1;
        }
        file_buffer = resized_buffer;
        printf("[STATUS]: Single buffer successfully reallocated to %ld bytes\n", new_file_size + 1);

        if (occurrence_count > 0)
        {
                // Store match indices to process replacements from right to left
                size_t *match_indices = (size_t *)malloc(occurrence_count * sizeof(size_t));
                if (match_indices == NULL)
                {
                        fprintf(stderr, "[STATUS]: Memory allocation failed for match indices\n");
                        free(file_buffer);
                        return 1;
                }

                int index_counter = 0;
                current_pointer = file_buffer;
                while ((current_pointer = strstr(current_pointer, target_word)) != NULL)
                {
                        match_indices[index_counter++] = (size_t)(current_pointer - file_buffer);
                        current_pointer += target_length;
                }

                printf("[STATUS]: Performing right-to-left in-place memory shifting and replacement\n");

                size_t current_tail_end = file_size;

                for (int i = occurrence_count - 1; i >= 0; i--)
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
        }

        file_buffer[new_file_size] = '\0';
        printf("[STATUS]: In-place replacement completed successfully\n");

        printf("[STATUS]: Reopening file '%s' in write-binary mode for truncation and saving\n", file_name);
        file_pointer = fopen(file_name, "wb");
        if (file_pointer == NULL)
        {
                fprintf(stderr, "[STATUS]: Error opening file for writing: %s\n", strerror(errno));
                free(file_buffer);
                return 1;
        }

        size_t bytes_written = fwrite(file_buffer, 1, new_file_size, file_pointer);
        if (bytes_written != (size_t)new_file_size)
        {
                fprintf(stderr, "[STATUS]: Error writing to file: expected %ld bytes, wrote %zu bytes\n", new_file_size, bytes_written);
                fclose(file_pointer);
                free(file_buffer);
                return 1;
        }

        printf("[STATUS]: Successfully wrote %zu bytes back to '%s'\n", bytes_written, file_name);

        // Final cleanup
        fclose(file_pointer);
        free(file_buffer);
        printf("[STATUS]: File handle closed and heap buffer freed. Program execution complete.\n");

        return 0;
}
