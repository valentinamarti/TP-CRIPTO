#include <stdint.h>
#include <stdlib.h>
#include "extract_utils.h"
#include <string.h>
/**
 * @brief Reconstructs a 4-byte Big Endian size from an unsigned char buffer.
 * (Inverse of write_size_header)
 */
uint32_t read_size_header(unsigned char *buffer) {
    // Big Endian (MSB first) to Host conversion
    return ((uint32_t)buffer[0] << 24) |
           ((uint32_t)buffer[1] << 16) |
           ((uint32_t)buffer[2] << 8)  |
           ((uint32_t)buffer[3]);
}


int extract_next_bit(BMPImage *image, int *bit_count, Pixel *current_pixel, int padding) {
    int bit = 0;
    unsigned char *component;

    // Determine which component (B, G, R) to read from
    int component_idx = (*bit_count) % 3;
    static int current_x = 0; 
    static int width = -1;
    if (width == -1) width = image->infoHeader->biWidth;
    // Read a new pixel if we've used all 3 components of the current one
    if (component_idx == 0) {
        if (fread(current_pixel, sizeof(Pixel), 1, image->in) != 1) {
            
            fprintf(stderr, "Error: Failed to read pixel data during extraction.\n");
            return -1; // Read error
        }

        if (current_x == width) {
            // We read the last pixel, now we must skip padding
            if (padding > 0) {
                unsigned char padding_bytes[3];
                if (fread(padding_bytes, 1, padding, image->in) != (size_t)padding) {
                    return -1; // Error skipping padding
                }
            }
            current_x = 0; // Reset for next row
        }
    }

    if (component_idx == 0) component = &(current_pixel->blue);
    else if (component_idx == 1) component = &(current_pixel->green);
    else component = &(current_pixel->red);

    // Extract the LSB
    bit = (*component) & 1;

    (*bit_count)++;
    return bit;
}

int write_secret_from_buffer(const char *out_base_path, unsigned char *buffer, size_t buffer_len) {
    if (buffer_len < 4) {
        fprintf(stderr, "Error: Extracted buffer is too small for size header.\n");
        return 1;
    }

    // 1. Read file size
    uint32_t real_file_size = read_size_header(buffer);

    // 2. Define data and extension pointers
    const unsigned char *data_ptr = buffer + sizeof(uint32_t);
    const char *ext_ptr = (const char *)(data_ptr + real_file_size);

    // Check for extension terminator within buffer bounds
    size_t ext_len = 0;
    for (size_t i = sizeof(uint32_t) + real_file_size; i < buffer_len; i++) {
        ext_len++;
        if (buffer[i] == '\0') {
            break;
        }
    }

    // 4. Construct full output path
    size_t base_len = strlen(out_base_path);
    char *full_out_path = malloc(base_len + ext_len + 1); // +1 for null terminator
    if (!full_out_path) {
        fprintf(stderr, "Error: Failed to allocate memory for output path.\n");
        return 1;
    }
    memcpy(full_out_path, out_base_path, base_len);
    memcpy(full_out_path + base_len, ext_ptr, ext_len); // ext_len includes the '\0'

    // 5. Write the file
    FILE *out_fp = fopen(full_out_path, "wb");
    if (!out_fp) {
        perror(full_out_path);
        free(full_out_path);
        return 1;
    }

    if (fwrite(data_ptr, 1, real_file_size, out_fp) != real_file_size) {
        fprintf(stderr, "Error: Failed to write all data to output file.\n");
        fclose(out_fp);
        free(full_out_path);
        return 1;
    }

    printf("File successfully extracted to: %s\n", full_out_path);
    
    fclose(out_fp);
    free(full_out_path);
    return 0; // Success
}