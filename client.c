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

/*-- CLIENT --*/

# include <stdio.h>
# include <sys/socket.h>
# include <arpa/inet.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

# define BUFSIZE 256

int main() {
	/* create a TCP socket */
	int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		printf("Error in opening a socket\n");
		exit(1);
	}
	printf("Client socket created\n");

	/* construct server address structure */
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(12345);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	printf("Server address assigned\n");

	/* establish a tcp connection */
	int c = connect(sock, (struct sockaddr*) &serverAddr,
			sizeof(serverAddr));
	printf("%d\n", c);
	if (c < 0) {
		printf("Error while establishing connection\n");
		exit(1);
	}
	printf("Connection established\n");

	/* interact with the server */
	int flag = 1; // can we accept the input
	while (1) {
		printf("Enter request: ");
		char msg[BUFSIZE]; // the input message
		memset(msg, 0, BUFSIZE);

		fgets(msg, BUFSIZE, stdin);
		flag = 0;
		
		// send the message to the server
		int bytesSent = send(sock, msg, strlen(msg), 0);
		if (bytesSent != strlen(msg)) {
			printf("Error sending message\n");
			exit(1);
		}
		
		// receive the response from the server
		char recvBuffer[BUFSIZE];
		memset(recvBuffer, 0, BUFSIZE);
		int bytesRecvd = recv(sock, recvBuffer, BUFSIZE - 1, 0);
		if (bytesRecvd < 0) {
			printf("Error receiving message");
			exit(0);
		}
		recvBuffer[bytesRecvd] = '\0';

		// display the response obtained
		printf("Server replied: %s\n", recvBuffer);
		flag = 1;

		// if the user entered Bye
		// break the connection - exit the loop
		if (strlen(msg) >= 3 && 
				msg[0] == 'B' && msg[1] == 'y' && msg[2] == 'e')
			break;
	}	

	// close the client socket
	close(sock);
	printf("Connection closed.\n");
}
