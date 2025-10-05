#ifndef ERROR_H
#define ERROR_H

// Error messages for argument parsing
#define ERR_IN_REQUIRES_FILENAME "Error: -in requires a filename\n"
#define ERR_P_REQUIRES_BITMAP "Error: -p requires a bitmap filename\n"
#define ERR_OUT_REQUIRES_BITMAP "Error: -out requires a bitmap filename\n"
#define ERR_STEG_REQUIRES_ALGORITHM "Error: -steg requires an algorithm (LSB1, LSB4, or LSBI)\n"
#define ERR_A_REQUIRES_ALGORITHM "Error: -a requires an algorithm (aes128, aes192, aes256, or 3des)\n"
#define ERR_M_REQUIRES_MODE "Error: -m requires a mode (ecb, cfb, ofb, or cbc)\n"
#define ERR_PASS_REQUIRES_PASSWORD "Error: -pass requires a password\n"
#define ERR_UNKNOWN_OPTION "Error: Unknown option '%s'\n"

// Error messages for argument validation
#define ERR_FLAG_REQUIRED "Error: -embed or -extract flag is required\n"
#define ERR_IN_PARAMETER_REQUIRED "Error: -in parameter is required\n"
#define ERR_P_PARAMETER_REQUIRED "Error: -p parameter is required\n"
#define ERR_OUT_PARAMETER_REQUIRED "Error: -out parameter is required\n"
#define ERR_STEG_PARAMETER_REQUIRED "Error: -steg parameter is required\n"
#define ERR_INVALID_STEG_ALGORITHM "Error: Invalid steganography algorithm '%s'. Must be LSB1, LSB4, or LSBI\n"
#define ERR_INVALID_ENCRYPTION_ALGORITHM "Error: Invalid encryption algorithm '%s'. Must be aes128, aes192, aes256, or 3des\n"
#define ERR_INVALID_MODE "Error: Invalid mode '%s'. Must be ecb, cfb, ofb, or cbc\n"
#define ERR_PASSWORD_REQUIRED_FOR_ENCRYPTION "Error: Password is required when encryption algorithm is specified\n"

// General error messages
#define ERR_FAILED_TO_PARSE_ARGS "Failed to parse arguments. Use -h for help.\n"
#define ERR_INVALID_ARGS "Invalid arguments. Use -h for help.\n"


// BMP error messages
#define ERR_INVALID_BMP "Invalid BMP file\n"
#define ERR_FAILED_TO_OPEN_BMP "Failed to open BMP file\n"
#define ERR_FAILED_TO_READ_BMP "Failed to read BMP file\n"
#define ERR_FAILED_TO_WRITE_BMP "Failed to write BMP file\n"
#define ERR_FAILED_TO_CLOSE_BMP "Failed to close BMP file\n"

#endif // ERROR_H
