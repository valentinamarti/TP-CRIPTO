#include "steganography.h"
#include "../bmp_lib.h"
#include "../error.h"
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