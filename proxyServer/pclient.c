/*
Author:		Jesus Rodriguez
Date:		7 February 2018
Program Name:	pclient.c
Description:	This program is used to send a webpage request to a proxy server and
		wait for a response from the server.
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
	char buffer[1024]; // the message buffer
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

	// receive input from user
	memset(buffer, 0, 1024);
	printf("URL: ");
	fgets(buffer, 1023, stdin);
	
	// remove the newline character
	for(i = 0; buffer[i] != 10; i++);
	buffer[i] = 0;

	// send the request to the server
	write(client_fd, buffer, strlen(buffer));

	// wait for a response from the server
	printf("Proxy Server Response: \n");
	while(1)
	{
		memset(buffer, 0, 1024);
		bytesRead = read(client_fd, buffer, 1023);
		buffer[bytesRead] = 0;
		if(bytesRead == 0) break; // if the server sends a message that it finished the request
		printf("%s", buffer);
	}
	printf("\n");

	// close the client socket
	if(close(client_fd) < 0)
	{
		printf("Error closing socket\n");
		exit(0);
	}

	return 0;
}
