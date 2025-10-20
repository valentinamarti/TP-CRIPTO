#include <stdint.h>
#include <stdlib.h>
#include "extract_utils.h"
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


int extract_next_bit(BMPImage *image, int *bit_count, Pixel *current_pixel) {
    int bit = 0;
    unsigned char *component;

    // Determine which component (B, G, R) to read from
    int component_idx = (*bit_count) % 3;

    // Read a new pixel if we've used all 3 components of the current one
    if (component_idx == 0) {
        if (fread(current_pixel, sizeof(Pixel), 1, image->in) != 1) {
            // Note: This might just be EOF, not necessarily a hard error
            // if the data size was calculated perfectly.
            fprintf(stderr, "Error: Failed to read pixel data during extraction.\n");
            return -1; // Read error
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