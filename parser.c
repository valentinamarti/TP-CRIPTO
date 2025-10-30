#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "error.h"
#include "parser.h"




void print_help(const char *program_name) {
    printf(
        "Usage: %s [OPTIONS]\n\n"
        "Required parameters:\n"
        "  -embed                    Indicates that information will be hidden\n"
        "  -extract                  Indicates that information will be extracted\n"
        "  -in file                  File to be hidden\n"
        "  -p bitmapfile             BMP file that will act as the carrier\n"
        "  -out bitmapfile           Output BMP file (with embedded data)\n"
        "  -steg <LSB1|LSB4|LSBI>    Steganographic algorithm to use\n"
        "                            LSB1: LSB of 1 bit\n"
        "                            LSB4: LSB of 4 bits\n"
        "                            LSBI: LSB Enhanced (Improved)\n\n"
        "Optional parameters:\n"
        "  -a <aes128|aes192|aes256|3des>  Encryption algorithm\n"
        "  -m <ecb|cfb|ofb|cbc>            Mode of operation\n"
        "  -pass password                   Encryption password\n"
        "  -h, --help                       Show this help message\n\n"
        "Example:\n"
        "  %s -embed -in secret.txt -p image.bmp -out stego.bmp -steg LSB1\n"
        "  %s -extract -p stego.bmp -out secret.txt -steg LSB1\n",
        program_name, program_name
    );
    
}

int parse_arguments(int argc, char *argv[], ProgramArgs *args) {
    // Initialize all fields to default values
    memset(args, 0, sizeof(ProgramArgs));

    static struct option long_opts[] = {
        {"embed",    no_argument,       0, 'E'},
        {"extract",  no_argument,       0, 'X'},
        {"in",       required_argument, 0, 'i'},
        {"p",        required_argument, 0, 'p'},
        {"out",      required_argument, 0, 'o'},
        {"steg",     required_argument, 0, 's'},
        {"a",        required_argument, 0, 'a'},
        {"m",        required_argument, 0, 'm'},
        {"pass",     required_argument, 0, 'P'},
        {"help",     no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int long_index = 0;
    
    while ((opt = getopt_long_only(argc, argv, "W;EXi:p:o:s:a:m:P:h", long_opts, &long_index)) != -1) {
        switch (opt) {
            case 'E': args->embed_mode = 1; break;
            case 'X': args->extract_mode = 1; break;
            case 'i': args->input_file = optarg; break;
            case 'p': args->bitmap_file = optarg; break;
            case 'o': args->output_file = optarg; break;
            case 's': args->steg_algorithm = optarg; break;
            case 'a': args->encryption_algo = optarg; break;
            case 'm': args->mode = optarg; break;
            case 'P': args->password = optarg; break;
            case 'h': args->help_requested = 1; print_help(argv[0]); return 0;
            default:
                fprintf(stderr, ERR_INVALID_ARGS);
                print_help(argv[0]);
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
    if (!args->embed_mode && !args->extract_mode) {
        fprintf(stderr, ERR_FLAG_REQUIRED);
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

void debug_arguments(const ProgramArgs *args) {
    // Print parsed parameters for debugging
    printf("Program parameters:\n");
    printf("  Mode: %s\n", args->embed_mode ? "embed" : "extract");
    printf("  Input file: %s\n", args->input_file);
    printf("  Bitmap file: %s\n", args->bitmap_file);
    printf("  Output file: %s\n", args->output_file);
    printf("  Steganography algorithm: %s\n", args->steg_algorithm);

    if (args->encryption_algo) {
        printf("  Encryption algorithm: %s\n", args->encryption_algo);
    }
    if (args->mode) {
        printf("  Mode: %s\n", args->mode);
    }
    if (args->password) {
        printf("  Password: [HIDDEN]\n");
    }

    // TODO: Here you would implement the actual steganography logic
    // For now, we just print that we would process the files
    printf("\nProcessing files...\n");
    printf("Would embed '%s' into '%s' using %s algorithm\n", 
        args->input_file, args->bitmap_file, args->steg_algorithm);
    printf("Output would be saved to '%s'\n", args->output_file);

    if (args->encryption_algo) {
        printf("Files would be encrypted using %s in %s mode\n", 
            args->encryption_algo, args->mode ? args->mode : "default");
}
}

void free_arguments(ProgramArgs *args) {
    // Note: We don't free strings since they point to argv, which is managed by the system
    memset(args, 0, sizeof(ProgramArgs));
}