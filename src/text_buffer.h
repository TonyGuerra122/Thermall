#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Define a struct to represent a text file buffer
typedef struct
{
    char *filename; // To store the name of the file
    FILE *filePtr;  // To store the file pointer
    char *content;  // To store the content o the file
    size_t size;    // To store the size of the content
    bool is_new_file;

} TextBuffer;

// Function to initialize the TextBuffer
TextBuffer *create_text_buffer(const char *);

// Function to save the TextBuffer content to the file
void save_text_buffer(TextBuffer *);

// Function to free the resources associated with the TextBuffer
void free_text_buffer(TextBuffer *);