#include "embed_utils.h"
#include "../error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int check_bmp_capacity(const BMPImage *image, size_t required_data_bits, int bits_per_pixel) {
    if (!image || !image->infoHeader) {
        fprintf(stderr, ERR_INVALID_BMP);
        return FALSE;
    }

    int pixel_count = get_pixel_count(image);

    size_t capacity_bits = (size_t)pixel_count * bits_per_pixel;

    if (required_data_bits > capacity_bits) {
        fprintf(stderr, ERR_INSUFFICIENT_CAPACITY);
        return FALSE;
    }

    return TRUE;
}


unsigned char *build_secret_buffer(const char *in_file, size_t *required_buffer_len) {
    FILE *secret_fp = fopen(in_file, "rb");
    if (!secret_fp) {
        perror(in_file);
        return NULL;
    }


    fseek(secret_fp, 0, SEEK_END);
    long secret_file_size = ftell(secret_fp);
    fseek(secret_fp, 0, SEEK_SET);

    char *ext = strrchr(in_file, '.');
    // The extension must begin with '.' and end with '\0'
    if (!ext) {
        ext = "";
    }
    size_t ext_len = strlen(ext) + 1; // Includes the '.' and the null terminator '\0'

    // 4 bytes (DWORD size) + secret_file_size + ext_len
    size_t total_len = sizeof(uint32_t) + (size_t)secret_file_size + ext_len;
    *required_buffer_len = total_len;

    // 3. Allocate buffer
    unsigned char *data_buffer = (unsigned char *)malloc(total_len);
    if (!data_buffer) {
        fprintf(stderr, "Error: Failed to allocate memory for the secret buffer.\n");
        fclose(secret_fp);
        return NULL;
    }


    uint32_t size_be = (uint32_t)secret_file_size;

    // Big Endian conversion (MSB first)
    data_buffer[0] = (size_be >> 24) & 0xFF;
    data_buffer[1] = (size_be >> 16) & 0xFF;
    data_buffer[2] = (size_be >> 8) & 0xFF;
    data_buffer[3] = (size_be >> 0) & 0xFF; // LSB

    size_t data_start_offset = sizeof(uint32_t);
    if (fread(data_buffer + data_start_offset, 1, (size_t)secret_file_size, secret_fp) != (size_t)secret_file_size) {
        fprintf(stderr, "Error: Failed to read all data from file '%s'.\n", in_file);
        free_secret_buffer(data_buffer);
        fclose(secret_fp);
        return NULL;
    }

    size_t ext_start_offset = data_start_offset + secret_file_size;
    memcpy(data_buffer + ext_start_offset, ext, ext_len);

    fclose(secret_fp);
    return data_buffer;
}

/**
 * @brief Frees the memory allocated for the secret buffer.
 */
void free_secret_buffer(unsigned char *buffer) {
    if (buffer) {
        free(buffer);
    }
}
