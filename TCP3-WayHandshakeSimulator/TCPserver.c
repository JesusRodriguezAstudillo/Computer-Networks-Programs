/*
Author:		Jesus Rodriguez
Date:		11 April 2018
Program Name:	TCPserver.c
Description:	This program is the server that a client can connect to in order to simulate
		a TCP 3-way handshake and TCP closing connection.
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

#define MAX 42949667296 // the max value for a sequence number

struct tcp_hdr
{
	unsigned short int src;
	unsigned short int des;
	unsigned int seq;
	unsigned int ack;
	unsigned short int hdr_flags;
	unsigned short int rec;
	unsigned short int cksum;
	unsigned short int ptr;
	unsigned int opt;
};

void processMessage(int client_fd, char* message, int charCount);
void checksum(struct tcp_hdr* packet);
void buildPacket(struct tcp_hdr* packet, int srcPortNum, int destPortNum, int seqNum, int ackNum, int ackBit, int finBit);
void writePacket(struct tcp_hdr packet, int append);

int main(int argc, char** argv)
{
	// check if the user entered tht correct amount of arguments
	if(argc != 2)
	{
		printf("Wrong amount of arguments.\nUsage: ./executable port_number.\n");
		exit(0);
	}
	
	srand(time(NULL));

	int server_fd; // the server descriptor
	int client_fd; // the client descriptor
	int cli_addr_size; // the size of the client address
	int error; // error handler
	struct sockaddr_in svr_addr; // the server address
	struct sockaddr_in cli_addr; // the client address
	char buffer[1028]; // the message buffer
	int byteCount = 0; // the number of bytes read
	int portnum = atoi(argv[1]); // determine the port to listen on

	// create file descriptor for the socket
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(server_fd < 0)
	{
		printf("Socket failure\n");
		exit(0);
	}

	// allow the port to be reused
	int key = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &key, sizeof(key));

	// set the address of the server
	bzero((char*)&svr_addr, sizeof(svr_addr));
	svr_addr.sin_family = AF_INET;
	svr_addr.sin_addr.s_addr = INADDR_ANY;
	svr_addr.sin_port = htons(portnum);

	// bind the socket
	if(bind(server_fd, (struct sockaddr*)&svr_addr, sizeof(svr_addr)) < 0)
	{
		printf("Error binding server socket\n");
		exit(0);
	}

	while(1)
	{
		// listen for connections
		if(listen(server_fd, 5) < 0)
		{	
			printf("Error listening.\n");
			exit(0);
		}

		printf("Connecting a client...\n");
		// connect the client
		cli_addr_size = sizeof(cli_addr);
		client_fd = accept(server_fd, (struct sockaddr*)&cli_addr, &cli_addr_size);
		if(client_fd < 0)
		{
			printf("Error accepting client\n");
			exit(0);
		}

		struct tcp_hdr packet, readPacket;
		int seqNum = (rand() % (MAX-1)) + 1;

		// simulate the 3 way handshake
		printf("\nStarting three way handshake...\n");

		// receive client's syn packet
		printf("Receiving client SYN packet...\n");
		read(client_fd, &readPacket, sizeof(struct tcp_hdr));
		writePacket(readPacket, 0);

		// send ACK packet
		printf("Sending ACK packet...\n");
		buildPacket(&packet, readPacket.des, readPacket.src, seqNum, readPacket.seq + 1, 1, 0);
		writePacket(packet, 1);
		write(client_fd, &packet, sizeof(struct tcp_hdr));

		// receive client's ACK
		printf("Receiving client's ACK packet...\n");
		read(client_fd, &readPacket, sizeof(struct tcp_hdr));
		writePacket(readPacket, 1);
	
		printf("Finished three way handshake...\n");

		// simulate closing connection
		printf("\nClosing connection with client...\n");

		// receive client's FIN packet
		printf("Receiving client's FIN packet...\n");
		read(client_fd, &readPacket, sizeof(struct tcp_hdr));
		writePacket(readPacket, 1);

		// send ACK to client
		printf("Sending ACK for client's FIN packet...\n");
		buildPacket(&packet, readPacket.des, readPacket.src, 512, readPacket.seq + 1, 1, 0);
		writePacket(packet, 1);
		write(client_fd, &packet, sizeof(struct tcp_hdr));
	
		// send server FIN packet
		printf("Sending FIN packet...\n");
		buildPacket(&packet, readPacket.des, readPacket.src, 512, readPacket.seq + 1, 0, 1);
		writePacket(packet, 1);
		write(client_fd, &packet, sizeof(struct tcp_hdr));

		// receive client's ACK packet
		printf("Receiving clint's ACK packet...\n");
		read(client_fd, &readPacket, sizeof(struct tcp_hdr));
		writePacket(readPacket, 1);

		// close the client socket
		if(close(client_fd) < 0)
		{
			printf("Error closing client socket.\n");
			exit(0);
		}
	}

	// close the server socket
	if(close(server_fd) < 0)
	{
		printf("Error closing server socket.\n");
		exit(0);
	}

	return 0;
}
/*
Author:		Dr. Robin Pottanthuparambil
Name:		checksum
Parameters:	a struct tcp_hdr* representing a packet
Return:		
Description:	this function is used to determine the checksum of a packet and assigns it to the packet
All credit goes to the author
*/
void checksum(struct tcp_hdr* packet)
{
	unsigned short int cksum_arr[12];
	unsigned int i,sum=0, cksum, wrap;

  	memcpy(cksum_arr, &packet, 24); //Copying 24 bytes
 
 // 	printf ("\n16-bit values for Checksum Calculation\n");
  	for (i=0;i<12;i++)            // Compute sum
  	{
   //  		printf("0x%04X\n", cksum_arr[i]);
	 	sum = sum + cksum_arr[i];
  	}

  	wrap = sum >> 16;             // Wrap around once
  	sum = sum & 0x0000FFFF; 
  	sum = wrap + sum;

  	wrap = sum >> 16;             // Wrap around once more
  	sum = sum & 0x0000FFFF;
  	cksum = wrap + sum;

  	printf("Sum Value: 0x%04X\n", cksum);
  	/* XOR the sum for checksum */
  	printf("Checksum Value: 0x%04X\n", (0xFFFF^cksum));
	packet->cksum = (0xFFFF^cksum);

	return;
}
/*
Name:		buildPacket
Parameters:	a tcp_hdr struct representing the packet that will be populated with values, an int representing the source port an int
		representing the destination port, int representing the sequence number, an int representing the acknowledgment number
		an int representing an ACK bit, and an int representing the FIN bit
Return;
Description:	This function is used to populate a struct tcp_hdr for 
*/
void buildPacket(struct tcp_hdr* packet, int srcPortNum, int destPortNum, int seqNum, int ackNum, int ackBit, int finBit)
{
	// assign values to the packet
	packet->seq = seqNum;
	packet->ack = ackNum;
	packet->des = destPortNum;
	packet->src = srcPortNum;
	packet->ptr = 0;
	packet->rec = 0;
	packet->opt = 0;
	checksum(packet);
	
	// determine which flags to set
        if(ackBit == 0 && finBit == 0) packet->hdr_flags = 0x6002;
        else if(ackBit == 1 && finBit == 0) packet->hdr_flags = 0x6010;
        else if(finBit == 1) packet->hdr_flags = 0x6001;
        else packet->hdr_flags = 0x6012;

	return;
}
/*
Name:		writePacket
Parameters:	a struct tcp_hdr representing a packet and an int representing whether to append of file
Return;
Description:	this function is used to write the contents of a packet to the file
*/
void writePacket(struct tcp_hdr packet, int append)
{
	FILE* output; // open the file

        if(append == 0) output = fopen("server.out", "w"); // if this is the first packet that needs to be saved
        else output = fopen("server.out", "a"); // else append packets to the file

	// check if the file opened correctly
        if(output == NULL)
        {
                printf("Failed to open the file.\n");
                exit(0);
        }

	// print values to the console
	printf("Packet Values\n");
	printf(" ------------------------------\n");
	printf("| Source          |");
        printf(" 0x%04X     |\n", packet.src);
	printf("| Destination     |");
        printf(" 0x%04X     |\n", packet.des);
	printf("| Sequence        |");
        printf(" 0x%08X |\n", packet.seq);
	printf("| Acknowledgement |");
        printf(" 0x%08X |\n", packet.ack);
	printf("| Header/Flags    |");
        printf(" 0x%04X     |\n", packet.hdr_flags);
	printf("| Receive Window  |");
        printf(" 0x%04X     |\n", packet.rec);
	printf("| CheckSum        |");
        printf(" 0x%04X     |\n", packet.cksum);
	printf("| Urgent Pointer  |");
        printf(" 0x%04X     |\n", packet.ptr);
	printf("| Options         |");
        printf(" 0x%08X |\n", packet.opt);
	printf(" ------------------------------\n");

	// print values to file
	fprintf(output, "\nPacket Values\n");
	fprintf(output, " ------------------------------\n");
	fprintf(output, "| Source          |");
        fprintf(output, " 0x%04X     |\n", packet.src); // Printing all values
	fprintf(output, "| Destination     |");
        fprintf(output, " 0x%04X     |\n", packet.des);
	fprintf(output, "| Sequence        |");
        fprintf(output, " 0x%08X |\n", packet.seq);
	fprintf(output, "| Acknowledgement |");
        fprintf(output, " 0x%08X |\n", packet.ack);
	fprintf(output, "| Header/Flags    |");
        fprintf(output, " 0x%04X     |\n", packet.hdr_flags);
	fprintf(output, "| Receive Window  |");
        fprintf(output, " 0x%04X     |\n", packet.rec);
	fprintf(output, "| CheckSum        |");
        fprintf(output, " 0x%04X     |\n", packet.cksum);
	fprintf(output, "| Urgent Pointer  |");
        fprintf(output, " 0x%04X     |\n", packet.ptr);
	fprintf(output, "| Options         |");
        fprintf(output, " 0x%08X |\n", packet.opt);
	fprintf(output, " ------------------------------\n");
     
        fclose(output); // close file

	return;
}
