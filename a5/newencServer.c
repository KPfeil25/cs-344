#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

int charToInt (char c){
	if (c == ' '){
		return 26;
	}
	else {
		return (c - 'A');
	}
	return 0;
}

char intToChar(int i){
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

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, int portNumber) {
 
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
	int connectionSocket, portNumber, charsRead, status;
	struct sockaddr_in serverAddress, clientAddress;
	socklen_t sizeOfClientInfo = sizeof(clientAddress);
	pid_t pid;

	if (argc < 2) { fprintf(stderr, "USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	setupAddressStruct(&serverAddress, atoi(argv[1]));

	int listenSocket = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocket < 0) {
		error("ERROR opening socket");
	}

	// Enable the socket to begin listening
	if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {// Connect socket to port
		error("ERROR on binding");
	}

    listen(listenSocket, 5); 	

	while(1){ 
		connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (connectionSocket < 0) {
			error("ERROR on accept");
		}

		pid = fork();
		if (pid == -1) {
			error("something forked up\n");
		}
		else if (pid == 0) {
			char buffer[1024];
			char* encryptedMessage[100000];
			char message[100000];
			char key[100000];
			int charsWritten = 0;
			memset(buffer, '\0', 1024); 
			charsRead = 0;
			while(charsRead == 0) {
				charsRead = recv(connectionSocket, buffer, 1023, 0); 
			}
			if (charsRead < 0) {
				error("ERROR reading from socket");
			}
			//printf("SERVER: I received this from the client: \"%s\"\n", buffer);
			if (strcmp(buffer, "otp_enc") != 0) {
				charsRead = send(connectionSocket, "no", 2, 0);
				exit(2);
			}
			else {
				memset(buffer, '\0', 1024); 
				//charsRead = send(connectionSocket, "yes", 3, 0); 
				charsRead = 0;
				while(charsRead == 0){ 
					charsRead = recv(connectionSocket, buffer, sizeof(buffer)-1, 0); 
					//printf("%d\n", charsRead);
				}
				//printf("buffer on server: %s", buffer);
				int size = atoi(buffer);
				//printf("size of file on server: %d\n", size);

				charsRead = send(connectionSocket, "more", 4, 0);
				charsRead = 0;
				int charsSent = 0;
				//printf("size: %d", size);
				while(charsRead < size){
					memset(buffer, '\0', 1024); 
					charsSent = recv(connectionSocket, buffer, sizeof(buffer)-1, 0); 
					charsRead += charsSent;
					charsSent = 0;
					strcat(message, buffer);
					//printf("charsRead: %d\n", charsRead);
					memset(buffer, '\0', 1024); 
				}
				//printf("%d ", charsRead);
				//printf("%s\n", message);
				charsRead = 0;
				charsSent = 0;
				//printf("size: %d", size);
				while(charsRead < size){
					memset(buffer, '\0', 1024); 
					charsSent = recv(connectionSocket, buffer, sizeof(buffer)-1, 0); 
					charsRead += charsSent;
					charsSent = 0;
					strcat(key, buffer);
					//printf("charsRead: %d\n", charsRead);
					memset(buffer, '\0', 1024);
				}
				//printf("%s\n", message);
				//printf("%s", key);

				encrypt(message, key);
				//printf("%s\n", message);
				memset(buffer, '\0', 1024); 

				charsWritten = 0;
				while(charsWritten < size){
					memset(buffer, '\0', sizeof(buffer)); 
					charsWritten += send(connectionSocket, message, sizeof(message), 0);
					memset(buffer, '\0', sizeof(buffer)); 						
				}	

				exit(0);

			}
		}
		else {
			pid_t actualpid = waitpid(pid, &status, WNOHANG);
		}
		close(connectionSocket); // Close the existing socket which is connected to the client
	}
	close(listenSocket); // Close the listening socket
	return 0;
}