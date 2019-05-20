/*
Author:		Jesus Rodriguez
Date:		7 February 2017
Program Name:	wcserver.c
Description:	This program is used to communicate with a client and determine the word
		count, the ASCII character count, and to change any capital letters to 
		lower case. The counts and the modified message are then sent back to the
		client.
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

void processMessage(int client_fd, char* message, int charCount);

int main(int argc, char** argv)
{
	// check if the user entered tht correct amount of arguments
	if(argc != 2)
	{
		printf("Wrong amount of arguments.\nUsage: ./executable port_number.\n");
		exit(0);
	}

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

	// listen for connections
	if(listen(server_fd, 5) < 0)
	{	
		printf("Error listening.\n");
		exit(0);
	}

	// connect the client
	cli_addr_size = sizeof(cli_addr);
	client_fd = accept(server_fd, (struct sockaddr*)&cli_addr, &cli_addr_size);
	if(client_fd < 0)
	{
		printf("Error accepting client\n");
		exit(0);
	}

	// receive input from the user
	while(1)
	{
		memset(buffer, 0, 1028); // clear the buffer
		byteCount = read(client_fd, buffer, 1027); // recieve message
		buffer[byteCount] = 0;

		printf("Recieved from client: %s\n", buffer);
	
		// if the user has quit
		if(strcmp(buffer, "quit") == 0)
		{
			write(client_fd, "Peace out!", strlen("Peace out!"));
			break;
		}
		else processMessage(client_fd, buffer, byteCount);	
	}

	// close the client socket
	if(close(client_fd) < 0)
	{
		printf("Error closing client socket.\n");
		exit(0);
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
name:		processMessage
parameters:	an int representing the client's socket descriptor, a char* representing a string, and an int representing the number of ASCII characters in the string
return:
description:	this function is used to count the number of words in the parameter message
		and converts any uppercase character to lowercase. The function then sends 
		the modified message back and send the character count and the word count.
*/
void processMessage(int client_fd, char* message, int charCount)
{
	char returnMessage[1028]; // a char array to send and recieve messages
	int wordCount = 0;
	char* tokens = strtok(message, " \n"); // the tokens of the client sent string
	int i, j = 0; // counters

	memset(returnMessage, 0, 1028); // clear the buffer

	// determine the number of words in the client's message
	while(tokens != NULL)
	{
		wordCount++;
		
		// determine if there are capital letters in the message
		for(i = 0; i < strlen(tokens); i++, j++)
		{
			// if there are convert them to lowercase
			if(tokens[i] >= 65 && tokens[i] <= 90) returnMessage[j] = tokens[i] + 32;
			else returnMessage[j] = tokens[i];
		}
		returnMessage[j] = ' '; // append a space
		j++;

		tokens = strtok(NULL, " \n"); // get the next token
	}
	returnMessage[j-1] = 0; // append a null character
	printf("Returning to the client: %s\n", returnMessage); 
	write(client_fd, returnMessage, strlen(returnMessage)); // return the message to the client

	// wait to see if user is ready to receive more information
	memset(returnMessage, 0, 1028);
	j = read(client_fd, returnMessage, 1028);
	returnMessage[j] = 0;

	// send the number of words and characters back to the client
	if(strcmp(returnMessage, "Ready") == 0)
	{
		memset(returnMessage, 0, 1028);
		sprintf(returnMessage, "Words: %d\nCharacters: %d\n", wordCount, charCount);
		write(client_fd, returnMessage, strlen(returnMessage));	
	}
	else printf("Error communicating with client.\n");
	
	return;
}
