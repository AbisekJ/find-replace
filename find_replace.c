#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
        // Validate command-line arguments
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

        // Determine file size using fseek and ftell
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

        // Clean up file handle (keeping buffer alive for Stage 2)
        fclose(file_pointer);
        printf("[STATUS]: Closed file handle safely\n");

        // Temporary cleanup to test Stage 1 independently
        free(file_buffer);
        printf("[STATUS]: Stage 1 memory freed successfully\n");

        return 0;
}
