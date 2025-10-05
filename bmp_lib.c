
#include "bmp_lib.h"
#include "error.h"

void recorrer_bmp(BMPImage *image,
    void (*callback)(Pixel *byte, void *ctx),
    void *ctx) {

    if (!image || !image->in || !image->out) {
        fprintf(stderr, ERR_INVALID_BMP);
        return;
    }
    
    // Process pixels
    Pixel pixel;
    while (fread(&pixel, sizeof(Pixel), 1, image->in) == 1) {
        callback(&pixel, ctx);   // apply chosen algorithm
        if (fwrite(&pixel, sizeof(Pixel), 1, image->out) != 1) {
            fprintf(stderr, ERR_FAILED_TO_WRITE_BMP);
            return;
        }
    }
}

BMPImage * start_bmp(const char *bmp_in){
    FILE *in = fopen(bmp_in, "rb");
    if (!in) {
        perror(ERR_FAILED_TO_OPEN_BMP);
        return NULL;
    }

    BMPImage *image = malloc(sizeof(BMPImage));
    // Copiar header intacto
    BMPFileHeader * fileHeader = malloc(sizeof(BMPFileHeader));
    BMPInfoHeader * infoHeader = malloc(sizeof(BMPInfoHeader));
    
    image->fileHeader = fileHeader;
    image->infoHeader = infoHeader;
    image->in = in;
    image->out = NULL;
    image->data = NULL;
    
    return image;
}

BMPImage * end_bmp(BMPImage *image){
    if (!image || !image->out) {
        fprintf(stderr, "Invalid image or output file\n");
        return NULL;
    }
    
    // Write headers to output file
    if (fwrite(image->fileHeader, sizeof(BMPFileHeader), 1, image->out) != 1) {
        fprintf(stderr, ERR_FAILED_TO_WRITE_BMP);
        return NULL;
    }
    
    if (fwrite(image->infoHeader, sizeof(BMPInfoHeader), 1, image->out) != 1) {
        fprintf(stderr, ERR_FAILED_TO_WRITE_BMP);
        return NULL;
    }

    // Write pixel data if available
    if (image->data) {
        // Calculate number of pixels
        int pixel_count = get_pixel_count(image);
        if (pixel_count > 0) {
            if (fwrite(image->data, sizeof(Pixel), pixel_count, image->out) != pixel_count) {
                fprintf(stderr, ERR_FAILED_TO_WRITE_BMP);
                return NULL;
            }
        }
    }
    
    if (fclose(image->out) != 0) {
        fprintf(stderr, ERR_FAILED_TO_CLOSE_BMP);
        return NULL;
    }
    
    image->out = NULL;
    return image;
}

int get_pixel_count(BMPImage *image) {
    if (!image || !image->infoHeader) {
        return -1;
    }
    
    return image->infoHeader->biWidth * image->infoHeader->biHeight;
}

void free_bmp_image(BMPImage *image) {
    if (!image) return;
    
    if (image->fileHeader) {
        free(image->fileHeader);
        image->fileHeader = NULL;
    }
    
    if (image->infoHeader) {
        free(image->infoHeader);
        image->infoHeader = NULL;
    }
    
    if (image->data) {
        free(image->data);
        image->data = NULL;
    }
    
    if (image->in) {
        fclose(image->in);
        image->in = NULL;
    }
    
    if (image->out) {
        fclose(image->out);
        image->out = NULL;
    }
    
    free(image);
}