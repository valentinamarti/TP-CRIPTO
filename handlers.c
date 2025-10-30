#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Módulos del proyecto
#include "handlers.h"
#include "error.h"
#include "bmp_lib.h"
#include "steganography/embed_utils.h"
#include "steganography/steganography.h"


int handle_embed_mode(const ProgramArgs *args) {
    BMPImage *image = NULL;
    unsigned char *secret_buffer = NULL;
    int result = NO_SUCCESS;

    size_t buffer_len_bytes = 0;
    size_t required_bits = 0;
    int bits_per_pixel = 0;

    image = open_bmp(args->bitmap_file);
    if (!image) {
        goto cleanup;
    }

    secret_buffer = build_secret_buffer(args->input_file, &buffer_len_bytes);
    if (!secret_buffer) {
        goto cleanup;
    }

    if (strcmp(args->steg_algorithm, "LSB1") == 0) {
        bits_per_pixel = LSB1_BITS_PER_PIXEL;
        required_bits = buffer_len_bytes * 8;
    } else if (strcmp(args->steg_algorithm, "LSB4") == 0) {
        bits_per_pixel = LSB4_BITS_PER_PIXEL;
        required_bits = buffer_len_bytes * 8;
    } else if (strcmp(args->steg_algorithm, "LSBI") == 0) {
        bits_per_pixel = LSB1_BITS_PER_PIXEL;
        required_bits = (buffer_len_bytes * 8) + LSBI_CONTROL_BITS;
    } else {
        fprintf(stderr, ERR_INVALID_STEG_ALGORITHM, args->steg_algorithm);
        goto cleanup;
    }

    if (check_bmp_capacity(image, required_bits, bits_per_pixel) != TRUE) {
        goto cleanup;
    }

    FILE *out_fp = fopen(args->output_file, "wb");
    if (!out_fp) {
        perror(args->output_file);
        goto cleanup;
    }
    image->out = out_fp;

    if (strcmp(args->steg_algorithm, "LSB1") == 0) {
        if (embed_lsb1(image, secret_buffer, buffer_len_bytes, args->output_file) == 0) {
            printf("success");
            result = SUCCESS;
        }
    } else if(strcmp(args->steg_algorithm, "LSB4") == 0) {
        return SUCCESS;
    } else if (strcmp(args->steg_algorithm, "LSBI") == 0) {
        return SUCCESS;
    }


    cleanup:
    free_secret_buffer(secret_buffer);

    if (image) {
        free_bmp_image(image);
    }

    return result;
}