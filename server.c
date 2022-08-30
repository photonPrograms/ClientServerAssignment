/* Yash Gupta
 * 2019A7PS1138P
 *
 * following features have been implemented:
 * insertion facility added
 * get facility added
 * deletion facility added
 * persistent database made
 * multiple clients allowed and implemented
 *
 * compiled successfully with gcc
 * tested successfully on Ubuntu 20.04 terminal
 */

/*-- SERVER --*/

# include <stdio.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

# define MAXPENDING 10
# define BUFSIZE 256

/* the maximum number of records allowed in the database
 * it can be increased by changing the following preprocessing directive
 */
# define MAXRECORDS 250

/* the database to be used */
char *datafile = "database.txt";

/* action: put the (key, value) into database, if new */
int putPair(char *msg);

/* helper function: ascertain whether a key already exists in database */
int isPresent(int key);

/* get the value associated with the key, if it exists */
void getValue(char *msg, char* response);

/* delete the (key, value) indexed by key, if it exists */
int deletePair(char *msg);

int main() {
	pid_t pid;

	/* create a TCP socket */
	int serverSocket = socket(PF_INET, SOCK_STREAM,
			IPPROTO_TCP);
	if (serverSocket < 0) {
		printf("Error creating server socket\n");
		exit(1);
	}
	printf("Server socket created\n");

	/* constructing local address stucture */
	struct sockaddr_in serverAddress, clientAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(12345);
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	printf("Server address assigned\n");

	/* bind the server */
	int successFlag = bind(serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
	if (successFlag < 0) {
		printf("Error while binding\n");
		exit(0);
	}
	printf("Binding successful\n");

	/* make the server to start listening */
	successFlag = listen(serverSocket, MAXPENDING);
	if (successFlag < 0) {
		printf("Error listening\n");
		exit(0);
	}
	printf("Server now listening\n");

	/* open the database file once
	 * to ensure that it is not read before
	 * it has been opened atleast once
	 */
	FILE *fp = fopen(datafile, "a");
	if (!fp) {
		printf("Unable to open the database\n");
		exit(1);
	}
	fclose(fp);

	/* client socket declaration */
	int clientLength = sizeof(clientAddress);
	int clientSocket;
	
	/* server listens to the clients and interacts with them */
	/*-----*/
	for (;;) {
		/* accept the incoming tcp connection */
		clientSocket = accept(serverSocket, (struct sockaddr*) &clientAddress, &clientLength);
		if (clientLength < 0) {
			printf("Error in connecting client socket\n");
			exit(0);
		}
	
		/* create a child process for the new client */
		/*|||*/
		if ((pid = fork()) == 0) {
			close(serverSocket);

			printf("Handling client %s\n",
					inet_ntoa(clientAddress.sin_addr));
			
			// to signal the breaking of this connection with a Bye
			int breakConn = 0;

			// interaction with the client
			while (1) {
				breakConn = 0;

				// message to be received from client
				char msg[BUFSIZE];
				memset(msg, 0, BUFSIZE);
		
				// receive the message
				successFlag = recv(clientSocket, msg, BUFSIZE, 0);
				if (successFlag < 0) {
					printf("Problem receiving data\n");
					exit(1);
				}
				printf("%s\n", msg);
		
				// response to be delivered to the client
				char response[BUFSIZE];
				memset(response, 0, BUFSIZE);
		
				// handle the case of Bye
				// exit the interaction loop
				// and proceed to break the connection
				if (strlen(msg) >= 3 && 
						msg[0] == 'B' && msg[1] == 'y' && msg[2] == 'e') {
					strcpy(response, "Goodbye");
					breakConn = 1;
				}
		
				// the case of put
				else if (strlen(msg) >= 3 
						&& msg[0] == 'p' && msg[1] == 'u' && msg[2] == 't') {
					if (putPair(msg) >= 0)
						strcpy(response, "OK");
					else
						strcpy(response, "Key already exists");
				}
		
				// the case of get
				else if (strlen(msg) >= 3 
						&& msg[0] == 'g' && msg[1] == 'e' && msg[2] == 't')
					getValue(msg, response);
		
				// the case of del
				else if (strlen(msg) >= 3 
						&& msg[0] == 'd' && msg[1] == 'e' && msg[2] == 'l') {
					if (deletePair(msg) >= 0)
						strcpy(response, "OK");
					else
						strcpy(response, "Key not found");
				}
		
				// default case
				// just echo back the unrecognized message
				else
					strcpy(response, msg);
		
		
				int bytesSent = send(clientSocket, response, strlen(response), 0);
				if (bytesSent != strlen(response)) {
					printf("Error sending message to client\n");
					exit(1);
				}
		
				// if Bye was selected
				// proceed to close the connection
				if (breakConn == 1)
					break;
			}

			// close this child socket
			close(clientSocket);
			exit(0);
		}
		/*|||*/
		// parent closes the socket
		close(clientSocket);
	}
	/*-----*/
	// wait possibly for more clients
}

/* put a (key, value) pair into the database
 * if it does not already exist
 * insertion success = 1, failure = -1
 */
int putPair(char *msg) {
	char value[BUFSIZE];
	int key;

	// read the put instruction
	if(sscanf(msg, "put %d %s", &key, value) == EOF) {
		printf("Error in put instruction!\n");
		exit(1);
	}

	// check if the key is already present
	if (isPresent(key) >= 0)
		return -1;
	
	// open the database file to append the new key,value
	FILE *fp = fopen(datafile, "a");
	if (!fp) {
		printf("Unable to open the database!\n");
		exit(1);
	}

	// append the new pair
	fprintf(fp, "%d %s\n", key, value);
	fclose(fp);

	return 1;
}

/* helper function:
 * check if the (key, value) pair already exists
 * in the database
 * exists: line number of the pair
 * does not exist: -1
 */
int isPresent(int key) {
	// open the database in read mode
	FILE *fp = fopen(datafile, "r");

	if (!fp) {
		printf("Unable to open the database!\n");
		exit(1);
	}

	char currLine[BUFSIZE];

	// read the file line by line
	int i = 0;
	while (fgets(currLine, BUFSIZE, fp) != NULL) {
		int currKey;
		char currValue[BUFSIZE];
		sscanf(currLine, "%d %s", &currKey, currValue);
		if (currKey == key) {
			fclose(fp);
			return i; // line number of the pair
		}
		i++;
	}
	fclose(fp);

	return -1;
}

/* get the value of the key
 * if it exists in the database
 * copy the value or a suitable message into response
 */
void getValue(char *msg, char *response) {
	int key;

	// parse the get instruction
	if(sscanf(msg, "get %d", &key) == EOF) {
		printf("Error in get instruction!\n");
		exit(1);
	}


	// read the file
	FILE *fp = fopen(datafile, "r");

	if (!fp) {
		printf("Unable to open the database!\n");
		exit(1);
	}

	char currLine[BUFSIZE];

	// read the file line by line
	int i = 0;
	while (fgets(currLine, BUFSIZE, fp) != NULL) {
		int currKey;
		char currValue[BUFSIZE];
		sscanf(currLine, "%d %s", &currKey, currValue);

		// check if the current line contains the key
		if (currKey == key) {
			fclose(fp);
			// copy the value into response
			strcpy(response, currValue);
			return;
		}
		i++;
	}
	fclose(fp);

	// copy the key not found message
	strcpy(response, "Key not found");
}

/* delete the pair identified by the key
 * if it exists
 * deletion success: 1, not found: -1
 */
int deletePair(char *msg) {
	int key;

	// parse the del instruction
	if(sscanf(msg, "del %d", &key) == EOF) {
		printf("Error in del instruction!\n");
		exit(1);
	}

	// find the line to be deleted
	// otherwise, signal failure (-1)
	int delLineNum;
	if ((delLineNum = isPresent(key)) < 0)
		return -1;

	// open the file in read mode
	FILE *fp = fopen(datafile, "r");

	if (!fp) {
		printf("Unable to open the database!\n");
		exit(1);
	}

	/* read the file line by line
	 * and store the key-value pairs
	 * except for the one to be deleted
	 */
	int keys[MAXRECORDS];
	char values[MAXRECORDS][BUFSIZE];
	char currLine[BUFSIZE];

	int i = 0, j = 0;
	while (fgets(currLine, BUFSIZE, fp) != NULL) {
		int currKey;
		char currValue[BUFSIZE];
		sscanf(currLine, "%d %s", &currKey, currValue);
		if (delLineNum != i) {
			keys[j] = currKey;
			strcpy(values[j], currValue);
			j++;
		}
		i++;
	}
	fclose(fp);

	// open the file in write mode
	fp = fopen(datafile, "w");
	if (!fp) {
		printf("Unable to open the database!\n");
		exit(1);
	}

	// write all the pairs that are not to be deleted back
	for (int k = 0; k < j; k++)
		fprintf(fp, "%d %s\n", keys[k], values[k]);
	fclose(fp);

	return 1;
}
