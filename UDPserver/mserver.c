/*
Author:		Jesus Rodriguez
Date:		23 March 2018
Program Name:	mserver.c
Description:	This program is the server that uses a UDP connection to wait for a client to
		send a mathematical expression. If the expression is properly formatted and
		contains only valid characters, the expression is evaluated and the result is
		sent to the client. This process continues until the client chooses to quit 
		by sending the command "quit."
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>
#include <math.h>

struct node
{
	char operator;
	double operand;
	struct node* next;
};

struct stack
{
	struct node* head;
	struct node* tail;
};

struct node* newNode(char operator, double operand);
void push(struct stack* st, struct node* tempNode);
struct node* pop(struct stack* st);
void clearStack(struct stack* st);
void pushFromStack(struct stack* destination, struct stack* source);
int operatorPrecedence(char newOperator, char topOperator);
int postFixConversion(struct stack* postfixExp, char* infixExp);
double* evalPostfix(struct stack* postfixStack);
double* getOperand(char* infixExp, int startIndex, int* endIndex, int allowNegative);

int main(int argc, char** argv)
{
	// check the number of arguments
	if(argc != 2)
	{
		printf("Wrong amount of arguments.\nUsage: ./executable port_number\n");
		exit(0);
	}

	struct sockaddr_in svrAddr, cliAddr; // the client and server addresses
	int cliAddrSize = sizeof(cliAddr); // the client addess size
	int server_fd; // the socket descriptor for the server
	int port = atoi(argv[1]); // the port number 
	int bytesRead;
	char buffer[1024];
	double* result; // the result from a mathematical expression

	// set up the UDP connection
	server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(server_fd < 0)
	{
		exit(0);
	}

	// set the address of the socket
	memset((char*)&svrAddr, 0, sizeof(svrAddr));
	svrAddr.sin_family = AF_INET;
	svrAddr.sin_port = htons(port);
	svrAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(server_fd, (struct sockaddr*)&svrAddr, sizeof(svrAddr)) < 0)
	{
		printf("Error binding the socket\n");
		exit(0);
	}

	// receive input from a user
	while(1)
	{	
		struct stack postfixStack; // a stack that will hold a mathematical expression in postfix format
		postfixStack.head = postfixStack.tail = NULL;

		printf("Waiting for an expression...\n");

		// read a command from the user
		memset(buffer, 0, sizeof(buffer));
		bytesRead = recvfrom(server_fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&cliAddr, &cliAddrSize);
		buffer[bytesRead] = 0;
		printf("Evaluating: %s\n", buffer); 
	
		if(strcmp(buffer, "quit") == 0)
		{
			memset(buffer, 0, sizeof(buffer));
			sprintf(buffer, "Peace out!");
			sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr*)&cliAddr, cliAddrSize);
		}
		else
		{
			if(postFixConversion(&postfixStack, buffer)) // if the expression is valid
			{
				result = evalPostfix(&postfixStack); // determine the result
				
				if(result == NULL) // if the result pointer returns null
				{
					// send an error message to the user
					memset(buffer, 0, sizeof(buffer));
					sprintf(buffer, "Can't divide by zero. The universe may explode.");
					sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr*)&cliAddr, cliAddrSize);
				}
				else // send the result to the user
				{
					printf("Computation Result: %f\n", *result);

					// place the result in a buffer and send it to the user
					memset(buffer, 0, sizeof(buffer));
					sprintf(buffer, "%f", *result);
					sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr*)&cliAddr, cliAddrSize);
					free(result);
				}
			}
			else // if the expression is invalid
			{
				// send a message to the user saying that the expression was not valid
				memset(buffer, 0, sizeof(buffer));
				sprintf(buffer, "Invalid Expression");
				sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr*)&cliAddr, cliAddrSize);
			}
			clearStack(&postfixStack); // free allocated memory
		}
	}
	printf("Closing connection.\n");

	close(server_fd);
	return 0;
}
/*
Name:		newNode
Parameters:	a char representing an operator and a double representing an operand
Return:		a node* representing a new node
Description:	This function is used to create a node* by allocating memory with the parameters as
		the node's values
*/
struct node* newNode(char operator, double operand)
{
	struct node* temp = (struct node*)malloc(sizeof(struct node)); // allocate memory

	// initialize values
	temp->operator = operator;
	temp->operand = operand;
	temp->next = NULL;

	return temp;
}
/*
Name:		push
Parameters:	a stack* representing a stack and a node* representing a node
Return:		
Description:	This function is used to push the node parameter to the stack parameter
*/
void push(struct stack* st, struct node* tempNode)
{
	// push the node to the stack
	tempNode->next = st->head;
	st->head = tempNode;

	return;
}
/*
Name:		pop
Parameters:	a stack* representing a stack
Return:		a node* representing the top node on the stack parameter
Description:	This function serves to remove the top element from the stack and 
		return it
*/
struct node* pop(struct stack* st)
{
	struct node* temp = st->head; // get the top element

	if(temp != NULL) st->head = temp->next; // if the element is not null move the stack pointers

	return temp;
}
/*
Name:		clearStack
Parameters:	a stack* representing a stack
Return:		
Description:	This function is used to delete the allocated memory used by the nodes in a stack
*/
void clearStack(struct stack* st)
{
	struct node* iter1 = st->head;
	struct node* iter2 = iter1;

	// iterate and free pointers in the stack
	while(iter1 != NULL)
	{	
		iter2 = iter1->next;
		free(iter1);
		iter1 = iter2;
	}

	return;
}
/*
Name:		postFixConverseion
Parameters:	a stack* which is a stack that will hold an expression in postfix format and
		a char* representing a mathematical expression in infix format
Return:		an int representing whether the expression was valid
Description:	This function is used to convert an infix expression to postfix format
*/
int postFixConversion(struct stack* postfixExp, char* infixExp)
{
	int i, j; // looping variables
	struct stack operatorStack; // the stack to hold operators while changing to a postfix expression
	struct stack tempPostfix;
	char intToString[1024]; // a buffer to hold an integr
	double operand; // an operand in the infix expression
	int sawOperator = 1; // a boolean to determine if an operator was the last char processed
	operatorStack.head = operatorStack.tail = NULL;
	tempPostfix.head = tempPostfix.tail = NULL;
	double* temp; // a temporary pointer to hold operands

	// read each character in the infix expression 
	for(i = 0; i < strlen(infixExp); i++)
	{
		memset(intToString, 0, 1024); 
		temp = NULL;

		if(isdigit(infixExp[i]) || (infixExp[i] == '-' && sawOperator == 1)) // if the char is a number or could be a negative number
		{
			for(j = i+1; isdigit(infixExp[j]); j++); // determine how long the number is
			strncpy(intToString, infixExp + i, j-i); // put the number in a buffer
			operand = atof(intToString); // turn the number string into a double
			push(&tempPostfix, newNode(0, operand)); // push the operand to the postfix expression
			
			sawOperator = 0; // update value

			i = j - 1; // move to the next value in the expression
		}
		else if(infixExp[i] == 'e') // if the value is Euler's number
		{
			push(&tempPostfix, newNode(0, exp(1))); // push the value to the stack
			sawOperator = 0;
		}
		else if(infixExp[i] == 'l') // if the value is the logarithmic expression
		{
			temp = getOperand(infixExp, i, &j, 0); // get the operand within parentheses
			if(temp == NULL) // if the operand is not valid
			{
				// clear the stacks and return zero
				clearStack(&operatorStack);
				clearStack(&tempPostfix);
				return 0;
			}
			push(&tempPostfix, newNode(0, log(*temp))); // evaluate the log of the operand and push
	
			// update values and free the malloced pointer
			sawOperator = 0;
			i = j;
			free(temp);
		}
		else if(infixExp[i] == 'c') // if the value is the cosine function
		{
			temp = getOperand(infixExp, i, &j, 1); //get the operand
			if(temp == NULL) // if the operand is not valid
			{
				// clear the stacks and return 0
				clearStack(&operatorStack);
				clearStack(&tempPostfix);
				return 0;
			}
			push(&tempPostfix, newNode(0, cos(*temp))); // evaluate cosine and push
			i = j;
			sawOperator = 0;
			free(temp);
		}
		else if(infixExp[i] == 's') // determine if the square root or sine
		{	
			if(infixExp[i+1] == 'i') // if the expression contains a sine
			{
				temp = getOperand(infixExp, i, &j, 1); // get the operand
				if(temp == NULL) // if the operand is invalid
				{
					// clear the datack and return 0
					clearStack(&operatorStack);
					clearStack(&tempPostfix);
					return 0;
				}
	
				// evaluate sine and push the value
				push(&tempPostfix, newNode(0, sin(*temp)));
				free(temp);
			}
			else if(infixExp[i+1] == 'q') // if the expression contains a square root
			{
				temp = getOperand(infixExp, i, &j, 0); // get the operand
				if(temp == NULL) // if the operand is not valid
				{
					// clear the stacks and return 0
					clearStack(&operatorStack);
					clearStack(&tempPostfix);
					return 0;
				}
				// evaluate the square root and push the result
				push(&tempPostfix, newNode(0, sqrt(*temp)));
				free(temp);
			}
			else // could not determine what the expression is so return error
			{
				clearStack(&operatorStack);
				clearStack(&tempPostfix);
				return 0;
			}
			sawOperator = 0;
			i = j;
		}
		else if(infixExp[i] == '(') // push a left parentheses to the operator stack
		{
			push(&operatorStack, newNode(infixExp[i], 0));
			sawOperator = 1;
		}
		else if(infixExp[i] == ')') // remove operators from the operator stack
		{
			struct node* iterator = operatorStack.head; // an iterator to move through the stack
			if(iterator == NULL || sawOperator == 1) // if the top of the stack is null
			{
				// there was no left parenthesis, so return error
				clearStack(&operatorStack);
				clearStack(&tempPostfix);
				return 0;
			}

			while(iterator->operator != '(') // while the top of the operator stack is not '('
			{
				pushFromStack(&tempPostfix, &operatorStack); // push values from the operator stack to the postfix stack
				iterator = operatorStack.head; // get the next operator

				if(iterator == NULL) // if the iterator is null, there was no matching '(' to ')'
				{
					// clear and return error
					clearStack(&operatorStack);
					clearStack(&tempPostfix);
					return 0;
				}
			}

			//remove the left parenthesis from the stack
			iterator = pop(&operatorStack);
			free(iterator);
			sawOperator = 0;
		}
		else if(infixExp[i] != ' ') // if the value in the array is an operand +, /, *, -, ^
		{
			sawOperator = 1;
			if(operatorStack.head == NULL || operatorStack.head->operator == '(') // if the operator stack is empty or the top is a '('
				push(&operatorStack, newNode(infixExp[i], 0));
			else if(operatorPrecedence(infixExp[i], operatorStack.head->operator)) // check the precedence of the operators
				push(&operatorStack, newNode(infixExp[i], 0)); // push if the operator has a higher precedence
			else
			{
				// take the top element from the operator stack and push it to the postfix stack
				pushFromStack(&tempPostfix, &operatorStack);
				push(&operatorStack, newNode(infixExp[i], 0)); // push the new operator to the temp stack
			}
		}
	} // end for loop

	if(sawOperator == 1) // if the last character in the infix expression is an operand return an error
	{
		clearStack(&operatorStack);
		clearStack(&tempPostfix);
		return 0;

	}

	struct node* iter = operatorStack.head; // an iterator for the operator stack

	while(iter != NULL) // while there are operators in the operator stack
	{ 
		if(iter->operator == '(') // if the stack has an unmatched parentheses
		{
			// return an error
			clearStack(&tempPostfix);
			clearStack(&operatorStack);
			return 0;
		}
		pushFromStack(&tempPostfix, &operatorStack); // push the operators to the postfix stack
		iter = operatorStack.head; // get the next operator
	}

	clearStack(&operatorStack);

	// push from the temp postfix stack to the real postfix stack
	iter = pop(&tempPostfix);
	while(iter != NULL)
	{
		push(postfixExp, iter);
		iter = pop(&tempPostfix);
	}

	return 1;
}
/*
Name:		pushFromStack
Parameters:	a stack* representing the destination stack and a stack* representing the source stack
Return:
Description:	This function takes the top element from the source stack to get it's values. Then the
		values are used to create a new node for the destination stack. The top node in the 
		source is freed at the end
*/
void pushFromStack(struct stack* destination, struct stack* source)
{
	struct node* temp = pop(source);
	struct node* nodeCopy = newNode(temp->operator, temp->operand); // copy the node from the source

	push(destination, nodeCopy); // push the copy to the destination

	free(temp); // free the old pointer

	return;
}
/*
Name:		operatorPrecedence
Parameters:	a char representing an new operator that will be added to a stack and char
		representing the operator at the top of the operator stack
Return:		an int represting whether an operator has a higher precedence thatn another.
Description:	This function returns the precedence between two operators. The function returns a 1
		if the top operator has a lower precedence than the new operator. Otherwise, the
		function returns a 0.
*/
int operatorPrecedence(char newOperator, char topOperator)
{
	if(newOperator == '(') return 1;
	else if((newOperator == '+' || newOperator == '-') && (topOperator == '*' || topOperator == '/')) return 0;
	else if((newOperator == '+' || newOperator == '-') && (topOperator == '+' || topOperator == '-')) return 0;
	else if((newOperator == '/' || newOperator == '*') && (topOperator == '+' || topOperator == '-')) return 1;
	else if((newOperator == '/' || newOperator == '*') && (topOperator == '/' || topOperator == '*')) return 0;
	else if(newOperator == '^') return 1;
	else if(topOperator == '^') return 0;
}
/*
Name:		evalPostfix
Parameters:	a stack* representing a mathematical expression in postfix format
Return:		a double* representing the result from evaluating the postfix expression
Description:	this function takes a postfix expression and evaluates the operands using
		the operators in the stack.
*/
double* evalPostfix(struct stack* postfixStack)
{
	struct stack resultStack; // a stack to temporary hold operands
	resultStack.head = resultStack.tail = NULL;
	double left = 0; // the left operand in the postfix stack
	double right = 0; // the right operands in the postfix stack
	struct node* temp; 

	while(postfixStack->head != NULL) // while there are operators or operands in the postfix stack
	{
		// if the head of the postfix expression is an operator
		if(postfixStack->head->operator != 0)
		{
			// get the left and right operands from the evaluation stack
			temp = pop(&resultStack);
			right = temp->operand;
			free(temp);
			temp = pop(&resultStack);
			left = temp->operand;
			free(temp);

			// evaluate the appropriate operation
			if(postfixStack->head->operator == '+')	push(&resultStack, newNode(0, left+right));
			else if(postfixStack->head->operator == '-') push(&resultStack, newNode(0, left-right));
			else if(postfixStack->head->operator == '*') push(&resultStack, newNode(0, left*right));
			else if(postfixStack->head->operator == '^') push(&resultStack, newNode(0, pow(left,right)));
			else if(postfixStack->head->operator == '/')
			{
				if(right == 0) return NULL; // check to see if the expression divides by zero
				push(&resultStack, newNode(0, left/right));
			}
			free(pop(postfixStack)); // delete the top element from the stack
		}
		else // the top of the stack is a number
		{
			// push the number to the evaluation stack
			temp = pop(postfixStack);
			push(&resultStack, newNode(0, temp->operand));
			free(temp);
		}
	}

	// remove the final result from the stack and return it
	temp = pop(&resultStack);
	double* result = (double*)malloc(sizeof(double));
	*result = temp->operand;

	// delete allocated pointers
	clearStack(&resultStack);
	free(temp);

	return result;
}
/*
Name:		getOperand
Parameters:	a char* representing an infix expression, an int representing the start index in
		the char* variable, an int representing an end index in the char*, and an int
		used to determine if negative numbers are allowed.
Return:		a double* which points to a location with the operand gotten from the char* parameter
Description:	This function is used to retrieve the operands from the equations cos, sin, log, and
		sqrt. As it scans if there are invalid characters in the equation, the function returns
		null to signify an error.
*/
double* getOperand(char* infixExp, int startIndex, int* endIndex, int allowNegative)
{
	int i, j; // looping variable
	char buffer[1024]; // a buffer to turn an string into a float
	double* temp = (double*)malloc(sizeof(double));
	int firstValidChar = 0; // a boolean representing whether the first valid char has been reached

	for(i = startIndex; infixExp[i] != '('; i++); // find the index where the number starts

	for(j = i+1; infixExp[j] != ')'; j++) // determine how long the number is
	{
		if(firstValidChar == 0) // if the first valid character hasn't been encountered
		{
			if(isdigit(infixExp[j])) firstValidChar = 1; // if the first charater encountered is a number
			else if(infixExp[j] == '-' && allowNegative == 0) return NULL; // if negative numbers are not allowed
			else if(infixExp[j] == '-' && allowNegative == 1) firstValidChar = 1; // if negative numbers are allowed
			else if(infixExp[j] == ' '); // if the character is a space, the first character hasn't been located
			else return NULL; // the expression contains invalid characters
		}
		else // already encountered one or more valid chars
		{
			// if the character encountered is neither a numerical value or a space return an error
			if(!isdigit(infixExp[j]) && infixExp[j] != ' ') return NULL;
		}
	}

	if(firstValidChar == 0) return NULL; // there were no valid characters in the expression, return error

	memset(buffer, 0, sizeof(buffer)); // clear the buffer before using it
	strncpy(buffer, infixExp + (i+1), j-i-1); // put the number in a buffer
	*temp = atof(buffer); // turn the number string into a double
	*endIndex = j; // update index j

	return temp; // return the pointer
}
