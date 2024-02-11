/* Filename: FileTransfer.cpp
*  Project: ReliableUDP
*  Programmer: Ismail Gangat, Hasan Dukanwala
*  First Version: Feb 4th 2024
*  Description: This file contains the File Transfer functions and the
*				sending data information
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ReliablePrototypes.h"

/* Function: void getFilename(char* filename, int size)
         * Description: This function gets the file name to send
         * Parameters: char* filename, int size
         * Returns: -
         */
void getFilename(char* filename, int size) {
    printf("Please enter the filename: ");
    fgets(filename, size, stdin);

    // Remove newline character if present
    if (filename[strlen(filename) - 1] == '\n') {
        filename[strlen(filename) - 1] = '\0';
    }
}

/* Function: char* SendMetadata(const char* file_name)
         * Description: This function Sends the Metadata
         * Parameters: onst char* file_name
         * Returns: NULL
         */
char* SendMetadata(const char* file_name) {

    // Open the file and get its size
    FILE* file = fopen(file_name, "rb");
    if (file == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory
    char* metadata = (char*)malloc(128 * sizeof(char));

    if (metadata == NULL)
    {
        printf("Failed to allocate memory");
        return NULL;
    }

    sprintf(metadata, "%s:%ld", file_name, file_size);

    fclose(file);
    return metadata;
}

/* Function: Chunk* breakFileIntoChunks(const char* filename, int* numChunks, const int chunkSize)
         * Description: This function breaks the file into pieces
         * Parameters: const char* filename, int* numChunks, const int chunkSize
         * Returns: chunks
         */
Chunk* breakFileIntoChunks(const char* filename, int* numChunks, const int chunkSize) {
    // Open file
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Failed to open file\n");
        return NULL;
    }

    // Determine the number of chunks
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    *numChunks = (fileSize + chunkSize - 1) / chunkSize;

    // Allocate memory for the chunks
    Chunk* chunks = (Chunk*)malloc(*numChunks * sizeof(Chunk));
    if (chunks == NULL) {
        printf("Failed to allocate memory\n");
        fclose(file);
        return NULL;
    }

    // Read the file into the chunks
    for (int i = 0; i < *numChunks; i++) {
        chunks[i].data = (unsigned char*)malloc(chunkSize);
        if (chunks[i].data == NULL) {
            printf("Failed to allocate memory\n");
            fclose(file);
            return NULL;
        }
        chunks[i].size = fread(chunks[i].data, 1, chunkSize, file);
    }

    // Close file
    fclose(file);
    return chunks;
}