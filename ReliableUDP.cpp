/*
	Reliability and Flow Control Example
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

/* Filename: ReliableUDP.cpp
*  Project: ReliableUDP
*  Programmer: Ismail Gangat, Hasan Dukanwala
*  First Version: Feb 4th 2024
*  Description: This file contains the main port and IP connection information as well as
*				UDP header content
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "Net.h"
#include "ReliablePrototypes.h"

//#pragma warning(disable:4996)
//#define SHOW_ACKS

using namespace std;
using namespace net;

const int ServerPort = 30000;
const int ClientPort = 30001;
const int ProtocolId = 0x11223344;
const float DeltaTime = 1.0f / 30.0f;
const float SendRate = 1.0f / 30.0f;
const float TimeOut = 10.0f;
const int PacketSize = 256;

/*
	NAME	:	FlowControl
	PURPOSE :	The purpose of this class is to calculate the time spent
				when sending and printing it.

*/
class FlowControl
{
public:

	FlowControl()
	{
		printf("flow control initialized\n");
		Reset();
	}

	void Reset()
	{
		mode = Bad;
		penalty_time = 4.0f;
		good_conditions_time = 0.0f;
		penalty_reduction_accumulator = 0.0f;
	}

	void Update(float deltaTime, float rtt)
	{
		const float RTT_Threshold = 250.0f;

		if (mode == Good)
		{
			if (rtt > RTT_Threshold)
			{
				printf("*** dropping to bad mode ***\n");
				mode = Bad;
				if (good_conditions_time < 10.0f && penalty_time < 60.0f)
				{
					penalty_time *= 2.0f;
					if (penalty_time > 60.0f)
						penalty_time = 60.0f;
					printf("penalty time increased to %.1f\n", penalty_time);
				}
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				return;
			}

			good_conditions_time += deltaTime;
			penalty_reduction_accumulator += deltaTime;

			if (penalty_reduction_accumulator > 10.0f && penalty_time > 1.0f)
			{
				penalty_time /= 2.0f;
				if (penalty_time < 1.0f)
					penalty_time = 1.0f;
				printf("penalty time reduced to %.1f\n", penalty_time);
				penalty_reduction_accumulator = 0.0f;
			}
		}

		if (mode == Bad)
		{
			if (rtt <= RTT_Threshold)
				good_conditions_time += deltaTime;
			else
				good_conditions_time = 0.0f;

			if (good_conditions_time > penalty_time)
			{
				printf("*** upgrading to good mode ***\n");
				good_conditions_time = 0.0f;
				penalty_reduction_accumulator = 0.0f;
				mode = Good;
				return;
			}
		}
	}


	float GetSendRate()
	{
		return mode == Good ? 30.0f : 10.0f;
	}

private:

	enum Mode
	{
		Good,
		Bad
	};

	Mode mode;
	float penalty_time;
	float good_conditions_time;
	float penalty_reduction_accumulator;
};

// ----------------------------------------------

int main(int argc, char* argv[])
{
	// parse command line

	enum Mode
	{
		Client,
		Server
	};

	Mode mode = Server;
	Address address;

	// retrieving Additional command line argument of filename to determine 
	// who is sending file and who is receiving

	boolean server_sending = false;
	boolean client_sending = false;
	boolean server_receiving = false;
	boolean client_receiving = false;
	int metaDataFlag = 0; // ) flag means metadata is not sent
	char filename[50] = { 0 };
	int numChunks;

	// Command line args parse 
	if (argc >= 2)
	{
		int a, b, c, d;
		if (sscanf(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d))
		{
			mode = Client;

			address = Address(a, b, c, d, ServerPort);
			if (argc >= 3 && strcmp(argv[2], "-send") == 0)
			{

				client_sending = true;
				server_receiving = true;

			}
			else
			{
				server_sending = true;
				client_receiving = true;
			}


		}
		else if (strcmp(argv[1], "-send") == 0)
		{
			mode = Server;

			server_sending = true;
			client_receiving = true;

		}
	}
	else
	{
		mode = Server;
		client_sending = true;
		server_receiving = true;

	}

	// initialize

	if (!InitializeSockets())
	{
		printf("failed to initialize sockets\n");
		return 1;
	}

	ReliableConnection connection(ProtocolId, TimeOut);

	const int port = mode == Server ? ServerPort : ClientPort;

	if (!connection.Start(port))
	{
		printf("could not start connection on port %d\n", port);
		return 1;
	}

	if (mode == Client)
		connection.Connect(address);
	else
		connection.Listen();

	bool connected = false;
	float sendAccumulator = 0.0f;
	float statsAccumulator = 0.0f;

	FlowControl flowControl;


	while (true)
	{
		// update flow control

		if (connection.IsConnected())
			flowControl.Update(DeltaTime, connection.GetReliabilitySystem().GetRoundTripTime() * 1000.0f);

		const float sendRate = flowControl.GetSendRate();

		// detect changes in connection state

		if (mode == Server && connected && !connection.IsConnected())
		{
			flowControl.Reset();
			printf("reset flow control\n");
			connected = false;
		}

		if (!connected && connection.IsConnected())
		{
			printf("client connected to server\n");
			connected = true;
		}

		if (!connected && connection.ConnectFailed())
		{
			printf("connection failed\n");
			break;
		}

		sendAccumulator += DeltaTime;
		static int count = 1;


		while (sendAccumulator > 1.0f / sendRate)
		{

			unsigned char packet[PacketSize];
			memset(packet, 0, sizeof(packet));


			// If client connection
			if (mode == Client && client_sending == true && server_sending == false)
			{

				if (filename[0] == '\0')
				{
					getFilename(filename, sizeof(filename));
				}

				// send file to extract metadata and receive metadata from function
				char* fileMetaData = SendMetadata(filename);

				if (fileMetaData != NULL && metaDataFlag == 0) {
					memcpy(packet, fileMetaData, strlen(fileMetaData));
					printf("Debug: Metadata is %s\n", fileMetaData);
					if (!connection.SendPacket(packet, sizeof(packet))) {
						perror("Failed to send metadata");
					}
					free(fileMetaData);

				}
				else
				{
					printf("failed to get filemetadata.\n");
				}

				printf("I am client sending the file %s in client mode.\n", filename);

				// Break data into chunks
				Chunk* chunks = breakFileIntoChunks(filename, &numChunks, PacketSize);

				for (int i = 0; i < numChunks; i++) {
					if (!connection.SendPacket(chunks[i].data, chunks[i].size)) {
						printf("Failed to send packet\n");
						return -1;
					}
				}
			}

			sendAccumulator -= 1.0f / sendRate;


		}

		while (true)
		{
			unsigned char packet[256];
			int bytes_read = connection.ReceivePacket(packet, sizeof(packet));
			if (bytes_read == 0)
				break;


			// Parse and print the metadata
			char file_name[128];
			long file_size;

			int ret;
			ret = sscanf((char*)packet, "%[^:]:%ld", file_name, &file_size);

			if (file_name[strlen(file_name) - 1] == '\n') {
				file_name[strlen(file_name) - 1] = '\0';
			}
			if (ret != 2) {
				// Handle the error
				printf("Failed to parse packet. Expected 2 items, but sscanf returned %d.\n", ret);
			}

			// Ensure buffer is large enough
			char filePath[1024];
			strcpy(filePath, "./output/");
			strcat(filePath, file_name);

			// Server Connection
			if (mode == Server && server_receiving == true && client_receiving == false)
			{
				// prompt whos connected
				printf("I am server receiving the file in server mode.\n");
				printf("Received metadata:\n");
				printf("File name: %s\n", file_name);
				printf("File size: %ld\n", file_size);

				// Recieve file
				FILE* file = fopen(filePath, "wb");
				if (file == NULL) {
					printf("Failed to open file\n");
					return -1;
				}

				unsigned char buffer[PacketSize];
				int size;

				while ((size = connection.ReceivePacket(buffer, PacketSize)) > 0) {
					fwrite(buffer, 1, size, file);
				}

				fclose(file);

			}

		}


		// show packets that were acked this frame

#ifdef SHOW_ACKS
		unsigned int* acks = NULL;
		int ack_count = 0;
		connection.GetReliabilitySystem().GetAcks(&acks, ack_count);
		if (ack_count > 0)
		{
			printf("acks: %d", acks[0]);
			for (int i = 1; i < ack_count; ++i)
				printf(",%d", acks[i]);
			printf("\n");
		}
#endif

		// update connection

		connection.Update(DeltaTime);

		// show connection stats

		statsAccumulator += DeltaTime;

		while (statsAccumulator >= 0.25f && connection.IsConnected())
		{
			float rtt = connection.GetReliabilitySystem().GetRoundTripTime();

			unsigned int sent_packets = connection.GetReliabilitySystem().GetSentPackets();
			unsigned int acked_packets = connection.GetReliabilitySystem().GetAckedPackets();
			unsigned int lost_packets = connection.GetReliabilitySystem().GetLostPackets();

			float sent_bandwidth = connection.GetReliabilitySystem().GetSentBandwidth();
			float acked_bandwidth = connection.GetReliabilitySystem().GetAckedBandwidth();

			printf("rtt %.1fms, sent %d, acked %d, lost %d (%.1f%%), sent bandwidth = %.1fkbps, acked bandwidth = %.1fkbps\n",
				rtt * 1000.0f, sent_packets, acked_packets, lost_packets,
				sent_packets > 0.0f ? (float)lost_packets / (float)sent_packets * 100.0f : 0.0f,
				sent_bandwidth, acked_bandwidth);

			statsAccumulator -= 0.25f;
		}

		net::wait(DeltaTime);
	}

	ShutdownSockets();

	return 0;
}