#include "common.h"

int main(int argc, char * argv[])
{
  char *fname;
  byte buf[MAX_RX_LINE];
  byte ackBuf[RXTX_BUFFER_SIZE];
  struct sockaddr_in sin;
  int len, dataLen, receiverSeqnum, firstSeqnum;
  int s, i;
  struct timeval tv;
  char seq_num = 1; 
  FILE *fp;
  struct Packet ackPkt;
  struct Packet rxPkt;

  if (argc==2) {
    fname = argv[1];
  }
  else {
    fprintf(stderr, "usage: ./server_udp filename\n");
    exit(1);
  }

  /* build address data structure */
  bzero((char *)&sin, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(SERVER_PORT);

  /* setup passive open */
  if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
    perror(": socket");
    exit(1);
  }
  if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
    perror(": bind");
    exit(1);
  }

  socklen_t sock_len = sizeof sin;
  srandom(time(NULL));

  fp = fopen(fname, "w");
  if (fp==NULL){
    printf("Can't open file\n");
    exit(1);
  }
  
  printf("sizeof struct Packet: %d\\n\r",(int)sizeof(struct Packet));
  memset((void*)&ackPkt,0,sizeof(struct Packet));
  memset((void*)&rxPkt,0,sizeof(struct Packet));
  
  printf("Server up and awaiting packets at ANY interface on port %d\r\n",SERVER_PORT);
  firstSeqnum = TRUE;

  /*
  Receiver just blocks, waiting for input to arrive.
  If rx:
    if rx length is 1 and == 0x02:
      treat as end of transmission, and exit comm loop
    else:
      -deserialize packet from rx message
      -write packet data to file
      -send ACK to sender
      -go back to wait for input
  */
  while(1){
    len = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&sin, &sock_len);
    printf("rxed pkt of len=%d\r\n",len);
    if(len == -1){
      perror("PError");
    }
    else if(len == 1){
      if (buf[0] == 0x02){
        printf("Transmission Complete\n");
        break;
      }
      else{
        perror("Error: Short packet??\n");
      }
    }
    else if(len > 1){
      //for local debugging: randomly fail to acknowledge ~1/3 of packets to test client recovery from missing acks
      if(DBG && !(random() % 3 == 0)){
        printf("Server dropped packet...");
      }
      else{
        //deserialize the received packet
        deserializePacket(buf,&rxPkt);
        
        //seqnum is part of the receiver's state, and must be initialized in alignment with the sender; this bootstraps it on the first received packet.
        //Subsequent packets are aligned with this seqnum, rejecting dupes that are re-sent if the receiver ACK packet is dropped.
        if(firstSeqnum){
          firstSeqnum = FALSE;
          //careful here: we write data to file based on a new seqnum; so don't initialize seqnum to rxPkt.seqnum, or the first line will not be written
          receiverSeqnum = (bytesToLint(rxPkt.seqnum) + 1) % 2;
        }
        printf("Receiver RXED client packet, seqnum=%d:  >%s<\r\n",bytesToLint(rxPkt.seqnum),rxPkt.data);
        printPacket(&rxPkt);

        //send ACK for every packet received
        //remember even the ACK could be dropped; hence sender needs to implement a timeout while waiting for ACK
        makePacket(bytesToLint(rxPkt.seqnum), ACK, 0, &ackPkt);
        serializePacket(&ackPkt,ackBuf);
        sendPacket(&ackPkt,s,&sin);

        //if this is a new packet, copy the packet data to file (making sure to null terminate it)
        if(receiverSeqnum != bytesToLint(rxPkt.seqnum)){
          //receiverSeqnum = bytesToLint(rxPkt.seqnum);
          dataLen = bytesToLint(rxPkt.dataLen);
          dataLen = dataLen < PKT_DATA_MAX_LEN ? dataLen : (PKT_DATA_MAX_LEN - 1);
          rxPkt.data[dataLen] = '\0';
          printf("Receiver rx'ed data: %s\r\n",(char*)rxPkt.data);
          if(fputs((char *)rxPkt.data, fp) < 1){
            printf("fputs() error\n");
          }

          //update the seqnum; for the alternating bit protocol, the seqnum just alternates between 0 and 1
          receiverSeqnum = (receiverSeqnum + 1) % 2;
        }
        else{
          printf("Sender dupe received with pkt.seqnum==%d receiver.seqnum=%d\r\n",bytesToLint(rxPkt.seqnum),receiverSeqnum);
        }
      }
    }
  }

  fclose(fp);
  close(s);
}
