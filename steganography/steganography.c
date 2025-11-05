#include "steganography.h"
#include "../error.h"
#include "embed_utils.h"
#include "extract_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Handles the generic extraction flow (Header -> Data -> Extension) using a specific byte extraction function.
 * @param image Pointer to the BMPImage.
 * @param data_size_out Pointer to store the extracted data size.
 * @param ext_len_out Pointer to store the extension length.
 * @param get_next_byte_func The algorithm-specific function to call for the next byte.
 * @param ctx Pointer to the context structure containing state (bit_count, pixel, map).
 * @return Pointer to the extracted payload buffer, or NULL on error.
 */
static unsigned char *extract_payload_generic(BMPImage *image, size_t *data_size_out, size_t *ext_len_out, get_next_byte_func_t get_next_byte_func, ExtractionContext *ctx) {
    // 1. Extract Header (4 bytes)
    unsigned char size_buffer[4] = {0};
    for (int i = 0; i < 4; i++) {
        int extracted_byte = get_next_byte_func(image, ctx);
        if (extracted_byte == -1) return NULL;
        size_buffer[i] = (unsigned char)extracted_byte;
    }

    uint32_t data_size = read_size_header(size_buffer);

    // Sanity check
    long max_capacity_bytes = get_pixel_count(image) * 3 / 8;
    if (data_size == 0 || data_size > max_capacity_bytes) {
        fprintf(stderr, "Error: Invalid or impossibly large data size extracted: %u\n", data_size);
        return NULL;
    }

    // 2. Allocate Buffer
    size_t total_buffer_allocation = (size_t)data_size + MAX_EXT_LEN;
    unsigned char *data_buffer = malloc(total_buffer_allocation);
    if (!data_buffer) return NULL;
    memset(data_buffer, 0, total_buffer_allocation);

    // 3. Extract Data
    size_t current_byte_idx = 0;
    while (current_byte_idx < data_size) {
        int extracted_byte = get_next_byte_func(image, ctx);
        if (extracted_byte == -1) {
            fprintf(stderr, "Error: Unexpected end of file during data extraction.\n");
            free(data_buffer);
            return NULL;
        }
        data_buffer[current_byte_idx] = (unsigned char)extracted_byte;
        current_byte_idx++;
    }

    // 4. Extract Extension
    size_t ext_start_byte = data_size;
    size_t ext_bytes_read = 0;

    while (ext_bytes_read < MAX_EXT_LEN) {
        int extracted_byte = get_next_byte_func(image, ctx);
        if (extracted_byte == -1) {
            fprintf(stderr, "Error: Unexpected end of file before finding extension terminator.\n");
            free(data_buffer);
            return NULL;
        }

        data_buffer[ext_start_byte + ext_bytes_read] = (unsigned char)extracted_byte;

        if (data_buffer[ext_start_byte + ext_bytes_read] == '\0') {
            break;
        }

        ext_bytes_read++;
    }

    if (ext_bytes_read >= MAX_EXT_LEN) {
        fprintf(stderr, "Error: Extension length exceeded maximum allowed size (%d bytes) without terminator.\n", MAX_EXT_LEN);
        free(data_buffer);
        return NULL;
    }

    // 5. Finalize and Assign Lengths
    *data_size_out = (size_t)data_size;
    *ext_len_out = ext_bytes_read + 1;

    return data_buffer;
}

static int get_next_byte_lsb1(BMPImage *image, ExtractionContext *ctx) {
    unsigned char assembled_byte = 0;
    for (int i = 0; i < 8; i++) {
        int bit = extract_next_bit(image, &ctx->bit_count, &ctx->current_pixel);
        if (bit == -1) return -1;

        size_t bit_pos = 7 - i;
        if (bit) {
            assembled_byte |= (1 << bit_pos);
        }
    }
    return (int)assembled_byte;
}

static int get_next_byte_lsb4(BMPImage *image, ExtractionContext *ctx) {
    unsigned char output_byte = 0;

    // NIBBLE 1 (MSB)
    unsigned char nibble1 = extract_nibble(image, &ctx->bit_count, &ctx->current_pixel);
    if (nibble1 == 0xFF) return -1;
    output_byte |= (nibble1 << 4);

    // NIBBLE 2 (LSB)
    unsigned char nibble2 = extract_nibble(image, &ctx->bit_count, &ctx->current_pixel);
    if (nibble2 == 0xFF) return -1;
    output_byte |= nibble2;

    return (int)output_byte;
}

// -------------------------------------- LSB1 --------------------------------------
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


int embed_lsb1(BMPImage *image, const unsigned char *secret_buffer, size_t buffer_len) {
    StegoContext ctx = {
            .data_buffer = (unsigned char *)secret_buffer,
            .data_buffer_len = buffer_len,
            .current_bit_idx = 0,
            .inversion_map = 0
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

unsigned char *lsb1_extract(BMPImage *image, size_t *extracted_data_len, size_t *extension_len) {
    ExtractionContext ctx = {0};
    return extract_payload_generic(image, extracted_data_len, extension_len, get_next_byte_lsb1, &ctx);
}

// -------------------------------------- LSB4 --------------------------------------
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

        // 1. Get the secret nibble (4 bits) in MSB-first order
        if (bit_offset == 0) {
            secret_4_bits = (stego_ctx->data_buffer[byte_idx] >> 4) & 0x0F;
        } else {
            secret_4_bits = stego_ctx->data_buffer[byte_idx] & 0x0F;
        }

        // 2. Clear the 4 LSBs of the component (AND con 0xF0 = 1111 0000)
        *components[i] &= 0xF0;

        // 3. Insert the secret nibble (OR)
        *components[i] |= secret_4_bits;

        // Advance the index by 4 bits (un nibble)
        stego_ctx->current_bit_idx += 4;
    }
}


int embed_lsb4(BMPImage *image, const unsigned char *secret_buffer, size_t buffer_len) {
    StegoContext ctx = {
            .data_buffer = (unsigned char *)secret_buffer,
            .data_buffer_len = buffer_len,
            .current_bit_idx = 0,
            .inversion_map = 0
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

unsigned char *lsb4_extract(BMPImage *image, size_t *extracted_data_len, size_t *extension_len) {
    ExtractionContext ctx = {0};
    return extract_payload_generic(image, extracted_data_len, extension_len, get_next_byte_lsb4, &ctx);
}

// -------------------------------------- LSBI --------------------------------------
void lsbi_embed_pixel_callback(Pixel *pixel, void *ctx) {
    StegoContext *stego_ctx = (StegoContext *)ctx;

    const size_t control_limit = 4;
    const size_t total_bits_with_overhead = (stego_ctx->data_buffer_len * 8) + control_limit;

    if (stego_ctx->current_bit_idx >= total_bits_with_overhead) {
        return;
    }

    unsigned char *components[3] = {&(pixel->blue), &(pixel->green), &(pixel->red)};

    for (int i = 0; i < 3; i++) {
        if (stego_ctx->current_bit_idx >= total_bits_with_overhead) {
            return;
        }

        // ==========================================================
        // FASE 1: CONTROL (Bits 0-3)
        // ==========================================================
        if (stego_ctx->current_bit_idx < control_limit) {

            // 1. Extraer Bit de Control: Lee el bit de la inversion_map (0 o 1)
            int control_bit = (stego_ctx->inversion_map >> stego_ctx->current_bit_idx) & 1;

            // 2. Inserción LSB1 Normal (Limpia LSB con 0xFE, inserta el bit)
            *components[i] &= 0xFE;
            *components[i] |= control_bit;

            // 3. Avance de índice y pasar al siguiente componente/píxel
            stego_ctx->current_bit_idx++;
            continue;
        }

        // ==========================================================
        // FASE 2: DATOS (Bit 4 en adelante)
        // ==========================================================

        // a) Regla LSBI: Ignorar Canal Red (i=2)
        // El LSBI original usa solo Blue y Green como portadores de datos.
        if (i == 2) {
            continue;
        }

        // b) Obtener Bit Secreto (del payload real)
        // Restamos el overhead (4 bits) al índice total para apuntar al buffer del payload.
        size_t data_bit_idx = stego_ctx->current_bit_idx - control_limit;
        int bit_to_insert = get_nth_bit(stego_ctx->data_buffer, data_bit_idx);

        // c) Obtener Patrón Original (Patrón P)
        // Guardamos el valor original y aislamos los Bits 1 y 2 para el patrón (00, 01, 10, 11)
        unsigned char original_value = *components[i];
        unsigned char pattern = (original_value >> 1) & 0x03; // Extrae bits 1 y 2

        // d) Aplicar LSB1 Inicial (El 'stego-pixel' inicial)
        *components[i] &= 0xFE;
        *components[i] |= bit_to_insert;

        // e) Lógica de Inversión Condicional (LSBI)
        // Verificamos si el bit en la 'inversion_map' (1010) correspondiente a 'pattern' es 1.
        unsigned char inversion_flag = (stego_ctx->inversion_map >> pattern) & 1;

        if (inversion_flag) {
            // Si el flag es 1, invertimos el LSB final con XOR 0x01
            *components[i] ^= 0x01;
        }

        // f) Avance de índice (solo si se insertó un bit de datos)
        stego_ctx->current_bit_idx++;
    }
}

int embed_lsbi(BMPImage *image, const unsigned char *secret_buffer, size_t buffer_len) {
    unsigned char *bmp_comp_data = NULL;
    unsigned char calculated_map = 0;

    free(bmp_comp_data);

    StegoContext ctx = {
            .data_buffer = (unsigned char *)secret_buffer,
            .data_buffer_len = buffer_len,
            .current_bit_idx = 0,
            .inversion_map = calculated_map
    };

    if (fwrite(image->fileHeader, sizeof(BMPFileHeader), 1, image->out) != 1 ||
        fwrite(image->infoHeader, sizeof(BMPInfoHeader), 1, image->out) != 1) {
        fprintf(stderr, ERR_FAILED_TO_WRITE_BMP);
        return EXIT_FAILURE;
    }

    if (fseek(image->in, image->fileHeader->bfOffBits, SEEK_SET) != 0) {
        fprintf(stderr, "Error: Failed to reset BMP file pointer for insertion.\n");
        return EXIT_FAILURE;
    }

    iterate_bmp(image, lsbi_embed_pixel_callback, &ctx);

    size_t required_bits = (buffer_len * 8) + 4;
    if (ctx.current_bit_idx < required_bits) {
        fprintf(stderr, "Warning: Steganography process finished prematurely. %zu bits of %zu were written.\n", ctx.current_bit_idx, required_bits);
    }

    return EXIT_SUCCESS;
}

unsigned char *lsbi_extract(BMPImage *image, size_t *extracted_data_len, size_t *extension_len) {
    if (!image || !image->in) {
        fprintf(stderr, ERR_INVALID_BMP);
        return NULL;
    }

    unsigned char inversion_map = 0;
    unsigned char size_buffer[4] = {0};
    Pixel current_pixel = {0};
    int bit_count = 0;
    uint32_t data_size = 0;
    long max_capacity_bytes = get_pixel_count(image) * 3 / 8;

    // --- Step 1: Extract Control Map (4 bits, LSB1 Standard) ---
    for (int i = 0; i < LSBI_CONTROL_BITS; i++) {
        int extracted_lsb = extract_next_bit(image, &bit_count, &current_pixel);
        if (extracted_lsb == -1) return NULL;
        if (extracted_lsb) {
            inversion_map |= (1 << i);
        }
    }

    // --- Step 2: Extract Header (4 Bytes, using LSBI Logic) ---
    for (int i = 0; i < 4; i++) {
        int extracted_byte = extract_msb_byte(image, &bit_count, &current_pixel, inversion_map, lsbi_extract_data_bit);
        if (extracted_byte == -1) return NULL;
        size_buffer[i] = (unsigned char)extracted_byte;
    }

    data_size = read_size_header(size_buffer);
    if (data_size == 0 || data_size > max_capacity_bytes) return NULL;

    // --- Step 3: Allocate memory for Data + Extension ---
    size_t total_data_allocation = (size_t)data_size + MAX_EXT_LEN;
    unsigned char *data_buffer = malloc(total_data_allocation);
    if (!data_buffer) return NULL;
    memset(data_buffer, 0, total_data_allocation);
    size_t current_byte_idx = 0;

    // --- Step 4: Extract Data (data_size bytes) ---
    while (current_byte_idx < data_size) {
        int extracted_byte = extract_msb_byte(image, &bit_count, &current_pixel, inversion_map, lsbi_extract_data_bit);
        if (extracted_byte == -1) {
            fprintf(stderr, "Error: Unexpected end of file during data extraction.\n");
            free(data_buffer);
            return NULL;
        }
        data_buffer[current_byte_idx] = (unsigned char)extracted_byte;
        current_byte_idx++;
    }

    // --- Step 5: Extract Extension (Sequentially until '\0') ---
    size_t ext_bytes_read = 0;
    size_t ext_start_byte = data_size;

    while (ext_bytes_read < MAX_EXT_LEN) {
        int extracted_byte = extract_msb_byte(image, &bit_count, &current_pixel, inversion_map, lsbi_extract_data_bit);
        if (extracted_byte == -1) {
            fprintf(stderr, "Error: Unexpected end of file before finding extension terminator.\n");
            free(data_buffer);
            return NULL;
        }

        data_buffer[ext_start_byte + ext_bytes_read] = (unsigned char)extracted_byte;
        if (data_buffer[ext_start_byte + ext_bytes_read] == '\0') {
            break;
        }
        ext_bytes_read++;
    }

    if (ext_bytes_read >= MAX_EXT_LEN) {
        fprintf(stderr, "Error: Extension length exceeded maximum allowed size (%d bytes) without terminator.\n", MAX_EXT_LEN);
        free(data_buffer);
        return NULL;
    }

    // --- Step 6: Finalize and Assign Lengths ---
    *extracted_data_len = (size_t)data_size;
    *extension_len = ext_bytes_read + 1;

    return data_buffer;
}