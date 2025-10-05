#include <stdio.h>
#include <stdlib.h>
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
    
    // Debug arguments
    debug_arguments(&args);
    // Clean up
    free_arguments(&args);
    
    return 0;
}
