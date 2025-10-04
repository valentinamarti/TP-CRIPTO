#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"

// Structure to hold all program parameters
typedef struct {
    int embed_mode;           // 1 if -embed is specified
    char *input_file;         // -in file
    char *bitmap_file;        // -p bitmapfile
    char *output_file;        // -out bitmapfile
    char *steg_algorithm;     // -steg <LSB1|LSB4|LSBI>
    char *encryption_algo;    // -a <aes128|aes192|aes256|3des>
    char *mode;              // -m <ecb|cfb|ofb|cbc>
    char *password;          // -pass password
    int help_requested;      // 1 if help is requested
} ProgramArgs;

// Function prototypes
void print_usage(const char *program_name);
int parse_arguments(int argc, char *argv[], ProgramArgs *args);
int validate_arguments(const ProgramArgs *args);
void free_arguments(ProgramArgs *args);

void print_usage(const char *program_name) {
    printf(
        "Usage: %s [OPTIONS]\n\n"
        "Required parameters:\n"
        "  -embed                    Indica que se va a ocultar información\n"
        "  -in file                  Archivo que se va a ocultar\n"
        "  -p bitmapfile             Archivo bmp que será el portador\n"
        "  -out bitmapfile           Archivo bmp de salida\n"
        "  -steg <LSB1|LSB4|LSBI>    Algoritmo de esteganografiado\n"
        "                            LSB1: LSB de 1 bit\n"
        "                            LSB4: LSB de 4 bits\n"
        "                            LSBI: LSB Enhanced\n\n"
        "Optional parameters:\n"
        "  -a <aes128|aes192|aes256|3des>  Algoritmo de encriptación\n"
        "  -m <ecb|cfb|ofb|cbc>            Modo de operación\n"
        "  -pass password                   Password de encriptación\n"
        "  -h, --help                       Mostrar esta ayuda\n\n"
        "Example:\n"
        "  %s -embed -in secret.txt -p image.bmp -out stego.bmp -steg LSB1\n",
        program_name, program_name
    );
}

int parse_arguments(int argc, char *argv[], ProgramArgs *args) {
    // Initialize all fields to default values
    memset(args, 0, sizeof(ProgramArgs));
    
    for (int i = 1; i < argc; i++) {
        // Use a switch-case on the first character for efficiency, then strcmp for full match
        switch (argv[i][0]) {
            case '-':
                if (strcmp(argv[i], "-embed") == 0) {
                    args->embed_mode = 1;
                }
                else if (strcmp(argv[i], "-in") == 0) {
                    if (i + 1 >= argc) {
                        fprintf(stderr, ERR_IN_REQUIRES_FILENAME);
                        return 0;
                    }
                    args->input_file = argv[++i];
                }
                else if (strcmp(argv[i], "-p") == 0) {
                    if (i + 1 >= argc) {
                        fprintf(stderr, ERR_P_REQUIRES_BITMAP);
                        return 0;
                    }
                    args->bitmap_file = argv[++i];
                }
                else if (strcmp(argv[i], "-out") == 0) {
                    if (i + 1 >= argc) {
                        fprintf(stderr, ERR_OUT_REQUIRES_BITMAP);
                        return 0;
                    }
                    args->output_file = argv[++i];
                }
                else if (strcmp(argv[i], "-steg") == 0) {
                    if (i + 1 >= argc) {
                        fprintf(stderr, ERR_STEG_REQUIRES_ALGORITHM);
                        return 0;
                    }
                    args->steg_algorithm = argv[++i];
                }
                else if (strcmp(argv[i], "-a") == 0) {
                    if (i + 1 >= argc) {
                        fprintf(stderr, ERR_A_REQUIRES_ALGORITHM);
                        return 0;
                    }
                    args->encryption_algo = argv[++i];
                }
                else if (strcmp(argv[i], "-m") == 0) {
                    if (i + 1 >= argc) {
                        fprintf(stderr, ERR_M_REQUIRES_MODE);
                        return 0;
                    }
                    args->mode = argv[++i];
                }
                else if (strcmp(argv[i], "-pass") == 0) {
                    if (i + 1 >= argc) {
                        fprintf(stderr, ERR_PASS_REQUIRES_PASSWORD);
                        return 0;
                    }
                    args->password = argv[++i];
                }
                else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
                    args->help_requested = 1;
                }
                else {
                    fprintf(stderr, ERR_UNKNOWN_OPTION, argv[i]);
                    return 0;
                }
                break;
            default:
                fprintf(stderr, ERR_UNKNOWN_OPTION, argv[i]);
                return 0;
        }
    }
    
    return 1;
}

int validate_arguments(const ProgramArgs *args) {
    // Check if help is requested
    if (args->help_requested) {
        return 1;
    }
    
    // Validate required parameters
    if (!args->embed_mode) {
        fprintf(stderr, ERR_EMBED_FLAG_REQUIRED);
        return 0;
    }
    
    if (!args->input_file) {
        fprintf(stderr, ERR_IN_PARAMETER_REQUIRED);
        return 0;
    }
    
    if (!args->bitmap_file) {
        fprintf(stderr, ERR_P_PARAMETER_REQUIRED);
        return 0;
    }
    
    if (!args->output_file) {
        fprintf(stderr, ERR_OUT_PARAMETER_REQUIRED);
        return 0;
    }
    
    if (!args->steg_algorithm) {
        fprintf(stderr, ERR_STEG_PARAMETER_REQUIRED);
        return 0;
    }
    
    // Validate steganography algorithm
    if (strcmp(args->steg_algorithm, "LSB1") != 0 && 
        strcmp(args->steg_algorithm, "LSB4") != 0 && 
        strcmp(args->steg_algorithm, "LSBI") != 0) {
        fprintf(stderr, ERR_INVALID_STEG_ALGORITHM, args->steg_algorithm);
        return 0;
    }
    
    // Validate encryption algorithm if provided
    if (args->encryption_algo) {
        if (strcmp(args->encryption_algo, "aes128") != 0 && 
            strcmp(args->encryption_algo, "aes192") != 0 && 
            strcmp(args->encryption_algo, "aes256") != 0 && 
            strcmp(args->encryption_algo, "3des") != 0) {
            fprintf(stderr, ERR_INVALID_ENCRYPTION_ALGORITHM, args->encryption_algo);
            return 0;
        }
    }
    
    // Validate mode if provided
    if (args->mode) {
        if (strcmp(args->mode, "ecb") != 0 && 
            strcmp(args->mode, "cfb") != 0 && 
            strcmp(args->mode, "ofb") != 0 && 
            strcmp(args->mode, "cbc") != 0) {
            fprintf(stderr, ERR_INVALID_MODE, args->mode);
            return 0;
        }
    }
    
    // Check if password is provided when encryption is specified
    if (args->encryption_algo && !args->password) {
        fprintf(stderr, ERR_PASSWORD_REQUIRED_FOR_ENCRYPTION);
        return 0;
    }
    
    return 1;
}

void free_arguments(ProgramArgs *args) {
    // Note: We don't free strings since they point to argv, which is managed by the system
    memset(args, 0, sizeof(ProgramArgs));
}

int main(int argc, char *argv[]) {
    ProgramArgs args;
    
    // Parse command line arguments
    if (!parse_arguments(argc, argv, &args)) {
        fprintf(stderr, ERR_FAILED_TO_PARSE_ARGS);
        return 1;
    }
    
    // Show help if requested
    if (args.help_requested) {
        print_usage(argv[0]);
        return 0;
    }
    
    // Validate arguments
    if (!validate_arguments(&args)) {
        fprintf(stderr, ERR_INVALID_ARGS);
        return 1;
    }
    
    // Print parsed parameters for debugging
    printf("Program parameters:\n");
    printf("  Embed mode: %s\n", args.embed_mode ? "Yes" : "No");
    printf("  Input file: %s\n", args.input_file);
    printf("  Bitmap file: %s\n", args.bitmap_file);
    printf("  Output file: %s\n", args.output_file);
    printf("  Steganography algorithm: %s\n", args.steg_algorithm);
    
    if (args.encryption_algo) {
        printf("  Encryption algorithm: %s\n", args.encryption_algo);
    }
    if (args.mode) {
        printf("  Mode: %s\n", args.mode);
    }
    if (args.password) {
        printf("  Password: [HIDDEN]\n");
    }
    
    // TODO: Here you would implement the actual steganography logic
    // For now, we just print that we would process the files
    printf("\nProcessing files...\n");
    printf("Would embed '%s' into '%s' using %s algorithm\n", 
           args.input_file, args.bitmap_file, args.steg_algorithm);
    printf("Output would be saved to '%s'\n", args.output_file);
    
    if (args.encryption_algo) {
        printf("Files would be encrypted using %s in %s mode\n", 
               args.encryption_algo, args.mode ? args.mode : "default");
    }
    
    // Clean up
    free_arguments(&args);
    
    return 0;
}
