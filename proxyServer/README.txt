In order to compile the server program use the line
	gcc pserver.c -o pserver

And execute the program with
	./pserver <portnumber>
	
In order to compile the client program use the line
	gcc pclient.c -o pclient
	
and execute the program with 
	./pclient <portnumber>
	
where the portnumber arguments are the same

When run, the client program will wait until the user enters a URL.
The server will process the URL and return some error message or the
webpage of the URL. Depending on the size of the webpage, the server
may take some time retrieving all the contents which will cause the
server to pause. However, the server will display the message:

"Finished getting page."

after all the contents have been retrieved. There were known cases
where some websites seemed to cause a permanent hangup.

The proxy server and client programs will close after the proxy server
completes the request so they must be recompiled to test the programs
further.