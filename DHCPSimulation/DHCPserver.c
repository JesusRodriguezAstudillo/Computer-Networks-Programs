/*
Author:		Jesus Rodriguez
Date:		25 April 2018
Program Name:	DHCPserver.c
Description:	This program acts as the server that simulates DHCP architecture.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>
#include <math.h>

struct dhcpPacket
{
        unsigned int siaddr[4];
        unsigned int yiaddr[4];
        unsigned int tranID;
        unsigned short int lifetime;
};

void ipToInt(char* IP, unsigned int* addrArr);
void assignAddr(int* gateway, unsigned int* mask, unsigned int* addrArr);
void printPacket(struct dhcpPacket packet);

int main(int argc, char** argv)
{
	// check the number of arguments
	if(argc != 2)
	{
		printf("Wrong amount of arguments.\nUsage: ./executable port_number\n");
		exit(0);
	}

	struct sockaddr_in svrAddr, cliAddr; // the client and server addresses
	int cliAddrSize = sizeof(cliAddr); // the client addess size
	int server_fd; // the socket descriptor for the server
	int port = atoi(argv[1]); // the port number 
	unsigned int subnetMask[4]; // the server's subnet mask
	int gateway[4]; // the server's gateway
	int gatewayCopy[4]; // a copy of the gateway that can be modified
	int i,j; // looping variables
	char mask[17]; // used to hold the subnet mask as a string
	char gate[17]; // used to hold the gateway as a string
	int bytesRead;

	// clear the arrays
	memset(mask, 0, 17);
	memset(mask, 0, 17);

	// set up the UDP connection
	server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(server_fd < 0)
	{
		exit(0);
	}

	// set the address of the socket
	memset((char*)&svrAddr, 0, sizeof(svrAddr));
	svrAddr.sin_family = AF_INET;
	svrAddr.sin_port = htons(port);
	svrAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(server_fd, (struct sockaddr*)&svrAddr, sizeof(svrAddr)) < 0)
	{
		printf("Error binding the socket\n");
		exit(0);
	}

	// read in the gateway and the subnet mask
	printf("Enter the server's gateway: ");
	fgets(gate, 16, stdin);
	printf("Enter the subnet mask: ");
	fgets(mask, 16, stdin);

	// remove the newlines from the strings
	for(i = 0; gate[i] != 10; i++);
	for(j = 0; mask[j] != 10; j++);
	gate[i] = 0;
	mask[j] = 0;

	printf("Waiting for dicover packets...\n");

	// turn the gateway and subnet mask strings into ints
	ipToInt(gate, gateway);
	printf("Gateway\n");
	printf("%u.%u.%u.%u\n", gateway[0], gateway[1], gateway[2], gateway[3]);
	for(i = 0; i < 4; i++) gatewayCopy[i] = gateway[i]; 

	ipToInt(mask, subnetMask);
	printf("\nSubnet mask\n");
	printf("%u.%u.%u.%u\n\n", subnetMask[0], subnetMask[1], subnetMask[2], subnetMask[3]);

	// start transaction with client
	struct dhcpPacket packet;

	// let the server receive packets
	while(1)
	{
		bytesRead = recvfrom(server_fd, &packet, sizeof(struct dhcpPacket), 0, (struct sockaddr*)&cliAddr, &cliAddrSize);
		printf("Receiving DHCP discover packet: \n");
		printPacket(packet);

		assignAddr(gatewayCopy, subnetMask, packet.yiaddr);
		
		if(packet.yiaddr[0] == 0 && packet.yiaddr[1] == 0 && packet.yiaddr[2] == 0 && packet.yiaddr[3] == 0) packet.lifetime = 0;
		else packet.lifetime = 3600;

		printf("Sending DHCP offer packet: \n");
		printPacket(packet);
		sendto(server_fd, &packet, sizeof(struct dhcpPacket), 0, (struct sockaddr*)&cliAddr, cliAddrSize);

		bytesRead = recvfrom(server_fd, &packet, sizeof(struct dhcpPacket), 0, (struct sockaddr*)&cliAddr, &cliAddrSize);
		printf("Receiving DCHP request packet: \n");
		printPacket(packet);

		printf("Sending DHCP ACK packet: \n");
		printPacket(packet);
		sendto(server_fd, &packet, sizeof(struct dhcpPacket), 0, (struct sockaddr*)&cliAddr, cliAddrSize);
	}

	printf("Closing connection.\n");

	close(server_fd);
	return 0;
}
/*
Name:		ipToInt
Parameters:	a char* representing an IP address and an unsigned int* representing an array of ints
Return:		
Description:	This function takes an IP address as a string, splits the string by ".", and saves the
		individual tokes as unsigned ints in the unsigned int* parameter
*/
void ipToInt(char* IP, unsigned int* addrArr)
{
	char *tokens = strtok(IP, ".\n"); // tokenize the IP address
	unsigned int temp;
	int counter = 0; // used to determine how many bits to shift by

	while(tokens != NULL)
	{
		temp = atoi(tokens); // turn the token into an int
		addrArr[counter] = temp;
		counter++; // decrease the counter
		tokens = strtok(NULL, ".\n"); // get next token
	}

	return;
}
/*
Name:		assignAddr
Parameters:	an int* representing an array of ints, an unsigned int* representing a gateway mask, an
		int* representing the array that will hold the IP address values
Return:
Description:	This function is used to assign IP values using the gateway and the mask to determine which
		address are available
*/
void assignAddr(int* gateway, unsigned int* mask, unsigned int* addrArr)
{
	int temp = 0;

	if(gateway[0] == 255 && gateway[1] == 255 && gateway[2] == 255) temp = 1; // if the address has the form 255.255.255.***

	if(mask[3] < 255 && gateway[3] < (255 - mask[3])) // if some of the lowest 8 bits can be used and are available
	{
		if(temp == 1 && gateway[3] < 254) // if the gateway has the form 255.255.255.***, avoid assigning 255.255.255.255
		{
			// assign values in the lowest bits
			addrArr[3] = ++gateway[3];
			addrArr[2] = gateway[2];
			addrArr[1] = gateway[1];
			addrArr[0] = gateway[0];
			
			gateway[3] = gateway[3]++;
		}
		else if(temp != 1)
		{
			// assign values in the lowest bits
			addrArr[3] = ++gateway[3];
			addrArr[2] = gateway[2];
			addrArr[1] = gateway[1];
			addrArr[0] = gateway[0];

			gateway[3] = gateway[3]++;

			// check if the address needs to be reset
			if(gateway[3] == 255 && mask[2] < 255)
			{
				gateway[2] = ++gateway[2];
				gateway[3] = -1;
			}
		}
	}
	else if(mask[2] < 255 && gateway[2] < (255 - mask[2]))
	{
		// assign an address in the right most 16 bits
		addrArr[3] = 0;
		addrArr[2] = ++gateway[2];
		addrArr[1] = gateway[1];
		addrArr[0] = gateway[0];

		gateway[2] = gateway[2]++; // remove the address from the being used
		gateway[3] = 0; // reset the rest of the address

		if(gateway[2] == 255 && mask[1] < 255) // check if the address needs to be reset
		{
			gateway[1] = ++gateway[1]; 
			gateway[3] = gateway[2] = 0;
		}
	}
	else if(mask[1] < 255 && gateway[1] < (255 - mask[1]))
	{
		// assign an address in the left most 16 bits
		addrArr[3] = 0;
		addrArr[2] = 0;
		addrArr[1] = ++gateway[1];
		addrArr[0] = gateway[0];


		gateway[1] = gateway[1]++; 
		gateway[3] = 0;
		gateway[2] = 0;

		if(gateway[1] == 255 && mask[0] < 255) // if there are more address at higher 8 bits
		{
			gateway[0] = ++gateway[0]; 
			gateway[3] = gateway[2] = gateway[1] = 0;
		}
	}
	else if(mask[0] < 255 && gateway[0] < (255 - mask[0]))
	{
		// assign values in the highest 8 bits
		addrArr[3] = 0;
		addrArr[2] = 0;
		addrArr[1] = 0;
		addrArr[0] = ++gateway[0];

		gateway[0] = gateway[0]++; // remove the address from the being used
		gateway[1] = gateway[2] = gateway[3] = 0;
	}

	return;
}
/*
Name:		printPacket
Parameters:	a dhcpPacket struct
Return:
Description:	this function is used to print the values of a dhcpPacket object
*/
void printPacket(struct dhcpPacket packet)
{
	printf(" _________________________\n");
	printf("| siaddr: %-3u.%-3u.%-3u.%-3u |\n", packet.siaddr[0], packet.siaddr[1], packet.siaddr[2], packet.siaddr[3]);
	printf("| yiaddr: %-3u.%-3u.%-3u.%-3u |\n", packet.yiaddr[0], packet.yiaddr[1], packet.yiaddr[2], packet.yiaddr[3]);
	printf("| lifetime: %13u |\n", packet.lifetime);
	printf("| tranID: %15u |\n",  packet.tranID);
	printf(" -------------------------\n\n");

	return;
}
