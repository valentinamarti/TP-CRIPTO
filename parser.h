#ifndef PARSER_H
#define PARSER_H

// Structure to hold all program parameters
typedef struct {
    int embed_mode;           // 1 if -embed is specified
    int extract_mode;         // 1 if -extract is specified
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
void print_help(const char *program_name);
int parse_arguments(int argc, char *argv[], ProgramArgs *args);
int validate_arguments(const ProgramArgs *args);
void free_arguments(ProgramArgs *args);

#endif // PARSER_H