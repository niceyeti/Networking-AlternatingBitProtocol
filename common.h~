#define ACK 0
#define NACK 1
#define PKT_DATA_MAX_LEN 65535
#define RXTX_BUFFER_SIZE PKT_DATA_MAX_LEN + 64
#define CORRUPT 1 
#define NOT_CORRUPT 0
#define U16_PRIME 65497
#define TRUE 1
#define FALSE 0

typedef unsigned char byte;

//Let all U16's, etc, be represented by byte buffers of length mod 2; this makes it easy to htons/htonl, etc
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

void serializePacket(struct Packet* pkt, byte buf[RXTX_BUFFER_SIZE]);
void deserializePacket(const char buf[RXTX_BUFFER_SIZE], struct Packet* pkt);
int isAck(struct Packet* pkt);
int isSequentialAck(struct Packet* pkt, int seqnum);
int getHeaderChecksum(struct Packet* pkt);
int getDataChecksum(struct Packet* pkt);
int isCorruptPacket(struct Packet* pkt);
void bytesToLint(const byte buf[4], int* checksum);
void lintToBytes(const int i, byte obuf[4]);
void setDataChecksum(byte chk[], byte* data);
void setSocketTimeout(int sockfd, int timeout_s);
void makePacket(int seqnum, int ack, byte* data, struct Packet* pkt);
void SendFile(f* fptr, int sock, struct sockaddr_in* sin);
int SendData(int sock, struct sockaddr_in* addr, int seqnum, byte* data);
int awaitAck(int sock, struct sockaddr_in* addr, int seqnum, struct Packet* ackPkt);
void cleanPacket(struct Packet* pkt);
void sendPacket(struct Packet* pkt, int sock, struct sockaddr_in * sin);