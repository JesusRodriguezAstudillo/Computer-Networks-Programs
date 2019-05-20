In order to compile the server program use the line
	gcc mserver.c -lm -o mserver

And execute the program with
	./mserver <portnumber>
	
In order to compile the client program use the line
	gcc mclient.c -o mclient
	
and execute the program with 
	./mclient <portnumber>
	
where the portnumber arguments are the same

The client program may be executed before the server program, but the server must be
running before sending the expression. Otherwise, an error will occur. In order to learn
how the program works, please look at the comments in the code and the header comments
for function.

In order to test the program, try cases such as:

(15  * 9) / 78   - -47
or
cos( -2  )*7+sin(5)
or
(e^3)+log(10)-sqrt(64)

Expression such as;
sqrt(-1), sqrt(2-1), log(-1), log(2-1), cos(1+2), sin(1+2), and division by 0 will
return errors.

In order to quit, the client must type "quit" when prompted for an expression by the
program. The sever will remain open so that another user may connect to the server if
the port number is the same. In order to shutdown the server, use the command:
ctrl C