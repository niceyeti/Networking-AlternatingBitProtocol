#include "common.h"

//// Super simplified serialization: raw memcpy between Packet/buffer. Needs more testing; this may depend on endianness of end systems.
void serializePacket(struct Packet* pkt, byte buf[RXTX_BUFFER_SIZE])
{	
	//super awesome benefit of not using dynamic mem in packet, since packet size is static
	memset((void*)buf,0,RXTX_BUFFER_SIZE);
	memcpy((void*)buf,(void*)pkt,sizeof(struct Packet));
}
void deserializePacket(const char buf[RXTX_BUFFER_SIZE], struct Packet* pkt)
{	
	//super awesome benefit of not using dynamic mem in packet, since packet size is static
	memcpy((void*)pkt,(void*)buf,sizeof(struct Packet));
}

//For simple protocols: just verify packet ack field == ACK. Returns TRUE if ACK, else FALSE.
int isAck(struct Packet* pkt)
{
	return pkt->ack == ACK ? TRUE : FALSE;
}

//For more complicated protocols: verify pkt contains ACK and has expected seqnum.
//Returns TRUE if packet is ACK and matches passed seqnum; else FALSE.
int isSequentialAck(struct Packet* pkt, int seqnum)
{
	int result = FALSE;
	
	if(isAck(pkt) == TRUE){
		result = bytesToLint(pkt->seqnum) == seqnum ? TRUE : FALSE;
		if(result == FALSE){
			printf("ERROR received ACK with incorrect seqnum: expected %d but rxed %d\r\n",seqnum,bytesToLint(pkt->seqnum));
		}
	}
	else{
		printf("ERROR received ACK with incorrect seqnum: expected %d but rxed %d\r\n",seqnum,bytesToLint(pkt->seqnum));		
	}
	
	return result;
}

/*
	Returns sum of byte mod some-16b-prime of all the header fields.
	This includes the data-checksum, but not the data. No other modifications
	can be made to the header after setting the checksum.
*/
int getHeaderChecksum(struct Packet* pkt)
{
	int i, sum = 0;
	
	sum += pkt->ack;
	for(i = 0; i < 4; i++){
		sum += pkt->seqnum[i];
	}
	for(i = 0; i < 4; i++){
		sum += pkt->dataLen[i];
	}
	for(i = 0; i < 4; i++){
		sum += pkt->name[i];
	}
	for(i = 0; i < 4; i++){
		sum += pkt->dataChecksum[i];
	}
	
	return sum % U16_PRIME;
}

//Get the checksum of some data buffer, given its length in bytes. Returns 0 if datalen is zero (no data)
int getDataChecksum(struct Packet* pkt)
{
	int i, dataLen, sum, checksum = 0;
	
	dataLen = bytesToLint(pkt->dataLen);

	if(dataLen > PKT_DATA_MAX_LEN){
		printf("ERROR overrun in getDataChecksum(): pkt->dataLen > PKT_DATA_LEN_MAX\r\n");
		return 0;
	}

	//only set checksum if there is any data; otherwise, it is set to zero
	if(dataLen > 0){
		for(i = 0, sum = 0; i < dataLen; i++){
			sum += (int)pkt->data[i];
		}
		checksum = sum % U16_PRIME;
	}

	return checksum;
}

int isCorruptPacket(struct Packet* pkt)
{
  	int isCorrupt, checksum;
	byte buf[4];

	isCorrupt = CORRUPT;
	
	//check the header checksum
	checksum = getHeaderChecksum(pkt);
	lintToBytes(checksum,buf);
	if(strncmp(buf,pkt->hdrChecksum,4) == 0){
		//check the data checksum
		checksum = getDataChecksum(pkt);
		lintToBytes(checksum,buf);
		if(strncmp(buf,pkt->dataChecksum,4) == 0){
			isCorrupt = NOT_CORRUPT;
		}
		else{
			printf("ERROR rxed packet with correct header checksum, but incorrect data checksum. flagged as CORRUPT\r\n");
		}
	}
	else{
		printf("ERROR rxed packet with incorrect header checksum, flagged as CORRUPT\r\n");
	}
	
	return isCorrupt;
}
//////////////// end packet class ///////////////

void testByteConversion()
{
	char buf[4];
	unsigned int y, x = 34;

	printf("converting: %0X\r\n",x);
	lintToBytes(x,buf);
	y = bytesToLint(buf);
	printf("converted: %0X\r\n",y);
	if(y == x)
		printf("pass\r\n");
	else
		printf("fail\r\n");
}

int bytesToLint(const byte buf[4])
{
	unsigned int lint = 0;
	lint  = (unsigned int)buf[3] & 0xFF;
	lint |= (((unsigned int)buf[2] & 0xFF) << 8);
	lint |= (((unsigned int)buf[1] & 0xFF) << 16);
	lint |= (((unsigned int)buf[0] & 0xFF) << 24);
	
    return ntohl(lint);
}
void lintToBytes(const int i, byte obuf[4])
{
	unsigned int lint = htonl(i);
	//printf("%0X htonled to %0X\r\n",i,lint);
	obuf[3] = (byte)(lint & 0xFF);
	obuf[2] = (byte)((lint & 0xFF00) >> 8);
	obuf[1] = (byte)((lint & 0xFF0000) >> 16);
	obuf[0] = (byte)((lint & 0xFF000000) >> 24);

	//printf("lintToBytes: %0X converted to %0X %0X %0X %0X  bytes[0-3]\r\n",lint,(int)obuf[0],(int)obuf[1],(int)obuf[2],(int)obuf[3]);
}

/*
Fills chk[] with bytes of the checksum of data, some null-terminated data buffer.
*/
void setDataChecksum(struct Packet* pkt)
{
	int checksum = getDataChecksum(pkt);
	lintToBytes(checksum,pkt->dataChecksum);
}

//Sets the socket timeout
void setSocketTimeout(int sockfd, int timeout_s, int timeout_us)
{	
	struct timeval tv;

	tv.tv_sec = timeout_s;  // second timeout
	tv.tv_usec = timeout_us;  // us timeout

	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&tv,sizeof(struct timeval));
}

/*
Constructs a packet from data by:
	-cleaning old packet contents (freeing old data)
	-allocing new data in packet, and copying in 
	-setting ack
	-setting name (just for debugging)
	-checksumming data and placing this in the checksum field
	
To make an empty packet (such as ACK/NACK packets), pass NULL or 0 for "char* data" param.
*/
void makePacket(int seqnum, int ack, byte* data, struct Packet* pkt)
{
	int dataLen, checksum, i;
	cleanPacket(pkt);

	//testByteConversion();

	//set the sequence number	
	lintToBytes(seqnum,pkt->seqnum);

	//copy in the data, if any
	if(data != 0){
		dataLen = strnlen((char*)data,PKT_DATA_MAX_LEN);
		lintToBytes(dataLen, pkt->dataLen);
		if(dataLen > PKT_DATA_MAX_LEN - 32){
			printf("ERROR length of data too long in makePacket: %d\r\n",dataLen);
		}
		strncpy((char*)pkt->data,data,dataLen);
		printf("datalen=%d\r\n",dataLen);
	}
	else{
		lintToBytes(0,pkt->dataLen);
		memset((void*)pkt->data,0,PKT_DATA_MAX_LEN);
	}
	
	//do the data checksum; must be done before the header checksum
	setDataChecksum(pkt);
	
	//set the ack field (also must be done before cksum)
	pkt->ack = ack == ACK ? (byte)ACK : (byte)NACK;
	
	//an apparent sequence of bytes, when viewed in wireshark
	pkt->name[0] = 'Z';
	pkt->name[1] = 'Z';
	pkt->name[2] = 'Z';
	pkt->name[3] = 'Z';
	
	//must be done only after data checksum is set
	checksum = getHeaderChecksum(pkt);
	lintToBytes(checksum,pkt->hdrChecksum);

	printf("pkt source data: seqnum=%d ACK=%d data=%s\r\n",seqnum,ack,(char*)data);
	printPacket(pkt);
}

void printRawPacket(const struct Packet* pkt)
{
	int i;

	printf("The packet, in byte order:\r\n");
	for(i = 0; i < (sizeof(struct Packet) - 65000); i++){
		printf("%0X ",(int)((char*)pkt)[i]);
		if(i % 8 == 7)
			printf("\r\n");
	}
}

void printPacket(const struct Packet* pkt)
{
	//char buf[256];
	printf("ACK:  %d\r\n",(int)pkt->ack);
	printf("SEQ:  %d %d %d %d\r\n",pkt->seqnum[0],pkt->seqnum[1],pkt->seqnum[2],pkt->seqnum[3]);
	printf("DLEN: %d %d %d %d\r\n",pkt->dataLen[0],pkt->dataLen[1],pkt->dataLen[2],pkt->dataLen[3]);
	printf("HSUM: %d %d %d %d\r\n",pkt->hdrChecksum[0],pkt->hdrChecksum[1],pkt->hdrChecksum[2],pkt->hdrChecksum[3]);
	printf("NAME: %d %d %d %d\r\n",pkt->name[0],pkt->name[1],pkt->name[2],pkt->name[3]);
	printf("DSUM: %d %d %d %d\r\n",pkt->dataChecksum[0],pkt->dataChecksum[1],pkt->dataChecksum[2],pkt->dataChecksum[3]);
	printf("DATA: %s\r\n",pkt->data);
	//gets(buf);
}

/*
Top level function for sending some file/stream. This implements the Kurose/Ross state rdt3.0 machine.
*/
void SendFile(FILE* fptr, int sock, struct sockaddr_in* sin)
{
	int seqnum;
	int slen;
	char buf[128];
	
	memset(buf,0,128);

	//set socket options; this assumes its safe to overwrite any previous socket options!
	//also: this is a requirement of the client state machine, which isn't apparent at this level. clean this if code is reused.
	setSocketTimeout(sock,0,250000); // sets a 0.25s max wait time for ACK receipt

	/* main loop: get and send lines of text */
	seqnum = 0;
	while(fgets(buf, 80, fptr) != NULL){
		slen = strnlen(buf,80);
		buf[slen] ='\0';

		SendData(sock,sin,seqnum,(byte*)buf);
		
		//update seqnum, which in this case just alternates between 0 and 1
		seqnum++;
		seqnum %= 2;
	}	
	
	printf("SEND COMPLETED!\r\n");
}


/*
Must make sure the transmission delay is larger than the propagation delay.
See Kurose on Ethernet.
Derivation of 51.2 us.
Propagation delay constraint is why ethernet must be short range.


	Top level client function for sending a single packet. This implements the
	stop-and-wait protocol state machine described by Kurose and Ross in figure 3.10:
		
	client calls CliSend():
		1) make and send a packet (with checksum)
		2) wait for ACK/NACK:
			if NACK:
			 goto 2
			if ACK:
			  goto 3
		3) return
*/
int SendData(int sock, struct sockaddr_in* addr, int seqnum, byte* data)
{
	int response, retries, failure;
	int sendSuccessful;
	int state;
	//TODO: These belong in some c++ class
	const int SENDING = 1;
	const int AWAIT_ACK = 2;

	//the packet for sending data
	struct Packet txPkt;
	//the packet in which to receive ACK/NACK messages, exclusively
	struct Packet ackPkt;
	
	memset((void*)&txPkt,0,sizeof(struct Packet));
	memset((void*)&ackPkt,0,sizeof(struct Packet));
	
	makePacket(seqnum,ACK,data,&txPkt);

	state = SENDING;
	sendSuccessful = FALSE;
	failure = FALSE;
	
	//The state machine for sending a single packet: send until a positive ACK is received
	retries = 0;
	while(sendSuccessful != TRUE && retries < MAX_RETRY_COUNT && failure == FALSE){

		//send this packet
		if(state == SENDING){
			sendPacket(&txPkt,sock,addr);
			state = AWAIT_ACK;
		}
		//await packet acknowledgment from receiver				
		else if(state == AWAIT_ACK){
			//block with timeout for ACK/NACK
			response = awaitAck(sock,addr,seqnum,&ackPkt);
			switch(response){
				case ACK:
					sendSuccessful = TRUE;
				break;
				
				//for all failure cases, just return to send state to re-send
				//TODO: add retry-count limit
				case NACK:
					retries++;
					state = SENDING;
					break;
				case CORRUPT:
					retries++;
					state = SENDING;
					break;
				case TIMEOUT:
					retries++;
					state = SENDING;
					break;
				
				default:
					failure = TRUE;
					printf("ERROR unmapped state result from _awaitAck function\r\n");
					break;
			}
		}
		//should be unreachable		
		else{
				printf("ERROR unmapped state in _send()\r\n");
		}
	}
	
	return sendSuccessful && !failure;
}

/*
Blocks until we receive an ACK packet from the destination, or timeout occurs.
This implements the wait-for-ack state. We call recvfrom (without blocking) until
we receive an uncorrupted ACK, NACK, or until a timout occurs.


Precondition: This function expects that sockfd is a socket with a timeout (socket for which
setsockopts has been called). The intended behavior is for recvfrom to block with a timeout.

Returns: ACK, NACK, TIMEOUT.
*/
int awaitAck(int sock, struct sockaddr_in* addr, int seqnum, struct Packet* ackPkt)
{
	int rxed, result;
	int sock_len = sizeof(struct sockaddr_in);
	char buf[RXTX_BUFFER_SIZE];
	int ack = NACK;
	
	memset(buf,0,RXTX_BUFFER_SIZE);
	
	printf("waiting for ack with seqnum=%d...\r\n",seqnum);

	//block until we receive an ACK packet, or timeout occurs (returns -1)
	rxed = recvfrom(sock,buf,RXTX_BUFFER_SIZE-1, 0, (struct sockaddr *)addr, &sock_len);
	
	//either a packet was received, or timeout occurred (other errors also possible, but timeout is most likely if packet was dropped)
	if(rxed > 0){
		//received packet: extract the received packet and proceed to check its validity
		deserializePacket(buf,ackPkt);
		
		//printPacket(ackPkt);
		//getchar();

		//check the packet's status
		//if(!isCorruptPacket(ackPkt)){
			if(isSequentialAck(ackPkt,seqnum)){
				printf("Successfully received ACK packet\r\n");
				result = ACK;
			}
			else{
				result = NACK;
				printf("NACK or incorrect seqnum received: pkt->seqnum=%d expected: %d ; pkt->ack=%s",bytesToLint(ackPkt->seqnum),seqnum, (ackPkt->ack == ACK ? "ACK" : "NACK"));
			}
		/*}
		else{
			printf("ERROR corrupt packet received. This should not be reachable in assignment's network conditions\r\n");
			//just flag it as a NACK to signal failure
			result = NACK;
		}*/
	}
	else{
		//a return of -1 and any of these errno's indicates SO_RCVTIMEO (socket timeout) according to linux.die.net/man/7/socket
		if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS){
			printf("WARN timeout waiting for packet ACK\r\n");
			result = TIMEOUT;
		}
		else{
			//all other errors are unexpected, right now
			printf("ERROR socket foo recvfrom returned -1 with unmapped errno=%d\r\n%s",(int)errno,strerror(errno));
			//This result isn't mapped or expected in this assignment; just return NACK
			result = NACK;
		}
	}
	
	return result;
}

void cleanPacket(struct Packet* pkt)
{
	memset((void*)pkt,0,sizeof(struct Packet));
}

int getPacketSize(struct Packet* pkt)
{
  return bytesToLint(pkt->dataLen) + PKT_HEADER_SIZE;
}

/*
The raw send utility. Serializes a packet into the provided sendBuf, then sends
it over udp.
*/
void sendPacket(struct Packet* pkt, int sock, struct sockaddr_in * sin)
{
	int pktLen;
	byte sendBuf[RXTX_BUFFER_SIZE];

	memset(sendBuf,0,RXTX_BUFFER_SIZE);
	
	serializePacket(pkt,sendBuf);
	pktLen = getPacketSize(pkt);

	if(pktLen > (PKT_DATA_MAX_LEN - 32)){
		printf("WARN possible packet data overrun: pkt data size > %d\r\n",(PKT_DATA_MAX_LEN-32));
	}
	
	if(sendto(sock, sendBuf, pktLen+1, 0, (struct sockaddr *)sin, sizeof(struct sockaddr)) < 0){
		perror("SendTo Error\n");
		exit(1);
	}
}



