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
