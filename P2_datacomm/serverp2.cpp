// Samuel Ball scb548 903528080

#include <stdlib.h>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sys/types.h>   
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h> 
#include <iostream>
#include <fstream>
#include <arpa/inet.h>  
#include <string.h>
#include <unistd.h>
#include "packet.h" 
#include <math.h>
#include <string>

using namespace std;

int main(int argc, char *argv[]){

  if(argc != 5){ cout << "Incorrect number of arguments.\n"; return -1; }
  
  struct hostent* emulatorhost = gethostbyname(argv[1]);
  int fromport = atoi(argv[3]); // from server to emulator
  int toport = atoi(argv[2]); // to server from emulator
  char* file = argv[4];

  int packetLen=512;

  int outsock = socket(AF_INET, SOCK_DGRAM, 0);
  int insock = socket(AF_INET, SOCK_DGRAM, 0);
  if(outsock < 0 || insock < 0){ cout << "Error making server socket\n"; return -1; }

  struct sockaddr_in inAddr, outAddr;
  memset(&inAddr, 0, sizeof(inAddr));
  memset(&outAddr, 0, sizeof(outAddr));
  socklen_t inlen = sizeof(inAddr);

  inAddr.sin_family = AF_INET;
  inAddr.sin_port = htons(toport);
  inAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(insock, (const struct sockaddr*)&inAddr, sizeof(inAddr)) < 0){ cout << "Error binding server port\n"; return -1;}

  outAddr.sin_family = AF_INET;
  outAddr.sin_port = htons(fromport);
  bcopy((char *)emulatorhost->h_addr, (char*)&outAddr.sin_addr.s_addr, emulatorhost->h_length);
  
  ofstream filename(file);
  ofstream log("arrival.log");
  char rpacket[packetLen];
  char spacket[packetLen];
  string data;
  char temp[12];
  char realdata[12];

  while(true){
    memset(rpacket, 0, 512);
    memset(spacket, 0, 512);
    memset(realdata, 0, 12);
    memset(temp, 0, 12);

    packet recvpacket(0, 0, 0, rpacket);
    recvfrom(insock, rpacket, 512, MSG_WAITALL, (struct sockaddr *)&inAddr, &inlen);
    recvpacket.deserialize(rpacket);
    log << recvpacket.getSeqNum() << endl;

    if(recvpacket.getType() == 3){
      packet eot(2, recvpacket.getSeqNum(), 0, NULL);
      eot.serialize(spacket);
      sendto(outsock, spacket, 512, MSG_CONFIRM, (struct sockaddr *)&outAddr, sizeof(struct sockaddr_in));
      break;
    }

    if(!recvpacket.countParity()){ // store data and send good ack if even parity
      data = recvpacket.getData();
      for(int i = 0; i < recvpacket.getLength()-1; i++){
        realdata[i] = data[i];
      }

      filename << realdata;
      packet sendpacket(0, recvpacket.getSeqNum(), 0, NULL);
      sendpacket.serialize(spacket);
    }
    else{ // send bad ack if odd parity
      packet sendpacket(0, !recvpacket.getSeqNum(), 0, NULL);
      sendpacket.serialize(spacket);
    }
    
    sendto(outsock, spacket, 512, MSG_CONFIRM, (struct sockaddr *)&outAddr, sizeof(struct sockaddr_in));
  }

  close(outsock);
  close(insock);

  return 0;
}
