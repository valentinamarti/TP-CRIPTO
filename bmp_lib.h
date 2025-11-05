#ifndef BMP_LIB_H
#define BMP_LIB_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// BMP File Header (14 bytes)
typedef struct {        
    uint16_t bfType;      // Specifies the file type (BM)
    uint32_t bfSize;      // Specifies the size of the file in bytes
    uint16_t bfReserved1; // Reserved; must be 0
    uint16_t bfReserved2; // Reserved; must be 0
    uint32_t bfOffBits;   // Specifies the offset to the pixel data
} __attribute__((packed)) BMPFileHeader;

// BMP Info Header (40 bytes)
typedef struct {        
    uint32_t biSize;          // Specifies the size of the Info Header
    int32_t  biWidth;         // Specifies the width of the image
    int32_t  biHeight;        // Specifies the height of the image
    uint16_t biPlanes;        // Must be 1
    uint16_t biBitCount;      // Specifies the number of bits per pixel
    uint32_t biCompression;   // Specifies the compression type
    uint32_t biSizeImage;     // Specifies the size of the image data
    int32_t  biXPelsPerMeter; // Horizontal resolution
    int32_t  biYPelsPerMeter; // Vertical resolution
    uint32_t biClrUsed;       // Number of colors in the palette
    uint32_t biClrImportant;  // Number of important colors
} __attribute__((packed)) BMPInfoHeader;

// Pixel structure (RGB)
typedef struct {
    unsigned char blue;
    unsigned char green;
    unsigned char red;
} __attribute__((packed)) Pixel;

// BMP Image structure
typedef struct {
    BMPFileHeader * fileHeader;
    BMPInfoHeader * infoHeader;
    Pixel * data;
    FILE * in;
    FILE * out;
} BMPImage;

// Constants
#define HEADER_SIZE 54

// Function prototypes

/**
 * @brief Opens a BMP file and initializes the BMPImage structure
 * @param bmp_in Path to the input BMP file
 * @return Pointer to initialized BMPImage structure, NULL on error
 */
BMPImage * open_bmp(const char *bmp_in);

/**
 * @brief Closes a BMP file and writes the final output
 * @param image Pointer to BMPImage structure
 * @return Pointer to BMPImage structure, NULL on error
 */
BMPImage * close_bmp(BMPImage *image);

/**
 * @brief Processes a BMP file pixel by pixel using a callback function
 * @param image Pointer to BMPImage structure
 * @param callback Function pointer to process each pixel
 * @param ctx Context pointer passed to callback function
 */
void iterate_bmp(BMPImage *image,
    void (*callback)(Pixel *byte, void *ctx),
    void *ctx);

/**
 * @brief Frees memory allocated for BMPImage structure
 * @param image Pointer to BMPImage structure to free
 */
void free_bmp_image(BMPImage *image);


/**
 * @brief Gets the total number of pixels in the BMP image
 * @param image Pointer to BMPImage structure
 * @return Number of pixels, -1 on error
 */
int get_pixel_count(const BMPImage *image);


#endif // BMP_LIB_H
