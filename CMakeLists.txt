cmake_minimum_required(VERSION 3.28)
project(TP_CRIPTO C)

set(CMAKE_C_STANDARD 11)

add_executable(TP_CRIPTO main.c
        bmp_lib.c
        parser.c
        steganography/steganography.h
        steganography/embed_utils.c
        handlers.c
        steganography/steganography.c)
