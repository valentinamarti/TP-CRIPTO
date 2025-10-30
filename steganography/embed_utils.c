#include "embed_utils.h"
#include "../error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


FILE *get_file_metadata(const char *in_file, SecretFileMetadata *metadata) {
    FILE *secret_fp = fopen(in_file, "rb");
    if (!secret_fp) {
        perror(in_file);
        return NULL;
    }

    fseek(secret_fp, 0, SEEK_END);
    metadata->file_size = ftell(secret_fp);
    fseek(secret_fp, 0, SEEK_SET);

    char *ext = strrchr(in_file, '.');
    if (!ext) {
        ext = "";
    }
    metadata->ext = ext;
    metadata->ext_len = strlen(ext) + 1;

    return secret_fp;
}

void write_size_header(unsigned char *buffer, long file_size) {
    // Cast to uint32_t to enforce the fixed 4-byte (DWORD) size requirement.
    uint32_t size_be = (uint32_t)file_size;

    // Big Endian conversion (MSB first)
    // This ensures protocol compatibility regardless of host architecture (Little Endian vs Big Endian)
    buffer[0] = (size_be >> 24) & 0xFF;
    buffer[1] = (size_be >> 16) & 0xFF;
    buffer[2] = (size_be >> 8) & 0xFF;
    buffer[3] = (size_be >> 0) & 0xFF; // LSB
}

unsigned char *build_secret_buffer(const char *in_file, size_t *required_buffer_len) {
    SecretFileMetadata metadata;
    FILE *secret_fp = NULL;
    unsigned char *data_buffer = NULL;
    size_t total_len;

    secret_fp = get_file_metadata(in_file, &metadata);
    if (!secret_fp) {
        return NULL;
    }

    total_len = sizeof(uint32_t) + (size_t)metadata.file_size + metadata.ext_len;
    *required_buffer_len = total_len;

    data_buffer = (unsigned char *)calloc(1, total_len);
    if (!data_buffer) {
        fprintf(stderr, "Error: Failed to allocate memory for the secret buffer.\n");
        fclose(secret_fp);
        return NULL;
    }

    write_size_header(data_buffer, metadata.file_size);


    size_t data_start_offset = sizeof(uint32_t);
    if (fread(data_buffer + data_start_offset, 1, (size_t)metadata.file_size, secret_fp) != (size_t)metadata.file_size) {
        fprintf(stderr, "Error: Failed to read all data from file '%s'.\n", in_file);
        free_secret_buffer(data_buffer);
        fclose(secret_fp);
        return NULL;
    }

    size_t ext_start_offset = data_start_offset + metadata.file_size;
    memcpy(data_buffer + ext_start_offset, metadata.ext, metadata.ext_len);

    fclose(secret_fp);
    return data_buffer;
}


int check_bmp_capacity(const BMPImage *image, size_t required_data_bits, int bits_per_pixel) {
    if (!image || !image->infoHeader) {
        fprintf(stderr, ERR_INVALID_BMP);
        return FALSE;
    }

    int pixel_count = get_pixel_count(image);

    size_t capacity_bits = (size_t)pixel_count * bits_per_pixel;

    if (required_data_bits > capacity_bits) {
        fprintf(stderr, ERR_INSUFFICIENT_CAPACITY);
        fprintf(stderr, "Capacity: %zu bits. Required: %zu bits.\n", capacity_bits, required_data_bits);
        return FALSE;
    }

    return TRUE;
}

void free_secret_buffer(unsigned char *buffer) {
    if (buffer) {
        free(buffer);
    }
}
