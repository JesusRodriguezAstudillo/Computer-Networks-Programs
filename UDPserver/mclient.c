/*
Author:		Jesus Rodriguez
Date;		23 March 2018
Program Name:	mclient.c
Description:	This program contains the client code that is used to connect with the
		a the UDP math server. The client code takes a mathematical expression
		and waits for some response from the server. This process continues
		until the client types "quit".
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define IP "129.120.151.94"

int main(int argc, char** argv)
{
	// check the number of arguments
	if(argc != 2)
	{
		printf("Wrong amount of arguments.\nUsage: ./executable port_number\n");
		exit(0);
	}

	struct sockaddr_in addr; // the address
	int client_fd; // the client's socket descriptor
	int addrLen = sizeof(addr); //  the size of the address
	int port = atoi(argv[1]); // the port
	char buffer[1024]; 
	int i; // looping variable
	int bytesRead;

	// set up the UDP socket
	if((client_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		printf("TEST1.1\n\n");
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

	// while the user wants an expression to evaluate
	while(1)
	{
		// recieve the expressioin
		memset(buffer, 0, sizeof(buffer));
		printf("Expression: ");
		fgets(buffer, 1023, stdin);
	
		// remove the newline and append a null character
		for(i = 0; buffer[i] != 10; i++);
		buffer[i] = 0;

		if(strcmp(buffer, "quit") == 0) // if the user is quitting
		{
			// let the server know the user is quitting
			sendto(client_fd, "quit", strlen("quit"), 0, (struct sockaddr*)&addr, addrLen);

			bytesRead = recvfrom(client_fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&addr, &addrLen);
			buffer[bytesRead] = 0;

			if(strcmp(buffer, "Peace out!") == 0)
			{
				printf("%s\n", buffer);
				break;
			}
			else printf("An error occur. Could not close the connection.");
		}
		else // send the expression to the server
		{
			sendto(client_fd, buffer, strlen(buffer), 0, (struct sockaddr*)&addr, addrLen);
			memset(buffer, 0, sizeof(buffer));

			// receive the result from the server
			bytesRead = recvfrom(client_fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&addr, &addrLen);
			buffer[bytesRead] = 0;
			printf("Result: %s\n", buffer);
		}
	}

	close(client_fd);
	return 0;
}
