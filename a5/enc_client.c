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
	int socketFD, portNumber, charsWritten, charsRead, bytesread;
	struct sockaddr_in serverAddress;
	char buffer[1024];
	char ciphertext[100000];

	if (argc < 3) {
		fprintf(stderr, "USAGE: %s hostname port\n", argv[0]);
		exit(0);
	}

	// Set up the server address struct
	setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

	socketFD = socket(AF_INET, SOCK_STREAM, 0); // create socket
	if (socketFD < 0) {
		error("CLIENT: ERROR opening socket");
	}
	int yes = 1;
	setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)); // just for convenience

	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){ // connect socket & address
		error("CLIENT: ERROR connecting");
	}

	long filelen = countCharsinFile(argv[1]);
	long keylen = countCharsinFile(argv[2]);

	if (filelen > keylen){ //if file is greater than key
		error("file is longer than key\n");
	}
	
	char* msg = "enc_side";
	charsWritten = send(socketFD, msg, strlen(msg), 0); // checking if communicating w/ server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	if (charsWritten < 0) {
		error("CLIENT: ERROR writing from socket");
	}
	charsRead = 0;
	while (charsRead == 0) { // getting one response from server saying all good
		charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
	}
	if (charsRead < 0) {
		error("CLIENT: ERROR reading from socket"); // case where the communication did not go well
	}

	if (strcmp(buffer, "no") == 0) { // server telling us that it is a bad port
		fprintf(stderr, "Bad client\n");
		exit(2);
	}

	// sending file len
	memset(buffer, '\0', sizeof(buffer)); // clear buffer
	sprintf(buffer, "%d", filelen); // convert filelen to string to send
	charsWritten = send(socketFD, buffer, sizeof(buffer), 0); // send
	memset(buffer, '\0', sizeof(buffer)); // clear buffer for next step
	charsRead = 0;
	while (charsRead == 0) {
		charsRead = recv(socketFD, buffer, sizeof(buffer)-1, 0); // recieve one message from the server about if theres more to send
	}
	
	if (strcmp(buffer, "more") == 0) { // continue sending
		int fd = open(argv[1], 'r'); // open the message file
		charsWritten = 0;
		while (charsWritten <= filelen) { // while all of it hasnt been sent
			memset(buffer, '\0', sizeof(buffer));
			bytesread = read(fd, buffer, sizeof(buffer)-1); // take all, leave the \0
			charsWritten += send(socketFD, buffer, strlen(buffer), 0); // increment charswritten
			memset(buffer, '\0', 1024); //clear buffer
		}
		fd = open(argv[2], 'r'); // open the file with the key in it
		charsWritten = 0;
		while (charsWritten <= filelen) {
			memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
			bytesread = read(fd, buffer, sizeof(buffer)-1);
			charsWritten += send(socketFD, buffer, strlen(buffer), 0);
			memset(buffer, '\0', 1024); //clear buffer
		}
	}
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse

	charsRead = 0;
	int charsSent = 0;
	while (charsRead < filelen) { // recieving encrypted text back from server
		memset(buffer, '\0', 1024); //clear buffer
		charsSent = recv(socketFD, buffer, sizeof(buffer)-1, 0); // recieve
		charsRead += charsSent;
		charsSent = 0;
		strcat(ciphertext, buffer); // add the buffer onto the current ciphertext
		memset(buffer, '\0', 1024); //clear buffer
	}
	strcat(ciphertext, "\n"); // add the newline on for later
	printf("%s", ciphertext); // print to standard output
	close(socketFD); // Close the socket
	return 0;
}