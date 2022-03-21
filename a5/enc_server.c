#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg) {
	perror(msg);
	exit(1);
} 

// helper to change the char into an int
int charToInt (char c) {
	if (c == ' '){ // case where its a space
		return 26;
	}
	else { // case where it is not a space, use ASCII values
		return (c - 'A');
	}
	return 0;
}

char intToChar(int i) { // same as above, just inverted
	if (i == 26){
		return ' ';
	}
	else {
		return (i + 'A');
	}
}

void encrypt(char* message, char* key){ //encrypt a given message
	int i;
	char c, c2;
	for (i = 0; message[i] != '\n'; i++){
	  	c = 0;
		c = (charToInt(message[i]) + charToInt(key[i]));
		c %= 27;
		c2 = intToChar(c);
		message[i] = c2;	
	}
	message[i] = '\0';
}

void setupAddressStruct(struct sockaddr_in* address, int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}

int main(int argc, char *argv[]) 
{
	int connectionSocket, charsRead, status;
	struct sockaddr_in serverAddress, clientAddress;
	socklen_t sizeOfClientInfo = sizeof(clientAddress);
	pid_t pid;
	char buffer[1024], message[100000], key[100000];
	char* encryptedMessage[100000];

	if (argc < 2) {
		fprintf(stderr, "USAGE: %s port\n", argv[0]);
		exit(1);
	}

	// creating socket
	int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket < 0) {
		error("ERROR opening socket");
	}

	setupAddressStruct(&serverAddress, atoi(argv[1])); // set up the address struct

	// allow socket to listen
	if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
		error("ERROR on binding"); // case where the socket cannot bind
	}

	while (1) { 
		listen(listenSocket, 5); // allow socket to hold up to 5 connections

		connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // accept the connection
		if (connectionSocket < 0) {
			error("ERROR on accept");
		}

		pid = fork();
		if (pid == -1) {
			error("somethings forked up\n"); // fork failed
		}
		if (pid == 0) { // fork success
			int charsWritten = 0;
			memset(buffer, '\0', 1024); // clear buffer
			charsRead = 0;
			while (charsRead == 0) {
				charsRead = recv(connectionSocket, buffer, 1023, 0); // get the auth
			}
			if (charsRead < 0) { // recv failed
				error("ERROR reading from socket");
			}
			if (strcmp(buffer, "enc_side") != 0) { // ensuring that the auth matches
				charsRead = send(connectionSocket, "no", 2, 0);
				exit(2);
			}
			else {
				memset(buffer, '\0', 1024); //clear buffer
				charsRead = send(connectionSocket, "yes", 3, 0); // sending ok
				charsRead = 0;
				while (charsRead == 0) { // get one message from the client that contains the filelen
					charsRead = recv(connectionSocket, buffer, sizeof(buffer)-1, 0);
				}
				int size = atoi(buffer); // convert this to an int from the recieved buffer
				charsRead = send(connectionSocket, "more", 4, 0); // more to go
				charsRead = 0;
				int charsSent = 0;
				while (charsRead < size) { // while the received # of chars is less than the file size
					memset(buffer, '\0', 1024); //clear buffer
					charsSent = recv(connectionSocket, buffer, sizeof(buffer)-1, 0); // recieve from client
					charsRead += charsSent;
					charsSent = 0;
					strcat(message, buffer); // add the recieved buffer onto the message array
					memset(buffer, '\0', 1024); //clear buffer
				}
				charsRead = 0; // reset these two
				charsSent = 0;
				while (charsRead < size) {
					memset(buffer, '\0', 1024); //clear buffer
					charsSent = recv(connectionSocket, buffer, sizeof(buffer)-1, 0); // receive the key from the client
					charsRead += charsSent;
					charsSent = 0;
					strcat(key, buffer); // add the buffer onto the current key
					memset(buffer, '\0', 1024); //clear buffer
				}

				encrypt(message, key); // encrypt the message
				memset(buffer, '\0', 1024); //clear buffer

				charsWritten = 0;
				while (charsWritten < size) { // send back the ciphertext
					memset(buffer, '\0', sizeof(buffer)); // clear buffer
					charsWritten += send(connectionSocket, message, sizeof(message), 0); // send
					memset(buffer, '\0', sizeof(buffer)); //clear buff again						
				}	
				exit(0);

				}
			}
		else {
			pid_t actualpid = waitpid(pid, &status, WNOHANG); // allow the parent to continue w/ out waiting for child
		}
		close(connectionSocket); // close current socket
	}
	close(listenSocket); // close socket thats listening
	return 0;
}