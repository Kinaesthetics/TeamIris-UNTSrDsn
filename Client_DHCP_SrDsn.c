
/******************************************************************************
 *                                                                            *
 *  Texas Space Grant Consortium in collaboration with NASA                   *
 *   TDC-30-F16: Dynamic Addressing of IPv6 in Space                          *
 *  University of North Texas                                                 *
 *   CSCE4910 Senior Design Project I                                         *
 *  Team IRIS                                                                 *
 *   2016-Dec-15                                                              *
 *  SrDsn_DHCP_Client_Current.c                                               *
 *       																	  *
 *	Compile Command:	gcc Client_DHCP_SrDsn.c -lm -o Client		          *
 *	Execute Command:	./Client <IP_Address> <PORT_NUM>					  *
 *                                                                            *
 ******************************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#define DELAY 5000000  //delay in microseconds

//dhcp_packet structure definition
struct dhcp_packet{
    unsigned short int op;       //operation code (0-7)
    unsigned short int hType;    //hardware type (8-15)
    unsigned short int hLen;     //hardware address length (16-23)
    unsigned short int hOps;     //hops(24-31)
    unsigned int xID;            //transaction identifier
    unsigned short int secs;     //seconds (0-15)
    unsigned short int flags;    //flags (16-31)
    unsigned int ciaddr;         //client IP address
    unsigned int yiaddr;         //"your" IP address
    unsigned int siaddr;         //"server" IP address
    unsigned int giaddr;         //"gateway" IP address
    unsigned int chaddr;         //client hardware address (16 bytes)
    unsigned int magicCookie;    //https://en.wikipedia.org/wiki/Magic_cookie
    unsigned int options;        //options (different sizes)
}; //end of structure dhcp_packet

typedef struct dhcp_packet DHCP_packet;

void setDHCP_Packet(DHCP_packet *temp,
	unsigned short int op, 
	unsigned short int hType,
	unsigned short int hLen,
	unsigned short int hOps,
	unsigned int xID,
	unsigned short int secs,
	unsigned short int flags,
	unsigned int ciaddr,
	unsigned int yiaddr,
	unsigned int siaddr,
	unsigned int giaddr,
	unsigned int chaddr,
	unsigned int magicCookie,
	unsigned int options){
	
	temp->op = op;
	temp->hType = hType;
	temp->hLen = hLen;
	temp->hOps = hOps;
	temp->xID = xID;
	temp->op = secs;
	temp->flags = flags;
	temp->ciaddr = ciaddr;
	temp->yiaddr = yiaddr;
	temp->siaddr = siaddr;
	temp->giaddr = giaddr;
	temp->chaddr = chaddr;
	temp->magicCookie = magicCookie;
	temp->options = options;
}

void printPacket(DHCP_packet *tempDHCP_Packet){
    printf("-------------------\t-------------------\n");
	printf("OP|HTYPE|HLEN|HOPS");
    printf("\t| %02X | %02X | %02X | %0X |", tempDHCP_Packet->op, tempDHCP_Packet->hType, tempDHCP_Packet->hLen,tempDHCP_Packet->hOps);
	printf("\n-------------------\t-------------------\n");
    printf("XID");
	printf("\t\t\t| %16X |", tempDHCP_Packet->xID);
	printf("\n-------------------\t-------------------\n");
    printf("SECS|FLAGS");
    printf("\t\t| %08X | %08X |", tempDHCP_Packet->secs, tempDHCP_Packet->flags);
	printf("\n-------------------\t-------------------\n");
    printf("CIADDR");
    printf("\t\t\t| %16X |", tempDHCP_Packet->ciaddr);
	printf("\n-------------------\t-------------------\n");
    printf("YIADDR");
    printf("\t\t\t| %16X |", tempDHCP_Packet->yiaddr);
	printf("\n-------------------\t-------------------\n");
    printf("SIADDR");
    printf("\t\t\t| %16X |", tempDHCP_Packet->siaddr);
	printf("\n-------------------\t-------------------\n");
    printf("GIADDR");;
    printf("\t\t\t| %16X |", tempDHCP_Packet->giaddr);
	printf("\n-------------------\t-------------------\n");
    printf("CHADDR");
    printf("\t\t\t| %16X |", tempDHCP_Packet->chaddr);
	printf("\n-------------------\t-------------------\n");
    printf("Magic Cookie");
    printf("\t\t| %16X |", tempDHCP_Packet->magicCookie);
	printf("\n-------------------\t-------------------\n");
    printf("Option");
	printf("\t\t\t| %16X |", tempDHCP_Packet->options);
	printf("\n-------------------\t-------------------\n");
}

void logIt(FILE *fp, char *string){
	
	char buffer[26];
	int millisec;
	struct tm* tm_info;
	struct timeval tv;
	
	gettimeofday(&tv, NULL);
	
	millisec = lrint(tv.tv_usec/1000.0); // Round to nearest millisec
	if (millisec>=1000) { // Allow for rounding up to nearest second
		millisec -=1000;
		tv.tv_sec++;
	}
	
	tm_info = localtime(&tv.tv_sec);
	
	strftime(buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);
	
	fprintf(fp, "CLIENT:\t\t%s.%03d\t\t%s", buffer, millisec, string);
}

int main(int argc, char *argv[]){
	char* ipAddress =NULL;
	DHCP_packet dhcp, ack, offer, request, discovery;
	DHCP_packet *dhcp_ptr = &dhcp;  //Pointer to DHCP packet
	DHCP_packet *ack_ptr = &ack;	//Pointer to ACK
	DHCP_packet *offer_ptr = &offer;	//Pointer to Offer
	DHCP_packet *request_ptr = &request; 	//Pointer to request
	DHCP_packet *discovery_ptr = &discovery;	//Pointer to Discovery
	
    int i, sockfd, portnum, recv_size, delay;
    int len = sizeof(struct sockaddr);
    struct sockaddr_in servaddr;
    struct hostent *server;
    FILE * logFile;
    size_t size = 0;

    if(argc < 3){
        fprintf(stderr, "!!ERROR!!!\nPlease enter an IP Address and port number as an argument.\n");
        exit(1);
    }
  else if(argc ==3) {
        ipAddress = argv[1];
      	portnum = strtol(argv[2], NULL, 10);
    }
	
	logFile = fopen( "ClientLog.txt", "w");
    if (!logFile){
        perror("!!ERROR!!!\nCannont open log file");
        exit(1);
    }

  // Initiallize all packets to F for error detection.
  setDHCP_Packet(discovery_ptr,
	0xFF, //op
	0xFF, //hType
	0xFF, //hLen
	0xFF, //hOps
	0xFFFFFFFF, //xID
	0xFFFF, //secs
	0xFFFF, //flags
	0xFFFFFFFF, //ciaddr
	0xFFFFFFFF, //yiaddr
	0xFFFFFFFF, //siaddr
	0xFFFFFFFF, //giaddr
	0xFFFFFFFF, //chaddr
	0xFFFFFFFF, //magicCookie
	0xF	//options
	);
  setDHCP_Packet(request_ptr,
	0xFF, //op
	0xFF, //hType
	0xFF, //hLen
	0xFF, //hOps
	0xFFFFFFFF, //xID
	0xFFFF, //secs
	0xFFFF, //flags
	0xFFFFFFFF, //ciaddr
	0xFFFFFFFF, //yiaddr
	0xFFFFFFFF, //siaddr
	0xFFFFFFFF, //giaddr
	0xFFFFFFFF, //chaddr
	0xFFFFFFFF, //magicCookie
	0xF	//options
	);
  setDHCP_Packet(offer_ptr,
	0xFF, //op
	0xFF, //hType
	0xFF, //hLen
	0xFF, //hOps
	0xFFFFFFFF, //xID
	0xFFFF, //secs
	0xFFFF, //flags
	0xFFFFFFFF, //ciaddr
	0xFFFFFFFF, //yiaddr
	0xFFFFFFFF, //siaddr
	0xFFFFFFFF, //giaddr
	0xFFFFFFFF, //chaddr
	0xFFFFFFFF, //magicCookie
	0xF	//options
	);
  setDHCP_Packet(offer_ptr,
	0xFF, //op
	0xFF, //hType
	0xFF, //hLen
	0xFF, //hOps
	0xFFFFFFFF, //xID
	0xFFFF, //secs
	0xFFFF, //flags
	0xFFFFFFFF, //ciaddr
	0xFFFFFFFF, //yiaddr
	0xFFFFFFFF, //siaddr
	0xFFFFFFFF, //giaddr
	0xFFFFFFFF, //chaddr
	0xFFFFFFFF, //magicCookie
	0xF	//options
	);
  
	//Create Socket
    // AF_INET - IPv4 IP , Type of socket, protocol
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("!!ERROR!!!\nCannont open socket");
        return 1;
    }
	puts("Socket Created");
    
	//Server IP
	servaddr.sin_addr.s_addr = inet_addr(ipAddress);
    servaddr.sin_family = AF_INET;
	
	//Server port
    servaddr.sin_port = htons(portnum); 
 
    
    // Connect to the server 
    if(connect(sockfd,(struct sockaddr *)&servaddr, sizeof(servaddr)) <0){
        fprintf(stderr,"!!ERROR!!! Cannot connect\n");
        return 1;
    }

    // Build Discover DHCP Packet
    setDHCP_Packet(discovery_ptr,
		0x02, //op
		0x01, //hType
		0x06, //hLen
		0x00, //hOps
		0x3903F326, //xID
		0x0000, //secs
		0x0000, //flags
		0x00000000, //ciaddr
		0xC0A80164, //yiaddr
		0x00000000, //siaddr
		0x00000000, //giaddr
		0x00053C04, //chaddr
		0x63825363, //magicCookie
		0	//options
		);

	//Send Discovery Packet
	printf("\nDiscovery Packet sent.\n");
	logIt(logFile, "Discovery Packet sent\n");
//	printPacket(discovery_ptr);	//Commented out for testing
	usleep(DELAY); //Add delay
	if(send(sockfd, &discovery, sizeof(discovery), 0) < 0){
		printf("!!ERROR!!!\n Cannot write to socket\n");
		return 1;	
	}



    // wait for Offer
    recv_size = recv(sockfd, &offer, sizeof(offer), 0);
    printf("\nOffer received:\n");
	printPacket(offer_ptr);
	logIt(logFile, "Offer received\n");
	
	
    // Build Request DHCP Packet
    setDHCP_Packet(request_ptr,
		0x02, //op
		0x01, //hType
		0x06, //hLen
		0x00, //hOps
		0x3903F326, //xID
		0x0000, //secs
		0x0000, //flags
		0x00000000, //ciaddr
		0xC0A80164, //yiaddr
		0x00000000, //siaddr
		0x00000000, //giaddr
		0x00053C04, //chaddr
		0x63825363, //magicCookie
		0	//options
		);
  
	//Send Request Packet
	printf("\nRequest Packet sent.\n");
//	printPacket(request_ptr); //Commented out for testing
	logIt(logFile, "Request Packet sent\n");
	usleep(DELAY); //Add delay
    if(send(sockfd, &request, sizeof(request), 0) < 0){
		printf("!!ERROR!!!\n Cannot write to socket\n");
		return 1;
	}

    recv_size = recv(sockfd, &ack, sizeof(ack), 0);
    printf("\nAck Received\n");
    printPacket(ack_ptr);
	logIt(logFile, "Ack Received\n");

    if(recv_size == 0){
		puts("Server Disconnected");
		logIt(logFile, "Server Disconnected\n");
	}

	//Clean Up
	close(sockfd);
	fclose(logFile);
    return 0;
 }
