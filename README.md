# TeamIris-UNTSrDsn
Repository for UNT Senior Design Team Iris, (Fall 2016 - Spring 2017)

Run the programs in this order: Server->TDRS->Clients (up to 2 clients).

How to compile and run the...:
 
Server:
 Compile:  gcc SrDsn_DHCP_Server_Current.c -lm -lpthread -o Server
 Execute Command:  ./Server  
 
TDRS:
 Compile:  gcc TDRS_SrDsn.c -lm -lpthread -o TDRS
 Execute Command:	./TDRS [ServerIP] [ServerPortNum]

Client:
 Compile:	gcc SrDsn_DHCP_Client_Current.c -lm -o Client	
 Execute Command:	./Client [IP_Address] [PORT_NUM]

  
