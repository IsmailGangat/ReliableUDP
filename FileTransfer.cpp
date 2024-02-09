/*
	File Transfer System File
	Author: Ismail Gangat
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "Net.h"
#pragma warning(disable:4996)

/*
	NAME	:	FileHandler
	PURPOSE :	The purpose of this class is to handle 
				the files which are being sent recieved.

*/
class FileHandler
{
public:
	bool LoadFromFile()
	{
		// Implement loading and storing files from and to the disk
		// Read file content and store it in a data member or buffer.
        //           may use std::ifstream for reading the file.
		return;
	}

	bool SendAndRecieveFromFile()
	{
		// Implement sending and recieveing the file and its contents
		// by using a networking library (e.g., net::send and net::receive) to send and receive file data.
		return;
	}

	bool VerifyFileContent()
	{
		// Implement verifying the files contents
		// by checking the integrity of the received file content, e.g., using checksums.
		return;
	}

	bool ValidateFile()
	{
		// Implement the validation of the file 
		// Check if the file meets certain criteria (e.g., file format, size limits).
		return;
	}

};