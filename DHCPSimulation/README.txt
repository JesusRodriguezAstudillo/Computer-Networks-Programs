In order to compile the server program use the line:

	gcc DHCPserver.c -o DHCPserver
	
and comile the program with

	./server [portnumber]
	
In order to compile the client program use the line:

	gcc DHCPclient.c -o DHCPclient

and compile the program with
	
	./client [portnumber]
	
In order for the programs to communicate, the port numbers must be the same.

A makefile is provided to create the executables but the server. The client makefile is called
clientMakefile but must have the client part of the file name in order to work. So remove the
client part from the name.

After compiling, the server will wait for a gateway input and a gateway mask input.
The input format expected by the program for both prompts is "123.145.25.35" without the quotes.

After compiling the client program, it will simply send and receive the packet sequence. If an 
address is available, after receiving an ack from the server, the client will go into an infinite
loop. If an address is not available, the client program will end.

Ctrl^c must be used to end the server program since it runs infinitely. The client program can be
terminated by pressing enter when prompted by the program to end the infinite loop.