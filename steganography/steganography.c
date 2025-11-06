#include "steganography.h"
#include "../error.h"
#include "embed_utils.h"
#include "extract_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// -------------------------------------- STATIC AUXILIARY FUNCTIONS --------------------------------------


/**
 * @brief Handles the generic extraction flow (Header -> Data -> Extension) using a specific byte extraction function.
 * @param image Pointer to the BMPImage.
 * @param data_size_out Pointer to store the extracted data size.
 * @param ext_len_out Pointer to store the extension length.
 * @param get_next_byte_func The algorithm-specific function to call for the next byte.
 * @param ctx Pointer to the context structure containing state (bit_count, pixel, map).
 * @return Pointer to the extracted payload buffer, or NULL on error.
 */
static unsigned char *extract_payload_generic(BMPImage *image, size_t *data_size_out, size_t *ext_len_out, get_next_byte_func_t get_next_byte_func, ExtractionContext *ctx, char encrypted) {
    // --- Step 1: Extract Header (4 bytes) ---
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

    // --- Step 2: Allocate Buffer ---
    size_t total_buffer_allocation = (size_t)data_size + MAX_EXT_LEN;
    unsigned char *data_buffer = malloc(total_buffer_allocation);
    if (!data_buffer) return NULL;
    memset(data_buffer, 0, total_buffer_allocation);

    // --- Step 3: Extract Data ---
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

    // --- Step 4: If encrypted the extension is in the data buffer ---
    if (encrypted != TRUE) {
        *data_size_out = (size_t)data_size;
        *ext_len_out = 0;
        return data_buffer;
    }

    // --- Step 5: Extract Extension ---
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

    // --- Step 6: Finalize and Assign Lengths ---
    *data_size_out = (size_t)data_size;
    *ext_len_out = ext_bytes_read + 1;

    return data_buffer;
}

/**
 * @brief PHASE 1: Simulates LSB insertion to calculate the 4-bit inversion map.
 * @param image Pointer to the BMPImage structure.
 * @param secret_buffer The buffer containing the payload (Size|Data|Ext).
 * @param payload_bits Total number of payload bits (excluding control map).
 * @param calculated_map_out Pointer to store the resulting 4-bit inversion map.
 * @return EXIT_SUCCESS or EXIT_FAILURE on read error.
 */
static int calculate_inversion_map(BMPImage *image, const unsigned char *secret_buffer, size_t payload_bits, unsigned char *calculated_map_out) {
    PatternStats stats[LSBI_PATTERNS] = {0};

    if (fseek(image->in, image->fileHeader->bfOffBits, SEEK_SET) != 0) {
        fprintf(stderr, "Error: Failed to reset file pointer for map calculation.\n");
        return EXIT_FAILURE;
    }

    Pixel current_pixel = {0};
    size_t data_bit_idx = 0;

    while (data_bit_idx < payload_bits) {
        if (fread(&current_pixel, sizeof(Pixel), 1, image->in) != 1) {
            fprintf(stderr, "Error: Unexpected EOF during LSBI simulation.\n");
            return EXIT_FAILURE;
        }

        unsigned char *components[3] = {&(current_pixel.blue), &(current_pixel.green), &(current_pixel.red)};

        for (int i = 0; i < 3; i++) {
            if (data_bit_idx >= payload_bits) break;

            // LSBI Rule: Skip the Red
            if (i == 2) {
                continue;
            }

            int secret_bit = get_nth_bit(secret_buffer, data_bit_idx);
            unsigned char cover_value = *components[i];

            unsigned char pattern = (cover_value >> 1) & 0x03;

            if ((cover_value & 1) != secret_bit) {
                stats[pattern].changed_count++;
            } else {
                stats[pattern].unchanged_count++;
            }

            data_bit_idx++;
        }
    }

    // Calculate the Final Map
    unsigned char calculated_map = 0;
    for (int i = 0; i < LSBI_PATTERNS; i++) {
        // Invert if the count of changed pixels is GREATER than the count of unchanged pixels
        if (stats[i].changed_count > stats[i].unchanged_count) {
            calculated_map |= (1 << i);
        }
    }

    *calculated_map_out = calculated_map;
    return EXIT_SUCCESS;
}

/**
 * @brief PHASE 2: Performs the actual embedding using the calculated inversion map.
 * @param image Pointer to the BMPImage structure.
 * @param secret_buffer The buffer containing the payload (Size|Data|Ext).
 * @param buffer_len Total payload length in bytes.
 * @param inversion_map The 4-bit map calculated in Phase 1.
 * @param required_bits The total number of bits to hide (payload + control).
 * @return EXIT_SUCCESS or EXIT_FAILURE on write error or premature finish.
 */
static int perform_final_embedding(BMPImage *image, const unsigned char *secret_buffer, size_t buffer_len, unsigned char inversion_map, size_t required_bits) {
    // Reset file pointer to the start of pixel data for the final embedding pass
    if (fseek(image->in, image->fileHeader->bfOffBits, SEEK_SET) != 0) {
        fprintf(stderr, "Error: Failed to reset file pointer for final embedding.\n");
        return EXIT_FAILURE;
    }

    // Configure the context with the calculated map
    StegoContext ctx = {
            .data_buffer = (unsigned char *)secret_buffer,
            .data_buffer_len = buffer_len,
            .current_bit_idx = 0, // Starts at 0, ready to embed the 4-bit map first
            .inversion_map = inversion_map
    };

    // Write headers to the output file before iterating through pixels
    if (fwrite(image->fileHeader, sizeof(BMPFileHeader), 1, image->out) != 1 ||
        fwrite(image->infoHeader, sizeof(BMPInfoHeader), 1, image->out) != 1) {
        fprintf(stderr, ERR_FAILED_TO_WRITE_BMP);
        return EXIT_FAILURE;
    }

    // Invoke iteration. The callback handles the map (LSB1) and the payload (LSBI).
    iterate_bmp(image, lsbi_embed_pixel_callback, &ctx);

    // Verification
    if (ctx.current_bit_idx < required_bits) {
        fprintf(stderr, "Error: Steganography process finished prematurely. Wrote %zu bits of %zu required.\n", ctx.current_bit_idx, required_bits);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
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

unsigned char *lsb1_extract(BMPImage *image, size_t *extracted_data_len, size_t *extension_len, char encrypted) {
    ExtractionContext ctx = {0};
    return extract_payload_generic(image, extracted_data_len, extension_len, get_next_byte_lsb1, &ctx, encrypted);
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

unsigned char *lsb4_extract(BMPImage *image, size_t *extracted_data_len, size_t *extension_len, char encrypted) {
    ExtractionContext ctx = {0};
    return extract_payload_generic(image, extracted_data_len, extension_len, get_next_byte_lsb4, &ctx, encrypted);
}

// -------------------------------------- LSBI --------------------------------------
/**
 * @brief Callback for LSBI: conditionally modifies a pixel component based on a bit inversion strategy.
 * PHASE 1 (Bits 0-3): Inserts the 4-bit Inversion Map using standard LSB1 logic (BGR order).
 * PHASE 2 (Bit 4 onwards): Inserts one secret bit into Blue or Green (skips Red), applying conditional LSB inversion to minimize changes.
 */
void lsbi_embed_pixel_callback(Pixel *pixel, void *ctx) {
    StegoContext *stego_ctx = (StegoContext *)ctx;
    const size_t control_limit = LSBI_CONTROL_BITS;
    const size_t total_bits_with_overhead = (stego_ctx->data_buffer_len * 8) + control_limit;

    if (stego_ctx->current_bit_idx >= total_bits_with_overhead) {
        return;
    }

    unsigned char *components[3] = {&(pixel->blue), &(pixel->green), &(pixel->red)};

    for (int i = 0; i < 3; i++) {
        if (stego_ctx->current_bit_idx >= total_bits_with_overhead) {
            return;
        }

        // -- Step 1: Insert the Map using LSB1 ---
        if (stego_ctx->current_bit_idx < control_limit) {

            // 1. Extract Control Bit from the inversion map
            int control_bit = (stego_ctx->inversion_map >> stego_ctx->current_bit_idx) & 1;

            // 2. Standard LSB1 Insertion (Clear LSB, insert bit)
            *components[i] &= 0xFE;
            *components[i] |= control_bit;

            // 3. Advance index and move to the next component/pixel
            stego_ctx->current_bit_idx++;
            continue;
        }

        // --- Step 2: Insert the payload using LSBI logic ---
        if (i == 2) {
            continue;   // Ignore RED
        }

        size_t data_bit_idx = stego_ctx->current_bit_idx - control_limit;
        int bit_to_insert = get_nth_bit(stego_ctx->data_buffer, data_bit_idx);

        unsigned char original_value = *components[i];
        unsigned char pattern = (original_value >> 1) & 0x03;

        *components[i] &= 0xFE;
        *components[i] |= bit_to_insert;

        unsigned char inversion_flag = (stego_ctx->inversion_map >> pattern) & 1;

        if (inversion_flag) {
            *components[i] ^= 0x01;
        }

        stego_ctx->current_bit_idx++;
    }
}

int embed_lsbi(BMPImage *image, const unsigned char *secret_buffer, size_t buffer_len) {
    const size_t payload_bits = buffer_len * 8;
    const size_t required_bits = payload_bits + LSBI_CONTROL_BITS;
    unsigned char inversion_map = 0;

    // LSBI uses 2 bits per pixel (Blue and Green) for the payload.
    if (!check_bmp_capacity(image, required_bits, LSBI_BITS_PER_PIXEL)) {
        return EXIT_FAILURE;
    }

    if (calculate_inversion_map(image, secret_buffer, payload_bits, &inversion_map) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    return perform_final_embedding(image, secret_buffer, buffer_len, inversion_map, required_bits);
}

unsigned char *lsbi_extract(BMPImage *image, size_t *extracted_data_len, size_t *extension_len, char encrypted) {
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

    // --- Step 5: If encrypted the extension is in the data buffer ---
    if (encrypted) {
        *extracted_data_len = (size_t)data_size;
        *extension_len = 0;
        return data_buffer;
    }

    // --- Step 6: Extract Extension (Sequentially until '\0') ---
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

    // --- Step 7: Finalize and Assign Lengths ---
    *extracted_data_len = (size_t)data_size;
    *extension_len = ext_bytes_read + 1;

    return data_buffer;
}