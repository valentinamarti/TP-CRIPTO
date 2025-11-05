#ifndef EXTRACT_UTILS_H
#define EXTRACT_UTILS_H

#include <stdint.h>
#include "../bmp_lib.h"



/**
 * @brief Reconstructs a 4-byte Big Endian size from an unsigned char buffer.
 * (Inverse of write_size_header)
 * 
 * @param buffer Pointer to the 4-byte buffer holding the size in Big Endian format.
 * @return The reconstructed size as a uint32_t.
 */
uint32_t read_size_header(unsigned char *buffer);

 /**
 * @brief Helper to read a single bit from the image stream.
 * @param image The BMPImage.
 * @param bit_count A pointer to the current bit counter (0-2).
 * @param current_pixel A pointer to the current pixel being read.
 * @return The extracted bit (0 or 1), or -1 on read error.
 */
 int extract_next_bit(BMPImage *image, int *bit_count, Pixel *current_pixel);

 /**
 * @brief Writes a secret from a buffer to a file.
 * @param out_base_path The base path for the output file.
 * @param buffer The buffer containing the secret.
 * @param buffer_len The length of the buffer.
 * @return 0 on success, 1 on error.
 */
 int write_secret_from_buffer(const char *out_base_path, unsigned char *buffer, size_t buffer_len,size_t extension_len);

/**
 * @brief Extracts the 4 Least Significant Bits (LSBs) from the next color component.
 * * Reads the LSB nibble (4 bits) from the next color component (Blue, Green, or Red)
 * in sequence, advancing the component counter. Reads a new pixel when transitioning
 * from Red to Blue.
 * @param image Pointer to the BMPImage structure.
 * @param bit_count Pointer to the component counter (0=B, 1=G, 2=R).
 * @param current_pixel Pointer to the current Pixel data structure.
 * @return The extracted nibble (0x00 - 0x0F), or 0xFF on read error (e.g., EOF).
 */
unsigned char extract_nibble(BMPImage *image, int *bit_count, Pixel *current_pixel);

/**
 * @brief Extrae un byte completo utilizando una función auxiliar de extracción de bits.
 * @param bit_extractor La función a usar para obtener el siguiente bit (ej: lsbi_extract_data_bit).
 * @return El byte ensamblado (MSB-first), o -1 en caso de error.
 */
int extract_msb_byte(BMPImage *image, int *bit_count, Pixel *current_pixel, unsigned char inversion_map, int (*bit_extractor)(BMPImage *, int *, Pixel *, unsigned char));


/**
 * @brief Extrae el siguiente bit de datos aplicando las reglas LSBI (MSB-First, Salto R, Re-inversión).
 * * @param image Puntero al BMPImage.
 * @param bit_count Puntero al contador de componentes (0=B, 1=G, 2=R).
 * @param current_pixel Puntero al píxel actual.
 * @param inversion_map Mapa de 4 bits para la lógica de inversión condicional.
 * @return El bit de secreto final (0 o 1), o -1 en caso de error de lectura.
 */
int lsbi_extract_data_bit(BMPImage *image, int *bit_count, Pixel *current_pixel, unsigned char inversion_map);

#endif