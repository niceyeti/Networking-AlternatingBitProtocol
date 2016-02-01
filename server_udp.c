#include "common.h"

int main(int argc, char * argv[])
{
	char *fname;
        byte buf[MAX_RX_LINE];
		byte ackBuf[RXTX_BUFFER_SIZE];
        struct sockaddr_in sin;
        int len;
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
	
	printf("Server up and waiting at ANY interface on port %d\r\n",SERVER_PORT);

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
				perror("Error: Short packet\n");
			}
		}
		else if(len > 1){

			deserializePacket(buf,&rxPkt);
			printf("RXED client packet, seqnum=%d:  >%s<\r\n",bytesToLint(rxPkt.seqnum),rxPkt.data);
			printPacket(&rxPkt);

			//send ACK for every packet received (this is for debugging the client)
			makePacket(bytesToLint(rxPkt.seqnum), ACK, 0, &ackPkt);
			serializePacket(&ackPkt,ackBuf);
			sendPacket(&ackPkt,s,&sin);

			/*
			if(sendto(s, ackBuf, strnlen(ackBuf,PKT_DATA_MAX_LEN), 0, (struct sockaddr *)&sin, sock_len) < 0){
				perror("SendTo Error\n");
				exit(1);
			}
			*/



			if(fputs((char *) buf, fp) < 1){
				printf("fputs() error\n");
			}
		}
        }
	fclose(fp);
	close(s);
}
