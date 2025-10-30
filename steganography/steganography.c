#include "steganography.h"
#include "../bmp_lib.h"
#include "../error.h"
#include "embed_utils.h"
#include "extract_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**
 * @brief Retrieves the N-th bit (0-indexed, LSB first) from the data buffer.
 *
 * @param data_buffer The byte array containing the secret message.
 * @param n The bit index to retrieve.
 * @return 1 if the bit is set, 0 otherwise.
 */
static int get_nth_bit(const unsigned char *data_buffer, size_t n) {
    size_t byte_idx = n / 8;
    size_t bit_pos = n % 8;

    if (byte_idx >= 0) {
        return (data_buffer[byte_idx] >> bit_pos) & 1;
    }

    return EXIT_SUCCESS;
}

/**
 * @brief Callback for LSB1: modifies a pixel by inserting 3 bits of the secret message.
 * * Inserts one bit into the LSB of Blue, then Green, then Red (BGR order).
 */
void lsb1_embed_pixel_callback(Pixel *pixel, void *ctx) {
    StegoContext *stego_ctx = (StegoContext *)ctx;

    if (stego_ctx->current_bit_idx >= stego_ctx->data_buffer_len * 8) {
        return;
    }

    unsigned char *components[3] = {&(pixel->blue), &(pixel->green), &(pixel->red)};

    for (int i = 0; i < 3; i++) {
        if (stego_ctx->current_bit_idx >= stego_ctx->data_buffer_len * 8) {
            return;
        }

        // 1. Get the secret bit (0 or 1)
        int secret_bit = get_nth_bit(stego_ctx->data_buffer, stego_ctx->current_bit_idx);

        // 2. Clear the LSB of the component (AND with 0xFE = 1111 1110)
        *components[i] &= 0xFE;

        // 3. Insert the secret bit (OR with 0 or 1)
        *components[i] |= secret_bit;

        // Advance to the next bit to hide
        stego_ctx->current_bit_idx++;
    }
}


int embed_lsb1(BMPImage *image, const unsigned char *secret_buffer, size_t buffer_len, const char *out_file_path) {
    StegoContext ctx = {
            .data_buffer = (unsigned char *)secret_buffer,
            .data_buffer_len = buffer_len,
            .current_bit_idx = 0
    };

    if (fwrite(image->fileHeader, sizeof(BMPFileHeader), 1, image->out) != 1 ||
        fwrite(image->infoHeader, sizeof(BMPInfoHeader), 1, image->out) != 1) {
        fprintf(stderr, ERR_FAILED_TO_WRITE_BMP);
        return EXIT_FAILURE;
    }

    // 3. Process Pixels (Read from image->in, modify, write to image->out)
    iterate_bmp(image, lsb1_embed_pixel_callback, &ctx);

    // 4. Verification (Optional but recommended)
    size_t required_bits = buffer_len * 8;
    if (ctx.current_bit_idx < required_bits) {
        fprintf(stderr, "Warning: Steganography process finished prematurely. %zu bits of %zu were written.\n", ctx.current_bit_idx, required_bits);
        // We still treat this as a success if the process didn't crash, but it's noted.
    }

    return EXIT_SUCCESS;
}

unsigned char *lsb1_extract(BMPImage *image, size_t *extracted_data_len) {
    if (!image || !image->in) {
        fprintf(stderr, ERR_INVALID_BMP);
        return NULL;
    }

    unsigned char size_buffer[4] = {0};
    Pixel current_pixel;
    int bit_count = 0; // Tracks component (0=B, 1=G, 2=R)
    int bit;

    // --- Step 1: Extract 32-bit Size Header ---
    for (int i = 0; i < 32; i++) {
        bit = extract_next_bit(image, &bit_count, &current_pixel);
        if (bit == -1) return NULL; // Read error

        // Set the bit in the correct position in size_buffer
        // We re-assemble LSB first (pos 0) to MSB first (pos 7)
        int byte_idx = i / 8;
        int bit_pos = i % 8;
        if (bit) {
            size_buffer[byte_idx] |= (1 << bit_pos);
        }
    }

    // Convert Big Endian size buffer to a usable integer
    int data_size = read_size_header(size_buffer);
    
    // Sanity check on the size
    long max_capacity_bytes = get_pixel_count(image) * 3 / 8;
    if (data_size == 0 || data_size > max_capacity_bytes) {
        fprintf(stderr, "Error: Invalid or impossibly large data size extracted: %u\n", data_size);
        return NULL;
    }
    
    *extracted_data_len = (size_t)data_size;

    // --- Step 2: Allocate memory for the secret data ---
    unsigned char *data_buffer = malloc(data_size);
    if (!data_buffer) {
        fprintf(stderr, "Error: Failed to allocate memory for extracted data.\n");
        return NULL;
    }
    memset(data_buffer, 0, data_size); // Initialize to zero

    // --- Step 3: Extract the Data (data_size * 8 bits) ---
    size_t total_bits_to_extract = (size_t)data_size * 8;
    for (size_t i = 0; i < total_bits_to_extract; i++) {
        bit = extract_next_bit(image, &bit_count, &current_pixel);
        if (bit == -1) {
            fprintf(stderr, "Error: Unexpected end of file during data extraction.\n");
            free(data_buffer);
            return NULL; // Read error
        }

        // Set the bit in the correct position in data_buffer
        size_t byte_idx = i / 8;
        size_t bit_pos = i % 8;
        if (bit) {
            data_buffer[byte_idx] |= (1 << bit_pos);
        }
    }

    return data_buffer;
}

/**
 * @brief Callback for LSB4: modifies a pixel by inserting 12 bits of the secret message.
 * * Inserts one bit into the LSB of Blue, then Green, then Red (BGR order).
 */
void lsb4_embed_pixel_callback(Pixel *pixel, void *ctx) {
    StegoContext *stego_ctx = (StegoContext *)ctx;
    const size_t total_bits = stego_ctx->data_buffer_len * 8;

    if (stego_ctx->current_bit_idx >= total_bits) {
        return;
    }

    unsigned char *components[3] = {&(pixel->blue), &(pixel->green), &(pixel->red)};

    for (int i = 0; i < 3; i++) {
        if (stego_ctx->current_bit_idx >= total_bits) {
            return;
        }

        size_t byte_idx = stego_ctx->current_bit_idx / 8;
        size_t bit_offset = stego_ctx->current_bit_idx % 8;
        unsigned char secret_4_bits;

        // 1. Get the secret bit (0 or 1)
        if (bit_offset == 0) {
            secret_4_bits = stego_ctx->data_buffer[byte_idx] & 0x0F;
        } else {
            secret_4_bits = (stego_ctx->data_buffer[byte_idx] >> 4) & 0x0F;
        }

        // 2. Clear the LSB of the component (AND con 0xF0 = 1111 0000)
        *components[i] &= 0xF0;

        // 3. Insert the secret bit (OR with 0 or 1)
        *components[i] |= secret_4_bits;

        // Advance to the next bit to hide
        stego_ctx->current_bit_idx += 4;
    }
}


int embed_lsb4(BMPImage *image, const unsigned char *secret_buffer, size_t buffer_len, const char *out_file_path) {
    StegoContext ctx = {
            .data_buffer = (unsigned char *)secret_buffer,
            .data_buffer_len = buffer_len,
            .current_bit_idx = 0
    };

    if (fwrite(image->fileHeader, sizeof(BMPFileHeader), 1, image->out) != 1 ||
        fwrite(image->infoHeader, sizeof(BMPInfoHeader), 1, image->out) != 1) {
        fprintf(stderr, ERR_FAILED_TO_WRITE_BMP);
        return EXIT_FAILURE;
    }

    iterate_bmp(image, lsb4_embed_pixel_callback, &ctx);

    size_t required_bits = buffer_len * 8;
    if (ctx.current_bit_idx < required_bits) {
        fprintf(stderr, "Warning: Steganography process finished prematurely. %zu bits of %zu were written.\n", ctx.current_bit_idx, required_bits);
    }

    return EXIT_SUCCESS;
}