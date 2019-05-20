/* Compilation: gcc -o UDPserver UDPserver.c
   Execution  : ./UDPserver <port_number> [eg. port_number = 5000, where port_number is the UDP server port number]
*/

#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
 
#define BUFLEN 512  //Max length of buffer
//#define filestring 1000
void die(char *s)
{
    perror(s);
    exit(1);
}

void clientIP(char* cIP);

int main(int argc, char *argv[])
{

    if (argc < 2) //error checking for number of arguments
    {
	printf("Missing port num\n");
	exit(0);
    }
    struct sockaddr_in si_me, si_other;
     
    int s, i, slen = sizeof(si_other) , recv_len, portno;
    char buf[BUFLEN], message[1024];
    int addCount = 0; //address counter for text file
    int count = 0;
	int lifetime = 3600;

    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
     
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
    portno = atoi(argv[1]); //The port on which to listen for incoming data
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(portno);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }
    
    system("clear");
    printf("...This is UDP server...\n\n");

   
    //keep listening for data
    while(1)
    {
		FILE *file;
		char line[1024]; //getting file lines
		//char c;
		int ch;
		int addrcount = 0;
		int cCount = 0;
		char addresstore[10][512]; //storing the number of addresses
		char *ip;
		//file opening---------------------------------------------------------------------
	
		file = fopen("IPaddress.txt", "r"); 	
		if (file == NULL)
		{
			printf("File did not open\n");
			return 0;
		}
		printf("Getting file addresses...\n");
		/*while (fgets(line, sizeof(line), file) != NULL)
		{

			addrcount++; //count number of addresses to new line
		}*/
		
		int i;
		
		for (i = 0; i < 10; i++)
		{
			bzero(ip, 512);
			fgets(ip, 512, file);
			strcpy(addresstore[i], ip);
		}
		//printf("%s", ip);
		fclose(file);
		/*printf("%d addresses loaded, \n", addrcount);*/
		/*while (fscanf(file, "%s", addresstore[i]) == 1)
		{ 
			i++;
		}*/
		//waiting for client----------------------------------------------------------------------
        printf("Waiting for client's message...\n\n");
        fflush(stdout);

		/*file = fopen("IPaddress.txt", "r"); //file opening
		if (file == NULL)
		{
			printf("File did not open\n");
			return 0;
		}
		/if (cCount == addrcount)//if there are no more addresses in the file, send message
		{
			printf("No more addresses...terminating server\n");
			close(s);
			exit(0);
		}*/
        //fclose(file);
		
        //Receiving data from client-----------------------------------------------------------------------------------
		printf("DHCP discover...\n");

        if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            die("recvfrom()");
        }

        //print details of the client/peer and the data received
        printf("Received packet from %s, port number:%d\n\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
        printf("Client has sent: %s\n", buf);

		/*file = fopen("IPaddress.txt", "w");
		if (file == NULL)
		{
			printf("File did not open\n");
			return 0;
		}
		while (for i = 
		{
			ip //count number of addresses to new line
		}
		fclose(file);
		printf("%d addresses loaded, \n", addrcount);*/

		//file.open()
        //Sending reply to the client----------------------------------------------------------------------------------------

	/** cIP is an array that holds the IP address from the IPaddress file **/
	char cIP[128];
	memset(cIP, 0, sizeof(cIP));
	clientIP(cIP);
		bzero(message, 1024);
		printf("Enter server's message:");
		gets(message);
        if (sendto(s, message, strlen(message), 0, (struct sockaddr*) &si_other, slen) == -1)
        {
            die("sendto()");
        }
    }
	//fclose(file);
    close(s);
    return 0;
}
void clientIP(char* cIP)
{
	FILE* file;

	// open the file and copy the contents into a string
	file = fopen("IPaddress.txt", "r");
	fseek(file, 0, SEEK_END);
	int bytes = ftell(file);
	char* IPs = (char*)malloc(bytes+1);
	rewind(file);
	fread(IPs, sizeof(char), bytes + 1, file);
	fclose(file);

	// if the file is empty, no addresses are available
	if(bytes == 0) strcpy(cIP, "There are no available IP address");
	else // get the first address from the file
	{
		// tokenize the file by newlines
		char* tokens = strtok(IPs, "\n");

		// copy the first address to the cIP
		printf("giving: %s\n", tokens);
		strcpy(cIP, tokens);

		// get the remaining tokens and put the remaining addresses back in the file
		file = fopen("IPaddress.txt", "w");
		char buffer[128];
		tokens = strtok(NULL, "\n");	

		while(tokens != NULL)
		{
			memset(buffer, 0, 128);
			strcpy(buffer, tokens);
			buffer[strlen(tokens)] = '\n';
			fwrite(buffer, sizeof(char), strlen(buffer), file);
			tokens = strtok(NULL, "\n");
		}
	}

	return;
}
