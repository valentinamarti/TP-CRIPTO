#include <stdint.h>
#include <stdlib.h>
#include "extract_utils.h"
#include "embed_utils.h"
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


int extract_next_bit(BMPImage *image, int *bit_count, Pixel *current_pixel) {
    int bit = 0;
    unsigned char *component;

    // Determine which component (B, G, R) to read from
    int component_idx = (*bit_count) % 3;
    // Read a new pixel if we've used all 3 components of the current one
    if (component_idx == 0) {
        if (fread(current_pixel, sizeof(Pixel), 1, image->in) != 1) {
            
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

int write_secret_from_buffer(const char *out_base_path, unsigned char *buffer, size_t buffer_len, size_t extension_len) {
    if (buffer_len < 4) {
        fprintf(stderr, "Error: Extracted buffer is too small for size header.\n");
        return 1;
    }


    // 2. Define data and extension pointers
    const unsigned char *data_ptr = buffer;
    const unsigned char *ext_ptr = data_ptr + buffer_len;

    // 4. Construct full output path
    size_t base_len = strlen(out_base_path);
    char *full_out_path = malloc(base_len + extension_len + 1); // +1 for null terminator
    if (!full_out_path) {
        fprintf(stderr, "Error: Failed to allocate memory for output path.\n");
        return 1;
    }
    memcpy(full_out_path, out_base_path, base_len);
    memcpy(full_out_path + base_len, ext_ptr, extension_len); // ext_len includes the '\0'

    // 5. Write the file
    FILE *out_fp = fopen(full_out_path, "wb");
    if (!out_fp) {
        perror(full_out_path);
        free(full_out_path);
        return 1;
    }

    // unsigned char * file_size = malloc(sizeof(uint32_t));
    // write_size_header(file_size,buffer_len);
    // fwrite(file_size,1,sizeof(uint32_t),out_fp);

    if (fwrite(data_ptr, 1, buffer_len, out_fp) != buffer_len) {
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

unsigned char extract_nibble(BMPImage *image, int *bit_count, Pixel *current_pixel) {
    unsigned char *component;
    int component_idx = (*bit_count) % 3; // 0=B, 1=G, 2=R

    if (component_idx == 0) {
        if (fread(current_pixel, sizeof(Pixel), 1, image->in) != 1) {
            fprintf(stderr, "Error: Falló la lectura de píxel durante la extracción LSB4.\n");
            return 0xFF;
        }
    }

    if (component_idx == 0) component = &(current_pixel->blue);
    else if (component_idx == 1) component = &(current_pixel->green);
    else component = &(current_pixel->red);

    unsigned char nibble = (*component) & 0x0F;

    (*bit_count)++;

    return nibble;
}