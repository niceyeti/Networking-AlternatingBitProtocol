//wraps SendData calls, implementing the alternating bit protocol
void SendFile(f* fptr, int sock, struct sockaddr_in* sin);
int SendData(int sock, struct sockaddr_in* addr, int seqnum, char* data);
void MakePacket(int seqnum, int ack, char* data, struct Packet* pkt);