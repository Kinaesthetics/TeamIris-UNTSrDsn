/******************************************************************************
 *                                                                            *
 *  Texas Space Grant Consortium in collaboration with NASA                   *
 *   TDC-30-F16: Dynamic Addressing of IPv6 in Space                          *
 *  University of North Texas                                                 *
 *   CSCE4910 Senior Design Project I                                         *
 *  Team IRIS                                                                 *
 *   2016-Dec-15                                                              *
 *  TDRS_SrDsn.c                                               				  *
 *       																	  *
 *	Compile Command:	gcc TDRS_SrDsn.c -lm -lpthread -o TDRS
 *	Execute Command:	./TDRS <ServerIP> <ServerPortNum>					  
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
 
 pthread_t tID; //Thread ID
 
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

//Argument structure for connectionHandler()
struct arg_struct{
	int sock_fd, conn_fd;
};

void *connectionHandler(void *arguments){
	struct arg_struct *args = arguments;
	
	DHCP_packet buff1, buff2;
	
	int sock_fd = args->sock_fd;
	int conn_fd = args->conn_fd;
	int recv_size;
	
	//receive from client
	recv_size = recv(conn_fd, &buff1, sizeof(buff1), 0);
	puts("received from client");
	//send to server
	if(send(sock_fd, &buff1, sizeof(buff1), 0) < 0){
		printf("!!ERROR!!! Cannot write to socket\n");
		exit(1);
	}
	puts("sent to server");
		
	//receive from server
	recv_size = recv(sock_fd, &buff2, sizeof(buff2), 0);
	puts("received from server");
	//send to client
	if(send(conn_fd, &buff2, sizeof(buff2), 0) < 0){
		printf("!!ERROR!!! Cannot write to socket\n");
		exit(1);
	}
	puts("sent to client");
		
	//receive from client
	recv_size = recv(conn_fd, &buff1, sizeof(buff1), 0);
	puts("received from client");
	//send to server
	if(send(sock_fd, &buff1, sizeof(buff1), 0) < 0){
		printf("!!ERROR!!! Cannot write to socket\n");
		exit(1);
	}
	puts("sent to server");	
	
	//receive from server
	recv_size = recv(sock_fd, &buff2, sizeof(buff2), 0);
	puts("received from server");
	//send to client
	if(send(conn_fd, &buff2, sizeof(buff2), 0) < 0){
		printf("!!ERROR!!! Cannot write to socket\n");
		exit(1);
	}
	puts("sent to client");
	
	return NULL;
}

int main(int argc, char *argv[]){
	
	int sock_fd, conn_fd, listen_fd, cli_len, portnum, err;
	char* ipAddress =NULL;
	
	int len = sizeof(struct sockaddr);
    struct sockaddr_in servaddr, tdrsaddr, clientaddr;
	struct arg_struct args;
	
	if(argc < 3){
        fprintf(stderr, "!!ERROR!!!\nPlease enter an IP Address and port number as an argument.\n");
        exit(1);
    }
	
	else if(argc == 3){
        ipAddress = argv[1];
      	portnum = strtol(argv[2], NULL, 10);
    }
	
	/*******************Connect to Server*******************/
	//Create Socket
    // AF_INET - IPv4 IP , Type of socket, protocol
    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("!!ERROR!!!\nCannont open socket");
        return 1;
    }
	args.sock_fd = sock_fd;
	puts("Server Socket Created");
	
	//Server IP
	servaddr.sin_addr.s_addr = inet_addr(ipAddress);
    servaddr.sin_family = AF_INET;
	
	//Server port
    servaddr.sin_port = htons(portnum); 
	
	// Connect to the server 
    if(connect(sock_fd,(struct sockaddr *)&servaddr, sizeof(servaddr)) <0){
        fprintf(stderr,"!!ERROR!!! Cannot connect to server.\n");
        return 1;
    }
	/*******************************************************/
	
	/*******************Starting a Server*******************/
	//GETTING CURRENT IP ADDRESS
	int gettingMyIP, tdrs_portnum;
	struct ifreq ifr;
	
	gettingMyIP = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
	ioctl(gettingMyIP, SIOCGIFADDR, &ifr);
	close(gettingMyIP);
	
	//Create Socket
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);//socket is made
	if(listen_fd < 0){
		printf("!!!ERROR!!! Cannot open socket");
		return 1;
    }
	
	memset(&tdrsaddr, '0', sizeof(tdrsaddr)); 
	
	//Prepare the sockaddr_in structure
	tdrsaddr.sin_family = AF_INET;
	tdrsaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	srand(time(NULL));	//seed random var
	
	//Get an open port number
	while(1){
		tdrs_portnum = rand() % 49150;
		while(tdrs_portnum < 1024)
			tdrs_portnum = rand() % 49150;
		tdrsaddr.sin_port = htons(tdrs_portnum);
		
		/* Binds the above details to the socket */
		if(bind(listen_fd,  (struct sockaddr *) &tdrsaddr, sizeof(tdrsaddr)) < 0)
			printf("!!!ERROR!!! Port number is taken. Choosing another port number.\n");
        else{
            printf("Server IP Address is %s \n", inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
            printf("Server Port Number is %d\n", tdrs_portnum);
			break;
		}
	}//End while loop.
	
	// Start listening to incoming connections 
	if(listen(listen_fd, 10) < 0)
		printf("!!!ERROR!!! Cannot listen\n");
	puts("Waiting for incoming connections...");
	
	cli_len = sizeof(struct sockaddr_in);
	
	while(conn_fd = accept(listen_fd, (struct sockaddr*)&clientaddr, (socklen_t*)&cli_len)){
		args.conn_fd = conn_fd;
		puts("Accepted incoming connection");
		//Create thread.
		err = pthread_create(&tID, NULL, &connectionHandler, (void *) &args);
	}
	/*******************************************************/

	return 0;
}