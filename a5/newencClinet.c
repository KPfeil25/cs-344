#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <ctype.h> // for use of isupper()

// simple error function
void error(const char *msg) { 
	perror(msg);
	exit(0);
}

int countCharsinFile(const char* filename){
	int c;
	int count = 0;
	FILE* fp = fopen(filename, "r"); // open the file passed in

    while (1) {
        c = fgetc(fp); // grab the char

        if (c == EOF || c == '\n') { // check if its the EOF or a newline
            break;
		}
		if(!isupper(c) && c != ' ') { // check if bad chars exist
            error("File has bad characters\n");
        }
        ++count; // preincrement counter
    }
	fclose(fp);
	return count;
}

// function given in sample code
void setupAddressStruct(struct sockaddr_in* address, int portNumber, char* hostname) {
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname(hostname); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
}

int main(int argc, char *argv[]) {
	int socketFD, portNumber, charsWritten, charsRead, bytesread;
	struct sockaddr_in serverAddress;
	char buffer[1024]; // 1024 because it is small enough that it will not send huge amounts of data, but still send a good amount
	char ciphertext[100000]; // this big just to ensure things

	if (argc < 3) { // case where the user did not enter enough command line args
		fprintf(stderr, "USAGE: %s hostname port\n", argv[0]);
		exit(0);
	}

	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) {
		error("CLIENT: ERROR opening socket");
	}

	setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){ // Connect socket to address
		error("CLIENT: ERROR connecting");
	}

	long filelen = countCharsinFile(argv[1]); // count chars in the file
	long keylen = countCharsinFile(argv[2]); // count chars in the key file

	if (filelen > keylen) { // ensure that the key is >= the messagelen
		error("file is longer than key\n");
	}
	
	char* msg = "otp_dec"; // sending message for authentication
	charsWritten = send(socketFD, msg, strlen(msg), 0); 
	memset(buffer, '\0', sizeof(buffer));

	if (charsWritten < 0) { // case where the auth wasnt sent
		error("CLIENT: ERROR writing from socket");
	}

	charsRead = 0;
	while(charsRead == 0) { 
		charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // read one message
	}

	if (charsRead < 0) {
		error("CLIENT: ERROR reading from socket");
	}

	if(strcmp(buffer, "no") == 0){ // check if server sent back an error
		fprintf(stderr, "Bad client\n");
		exit(2);
	}

	memset(buffer, '\0', sizeof(buffer)); // reset buffer for use
	sprintf(buffer, "%d", filelen); // convert filelen to string to be sent
	charsWritten = send(socketFD, buffer, sizeof(buffer), 0); // send the filelen
	memset(buffer, '\0', sizeof(buffer)); // reset buffer
	charsRead = 0;
	while(charsRead == 0) {
		charsRead = recv(socketFD, buffer, sizeof(buffer)-1, 0); // take in one message
	}
	char* otherbuf;
	if(strcmp(buffer, "more") == 0) { // when the server says to send more
		int fd = open(argv[1], 'r'); // open the message file
		charsWritten = 0;
		while (charsWritten <= filelen) { // while all of the chars have not been recieved
			memset(buffer, '\0', sizeof(buffer)); // reset buffer
			bytesread = read(fd, buffer, sizeof(buffer)-1); // read in all, leaving \0
			charsWritten += send(socketFD, buffer, strlen(buffer), 0); // increment
			memset(buffer, '\0', 1024); // reset buffer
		}
		fd = open(argv[2], 'r'); // open key file
		charsWritten = 0;
		while (charsWritten <= filelen) { // filelen used because we only need filelen # of keys
			memset(buffer, '\0', sizeof(buffer)); 
			bytesread = read(fd, buffer, sizeof(buffer)-1); // read key in
			charsWritten += send(socketFD, buffer, strlen(buffer), 0);
			memset(buffer, '\0', 1024); // reset buffer again
		}
	}
	memset(buffer, '\0', sizeof(buffer)); 

	charsRead = 0;
	int charsSent = 0;
	while (charsRead < filelen) { // recieving the encrypted text back from the server
		memset(buffer, '\0', 1024); 
		charsSent = recv(socketFD, buffer, sizeof(buffer)-1, 0); // recieve
		charsRead += charsSent;
		charsSent = 0;
		strcat(ciphertext, buffer); // add the newly sent buffer to current ciphertext
		memset(buffer, '\0', 1024); 
	}
	strcat(ciphertext, "\n"); // add the endline so reading file knows when to end
	printf("%s", ciphertext); // print to stdout
	close(socketFD); 
	return 0;
}