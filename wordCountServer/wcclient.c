/*
Author:		Jesus Rodriguez
Date:		7 February 2018
Program Name:	wcclient.c
Description:	This program is used to connect with a server so that the server
		can determine the word count, the character count, and to change
		the any uppercase letters to lowercase letters.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#define SERVER "129.120.151.94" //cse01.cse.unt.edu

int main(int argc, char** argv)
{
	// determine if the user entered the right amount of arguments
	if(argc != 2)
	{
		printf("Wrong amount of arguments.\nUsage: ./executable port_number.\n");
		exit(0);
	}

	int client_fd; // the client's socket descriptor
	int bytesRead; // the number of bytes read
	struct sockaddr_in cli_addr; // the address used by the client
	char buffer[1028]; // the message buffer
	int portnum = atoi(argv[1]); // determine which port to connect to
	int i; // a looping variable

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

	// connect to the server
	if(connect(client_fd, (struct sockaddr*)&cli_addr, sizeof(cli_addr)) < 0)
	{
		printf("Error connecting socket");
		exit(0);
	}

	do
	{
		// receive input from user
		memset(buffer, 0, 1028);
		printf("Input: ");
		fgets(buffer, 1027, stdin);
		
		// remove the newline character
		for(i = 0; buffer[i] != 10; i++);
		buffer[i] = 0;

		if(strcmp(buffer, "quit") != 0) // if the message is not quitting
		{
			// send the input
			write(client_fd, buffer, strlen(buffer));

			// read the message from the server
			memset(buffer, 0, 1028);
			bytesRead = read(client_fd, buffer, 1027);
			buffer[bytesRead] = 0;
			printf("Output: %s\n", buffer);

			// send an ok message
			write(client_fd, "Ready", strlen("Ready"));

			// read the remaining information
			memset(buffer, 0, 1028);
			bytesRead = read(client_fd, buffer, 1027);
			buffer[bytesRead] = 0;
			printf("%s", buffer);
		}
		else
		{
			write(client_fd, "quit", strlen("quit")); // the user is quitting so let the server know
		}
	}while(strcmp(buffer, "quit") != 0);

	// receive the quit message from the server
	memset(buffer, 0, 1028);
	bytesRead = read(client_fd, buffer, 1027);
	buffer[bytesRead] = 0;
	printf("Server says: %s\n", buffer);

	// close the client socket
	if(close(client_fd) < 0)
	{
		printf("Error closing socket\n");
		exit(0);
	}

	return 0;
}
