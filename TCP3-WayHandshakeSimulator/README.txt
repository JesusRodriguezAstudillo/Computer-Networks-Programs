In order to compile the server program use the line

	gcc TCPserver.c -o TCPserver

and run with the line 

	./TCPserver <port_number>

In order to compile the client program use the line

	gcc TCPclient.c -o TCPclient

and run with the line 

	./TCPclient <port_number>

This ZIP file includes a makefile used to create the TCPserver executable. To avoid making
a duplicate and execution errors for the makefile, if you would like to use the makefile for
the client program change the code inside the Makefile to:

TCPclient: TCPclient.c
	gcc -o TCPclient TCPclient.c
	
In order to test the program, make sure to run the server before running the client program.
Once the client program launches, the client will send a SYN packet, wait for an ACK, and
finally send an ACK to the server. Next, the client will send a FIN packet, the server will
respond with an ACK. The server will send a FIN packet and the client will send an ACK.
The packets received and the client packets generatebefore sending will be printed to the
console and written to the file "client.out". The server will follow the same process but
will place the packet contents to the file "server.out"
The client will close after finishing the sequence so it must be run again to simulate the
sequence again. The server will remain open and wait for another connection so it must be 
shutdown with ctrl^c.


