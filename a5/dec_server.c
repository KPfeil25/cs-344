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

// same functions as in enc_server
int charToInt (char c) {
	if (c == ' '){
		return 26;
	}
	else {
		return (c - 'A');
	}
}

char intToChar(int j) { 
	if (j == 26){
	    return ' ';
	}
	else {
		return (j + 'A');
	}
}

void decrypt(char message[], char key[]) { //decrypt the message
    int i;
    char c, c2;

    for (i = 0; message[i] != '\n'; i++){
        c = charToInt(message[i]) - charToInt(key[i]); // subtract
        if (c < 0) {
            c += 27; // make sure that the num is in correct range
        }
        c2 = intToChar(c);
        message[i] = c2; // set the message
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

int main(int argc, char *argv[]) {
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
	
	int listenSocket = socket(AF_INET, SOCK_STREAM, 0); 
	if (listenSocket < 0) {
        error("ERROR opening socket");
    }

    setupAddressStruct(&serverAddress, atoi(argv[1]));

	if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
		error("ERROR on binding"); // socket couldnt connect w port
    }

    listen(listenSocket, 5); // allow the 5 connections

	while(1) {
		connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (connectionSocket < 0) {
            error("ERROR on accept");
        }

		pid = fork(); // fork the process
        if (pid == -1) {
            error("something forked up\n");
        }
        else if (pid == 0) {
            int charsWritten = 0;
            memset(buffer, '\0', 1024);
            charsRead = 0;
            while (charsRead == 0) {
                charsRead = recv(connectionSocket, buffer, 1023, 0); // receive one message from the client, the auth
            }
            if (charsRead < 0) {
                error("ERROR reading from socket"); // auth not sent
            }
            if (strcmp(buffer, "dec_side") != 0) { // ensuring that the auth was correct
                charsRead = send(connectionSocket, "no", 2, 0); // case where it wasnt
                exit(2);
            }
            else {
                memset(buffer, '\0', 1024); // clear buffer
                charsRead = send(connectionSocket, "yes", 3, 0); // send ok
                charsRead = 0;
                while(charsRead == 0){ 
                    charsRead = recv(connectionSocket, buffer, sizeof(buffer)-1, 0); // take in one message, the file size
                }
                int size = atoi(buffer); // convert the received buffer to an int

                charsRead = send(connectionSocket, "more", 4, 0); // continue to send
                charsRead = 0;
                int charsSent = 0;
                while (charsRead < size) { // continue recieving while the chars recieved is less than size
                    memset(buffer, '\0', 1024); 
                    charsSent = recv(connectionSocket, buffer, sizeof(buffer)-1, 0); // receive all, leave the \0
                    charsRead += charsSent;
                    charsSent = 0;
                    strcat(message, buffer); // add the recieved ciphertext onto the current
                    memset(buffer, '\0', 1024); // reset buffer for next message
                }

                charsRead = 0;
                charsSent = 0;
                while (charsRead < size) { // receiving the key, largely same as previous snippet
                    memset(buffer, '\0', 1024); 
                    charsSent = recv(connectionSocket, buffer, sizeof(buffer)-1, 0); 
                    charsRead += charsSent;
                    charsSent = 0;
                    strcat(key, buffer); // add the newly recieved letters to the current key
                    memset(buffer, '\0', 1024); //clear for next segment of key
                }

                decrypt(message, key); // decrypt the message
                memset(buffer, '\0', 1024); 

                charsWritten = 0;
                while (charsWritten < size) { // send back, decrypted
                    memset(buffer, '\0', sizeof(buffer)); // reset buffer
                    charsWritten += send(connectionSocket, message, sizeof(message), 0); // send
                    memset(buffer, '\0', sizeof(buffer)); // reset buffer
                }	
                exit(0);

            }
		}
        else {
            pid_t actualpid = waitpid(pid, &status, WNOHANG);
        }
    }
    close(connectionSocket); // Close the existing socket which is connected to the client
    close(listenSocket); // Close the listening socket
    return 0;
}