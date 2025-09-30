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
    BMPFileHeader * fileHeader;
    BMPInfoHeader * infoHeader;
    Pixel * data;
    FILE * in;
    FILE * out;
} BMPImage;


#define HEADER_SIZE 54

void recorrer_bmp(BMPImage *image,
    void (*callback)(Pixel *byte, void *ctx),
    void *ctx) {

    
    // Procesar cuerpo
    Pixel pixel;
    while (fread(&pixel, sizeof(Pixel), 1, image->in) == 1) {
        callback(&pixel, ctx);   // aplicar algoritmo elegido
        fwrite(&pixel, sizeof(Pixel), 1, image->out);
    }

    fclose(image->in);
    fclose(image->out);
    
}

BMPImage * start_bmp(const char *bmp_in){
    FILE *in = fopen(bmp_in, "rb");
    if (!in) {
        perror("Error abriendo archivos BMP");
        exit(1);
    }
    // Copiar header intacto
    BMPFileHeader * fileHeader = malloc(sizeof(BMPFileHeader));
    BMPInfoHeader * infoHeader = malloc(sizeof(BMPInfoHeader));

    fread(&fileHeader, sizeof(BMPFileHeader), 1, in);
    fread(&infoHeader, sizeof(BMPInfoHeader), 1, in);

    BMPImage *image = malloc(sizeof(BMPImage));
    image->fileHeader = fileHeader;
    image->infoHeader = infoHeader;
    image->in = in;
    return image;
}

BMPImage * end_bmp(BMPImage *image){
    fwrite(image->fileHeader, sizeof(BMPFileHeader), 1, image->out);
    fwrite(image->infoHeader, sizeof(BMPInfoHeader), 1, image->out);

    Pixel pixel;
    while(image->data != NULL){
        pixel = image->data[0];
        fwrite(&pixel, sizeof(Pixel), 1, image->out);
        image->data+=sizeof(Pixel);
    }
    fclose(image->out);
    return image;
}