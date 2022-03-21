#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <ctype.h> // for using isupper()

void error(const char *msg) { 
    perror(msg);
    exit(0); 
}

// same counting fn as in the enc client
int countCharsinFile(const char* filename){
	int c;
	int count = 0;
	FILE* fp = fopen(filename, "r");

    while (1) {
        c = fgetc(fp);

        if (c == EOF || c == '\n')
            break;
		if(!isupper(c) && c != ' '){
            error("File has bad characters\n");
        }
        ++count;
    }
	fclose(fp);
	return count;
}

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

int main(int argc, char *argv[])
{
	int socketFD, portNumber, charsWritten, charsRead, numread;
	struct sockaddr_in serverAddress;
	char buffer[1024];
	char ciphertext[100000];

	if (argc < 3) { 
        fprintf(stderr, "USAGE: %s hostname port\n", argv[0]); // when the user has not entered the correct num of command line args
        exit(0);
    }

	socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFD < 0) {
        error("CLIENT: ERROR opening socket");
    }
	
    setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost"); // set up the address struct

	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){ //connect to server
		error("CLIENT: ERROR connecting");
	}

	long filelen = countCharsinFile(argv[1]); // count the chars in the ciphertext
	long keylen = countCharsinFile(argv[2]); // count the chars in the key

    if (filelen > keylen) { // case where the key isnt long enough
		error("file is longer than key\n");
	}
	
	char* msg = "dec_side"; // message for auth
	charsWritten = send(socketFD, msg, strlen(msg), 0); // send one mesage over
	memset(buffer, '\0', sizeof(buffer)); 
	if (charsWritten < 0) {
		error("CLIENT: ERROR writing from socket");
    }
	charsRead = 0;
	while(charsRead == 0) {
		charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);  // receive the ok back
    }
	if (charsRead < 0) {
        error("CLIENT: ERROR reading from socket"); // case where the ok didnt go ok
    }

	if (strcmp(buffer, "no") == 0) { // bnad port, dont connect
		fprintf(stderr, "Bad client\n");
		exit(2);
	}

	memset(buffer, '\0', sizeof(buffer)); // reset buffer
	sprintf(buffer, "%d", filelen); // convert the filelen into a string
	charsWritten = send(socketFD, buffer, sizeof(buffer), 0); // send one message, being the length
	memset(buffer, '\0', sizeof(buffer)); 
	charsRead = 0;
	while(charsRead == 0) {
		charsRead = recv(socketFD, buffer, sizeof(buffer)-1, 0); // recieve one message from the server
    }

	if(strcmp(buffer, "more") == 0) { // more to be sent
		int fd = open(argv[1], 'r'); // open the ciphertext file
		charsWritten = 0;
		
		while (charsWritten <= filelen) { // while not all of the ciphertext has been sent over
			memset(buffer, '\0', sizeof(buffer)); // reset buffer
			numread = read(fd, buffer, sizeof(buffer)-1); // read from file
			charsWritten += send(socketFD, buffer, strlen(buffer), 0); // send over
			memset(buffer, '\0', 1024); //clear buffer
		}
		fd = open(argv[2], 'r'); // same as above for the key file
		charsWritten = 0;
		while (charsWritten <= filelen) {
			memset(buffer, '\0', sizeof(buffer)); // ensure buffer is clear 
			numread = read(fd, buffer, sizeof(buffer)-1); // read from file
			charsWritten += send(socketFD, buffer, strlen(buffer), 0); // send
			memset(buffer, '\0', 1024); 
		}
	}
	memset(buffer, '\0', sizeof(buffer)); 

	
	charsRead = 0;
	int charsSent = 0;

	while (charsRead < filelen) { // read the new text from the server
		memset(buffer, '\0', 1024); // clear buffer
		charsSent = recv(socketFD, buffer, sizeof(buffer)-1, 0); // receive
		charsRead += charsSent; // add to keep track of length
		charsSent = 0;
		strcat(ciphertext, buffer); // add the newly recieved buffer onto current ciphertext
		memset(buffer, '\0', 1024); // clear buffer
	}
	strcat(ciphertext, "\n"); // add the newline for EOF
	printf("%s", ciphertext); // print to stdout
	close(socketFD); // close socket
	return 0;
}