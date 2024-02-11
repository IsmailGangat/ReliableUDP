/* Filename: ReliablePrototypes.h
*  Project: ReliableUDP
*  Programmer: Ismail Gangat, Hasan Dukanwala
*  First Version: Feb 4th 2024
*  Description: This header file contains the prototypes and struct for the functions
*/

#pragma once

#ifndef RELIABLEPROTOTYPES_H
#define RELIABLEPROTOTYPES_H

#pragma warning(disable:4996)

// Struct to represent chunks of data
typedef struct {
    unsigned char* data;
    size_t size;
} Chunk;


// prototypes
void getFilename(char* filename, int size);
char* SendMetadata(const char* file_name);
Chunk* breakFileIntoChunks(const char* filename, int* numChunks, const int chunkSize);


#endif // !RELIABLEPROTOTYPES_H