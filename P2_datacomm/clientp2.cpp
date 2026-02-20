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
  int toport = atoi(argv[3]); // to client, from emulator
  int fromport = atoi(argv[2]); // from client, to emulator
  char* file = argv[4];

  int packetLen = 512;
  
  int insock = socket(AF_INET, SOCK_DGRAM, 0);
  int outsock = socket(AF_INET, SOCK_DGRAM, 0);
	if(insock < 0 || outsock < 0){ printf("Error making client socket\n"); return -1; }

  struct sockaddr_in inAddr;
  memset(&inAddr, 0, sizeof(inAddr));
  socklen_t inlen = sizeof(inAddr);
  
  inAddr.sin_family = AF_INET;
  inAddr.sin_port = htons(toport);
  inAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  if(bind(insock, (const struct sockaddr *)&inAddr, sizeof(inAddr)) < 0){ cout << "Error binding socket\n"; return -1; }


  struct sockaddr_in outAddr;
  memset(&outAddr, 0, sizeof(outAddr));
  socklen_t outlen = sizeof(outAddr);

  outAddr.sin_family = AF_INET;
  outAddr.sin_port = htons(fromport);
  bcopy((char *)emulatorhost->h_addr, (char *)&outAddr.sin_addr.s_addr, emulatorhost->h_length);

  ifstream infile(file, ios::in);
  ofstream seqlog("clientseqnum.log");
  ofstream acklog("clientack.log");
  int seqnum = 0;
  string data;
  char testdata[12];
  memset(testdata, 0, 12);
  char senddata[11];
  memset(senddata, 0, 12);
  char spacket[packetLen];
  memset(spacket, 0, 512);
  char rpacket[packetLen];
  memset(rpacket, 0, 512);

  if(infile.is_open()){
    while(!infile.eof()){
      infile.read(testdata, 10);
      int size = strlen(testdata);
      data = testdata;

      // make a packet with even parity 
      packet testpacket(1, seqnum, sizeof(testdata), testdata);

      if(testpacket.countParity()){data.append("1"); sprintf(senddata, "%.12s", data.c_str()); } //strcpy(senddata, data.c_str());
      else{ data.append("0"); sprintf(senddata, "%.12s", data.c_str());}

      packet sendpacket(1, seqnum, size + 1, senddata);

      while(true){
        // serialize and send packet
        sendpacket.serialize(spacket);
        seqlog << sendpacket.getSeqNum() << endl;
        sendto(outsock, spacket, packetLen, MSG_CONFIRM, (const struct sockaddr *)&outAddr, outlen);
        
        // recieve ack from server
        packet recvpacket(0, !seqnum, 0, rpacket);
        recvfrom(insock, rpacket, packetLen, MSG_WAITALL, (struct sockaddr *)&inAddr, &inlen);
        recvpacket.deserialize(rpacket);
        acklog << recvpacket.getSeqNum() << endl;

        // continue if ack is for outstanding packet seqnum, else resend packet
        if(recvpacket.getSeqNum() == seqnum){ break; }
      }

      // reset data
      memset(testdata, 0, 12);
      memset(senddata, 0, 12);
      memset(spacket, 0, 512);
      memset(rpacket, 0, 512);

      seqnum = !seqnum;
      
    }
  } else{ cout << "Unable to open file\n"; return -1; }

  // send eot packet

  char endp[packetLen];
  memset(endp, 0, 512);
  packet eot(3, seqnum, 0, NULL);
  seqlog << eot.getSeqNum() << endl;
  eot.serialize(endp);
  sendto(outsock, endp, 512, MSG_CONFIRM, (const struct sockaddr *)&outAddr, outlen);

  // recieve eot reply
  memset(endp, 0, 512);
  recvfrom(insock, endp, 512, MSG_WAITALL, (struct sockaddr *)&inAddr, &inlen);
  eot.deserialize(endp);
  acklog << eot.getSeqNum() << endl;

  close(insock);
  close(outsock);
  
  return 0;
}
