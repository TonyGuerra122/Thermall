#include "text_buffer.h"

TextBuffer *create_text_buffer(const char *fileName)
{
    TextBuffer *buffer = (TextBuffer *)malloc(sizeof(TextBuffer));
    if (!buffer)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    buffer->filename = _strdup(fileName);
    if (!buffer->filename)
    {
        fprintf(stderr, "Memory allocation for fileName failed\n");
        free(buffer);
        exit(1);
    }

    fopen_s(&buffer->filePtr, fileName, "r+");
    buffer->is_new_file = !buffer->filePtr;

    if (buffer->is_new_file)
    {
        fopen_s(&buffer->filePtr, fileName, "w+"); // Create the file if it does not exist
        if (!buffer->filePtr)
        {
            fprintf(stderr, "Failed to create file: %s\n", fileName);
            free(buffer->filename);
            free(buffer);
            exit(1);
        }
        buffer->size = 0;
        buffer->content = _strdup("");
    }
    else
    {
        // Determine the size of the file
        fseek(buffer->filePtr, 0, SEEK_END);
        buffer->size = ftell(buffer->filePtr);
        fseek(buffer->filePtr, 0, SEEK_SET);

        // Allocate memory for the content
        buffer->content = (char *)malloc(buffer->size + 1);
        if (!buffer->content)
        {
            fprintf(stderr, "Memory allocation for content failed\n");
            fclose(buffer->filePtr);
            free(buffer->filename);
            free(buffer);
            exit(1);
        }

        // Read the content of the file into the buffer
        fread(buffer->content, 1, buffer->size, buffer->filePtr);
        buffer->content[buffer->size] = '\0'; // Null-terminate the content
    }

    return buffer;
}

void save_text_buffer(TextBuffer *buffer)
{
    fclose(buffer->filePtr);

    FILE *filePtr;
    fopen_s(&filePtr, buffer->filename, "w"); // Open the file for writing and truncate
    if (!filePtr)
    {
        fprintf(stderr, "Failed to open file: %s\n", buffer->filename);
        return;
    }

    fwrite(buffer->content, 1, buffer->size, filePtr);
    fclose(filePtr);
    buffer->is_new_file = false;

    fopen_s(&buffer->filePtr, buffer->filename, "r+");
}

void free_text_buffer(TextBuffer *buffer)
{
    if (!buffer)
        return;
    if (buffer->filename)
        free(buffer->filename);
    if (buffer->content)
        free(buffer->content);
    if (buffer->filePtr)
        fclose(buffer->filePtr);

    free(buffer);
}
