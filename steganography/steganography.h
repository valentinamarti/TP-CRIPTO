#ifndef STEGANOGRAPHY_H
#define STEGANOGRAPHY_H

#include <stddef.h>
#include <stdint.h>
#include "../bmp_lib.h"


/**
 * @brief Steganography context structure.
 *
 * Holds the state of the secret buffer and the current position
 * during the embedding/extraction process.
 */
typedef struct {
    unsigned char *data_buffer;     // Complete buffer: [Size 4B] | [Data...] | [Ext. \0]
    size_t data_buffer_len;         // Total length of the buffer in bytes
    size_t current_bit_idx;         // Index of the current bit being inserted (0-based)
} StegoContext;


/**
 * @brief Hides the pre-built secret buffer inside a BMP using the LSB1 algorithm.
 * The BMPImage is already open (image->in) and the output file is linked (image->out).
 *
 * @param image Pointer to the initialized BMPImage structure (open by the caller)
 * @param secret_buffer Pointer to the pre-built buffer containing the message
 * @param buffer_len Total length of the secret_buffer in bytes
 * @param out_file_path Path to the output file (used for writing the final stego-image)
 * @return 0 on success, 1 on error.
 */
int embed_lsb1(BMPImage *image, const unsigned char *secret_buffer, size_t buffer_len, const char *out_file_path);

/**
 * @brief Callback for LSB1: modifies a pixel by inserting 3 bits of the secret message
 *
 * This function is designed to be passed to the BMP iteration function (recorrer_bmp)
 * It inserts one bit of secret data into the Least Significant Bit (LSB)
 * of the Blue, Green, and Red components, sequentially
 *
 * @param pixel Pointer to the pixel (BGR) to modify
 * @param ctx Pointer to the context (StegoContext) holding the secret buffer and index.
 */
void lsb1_embed_pixel_callback(Pixel *pixel, void *ctx);


/**
 * @brief Extracts a secret buffer from a BMP image using LSB1.
 *
 * This function reads the BMP pixel data, extracts the LSB from each
 * color component, and reconstructs the hidden data.
 * It first extracts a 4-byte size header (Big Endian) to determine
 * the full data length, then extracts the data itself.
 *
 * NOTE: The caller is responsible for freeing the returned buffer.
 *
 * @param image Pointer to an *opened* BMPImage structure (must have valid 'in' file).
 * @param extracted_data_len Pointer to a size_t variable to store the length of the extracted data.
 * @return A pointer to the dynamically allocated buffer containing the extracted data,
 * or NULL on error (e.g., read error, allocation failure).
 */
 unsigned char *lsb1_extract(BMPImage *image, size_t *extracted_data_len);

/**
 * @brief Hides the pre-built secret buffer inside a BMP using the LSB4 algorithm.
 *
 * @param image Pointer to the initialized BMPImage structure (open by the caller)
 * @param secret_buffer Pointer to the pre-built buffer containing the message
 * @param buffer_len Total length of the secret_buffer in bytes
 * @param out_file_path Path to the output file (used for writing the final stego-image)
 * @return 0 on success, 1 on error.
 */
int embed_lsb4(BMPImage *image, const unsigned char *secret_buffer, size_t buffer_len, const char *out_file_path);

/**
 * @brief Callback for LSB4: modifies a pixel by inserting 12 bits of the secret message.
 *
 * This function is designed to be passed to the BMP iteration function (iterate_bmp).
 * It inserts four bits of secret data into the 4 Least Significant Bits (LSBs)
 * of the Blue, Green, and Red components, sequentially.
 *
 * @param pixel Pointer to the pixel (BGR) to modify
 * @param ctx Pointer to the context (StegoContext) holding the secret buffer and index.
 */
void lsb4_embed_pixel_callback(Pixel *pixel, void *ctx);


#endif