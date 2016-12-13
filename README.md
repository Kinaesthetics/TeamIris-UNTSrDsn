# TeamIris-UNTSrDsn
Repository for UNT Senior Design Team Iris, (Fall 2016 - Spring 2017)

How to compile and run the
  Server:
    Compile:  gcc SrDsn_DHCP_Server_Current.c -lm -lpthread -o Server
    Execute:  ./Server  
  
  Client:
    Compile:	gcc SrDsn_DHCP_Client_Current.c -lm -o Client	
 	  Execute Command:	./Client <IP_Address> <PORT_NUM>

  TDRS:
    Compile:  gcc TDRS_SrDsn.c -lm -lpthread -o TDRS
    Execute Command:	./TDRS <ServerIP> <ServerPortNum>	
