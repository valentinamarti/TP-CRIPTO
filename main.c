#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "error.h"
#include "parser.h"



int main(int argc, char *argv[]) {
    ProgramArgs args;
    
    // Parse command line arguments
    if (!parse_arguments(argc, argv, &args)) {
        fprintf(stderr, ERR_FAILED_TO_PARSE_ARGS);
        return 1;
    }
    
    // Show help if requested
    if (args.help_requested) {
        print_help(argv[0]);
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
