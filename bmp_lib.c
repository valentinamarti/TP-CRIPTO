#include <stdio.h>
#include <stdlib.h>
#include <string.h>

    // BMP File Header
    typedef struct {        //14 bytes 
        uint16_t bfType;      // Specifies the file type (BM)
        uint32_t bfSize;      // Specifies the size of the file in bytes
        uint16_t bfReserved1; // Reserved; must be 0
        uint16_t bfReserved2; // Reserved; must be 0
        uint32_t bfOffBits;   // Specifies the offset to the pixel data
    } __attribute__((packed)) BMPFileHeader; // Use packed attribute to prevent padding

    // BMP Info Header
    typedef struct {        //40 bytes 
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

typedef struct {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
} Pixel;

typedef struct {
    Pixel * data;
    int width;
    int height;
} BMPImage;


#define HEADER_SIZE 54

BMPImage * recorrer_bmp(const char *bmp_in, const char *bmp_out,
    void (*callback)(unsigned char *byte, void *ctx),
    void *ctx) {
    FILE *in = fopen(bmp_in, "rb");
    FILE *out = fopen(bmp_out, "wb");
    if (!in || !out) {
        perror("Error abriendo archivos BMP");
        exit(1);
    }

    // Copiar header intacto
    BMPFileHeader fileHeader;
    BMPInfoHeader infoHeader;

    fread(&fileHeader, sizeof(BMPFileHeader), 1, in);
    fread(&infoHeader, sizeof(BMPInfoHeader), 1, in);

    fwrite(&fileHeader, sizeof(BMPFileHeader), 1, out);
    fwrite(&infoHeader, sizeof(BMPInfoHeader), 1, out);
    // Procesar cuerpo
    unsigned char byte;
    while (fread(&byte, 1, 1, in) == 1) {
        callback(&byte, ctx);   // aplicar algoritmo elegido
        fwrite(&byte, 1, 1, out);
    }

    fclose(in);
    fclose(out);
    BMPImage *image = malloc(sizeof(BMPImage));
    // Falta asignar los valores 
    return image;

}