/*
	Reliability and Flow Control Example
	From "Networking for Game Programmers" - http://www.gaffer.org/networking-for-game-programmers
	Author: Glenn Fiedler <gaffer@gaffer.org>
*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "Net.h"
#include "ReliablePrototypes.h"

#pragma warning(disable:4996)
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
	// if server has filename that means server wants to send, and if client have file name
	//  then client wants to send, and if both have then error

	/*if (argc >= 2)
	{
		int a, b, c, d;
		#pragma warning(suppress:4996)
		if (sscanf(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d))
		{
			mode = Client;
			address = Address(a, b, c, d, ServerPort);
		}
	}*/

	boolean server_sending = false;
	boolean client_sending = false;
	boolean server_receiving = false;
	boolean client_receiving = false;
	char filename[50];

	if (argc >= 2)
	{
		int a, b, c, d;
		if (sscanf(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d))
		{
			mode = Client;

			address = Address(a, b, c, d, ServerPort);
			if (argc >= 3 && strcmp(argv[2], "-send") == 0)
			{
				// This client will send a file
				client_sending = true;
				server_receiving = true;
				getFilename(filename, sizeof(filename));
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
			// This server will send a file
			server_sending = true;
			client_receiving = true;
			getFilename(filename, sizeof(filename));
		}
	}
	else
	{
		mode = Server;
		client_sending = true;
		server_receiving = true;
		// This server will wait to receive a file
	}

	// initialize

// After setting mode and flags
//std::cout << "Mode: " << (mode == Server ? "Server" : "Client") << std::endl;
//std::cout << "Server Sending: " << (server_sending ? "true" : "false") << std::endl;
//std::cout << "Client Sending: " << (client_sending ? "true" : "false") << std::endl;
//std::cout << "Server Receiving: " << (server_receiving ? "true" : "false") << std::endl;
//std::cout << "Client Receiving: " << (client_receiving ? "true" : "false") << std::endl;

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


		// Determine if server is sending then file will be devided into chunks
		// ,file metadata will be added and packet will be sent.
		// then there will be a code for client that will receive packets, put back into order and
		// will write data to the file and store it.
		// 


		// After setting mode and flags
		/*std::cout << "Mode: " << (mode == Server ? "Server" : "Client") << std::endl;
		std::cout << "Server Sending: " << (server_sending ? "true" : "false") << std::endl;
		std::cout << "Client Sending: " << (client_sending ? "true" : "false") << std::endl;
		std::cout << "Server Receiving: " << (server_receiving ? "true" : "false") << std::endl;
		std::cout << "Client Receiving: " << (client_receiving ? "true" : "false") << std::endl;*/

		sendAccumulator += DeltaTime;
		static int count = 1;
		// Here filename will be given to a function that will
		// break it into chunks of data before sending


		while (sendAccumulator > 1.0f / sendRate)
		{
			unsigned char packet[PacketSize];
			memset(packet, 0, sizeof(packet));

			string greeting;
			if (mode == Server && server_sending == true && client_sending == false)
			{
				greeting = "Server is sending packet number: " + to_string(count++);
				printf("I am server sending the file %s in server mode.\n", filename);
			}
			else if (mode == Client && client_sending == true && server_sending == false)
			{
				greeting = "Client is sending packet number: " + to_string(count++);
				printf("I am client sending the file %s in client mode.\n", filename);
			}

			memcpy(packet, greeting.c_str(), greeting.size() < PacketSize ? greeting.size() : PacketSize - 1);
			connection.SendPacket(packet, sizeof(packet));
			sendAccumulator -= 1.0f / sendRate;

			//// For Sending tasks:
			//		//// 
			//		//// 1. Receiving the file metadata
			//		////    - Extract the file metadata from the received packet.
			//		////    - Implement logic to identify and handle metadata packets.
			//		////    - Store or process the received metadata.

			//		//// 2. Receiving the file pieces
			//		////    - Extract file pieces from the received packet.
			//		////    - Identify packets containing file pieces based on their structure or flags.
			//		////    - Assemble the file pieces into a complete file back on the server side.

			//		//// 3. Writing the pieces out to the disk
			//		////    - Write the received file pieces to a file on the server's disk.
			//		////    - Keep track of the received file pieces and ensure proper order.
			//		////    - Implement error handling

			//		//// 4. Verifying the file integrity
			//		////    - Use checksums or other methods to verify the correctness of the file.
			//		////    - Handle cases where the file integrity check fails.
			//	
		}

		while (true)
		{
			unsigned char packet[256];
			int bytes_read = connection.ReceivePacket(packet, sizeof(packet));
			if (bytes_read == 0)
				break;

			// For Receiving tasks:

			// 1. Retrieving the file from disk
			// - Read the file from disk into memory for further processing.
			// - Handle file not found or access errors appropriately.

			// 2. Sending file metadata
			// - Prepare metadata for the file (e.g., file name, size, type).
			// - Create a packet containing the file metadata and send it to the server.

			// 3. Breaking the file in pieces to send
			// - Divide the file into smaller pieces to fit into network packets.
			// - Add necessary information to each piece (e.g., sequence number).
			// - Create packets with file pieces and send them to the server.

			// 4. Sending the pieces
			// - Send the packets containing file pieces to the server.
			// - Implement reliable and ordered delivery to ensure correct reconstruction.
			// - Handle acknowledgment and retransmission for reliable delivery.

			if (mode == Server && server_receiving == true && client_receiving == false)
			{
				printf("%.*s\n", bytes_read, packet);
				printf("I am server receiving the file in server mode.\n");


			}
			else if (mode == Client && client_receiving == true && server_receiving == false)
			{
				printf("%.*s\n", bytes_read, packet);
				printf("I am client receiving the file in client mode.\n");


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