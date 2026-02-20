// Samuel Ball scb548 903528080

// this is the server
#include <iostream>
#include <random>
#include <time.h>
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
#include <cstdint>
#include <cstring>
#include <iostream>

int main(int argc, char *argv[]){
	if(argc != 2){ printf("Incorrect number of inputs\n"); return -1; }
	int port = atoi(argv[1]);

	int s_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(s_sock < 0){ printf("Error making server port\n"); return -1; }

	sockaddr_in sAddress, cAddress;
	memset(&sAddress, 0, sizeof(sAddress));
	memset(&cAddress, 0, sizeof(cAddress));
	sAddress.sin_family = AF_INET;
	sAddress.sin_port = htons(port);
	sAddress.sin_addr.s_addr = htonl(INADDR_ANY);

	int binded = bind(s_sock, (const struct sockaddr *)&sAddress, sizeof(sAddress));
	if(binded < 0){printf("Error binding server socket to port\n"); return -1; }

	int bytesout, bytesin = 0;
	socklen_t len;
	len = sizeof(cAddress);
	char handshake[50];

	printf("Server up, waiting for client\n");
	bytesin = recvfrom(s_sock, handshake, sizeof(handshake), MSG_WAITALL, 
						(struct sockaddr *)&cAddress, &len);
	if(bytesin == 0){printf("Nothing read from client\n"); return -1; }
	std::cout << "Read " << handshake << " from client\n";

	srand(time(NULL));
	int num_ports = 64511;
	int rand_port = rand() % num_ports + 1024;
	printf("The random port the server selected is: %d\n", rand_port);

	std::string temp = std::to_string(rand_port);

	char new_port[25];
	strcpy(new_port, temp.c_str());

	bytesout = sendto(s_sock, (char *)new_port, sizeof(new_port), MSG_CONFIRM, 
						(const struct sockaddr *)&cAddress, len);
	if(bytesout < 0){printf("Error sending new port\n"); return -1;}
	printf("New port sent\n");

	close(s_sock);

	int s_sock2 = socket(AF_INET, SOCK_DGRAM, 0);
	if(s_sock2 < 0){ printf("Error making server port\n"); return -1; }

	memset(&sAddress, 0, sizeof(sAddress));
	memset(&cAddress, 0, sizeof(cAddress));
	sAddress.sin_family = AF_INET;
	sAddress.sin_port = htons(rand_port);
	sAddress.sin_addr.s_addr = htonl(INADDR_ANY);

	binded = bind(s_sock2, (const struct sockaddr *)&sAddress, sizeof(sAddress));
	if(binded < 0){printf("Error binding server socket to port\n"); return -1; }

	char data[5];
	std::ofstream outfile("upload.txt", std::ios::out);

	if(outfile.is_open()){
		printf("Waiting for data from client\n");
		while(true){
			memset(&data, 0, sizeof(data));

			recvfrom(s_sock2, data, sizeof(data), MSG_WAITALL, 
					(struct sockaddr *)&cAddress, &len);

			if(data[0] != 'e' || data[1] != 'n' || data[2] != 'd' || data[3] != '\0'){
				outfile << data;

				for(int i=0; i<strlen(data); i++){
					data[i] = toupper(data[i]);
				}

				sendto(s_sock2, data, sizeof(data), MSG_WAITALL, 
						(struct sockaddr *)&cAddress, len);
			} else{ break; }
		}
	} else { printf("Unable to open file\n"); return -1;}

	printf("File recorded\n");

	close(s_sock2);
	
	return 0;
}