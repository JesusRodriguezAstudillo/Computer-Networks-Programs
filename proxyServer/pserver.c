/*
Author:		Jesus Rodriguez
Date:		2 March 2017
Program Name:	pserver.c
Description:	This program is used to act as a proxy server to a single client. When a client connects, the
		server waits for a URL message. The URL is then checked to see if it is in the allow list. If
		the URL is allowed, the program checks the cache and processes the URL accordingly before
		sending the contents of the URL page to the user.
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

void processURL(int client_fd, char* URL, int numCached);
void getPage(char* hostname, char* path, char* URL, int numCached, int client_fd); 
void checkCacheList(int client_fd, char* URL);
void addToCacheList(char* URL, char* cachedPage, int numCached);
void sendWebpage(char* fileName, int client_fd);
int checkAllowList(char* URL);

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

	memset(buffer, 0, 1028); // clear the buffer
	byteCount = read(client_fd, buffer, 1027); // recieve URL
	buffer[byteCount] = 0;

	printf("Recieved from client: %s\n", buffer);

	if(checkAllowList(buffer)) // if the url is allowed
	{
		printf("The URL is on the allow list.\n");	
		// call the check cache after this
		checkCacheList(client_fd, buffer);
	}
	else
	{
		// the URL was not on the allowed list so send the a message to the user
		printf("The URl: %s was not on the allowed list.\n", buffer);
		write(client_fd, "The URL: ", strlen("The URL: "));
		write(client_fd, buffer, strlen(buffer));
		write(client_fd, "was not on the allowed list.\n", strlen("was not on the allowed list.\n"));
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
name:		processURL
parameters:	an int representing a socket descriptor, a char* representing a URL, and an int	
		representing the number of cached websites
return:	
description:	This function is used to take a URL and parse it into it's hostname and path.
		Both are then passed to a function that uses those variables.
*/
void processURL(int client_fd, char* URL, int numCached)
{
	// check if the URL path ends with a '/'
	int endsWithPath = 0;
	if(URL[strlen(URL) - 1] == '/') endsWithPath = 1; 

	// create a copy of the URL to tokenize
	char* URLcopy = (char*)malloc(strlen(URL)+1);
	strcpy(URLcopy, URL);

	// get the tokens from the copy
	char* tokens = strtok(URLcopy, "/\n");
	tokens = strtok(NULL, "/\n");

	// get the hostname from the URL
	char hostname[1028];
	memset(hostname, 0, sizeof(hostname));
	strcpy(hostname, tokens);

	// rebuild the path by appending the tokens together with /'s
	tokens = strtok(NULL, " /\n");
	char path[1028];
	memset(path, 0, sizeof(path));
	while(tokens != NULL)
	{
		strcat(path, "/");
		strcat(path, tokens);
		tokens = strtok(NULL, " /\n");
	}
	if(endsWithPath == 1) strcat(path, "/");

	getPage(hostname, path, URL, numCached, client_fd); // use the hostname and path to get the webpage

	free(URLcopy); // free the URL copy

	return;
}
/*
Name:		getPage
Parameters:	a char* representing the hostname, a char* representing the path to a specific webpage a char*
		represting the complete URL, an in representing the number of cached websites, and an int 
		representing a socket descriptor
Return:	
Description:	This function is used to connect to a websites server and make a get request. If the request
		succeeds, the webpage is added to the cache and the webpage contents are saved to a file
		using a time stamp as the name.
*/
void getPage(char* hostname, char* path, char* URL, int numCached, int client_fd)
{

	int svrCli_fd; // the server client descriptor
	struct sockaddr_in svrCli_addr; // the server client address
	int portnum = 80; // http port
	struct hostent *hostInfo = gethostbyname(hostname); // determine the ip of the host
	int bytesRead; // a int to check the number of bytes read
	char buffer[1024]; // a buffer to read the webpage
	char headerCode[13]; // a buffer to hold the header response
	char headerCopy[13]; // a buffer to hold a copy of the header so that it can be tokenized
	time_t temp;

	if(hostInfo == NULL)
	{
		printf("Could not determine the IP address of %s\n", hostname);
		write(client_fd, "Could not determine the IP address of ", strlen("Could not determine the IP address of "));
		write(client_fd, hostname, strlen(hostname));
		write(client_fd, "\n", strlen("\n"));
		exit(0);
	}

	// if the webpage holds no path, add a /
	if(strlen(path) == 0) strcat(path, "/");

	// build the GET request
	char request[1024];// * 10];
	memset(request, 0, sizeof(request));
	sprintf(request, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", path, hostname);
	
	// get the socket descriptor
	svrCli_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(svrCli_fd < 0)
	{
		printf("Error creating client socket.\n");
		exit(0);
	}

	// set up the address of the server client
	bzero((char*)&svrCli_addr, sizeof(svrCli_addr));
	svrCli_addr.sin_family = AF_INET;
	svrCli_addr.sin_addr = *((struct in_addr*)hostInfo->h_addr);
	svrCli_addr.sin_port = htons(portnum);

	// connect to the webpage server
	if(connect(svrCli_fd, (struct sockaddr*)&svrCli_addr, sizeof(svrCli_addr)) < 0)
	{
		printf("Error connecting to website");
		exit(0);
	}

	// make the GET request
	time(&temp);
	write(svrCli_fd, request, strlen(request));

	// get the first line from the resonse header
	bytesRead = read(svrCli_fd, headerCode, 12);
	headerCode[bytesRead] = 0;
	char* httpCode = strstr(headerCode, "200");

	// if the page did not contain a 200 code
	if(httpCode == NULL)
	{
		// send the header code to the user
		write(client_fd, headerCode, strlen(headerCode));
		while(1) // send the rest of the page
		{
			memset(buffer, 0, sizeof(buffer));
			bytesRead = read(svrCli_fd, buffer, 1023);
			buffer[bytesRead] = 0;
			if(bytesRead == 0) break;
			write(client_fd, buffer, strlen(buffer));
		}
	}
	else // got an ok response from the webpage server
	{
	
		char fileName[15]; // a char array that will hold the filename
		fileName[14] = 0; // set a null value at the end

		// determine the time to set it as the file name of the webpage
		struct tm* now;
		now = localtime(&temp);
		sprintf(fileName, "%d%.2d%.2d%.2d%.2d%.2d", now->tm_year + 1900,
		 now-> tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);

		// add the name of the page to the cache
		addToCacheList(URL, fileName, numCached);

		// create a file to save the webpage in
		FILE* cachedPage = fopen(fileName, "w");
		fwrite(headerCode, sizeof(char), strlen(headerCode), cachedPage);

		printf("Getting webpage...\n");
		// loop until then entire page has been saved into a file
		while(bytesRead > 0)
		{
			memset(buffer, 0, sizeof(buffer));
			bytesRead = read(svrCli_fd, buffer, 1023);
			buffer[bytesRead] = 0;
			if(bytesRead == 0) break;
			fwrite(buffer, sizeof(char), strlen(buffer), cachedPage);
		}
		fclose(cachedPage);
		printf("Finished getting page.\n");

		// send the cached page to the user
		sendWebpage(fileName, client_fd);
	}

	return;
}
/*
Name:		checkCacheList
Parameters:	an int representing a socket descriptor and a char* representing a URL string
Return:
Description:	This function is used to determine if a url is already cached. If the URL is not
		cached, the URL is passed to the processURL function otherwise the file containing
		the webpage is opened and sent to the user.
*/
void checkCacheList(int client_fd, char* URL)
{
	int counter = 0; // a counter for the number of files
	FILE* cacheList = fopen("list.txt", "r"); // open the list file

	// if the file opened correctly
	if(cacheList != NULL)
	{
		// determine the size of the file
		fseek(cacheList, 0, SEEK_END);
		int numOfBytes = ftell(cacheList);

		// if the file is empty, get the page
		if(numOfBytes == 0) processURL(client_fd, URL, 0);
		else // check the file contents to see if the URL is logged
		{	
			char* fileBuffer = (char*)malloc(numOfBytes+1);	// create a buffer for the file contents
			rewind(cacheList);
			int URLMatch = 0;
			char* subURL; // a char* that will point to the line with the URL if it exists
			int lineCounter = 0; // an int to determine how many URLs are in the list
	
			// get the file contents
			fread(fileBuffer, sizeof(char), numOfBytes+1, cacheList);

			// break up the file into tokens
			char* tokens = strtok(fileBuffer, " \n");
			

			while(tokens != NULL)
			{
				lineCounter++; // count the number of lines
		
				// check if the URL is a match to one already in the list
				if(strcmp(URL, tokens) == 0)
				{	
					URLMatch = 1;
					break;
				}
				tokens = strtok(NULL, " \n"); // jump over the file name
				tokens = strtok(NULL, " \n"); // get the next line
			}
	
			if(URLMatch == 0) processURL(client_fd, URL, lineCounter); // if URL is not found get the page
			else // the URL is already cached
			{
				tokens = strtok(NULL, " \n"); // get the file name
				sendWebpage(tokens, client_fd); // call the function that sends the page to the user
			}

			free(fileBuffer);
		}
	}
	fclose(cacheList);

	return;
}
/*
Name:		addToCacheList
Parameters:	a char* representing the URL, a char* representing the filename, and an int representing
		the number of cached webpages
Return:
Description:	this function is used to add a URL and the file name to the cache list. If the file has less
		than 5 cached pages the URL is simply appended. Otherwise, the first file is removed 
		before adding the new URL.
*/
void addToCacheList(char* URL, char* cachedPage, int numCached)
{
	// if the number of cached websites is less than 5
	if(numCached < 5)
	{
		FILE* cacheListFile = fopen("list.txt", "a");
		
		// append the new website to the list
		if(cacheListFile != NULL)
		{
			fprintf(cacheListFile, "%s %s\n", URL, cachedPage);
			fclose(cacheListFile);
		}
		else printf("Error opening the list file.\n");
	}
	else
	{		
		FILE* cacheListFile = fopen("list.txt", "r"); // open the list file

		// if the file opened correctly
		if(cacheListFile != NULL)
		{
			// determine the size of the file
			fseek(cacheListFile, 0, SEEK_END);
			int numOfBytes = ftell(cacheListFile);

			char* fileBuffer = (char*)malloc(numOfBytes+1);	// create a buffer for the file contents
			rewind(cacheListFile);

			// get the file contents
			fread(fileBuffer, sizeof(char), numOfBytes+1, cacheListFile);

			freopen("list.txt", "w", cacheListFile); // reopen the file to modify the cache list

			char* tokens = strtok(fileBuffer, "\n"); // remove the first line from the file
	
			// get the file name from the token
			int i; // a looping variable
			for(i = 0; tokens[i] != ' '; i++); // determine the location of the space char
			char fileName[15]; // an array for the filename
			fileName[14] = 0;
			strncpy(fileName, tokens + i + 1, strlen(tokens) - i); // copy the filename into an array
			remove(fileName); // delete the file

			// place the remaining urls and filesnames back into the file
			tokens = strtok(NULL, "\n");
			while(tokens != NULL)
			{
				fprintf(cacheListFile, "%s\n", tokens);
				tokens = strtok(NULL, "\n");
			}
			fprintf(cacheListFile, "%s %s\n", URL, cachedPage);
		}
		fclose(cacheListFile);
	}

	return;
}
/*
Name:		sendWebpage
Parameters:	a char* representing a file name and an int representing a socket descriptor
Return:
Description:	This funtion uses the file name to open a file and then uses the socket 
		descriptor to send the contents of the file.
*/
void sendWebpage(char* fileName, int client_fd)
{
	// open the file in reading mode
	FILE* cachedPage = fopen(fileName, "r");
	char buffer[1024]; // a buffer to hold the cached file contents
	int bytesRead; // the number of bytes read
				
	// get the contents of the file
	while(1)
	{
		memset(buffer, 0, sizeof(buffer));
		bytesRead = fread(buffer, sizeof(char), sizeof(buffer)- 1, cachedPage);
		buffer[bytesRead] = 0;

		if(bytesRead == 0) break; // if there is nothing left in the file end looping

		write(client_fd, buffer, strlen(buffer)); // send contents to the client
	}

	return;
}
/*
Name:		checkAllowList
Parameters:	a char* that represnts a URl
Return:		an int representing whether the URL is on the allowed list
Description:	This function is used to check if the URL paramaters is allowed according to one of
		allow list URLs
*/
int checkAllowList(char* URL)
{
	int allowed = 0; // an int to determine allowence of a website
	char* subLevelDomain = NULL;
	char* domain;
	char* topLevelDomain;

	// open the file
	FILE* allowListFile = fopen("allowlist.txt", "r");

	// make a copy of the URL without the path to compare later
	char URLcompare[1024];
	memset(URLcompare, 0, 1024);
	strcat(URLcompare, "http://");

	// copy the URL to tokenize
	char* URLcopy = (char*)malloc(strlen(URL) + 1);
	strcpy(URLcopy, URL);

	char* tokens = strtok(URLcopy, "/\n");
	tokens = strtok(NULL, "/\n"); // get the string between http:// and /path
	strcat(URLcompare, tokens); // add the token back to ULRcompare

	// copy the string between http:// and /path to tokenize
	char* hostCopy = (char*)malloc(strlen(tokens) + 1);
	strcpy(hostCopy, tokens);
	
	int i, j; // looping variables

	// count the number of dots in the URL
	for(i = 0, j = 0; hostCopy[i] != 0; i++) if(hostCopy[i] == '.') j++;

	if(j == 1) // if the url only has a domain and TLD
	{
		char* domainTokens = strtok(hostCopy, ".");

		// get the domain
		domain = (char*)malloc(strlen(domainTokens) + 1);
		strcpy(domain, domainTokens);
		domainTokens = strtok(NULL, ".");

		// get the top level domain
		topLevelDomain = (char*)malloc(strlen(domainTokens) + 1);
		strcpy(topLevelDomain, domainTokens);
	}
	else if(j == 2) // else if the URL has sub, main, and top level domain
	{
		char* domainTokens = strtok(hostCopy, ".");

		// get the sub domain
		subLevelDomain = (char*)malloc(strlen(domainTokens) + 1);
		strcpy(subLevelDomain, domainTokens);
		domainTokens = strtok(NULL, ".");

		// get the main domain
		domain = (char*)malloc(strlen(domainTokens) + 1);
		strcpy(domain, domainTokens);
		domainTokens = strtok(NULL, ".");

		// get the top level domain
		topLevelDomain = (char*)malloc(strlen(domainTokens) + 1);
		strcpy(topLevelDomain, domainTokens);
	}
	else printf("The URL does not have the appropriate format.\n"); // the url has too many or too few domains

	// determine the size of the file
	fseek(allowListFile, 0, SEEK_END);
	int byteCount = ftell(allowListFile);

	// get the file contents
	char* allowListContents = (char*)malloc(byteCount + 1);
	rewind(allowListFile);
	fread(allowListContents, sizeof(char), byteCount + 1, allowListFile);

	char* save1 = allowListContents; // a save state to use with strtok_r
	char* allowListTokens = strtok_r(allowListContents, "\n", &save1); // tokenize the string in the file

	// loop to see if the URL is allowed
	while(allowListTokens != NULL)
	{
		int k; // looping variable

		// check the number of '.'s in the token
		for(i = 0,k = 0; allowListTokens[i] != 0; i++) if(allowListTokens[i] == '.') k++;

		char* temp = strstr(allowListTokens, domain); // used to check if the URL entered has the same amount of domains

		if(strcmp(allowListTokens, "http://*.*.*") == 0) // if the allow list allows any website
		{
			allowed = 1; // the website is allowed
			break; // done looking through the tokens
		}
		else if(strcmp(URLcompare, allowListTokens) == 0) // if URL is the same as that in the file
		{
			allowed = 1; // the website is allowed
			break; // done looking through the tokens
		}
		else if(k != j);// else if the url has a subdomain but the allowlist url doesn't
		else if(temp != NULL) // else if the domain is part of the allow list
		{
			// need to check the if the sub and top level domains match
			// make a copy to tokenize URL in the list
			char* tokenCopy = (char*)malloc(strlen(allowListTokens) + 1);
			strcpy(tokenCopy, allowListTokens);
			char* urlTokens = strtok(tokenCopy, "/.\n"); // skip over the http: part of the URL
			urlTokens = strtok(NULL, "/.\n"); // get the domain or subdomain
			
			if(strcmp(urlTokens, domain) == 0) // if the first token is the domain, only check the top level domain allowance
			{
				urlTokens = strtok(NULL, "."); // get the top level domain

				// check if the TLD is the same or if it's a wildcard
				if(strcmp(urlTokens, "*") == 0 || strcmp(urlTokens, topLevelDomain) == 0)
				{
					allowed = 1; // the website is allowed 
					break; // done searching the list
				}
				free(tokenCopy);
			}
			else
			{
				// need to check if the sub domain matches or if is a wild card
				if(strcmp(urlTokens, "*") == 0 || strcmp(urlTokens, subLevelDomain) == 0)
				{
					urlTokens = strtok(NULL, "."); // skip over the domain
					urlTokens = strtok(NULL, "."); // get the top level domain
					// check the TLD validity
					if(strcmp(urlTokens, "*") == 0 || strcmp(urlTokens, topLevelDomain) == 0)
					{
						allowed = 1; // the website is allowed
						break; // done looking through allowlist
					}
				}
				free(tokenCopy); // free the token copy
			}
		}
		allowListTokens = strtok_r(NULL, "\n", &save1); // get the next token
	} 

	// free all malloced variables
	if(j == 1)
	{
		free(domain);
		free(topLevelDomain);
	}
	else if(j == 2)
	{
		free(subLevelDomain);
		free(domain);
		free(topLevelDomain);
	}
	free(hostCopy);
	free(URLcopy);

	// close the file
	fclose(allowListFile);

	return allowed; // return the variable that determines if the wesite is allowed
}
