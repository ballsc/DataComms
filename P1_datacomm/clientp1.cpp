// Samuel Ball scb548 903528080

// this is the client
#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>

int main (int argc, char *argv[]) {
	if(argc != 4){ printf("Incorrect number of inputs\n"); return -1;}
	char *ip = argv[1];
	struct hostent* host = gethostbyname(ip); // from https://github.com/bozkurthan/Simple-TCP-Server-Client-CPP-Example/blob/master/tcp-Client.cpp, other ways didnt work
	int port = atoi(argv[2]);
	char* filename = argv[3];
	char buffer[50];
	
	int c_sock = socket(AF_INET, SOCK_DGRAM, 0); 
	if(c_sock < 0){ printf("Error making client socket\n"); return -1; }

	sockaddr_in sAddress;
	memset(&sAddress, 0, sizeof(sAddress));
	sAddress.sin_family = AF_INET;
	sAddress.sin_port = htons(port);
	sAddress.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list)); // from https://github.com/bozkurthan/Simple-TCP-Server-Client-CPP-Example/blob/master/tcp-Client.cpp, other ways didnt work

	int bytesin, bytesout = 0;
	const char *handshake = "ABCDEF";

	socklen_t len;

	bytesout = sendto(c_sock, handshake, strlen(handshake), MSG_CONFIRM, 
					(const struct sockaddr *)&sAddress, sizeof(sAddress));
	printf("Handshake written, waiting on response\n");

	bytesin = recvfrom(c_sock, (char *)buffer, 50, MSG_WAITALL, 
					(struct sockaddr *) &sAddress, &len);
	
	int new_port = std::stoi(buffer);
	printf("Response recieved, reconnecting at port: %d\n", new_port);

	close(c_sock);

	int c_sock2 = socket(AF_INET, SOCK_DGRAM, 0); 
	if(c_sock2 < 0){ printf("Error making client socket\n"); return -1; }

	memset(&sAddress, 0, sizeof(sAddress));
	sAddress.sin_family = AF_INET;
	sAddress.sin_port = htons(new_port);
	sAddress.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));

	char data[5];
	std::ifstream infile(filename, std::ios::in);

	if(infile.is_open()){
		printf("Sending data from file: %s\n", filename);
		while(!infile.eof()){
			infile.read(data, 4);
			sendto(c_sock2, data, sizeof(data), MSG_CONFIRM, 
					(const struct sockaddr *)&sAddress, sizeof(sAddress));

			memset(data, 0, sizeof(data));

			recvfrom(c_sock2, data, sizeof(data), MSG_WAITALL, 
					(struct sockaddr *) &sAddress, &len);

			printf("%s\n", data);

			memset(data, 0, sizeof(data));
		}
		char datum[5] = { "end\0" };
		sendto(c_sock, datum, sizeof(datum), MSG_CONFIRM, (const struct sockaddr *)&sAddress, sizeof(sAddress));
	} else("Cannot open file\n");
	printf("File sent\n");
	infile.close();
	close(c_sock2);

	return 0;
}