/*
	File Transfer System File
	Author: Ismail Gangat
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <string.h>

#include "Net.h"
#include "ReliablePrototypes.h"

#pragma warning(disable:4996)

/*
	NAME	:	FileHandler
	PURPOSE :	The purpose of this class is to handle 
				the files which are being sent recieved.

*/


	void getFilename(char* filename, int size) {
		printf("Please enter the filename: ");
		fgets(filename, size, stdin);

		// Remove newline character if present
		if (filename[strlen(filename) - 1] == '\n') {
			filename[strlen(filename) - 1] = '\0';
		}
	}


    char* SendMetadata(const char* file_name) {
        // Open the file and get its size
        FILE* file = fopen(file_name, "r");
        if (file == NULL) {
            printf("Error opening file: %s\n", strerror(errno));
            return NULL;
        }

        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        // Prepare the metadata
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