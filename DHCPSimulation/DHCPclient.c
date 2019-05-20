/*
Author:		Jesus Rodriguez
Date;		25 April 2018
Program Name:	DHCPclient.c
Description:	This program iss used to act as a DHCP client requesting an IP adress from
		an DHCP server.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define IP "129.120.151.94"
#define MAX 2147483647

struct dhcpPacket
{
	unsigned int siaddr[4];
	unsigned int yiaddr[4];
	unsigned int tranID;
	unsigned short int lifetime;
};

void printPacket(struct dhcpPacket packet);

int main(int argc, char** argv)
{
	// check the number of arguments
	if(argc != 2)
	{
		printf("Wrong amount of arguments.\nUsage: ./executable port_number\n");
		exit(0);
	}

	srand(time(NULL));

	struct sockaddr_in addr; // the address
	int client_fd; // the client's socket descriptor
	int addrLen = sizeof(addr); //  the size of the address
	int port = atoi(argv[1]); // the port
	int i; // looping variable
	int bytesRead;

	// set up the UDP socket
	if((client_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		printf("Failed to set up UDP socket\n");
		exit(0);
	}

	// set up the address
	memset((char*)&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	// turn the server's address to binary
	if(inet_aton(IP, &addr.sin_addr) == 0)
	{
		exit(0);
	}
	
	struct dhcpPacket packet;

	// place the server address values in the packet source IP address
	packet.siaddr[0] = 129;
	packet.siaddr[1] = 120;
	packet.siaddr[2] = 151;
	packet.siaddr[3] = 94;

	// initialize your IP address to 0.0.0.0
	packet.yiaddr[0] = packet.yiaddr[1] = packet.yiaddr[2] = packet.yiaddr[3] = 0;

	packet.tranID = (rand() % MAX) + 1; // assign a random transaction ID
	packet.lifetime = 0; // set the lifetime to 0

	// send the DHCP packets
        printf("Sending DHCP discover packet: \n");
	printPacket(packet);
	sendto(client_fd, &packet, sizeof(struct dhcpPacket), 0, (struct sockaddr*)&addr, addrLen);

	bytesRead = recvfrom(client_fd, &packet, sizeof(struct dhcpPacket), 0, (struct sockaddr*)&addr, &addrLen);
        printf("\nReceiving DHCP offer Packet: \n");
	printPacket(packet);

	if(packet.yiaddr[0] == 0 && packet.yiaddr[1] == 0 && packet.yiaddr[2] == 0 && packet.yiaddr[3] == 0)
	{
		printf("There were no address available\n");
	}
	else
	{
		packet.tranID += 1;
	        printf("Sending DHCP request packet: \n");
		printPacket(packet);
		sendto(client_fd, &packet, sizeof(struct dhcpPacket), 0, (struct sockaddr*)&addr, addrLen);

		bytesRead = recvfrom(client_fd, &packet, sizeof(struct dhcpPacket), 0, (struct sockaddr*)&addr, &addrLen);
	     	printf("\nReceiving DHCP ACK Packet: \n");
		printPacket(packet);
	
		printf("The client will go into an infinite loop.\n");
		printf("Press enter to close client program: ");
		char exitChar = 0;
		// keep the client program alive with an infinite loop
		while(exitChar != '\n')
		{
			scanf("%c", &exitChar);
		}
	}


	close(client_fd);
	return 0;
}
/*
Name:		printPacket
Parameters:	a dhcpPacket struct representing a packet
Return;
Description:	this function prints the values of a DHCP packet object
*/
void printPacket(struct dhcpPacket packet)
{
	printf(" _________________________\n");
        printf("| siaddr: %-3u.%-3u.%-3u.%-3u |\n", packet.siaddr[0], packet.siaddr[1], packet.siaddr[2], packet.siaddr[3]);
        printf("| yiaddr: %-3u.%-3u.%-3u.%-3u |\n", packet.yiaddr[0], packet.yiaddr[1], packet.yiaddr[2], packet.yiaddr[3]);
        printf("| lifetime: %13u |\n", packet.lifetime);
        printf("| tranID: %15u |\n",  packet.tranID);
        printf(" -------------------------\n");

	return;
}
