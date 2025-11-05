#include "steganography.h"
#include "../error.h"
#include "embed_utils.h"
#include "extract_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_EXT_LEN 256

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

    return (data_buffer[byte_idx] >> (7 - bit_pos)) & 1;
}

/**
 * @brief Calculates padding bytes per row.
 * @param width Image width in pixels.
 * @return Number of padding bytes (0-3).
 */
 static int get_padding(int width) {
    return (4 - (width * sizeof(Pixel)) % 4) % 4;
}

/**
 * @brief Loads the usable color components (Blue and Green) from the carrier BMP into memory.
 *
 * This function performs the necessary file reading (LSBI Phase 1) to extract only the
 * Blue and Green bytes from the image data, ignoring the Red channel, and stores them in
 * a contiguous memory array. This array is used by the analysis function
 * (lsbi_analyze_and_generate_map) to dynamically determine the inversion map.
 *
 * NOTE: The caller is responsible for freeing the memory allocated to *comp_data.
 *
 * @param image Pointer to the initialized BMPImage structure.
 * @param comp_data Pointer to a memory location that will store the address of the
 * dynamically allocated array (B, G, B, G, ...) upon success.
 * @param count Pointer to a size_t variable that will store the total number of components loaded (2 * pixel_count).
 * @return 0 on success, EXIT_FAILURE on error (I/O or memory allocation).
 */
int lsbi_load_bg_components(const BMPImage *image, unsigned char **comp_data, size_t *count) {
    int pixel_count = get_pixel_count(image);
    if (pixel_count <= 0) {
        fprintf(stderr, "Error: Invalid BMP dimensions for analysis.\n");
        return EXIT_FAILURE;
    }

    size_t required_bytes = (size_t)pixel_count * 2;
    *comp_data = (unsigned char *)malloc(required_bytes);
    if (!(*comp_data)) {
        fprintf(stderr, "Error: Failed to allocate memory for LSBI analysis data.\n");
        return EXIT_FAILURE;
    }

    *count = required_bytes;
    if (fseek(image->in, image->fileHeader->bfOffBits, SEEK_SET) != 0) {
        fprintf(stderr, "Error: Failed to reset BMP file pointer.\n");
        free(*comp_data);
        return EXIT_FAILURE;
    }

    unsigned char pixel_bytes[3];
    size_t current_comp_index = 0;

    for (int i = 0; i < pixel_count; i++) {
        if (fread(pixel_bytes, 1, 3, image->in) != 3) {
            fprintf(stderr, "Error: Unexpected end of file during component loading.\n");
            free(*comp_data);
            return EXIT_FAILURE;
        }

        (*comp_data)[current_comp_index++] = pixel_bytes[0];
        (*comp_data)[current_comp_index++] = pixel_bytes[1];
    }

    return EXIT_SUCCESS;
}

/**
 * @brief Simulates the insertion process to generate the inversion map based on change minimization.
 *
 * This function performs the necessary analysis (LSBI Phase 1) to determine the optimal
 * inversion rule for each of the four possible 2-bit patterns (00, 01, 10, 11).
 * It counts the total modifications that would occur with and without LSB inversion for each pattern.
 *
 * @param secret_buffer The complete secret payload (Size | Data | Ext) to be hidden.
 * @param buffer_len Length in bytes of the payload.
 * @param bmp_comp_data Array of all usable Blue and Green component bytes from the carrier BMP (must be pre-loaded).
 * @param total_comp_count Total number of usable Blue/Green components available in the array.
 * @return The 4-bit inversion map (unsigned char), where a '1' at bit 'p' means Pattern 'p' must be inverted during embedding.
 */
unsigned char lsbi_analyze_and_generate_map(const unsigned char *secret_buffer, size_t buffer_len, const unsigned char *bmp_comp_data, size_t total_comp_count) {
    LSBIChangeCounts counts;
    for (int i = 0; i < LSBI_PATTERNS; i++) {
        counts.count_normal[i] = 0;
        counts.count_inverted[i] = 0;
    }

    unsigned char inversion_map = 0;
    const size_t total_secret_bits = buffer_len * 8;
    size_t limit = (total_secret_bits > total_comp_count) ? total_comp_count : total_secret_bits;

    for (size_t comp_idx = 0; comp_idx < limit; comp_idx++) {

        unsigned char original_value = bmp_comp_data[comp_idx];
        unsigned char pattern = (original_value >> 1) & 0x03;
        int secret_bit = get_nth_bit(secret_buffer, comp_idx);


        unsigned char normal_stego = (original_value & 0xFE) | secret_bit;
        unsigned char inverted_stego = normal_stego ^ 0x01;


        if (normal_stego != original_value) {
            counts.count_normal[pattern]++;
        }
        if (inverted_stego != original_value) {
            counts.count_inverted[pattern]++;
        }
    }

    for (int p = 0; p < LSBI_PATTERNS; p++) {
        if (counts.count_inverted[p] <= counts.count_normal[p]) {
            inversion_map |= (1 << p);
        }
    }

    return inversion_map;
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
    if (!image || !image->in) {
        fprintf(stderr, ERR_INVALID_BMP);
        return NULL;
    }

    unsigned char size_buffer[4] = {0};
    Pixel current_pixel;
    int bit_count = 0;
    int bit;

    // --- Step 1: Extract 32-bit Size Header (MSB-first for Big Endian) ---
    for (int i = 0; i < 32; i++) {
        bit = extract_next_bit(image, &bit_count, &current_pixel);
        if (bit == -1) return NULL;

        int byte_idx = i / 8;
        size_t bit_pos = 7 - (i % 8); // MSB-first

        if (bit) {
            size_buffer[byte_idx] |= (1 << bit_pos);
        }
    }

    // Convert Big Endian size buffer to a usable integer
    uint32_t data_size = read_size_header(size_buffer);

    // Sanity check
    long max_capacity_bytes = get_pixel_count(image) * 3 / 8;
    if (data_size == 0 || data_size > max_capacity_bytes) {
        fprintf(stderr, "Error: Invalid or impossibly large data size extracted: %u\n", data_size);
        return NULL;
    }

    // --- Step 2: Allocate memory for the file data ---
    unsigned char *data_buffer = malloc(data_size);
    if (!data_buffer) {
        fprintf(stderr, "Error: Failed to allocate memory for extracted data.\n");
        return NULL;
    }
    memset(data_buffer, 0, data_size);

    // --- Step 3: Extract Data (data_size * 8 bits, MSB-first) ---
    size_t total_bits_to_extract = (size_t)data_size * 8;
    for (size_t i = 0; i < total_bits_to_extract; i++) {
        bit = extract_next_bit(image, &bit_count, &current_pixel);
        if (bit == -1) {
            fprintf(stderr, "Error: Unexpected end of file during data extraction.\n");
            free(data_buffer);
            return NULL;
        }

        size_t byte_idx = i / 8;
        size_t bit_pos = 7 - (i % 8); // MSB-first
        if (bit) {
            data_buffer[byte_idx] |= (1 << bit_pos);
        }
    }

    // --- Step 4: Extract Extension (secuencial hasta '\0') ---
    size_t current_buffer_len = data_size;
    size_t ext_bytes_read = 0;

    while (ext_bytes_read < MAX_EXT_LEN) {
        unsigned char current_ext_byte = 0;

        for (int i = 0; i < 8; i++) {
            bit = extract_next_bit(image, &bit_count, &current_pixel);
            if (bit == -1) {
                fprintf(stderr, "Error: File ended before finding extension terminator.\n");
                free(data_buffer);
                return NULL;
            }

            size_t bit_pos = 7 - i; // MSB-first
            if (bit) {
                current_ext_byte |= (1 << bit_pos);
            }
        }

        size_t new_total_size = current_buffer_len + 1;
        unsigned char *new_buffer = realloc(data_buffer, new_total_size);
        if (!new_buffer) {
            fprintf(stderr, "Error: Failed to reallocate memory for extension.\n");
            free(data_buffer);
            return NULL;
        }
        data_buffer = new_buffer;

        data_buffer[current_buffer_len] = current_ext_byte;
        current_buffer_len = new_total_size;

        if (current_ext_byte == '\0') {
            break; // Found extension
        }

        ext_bytes_read++;
    }

    // --- Step 5: Finalizar y Asignar Tamaños ---
    *extracted_data_len = (size_t)data_size;
    *extension_len = ext_bytes_read + 1;

    return data_buffer;
}

unsigned char *lsb4_extract(BMPImage *image, size_t *extracted_data_len) {
    if (!image || !image->in) {
        fprintf(stderr, ERR_INVALID_BMP);
        return NULL;
    }

    unsigned char size_buffer[4] = {0};
    Pixel current_pixel;
    int bit_count = 0; // Tracks component (0=B, 1=G, 2=R)
    int bit;
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
    unsigned char *bmp_comp_data = NULL; // Componentes B y G cargados
    size_t total_comp_count = 0;         // Cantidad de componentes B y G cargados
    unsigned char calculated_map = 0;

    if (lsbi_load_bg_components(image, &bmp_comp_data, &total_comp_count) != 0) {
        return EXIT_FAILURE;
    }

    calculated_map = lsbi_analyze_and_generate_map(
            secret_buffer,
            buffer_len,
            bmp_comp_data,
            total_comp_count
    );

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