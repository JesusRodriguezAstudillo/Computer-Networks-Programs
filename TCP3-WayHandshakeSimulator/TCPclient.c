/*
Author:		Jesus Rodriguez
Date:		11 April 2018
Program Name:	TCPclient.c
Description:	This program is used to act as the client in a TCP connection. This program will send packets to the server
		to simulate a 3 way handshake before simulating closing the connection.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#define SERVER "129.120.151.94" //cse01.cse.unt.edu
#define MAX 4294967296 // the max sequence value

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

void buildPacket(struct tcp_hdr* packet, int srcPortNum, int destPortNum, int seqNum, int ackNum, int ackBit, int finBit);
void checksum(struct tcp_hdr* packet);
void printPacket(struct tcp_hdr packet, int append);

int main(int argc, char** argv)
{
	// determine if the user entered the right amount of arguments
	if(argc != 2)
	{
		printf("Wrong amount of arguments.\nUsage: ./executable port_number.\n");
		exit(0);
	}

	srand(time(NULL));
	
	int client_fd; // the client's socket descriptor
	int bytesRead; // the number of bytes read
	struct sockaddr_in cli_addr, addr; // the address used by the client
	char buffer[1028]; // the message buffer
	int portnum = atoi(argv[1]); // determine which port to connect to
	int i; // a looping variable
	int length;

	// create the socket
	client_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(client_fd < 0)
	{
		printf("Error creating client socket.\n");
		exit(0);
	}

	// set up the address and the port number to connect to
	bzero((char*)&cli_addr, sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_addr.s_addr = inet_addr(SERVER);
	cli_addr.sin_port = htons(portnum);

	// determine the client's socket information
	bzero((char*)&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(SERVER);
	addr.sin_port = htons(portnum);

	bzero(&addr, sizeof(addr));
	length = sizeof(addr);

	// connect to the server
	if(connect(client_fd, (struct sockaddr*)&cli_addr, sizeof(cli_addr)) < 0)
	{
		printf("Error connecting socket");
		exit(0);
	}

	// get the address of the client to determine the port
	getsockname(client_fd, (struct sockaddr*)&addr, &length);
	int port = ntohs(addr.sin_port);

	// generate a sequence number
	int seqNumber = (rand() % (MAX-1)) + 1;
	struct tcp_hdr packet;

	// simulate 3 way handshake
	printf("Starting three way handshake...\n");

	// client sent sync packet
	printf("Sending SYN packet...\n");
	buildPacket(&packet, ntohs(addr.sin_port), portnum, seqNumber, 0, 0, 0);
	printPacket(packet, 0);
	write(client_fd, &packet, sizeof(struct tcp_hdr));

	// read the server's ack
	printf("Receiving server ACK packet...\n");
	read(client_fd, &packet, sizeof(struct tcp_hdr));
	printPacket(packet, 1);

	// send an ack to the server
	printf("Sending ACK packet to server...\n");
	buildPacket(&packet, port, portnum, seqNumber+1, packet.seq+1, 1, 0);
	printPacket(packet, 1);
	write(client_fd, &packet, sizeof(struct tcp_hdr));
	printf("Finished three way handshake...\n");

	// simulate closing the connection
	printf("Closing connection...\n");

	// send FIN packet
	printf("Sending FIN packet...\n");
	buildPacket(&packet, port, portnum, 1024, 512, 0, 1);
	printPacket(packet, 1);
	write(client_fd, &packet, sizeof(struct tcp_hdr));

	// read server response
	printf("Receiving server FIN ACK packet...\n");
	read(client_fd, &packet, sizeof(struct tcp_hdr));
	printPacket(packet, 1);

	// read server FIN packet
	printf("Receiving Server FIN packet..\n");
	read(client_fd, &packet, sizeof(struct tcp_hdr));
	printPacket(packet, 1);
		
	// send ack to the server
	printf("Sending ACK packet to server...\n");
	buildPacket(&packet, port, portnum, packet.ack+1, packet.seq+1, 1, 0);
	printPacket(packet, 1);
	write(client_fd, &packet, sizeof(struct tcp_hdr));

	printf("Finished closing connection...\n");

	// close the client socket
	if(close(client_fd) < 0)
	{
		printf("Error closing socket\n");
		exit(0);
	}

	return 0;
}
/*
Name:           buildPacket
Parameters:     a tcp_hdr struct representing the packet that will be populated with values, an int representing the source port an int
                representing the destination port, int representing the sequence number, an int representing the acknowledgment number
                an int representing an ACK bit, and an int representing the FIN bit
Return;
Description:    This function is used to populate a struct tcp_hdr for
*/
void buildPacket(struct tcp_hdr* packet, int srcPortNum, int destPortNum, int seqNum, int ackNum, int ackBit, int finBit)
{
	// assign values to the packet
	packet->src = srcPortNum;
	packet->seq = seqNum;
	packet->des = destPortNum;
	packet->ack = ackNum;
	packet->rec = 0;
	packet->ptr = 0;
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
Author:         Dr. Robin Pottanthuparambil
Name:           checksum
Parameters:     a struct tcp_hdr* representing a packet
Return:
Description:    this function is used to determine the checksum of a packet and assigns it to the packet
All credit goes to the author
*/
void checksum(struct tcp_hdr* packet)
{
	unsigned short int cksum_arr[12];
	unsigned int i,sum=0, cksum, wrap;

	memcpy(cksum_arr, &packet, 24); //Copying 24 bytes
  
//	printf ("\n16-bit values for Checksum Calculation\n");
	for (i=0;i<12;i++)            // Compute sum
	{
//		printf("0x%04X\n", cksum_arr[i]);
		sum = sum + cksum_arr[i];
	}

	wrap = sum >> 16;             // Wrap around once
	sum = sum & 0x0000FFFF; 
	sum = wrap + sum;

	wrap = sum >> 16;             // Wrap around once more
	sum = sum & 0x0000FFFF;
	cksum = wrap + sum;

	printf("\nSum Value: 0x%04X\n", cksum);
	/* XOR the sum for checksum */
	printf("\nChecksum Value: 0x%04X\n", (0xFFFF^cksum));
	packet->cksum = cksum;

	return;
}
/*
Name:           printPacket
Parameters:     a struct tcp_hdr representing a packet and an int representing whether to append of file
Return:
Description:    this function is used to write the contents of a packet to the file
*/
void printPacket(struct tcp_hdr packet, int append)
{
	FILE* output; // file pointer
	
	if(append == 0) output = fopen("client.out", "w"); // if it's the first packet, create a new file
	else output = fopen("client.out", "a"); // else append append to the file

	// check if the 
	if(output == NULL)
	{
		printf("Failed to open the file.\n");
		exit(0);
	}

	// print packet values to console
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

	// print packet values to file	
	fprintf(output, "\nPacket Values\n");
        fprintf(output, " ------------------------------\n");
        fprintf(output, "| Source          |");
        fprintf(output, " 0x%04X     |\n", packet.src);
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

	fclose(output);

	return;
}
