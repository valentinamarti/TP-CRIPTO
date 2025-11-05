#ifndef EMBED_UTILS_H
#define EMBED_UTILS_H

#include <stddef.h>
#include <stdint.h>
#include "../bmp_lib.h"

#define LSB1_BITS_PER_PIXEL 3
#define LSB4_BITS_PER_PIXEL 12
#define LSBI_BITS_PER_PIXEL 2   // Because it does not use R
#define LSBI_CONTROL_BITS 4

#define TRUE 1
#define FALSE 0

/**
 * @brief Auxiliary structure to hold file size and extension details after I/O.
 */
typedef struct {
    long file_size;       // Tamaño real del archivo en bytes (para el campo DWORD)
    char *ext;            // Puntero a la extensión (ej: ".txt")
    size_t ext_len;       // Longitud de la extensión incluyendo '.' y '\0'
} SecretFileMetadata;

/**
 * @brief Reads the size and extension of the secret file.
 *
 * @param in_file Path to the secret file.
 * @param metadata Pointer to the struct where results will be stored.
 * @return FILE* pointer to the opened file (reset to SEEK_SET), or NULL on error.
 */
FILE *get_file_metadata(const char *in_file, SecretFileMetadata *metadata);

/**
 * @brief Writes the 4-byte file size into the buffer in Big Endian format.
 * * This function performs the necessary Host Endian to Big Endian conversion.
 *
 * @param buffer Pointer to the destination buffer (must have at least 4 bytes allocated).
 * @param file_size The real size of the file to be encoded.
 */
void write_size_header(unsigned char *buffer, long file_size);

/**
 * @brief Utility function to check if the secret data fits in the carrier BMP.
 * * Compares the required bits (from buffer length) with the available capacity
 * based on the steganography algorithm's efficiency (bits_per_pixel).
 *
 * @param image Pointer to the initialized BMPImage structure (should be non-NULL).
 * @param p_file Path to the carrier BMP file (used only for error messaging).
 * @param required_data_bits Total number of bits needed to hide (Message Bits + Overhead).
 * @param bits_per_pixel The number of bits the steganography algorithm can hide per pixel (e.g., 3 for LSB1).
 * @return TRUE on success (fits), FALSE on error (doesn't fit or invalid image).
 */
int check_bmp_capacity(const BMPImage *image, size_t required_data_bits, int bits_per_pixel);

/**
 * @brief Constructs the final secret buffer (Size|Data|Ext) ready for steganography
 *
 * The format is: Real Size (4 bytes, Big Endian) || Data || .ext\0
 * NOTE: This function allocates memory for the buffer, which MUST be freed by the caller
 *
 * @param in_file Path to the secret file
 * @param required_buffer_len Pointer to store the total length of the constructed buffer in bytes
 * @return Pointer to the allocated and filled secret buffer, or NULL on error
 */
unsigned char *build_secret_buffer(const char *in_file, size_t *required_buffer_len);

/**
 * @brief Retrieves the N-th bit (0-indexed, LSB first) from the data buffer.
 *
 * @param data_buffer The byte array containing the secret message.
 * @param n The bit index to retrieve.
 * @return 1 if the bit is set, 0 otherwise.
 */
int get_nth_bit(const unsigned char *data_buffer, size_t n);

/**
 * @brief Frees the memory allocated for the secret buffer.
 * * Should be called by the module responsible for consuming the buffer
 * after the embedding process is complete.
 *
 * @param buffer Pointer to the secret buffer to free.
 */
void free_secret_buffer(unsigned char *buffer);

#endif