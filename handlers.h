#ifndef HANDLERS_H
#define HANDLERS_H

#include "parser.h" // For ProgramArgs

/**
 * @brief Handles the entire embedding process, from file preparation to execution.
 * * This function performs file I/O, resource management (BMPImage, secret_buffer),
 * capacity checks, and orchestrates the call to the specific steganography algorithm.
 * * @param args Program arguments parsed from command line.
 * @return 0 on success, 1 on failure.
 */

#define SUCCESS 1
#define NO_SUCCESS 0

int handle_embed_mode(const ProgramArgs *args);

#endif //HANDLERS_H