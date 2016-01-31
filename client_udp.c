#include "common.h"

int main(int argc, char * argv[])
{
	FILE *fp;
	struct hostent *hp;
	struct sockaddr_in sin;
	char *host;
	char *fname;
	char buf[MAX_LINE];
	int s;
	int slen;

	if (argc==3) {
		host = argv[1];
		fname= argv[2];
	}
	else {
		fprintf(stderr, "Usage: ./client_udp host filename\n");
		exit(1);
	}
	/* translate host name into peer’s IP address */
	if (!hp) {
		fprintf(stderr, "Unknown host: %s\n", host);
		exit(1);
	}

	fp = fopen(fname, "r");
	if (fp==NULL){
		fprintf(stderr, "Can't open file: %s\n", fname);
		exit(1);
	}

	hp = gethostbyname(host);
	/* build address data structure */
	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = htons(SERVER_PORT);

	/* active open */
	if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Socket");
		exit(1);
	}

	socklen_t sock_len= sizeof sin;

	printf("Sending file\r\n");
	SendFile(fp,s,&sin);
	
	/*
	// main loop: get and send lines of text 
	while(fgets(buf, 80, fp) != NULL){
		slen = strlen(buf);
		buf[slen] ='\0';
		
		if(sendto(s, buf, slen+1, 0, (struct sockaddr *)&sin, sock_len) < 0){
			perror("SendTo Error\n");
			exit(1);
		}

	}
	*/
	*buf = 0x02;	
        if(sendto(s, buf, 1, 0, (struct sockaddr *)&sin, sock_len)<0){
		perror("SendTo Error\n");
		exit(1);
	}
	fclose(fp);
}


