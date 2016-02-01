#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#define ACK 1
#define NACK 2
#define CORRUPT 3
#define NOT_CORRUPT 4
#define TIMEOUT 5


//TODO: get rid fo magic numbers and define maxes in terms of a single parameter, eg sizeof(struct Packet)
#define PKT_DATA_MAX_LEN 65535
#define RXTX_BUFFER_SIZE PKT_DATA_MAX_LEN + 64
#define PKT_HEADER_SIZE 21
#define U16_PRIME 65497
#define TRUE 1
#define FALSE 0
#define SERVER_PORT 5432
#define MAX_RX_LINE 256
#define MAX_LINE 80


typedef unsigned char byte;

//Let all U16's, etc, be represented by byte buffers of length mod 2; this makes it easy to htons/htonl, etc.
//Read the 4-byte buffers from left to right: so 3 == [0,0,0,0011]
struct Packet{
	//Let zero represent ACK
	byte ack;
    //This packet's sequence number (may not be used)
	byte seqnum[4];
	//Length of the data in this packet
	byte dataLen[4];
	//the header checksum
	byte hdrChecksum[4];
	//purely for id purposes, eg with wireshark
	byte name[4];
	//checksum over packet data
	byte dataChecksum[4];
	//Bytes allocated for this packet's data buffer. Its easier to statically allocate the data for now, instead of managing a ptr to dynamic mem.
	byte data[PKT_DATA_MAX_LEN];
};

void testByteConversion();
void serializePacket(struct Packet* pkt, byte buf[RXTX_BUFFER_SIZE]);
void deserializePacket(const char buf[RXTX_BUFFER_SIZE], struct Packet* pkt);
int isAck(struct Packet* pkt);
int isSequentialAck(struct Packet* pkt, int seqnum);
int getHeaderChecksum(struct Packet* pkt);
int getDataChecksum(struct Packet* pkt);
int isCorruptPacket(struct Packet* pkt);
int bytesToLint(const byte buf[4]);
void lintToBytes(const int i, byte obuf[4]);
void setDataChecksum(struct Packet* pkt);
void setSocketTimeout(int sockfd, int timeout_s);
void printPacket(const struct Packet* pkt);
void printRawPacket(const struct Packet* pkt);
void makePacket(int seqnum, int ack, byte* data, struct Packet* pkt);
void SendFile(FILE* fptr, int sock, struct sockaddr_in* sin);
int SendData(int sock, struct sockaddr_in* addr, int seqnum, byte* data);
int awaitAck(int sock, struct sockaddr_in* addr, int seqnum, struct Packet* ackPkt);
void cleanPacket(struct Packet* pkt);
void sendPacket(struct Packet* pkt, int sock, struct sockaddr_in * sin);
