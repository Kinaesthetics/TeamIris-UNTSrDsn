
/******************************************************************************
 *                                                                            *
 *  Texas Space Grant Consortium in collaboration with NASA                   *
 *   TDC-30-F16: Dynamic Addressing of IPv6 in Space                          *
 *  University of North Texas                                                 *
 *   CSCE4910 Senior Design Project I                                         *
 *  Team IRIS                                                                 *
 *   2016-Dec-15                                                              *
 *  SrDsn_DHCP_Server_Current.c                                               *
 *  																		                                      *
 *	Compile Command:  gcc SrDsn_DHCP_Server_Current.c -lm -lpthread -o Server *
 *	Execute Command:  ./Server                                                *
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
#include <unistd.h>	//for IP
#include <sys/ioctl.h>	//for IP
#include <net/if.h>
#include <arpa/inet.h>
#include <pthread.h>
#define DELAY 5000000  //delay in microseconds
#define NUM_CLIENTS 2 //Max number of clients.

pthread_t tID[NUM_CLIENTS]; //Thread ID

//dhcp_packet arguments structure definition
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

//Function prototypes
void printPacket(DHCP_packet *tempDHCP_Packet);
void logIt(FILE *fp, char *string, int thread_ID);
void packetInitialization(DHCP_packet *discovery, DHCP_packet *request,
	DHCP_packet *offer, DHCP_packet *ack);
void buildOffer(DHCP_packet *offer_ptr);
void buildACK(DHCP_packet *ack_ptr);


//Argument structure for connectionHandler()
struct arg_struct{
	int ID;
	int conn_fd;
	FILE *logFile;
};

void *connectionHandler(void *arguments){
	struct arg_struct *args = arguments;
	pthread_t id = pthread_self();
	int i;
	
	//Get thread ID for logging purposes.
	for(i = 0; i < NUM_CLIENTS; i++){
		if(pthread_equal(id, tID[i]))
			args->ID = i;
	}
	
	DHCP_packet dhcp, ack, offer, request, discovery;
	DHCP_packet *dhcp_ptr = &dhcp;  //Pointer to DHCP packet
	DHCP_packet *ack_ptr = &ack;	//Pointer to ACK
	DHCP_packet *offer_ptr = &offer;	//Pointer to Offer
	DHCP_packet *request_ptr = &request; 	//Pointer to request
	DHCP_packet *discovery_ptr = &discovery;	//Pointer to Discovery
	
	int conn_fd = args->conn_fd;
	int recv_size;
	
	packetInitialization(discovery_ptr, request_ptr, offer_ptr, ack_ptr);
	
	// wait for Discovery
	recv_size = recv(conn_fd, &discovery, sizeof(discovery), 0);
	printf("Discovery received\n");
	logIt(args->logFile, "Discovery Received\n", args->ID);
	printPacket(discovery_ptr);
	
  /*Psuedo Code************************
  test = parsePacket(discovery);
  
  if (test)
  	buildOffer();
    send(offer);
  else
  	send(conn_fd, &gtfo, sizeof(gtfo));
  
  //I assume we dont always send and build packets no matter what.
  //Therefore, probably need to parse packets that are incoming.
  
  ************************************/
  		//-----------------------------------------------------
  		//-----------------------------------------------------
  		//ALEX'S ATTEMPT AT PARSING A PACKET 
  		//-----------------------------------------------------
  		//-----------------------------------------------------
  
	//Build Offer packet
	buildOffer(offer_ptr);
	
	//Send Offer Packet
    printf("Offer Packet sent\n");
	logIt(args->logFile, "Offer Packet Sent\n", args->ID);
	printPacket(offer_ptr);
	usleep(DELAY); //Add delay
	if(send(conn_fd, &offer, sizeof(offer), 0) < 0){
		printf("!!ERROR!!! Cannot write to socket\n");
		exit(1);
	}

	// wait for Request
	recv_size = recv(conn_fd, &request, sizeof(request), 0);
	printf("Requst Received\n");
	logIt(args->logFile, "Request Received\n", args->ID);
	printPacket(request_ptr);
	
	// Build ACK DHCP Packet
	buildACK(ack_ptr);
	
	//Send ACK Packet
	printf("Ack Packet Sent\n");
	logIt(args->logFile, "ACK Packet Sent\n", args->ID);
	printPacket(ack_ptr);
	usleep(DELAY); //Add delay
	if(send(conn_fd, &ack, sizeof(ack), 0) < 0){
		printf("!!ERROR!!!\n Cannot write to socket\n");
		exit(1);
	}
		
	recv_size = recv(conn_fd, &dhcp, sizeof(dhcp), 0);
	if(recv_size == 0){
		puts("Client Disconnected");
		logIt(args->logFile, "Client Disconnected.\n", args->ID);
	}
		
	close(conn_fd);
}


void printPacket(DHCP_packet *tempDHCP_Packet){
    printf("---------------------\n| OP|HTYPE|HLEN|HOPS|\n---------------------\n");
    printf("| %02X | %02X | %02X | %0X |\n", tempDHCP_Packet->op, tempDHCP_Packet->hType, tempDHCP_Packet->hLen,tempDHCP_Packet->hOps);
    printf("---------------------\n|        XID        |\n---------------------\n");
    printf("| %16X  |\n", tempDHCP_Packet->xID);
    printf("---------------------\n|  SECS   |  FLAGS  |\n---------------------\n");
    printf("|%08X |%08X |\n", tempDHCP_Packet->secs, tempDHCP_Packet->flags);
    printf("---------------------\n|      CIADDR       |\n---------------------\n");
    printf("| %16X  |\n", tempDHCP_Packet->ciaddr);
    printf("---------------------\n|      YIADDR       |\n---------------------\n");
    printf("| %16X  |\n", tempDHCP_Packet->yiaddr);
    printf("---------------------\n|      SIADDR       |\n---------------------\n");
    printf("| %16X  |\n", tempDHCP_Packet->giaddr);
    printf("---------------------\n|       GIADD       |\n---------------------\n");
    printf("| %16X  |\n", tempDHCP_Packet->chaddr);
    printf("---------------------\n|       CHADDR      |\n---------------------\n");
    printf("| %16X  |\n", tempDHCP_Packet->magicCookie);
    printf("---------------------\n|    Magic Cookie   |\n---------------------\n");
    printf("| %16X  |\n", tempDHCP_Packet->options);
    printf("---------------------\n|       Option      |\n---------------------\n\n\n");
}

void logIt(FILE *fp, char *string, int thread_ID){
	
	char buffer[26];
	int millisec;
	struct tm* tm_info;
	struct timeval tv;
	
	gettimeofday(&tv, NULL);
	
	millisec = lrint(tv.tv_usec/1000.0); // Round to nearest millisec
	if (millisec>=1000){ // Allow for rounding up to nearest second
		millisec -=1000;
		tv.tv_sec++;
	}
	
	tm_info = localtime(&tv.tv_sec);
	
	strftime(buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);
	
	fprintf(fp, "Thread %d: SERVER:\t\t%s.%03d\t\t%s", thread_ID, buffer, millisec, string);
}


void packetInitialization(DHCP_packet *discovery, DHCP_packet *request,
	DHCP_packet *offer, DHCP_packet *ack){
	
	// Initiallize all packets to F for error detection.
	//Discovery
    discovery->op = 0xFF; //op
    discovery->hType = 0xFF; //hType
    discovery->hLen = 0xFf; //hLen
    discovery->hOps = 0xFF; //hOps
    discovery->xID = 0xFFFFFFFF; //xID
    discovery->secs = 0xFFFF; //secs
    discovery->flags = 0xFFFF; //flags
    discovery->ciaddr = 0xFFFFFFFF; //ciaddr
    discovery->yiaddr = 0xFFFFFFFF; //yiaddr
    discovery->siaddr = 0xFFFFFFFF; //siaddr
    discovery->giaddr = 0xFFFFFFFF; //giaddr
    discovery->chaddr = 0xFFFFFFFF; //chaddr
    discovery->magicCookie = 0xFFFFFFFF; //magicCookie
    discovery->options = 0xF;	//options

	//Request
	request->op = 0xFF; //op
    request->hType = 0xFF; //hType
    request->hLen = 0xFf; //hLen
    request->hOps = 0xFF; //hOps
    request->xID = 0xFFFFFFFF; //xID
    request->secs = 0xFFFF; //secs
    request->flags = 0xFFFF; //flags
    request->ciaddr = 0xFFFFFFFF; //ciaddr
    request->yiaddr = 0xFFFFFFFF; //yiaddr
    request->siaddr = 0xFFFFFFFF; //siaddr
    request->giaddr = 0xFFFFFFFF; //giaddr
    request->chaddr = 0xFFFFFFFF; //chaddr
    request->magicCookie = 0xFFFFFFFF; //magicCookie
    request->options = 0xF;	//options
	
	//Offer
	offer->op = 0xFF; //op
    offer->hType = 0xFF; //hType
    offer->hLen = 0xFf; //hLen
    offer->hOps = 0xFF; //hOps
    offer->xID = 0xFFFFFFFF; //xID
    offer->secs = 0xFFFF; //secs
    offer->flags = 0xFFFF; //flags
    offer->ciaddr = 0xFFFFFFFF; //ciaddr
    offer->yiaddr = 0xFFFFFFFF; //yiaddr
    offer->siaddr = 0xFFFFFFFF; //siaddr
    offer->giaddr = 0xFFFFFFFF; //giaddr
    offer->chaddr = 0xFFFFFFFF; //chaddr
    offer->magicCookie = 0xFFFFFFFF; //magicCookie
    offer->options = 0xF;	//options
	
	//ACK
	ack->op = 0xFF; //op
    ack->hType = 0xFF; //hType
    ack->hLen = 0xFf; //hLen
    ack->hOps = 0xFF; //hOps
    ack->xID = 0xFFFFFFFF; //xID
    ack->secs = 0xFFFF; //secs
    ack->flags = 0xFFFF; //flags
    ack->ciaddr = 0xFFFFFFFF; //ciaddr
    ack->yiaddr = 0xFFFFFFFF; //yiaddr
    ack->siaddr = 0xFFFFFFFF; //siaddr
    ack->giaddr = 0xFFFFFFFF; //giaddr
    ack->chaddr = 0xFFFFFFFF; //chaddr
    ack->magicCookie = 0xFFFFFFFF; //magicCookie
    ack->options = 0xF;	//options

}

void buildOffer(DHCP_packet *offer_ptr){
	// Build Offer DHCP Packet
	offer_ptr->op = 0x02; //op
	offer_ptr->hType = 0x01; //hType
	offer_ptr->hLen = 0x06; //hLen
	offer_ptr->hOps = 0x00; //hOps
	offer_ptr->xID = 0x3903F326; //xID
	offer_ptr->secs = 0x0000; //secs
	offer_ptr->flags = 0x0000; //flags
	offer_ptr->ciaddr = 0x00000000; //ciaddr
	offer_ptr->yiaddr = 0xC0A80164; //yiaddr
	offer_ptr->siaddr = 0x00000000; //siaddr
	offer_ptr->giaddr = 0x00000000; //giaddr
	offer_ptr->chaddr = 0x00053C04; //chaddr
	offer_ptr->magicCookie = 0x63825363; //magicCookie
	offer_ptr->options = 0;	//options
}

void buildACK(DHCP_packet *ack_ptr){
	ack_ptr->op = 0x02; //op
	ack_ptr->hType = 0x01; //hType
	ack_ptr->hLen = 0x06; //hLen
	ack_ptr->hOps = 0x00; //hOps
	ack_ptr->xID = 0x3903F326; //xID
	ack_ptr->secs = 0x0000; //secs
	ack_ptr->flags = 0x0000; //flags
	ack_ptr->ciaddr = 0x00000000; //ciaddr
	ack_ptr->yiaddr = 0xC0A80164; //yiaddr
	ack_ptr->siaddr = 0x00000000; //siaddr
	ack_ptr->giaddr = 0x00000000; //giaddr
	ack_ptr->chaddr = 0x00053C04; //chaddr
	ack_ptr->magicCookie = 0x63825363; //magicCookie
	ack_ptr->options = 0;	//options
}

//--------------MAIN----------------//
int main(int argc, char *argv[]){
    int i = 0, test = 0, err;
	struct arg_struct args;

    int listen_fd, conn_fd, newsockfd, portnum, clilen;
    pid_t p_id;
    struct sockaddr_in servaddr, cli_addr;
    int recv_size, pid;
    
	FILE *logFile;
	logFile = fopen( "ServerLog.txt", "w");
	if (!logFile){
		perror("!!ERROR!!!\nCannont open log file");
		exit(1);
	}
	args.logFile = logFile;
	
 

	//GETTING CURRENT IP ADDRESS
	int gettingMyIP;
	struct ifreq ifr;
	
	gettingMyIP = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
	ioctl(gettingMyIP, SIOCGIFADDR, &ifr);
	close(gettingMyIP);
//	printf("WOO WOO IP IS THIS: %s \n", inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

	//Create Socket
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);//socket is made
	if(listen_fd < 0){
		printf("!!!ERROR!!! Cannot open socket");
		return 1;
    }
	  
	memset(&servaddr, '0', sizeof(servaddr));  
	
	//Prepare the sockaddr_in structure
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	srand(time(NULL));	//seed random var (ALWAYS LET THIS RUN BEFORE rand(), OR IT WILL ONLY GIVE ONE VALUE FOREVER)
	
	//Get an open port number
	while(1){
		


		portnum = rand() % 49150;
    //Sadly, this is the easiest way I've found to get a range on a random number. 
    while(portnum < 1024) // doing this bc it can not get higher than 49150, this way it will also be above the min
    {
      portnum = rand() % 49150;
    }		
		servaddr.sin_port = htons(portnum);
 
		/* Binds the above details to the socket */
		if(bind(listen_fd,  (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
			fprintf(stderr,"!!!ERROR!!! Port number is taken. Choosing another port number.\n");
        }
		else{
			printf("\nSocket Created\n");
                        printf("Server IP Address is %s \n", inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
                        printf("Server Port Number is %d\n", portnum);
			break;
		}
    } //END RANDOM NUM GEN ON PORT
	
	/* Start listening to incoming connections */
	if(listen(listen_fd, 10) < 0)
		printf("!!!ERROR!!! Cannot listen");
	puts("Waiting for incoming connections...");
	logIt(logFile, "Waiting for incoming connections...\n", 0);
	
	clilen = sizeof(struct sockaddr_in);

		
	/* Accepts an incoming connection */
	/**Threaded**/
	while(conn_fd = accept(listen_fd, (struct sockaddr*)&cli_addr, (socklen_t*)&clilen)){
		args.conn_fd = conn_fd;

		err = pthread_create(&(tID[i]), NULL, &connectionHandler, (void *) &args);
		if(err != 0)
			printf("!!!ERROR!!! Cannot create thread.");
		
		i++;
		if(i > NUM_CLIENTS)
			break;
	}

	if(conn_fd < 0){
		fprintf(stderr,"!!!ERROR!!! Cannot accept\n");
		return 1;
	}
	
	//Clean up
	fclose(logFile);
	
	return 0;
}
