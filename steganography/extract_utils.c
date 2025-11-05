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
    const unsigned char *data_ptr = buffer;
    const unsigned char *ext_ptr = data_ptr + buffer_len;

    size_t base_len = strlen(out_base_path);
    char *full_out_path = malloc(base_len + extension_len + 1);
    if (!full_out_path) {
        fprintf(stderr, "Error: Failed to allocate memory for output path.\n");
        return 1;
    }
    memcpy(full_out_path, out_base_path, base_len);
    memcpy(full_out_path + base_len, ext_ptr, extension_len);

    FILE *out_fp = fopen(full_out_path, "wb");
    if (!out_fp) {
        perror(full_out_path);
        free(full_out_path);
        return 1;
    }

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


int extract_msb_byte(BMPImage *image, int *bit_count, Pixel *current_pixel, unsigned char inversion_map, int (*bit_extractor)(BMPImage *, int *, Pixel *, unsigned char)) {
    unsigned char assembled_byte = 0;

    for (int i = 0; i < 8; i++) {
        int bit = bit_extractor(image, bit_count, current_pixel, inversion_map);
        if (bit == -1) return -1;

        size_t bit_pos = 7 - i;
        if (bit) {
            assembled_byte |= (1 << bit_pos);
        }
    }
    return (int)assembled_byte;
}

/**
 * @brief Extrae el siguiente bit de datos aplicando las reglas LSBI (MSB-First, Salto R, Re-inversión).
 * * @param image Puntero al BMPImage.
 * @param bit_count Puntero al contador de componentes (0=B, 1=G, 2=R).
 * @param current_pixel Puntero al píxel actual.
 * @param inversion_map Mapa de 4 bits para la lógica de inversión condicional.
 * @return El bit de secreto final (0 o 1), o -1 en caso de error de lectura.
 */
int lsbi_extract_data_bit(BMPImage *image, int *bit_count, Pixel *current_pixel, unsigned char inversion_map) {

    unsigned char stego_value;
    int extracted_lsb, secret_bit;

    // 1. Saltar Canal Rojo (R, índice 2) y leer/avanzar al componente B o G
    do {
        int component_idx = (*bit_count) % 3;

        // Si estamos en el canal Azul (indice 0), leemos un nuevo píxel
        if (component_idx == 0) {
            if (fread(current_pixel, sizeof(Pixel), 1, image->in) != 1) return -1;
        }

        if (component_idx == 2) { // Canal R: saltar y avanzar
            (*bit_count)++;
            continue;
        }

        // Asignar puntero al componente B o G
        unsigned char *component_ptr = (component_idx == 0) ? &(current_pixel->blue) : &(current_pixel->green);
        stego_value = *component_ptr;

        extracted_lsb = stego_value & 1; // Extraer LSB

        (*bit_count)++; // Avanzar al siguiente componente
        break;
    } while (1);

    // 2. Lógica LSBI de Re-Inversión Condicional
    unsigned char pattern = (stego_value >> 1) & 0x03; // Extraer Bits 1 y 2
    unsigned char inversion_flag = (inversion_map >> pattern) & 1;

    secret_bit = (inversion_flag) ? (extracted_lsb ^ 1) : extracted_lsb; // Re-invertir si el flag está activo

    return secret_bit;
}