#ifndef _MYSOCKET_H_
#define _MYSOCKET_H_
#include <windows.networking.sockets.h>
#include <iostream>
#pragma comment(lib, "Ws2_32.lib")

//using namespace std;

enum SocketType {CLIENT, SERVER};
enum ConnectionType
{
	TCP, UDP
};
 

const int DEFAULT_SIZE=12;//CHANGED: to accomodate telemetry body size of 5 bytes  //used to be assuming default size to be size of myPkt header (6) plus MotorBody(2) plus trailer (1)=9.

class MySocket
{
	char * Buffer;				//to dynamically allocate RAW buffer space for communication activities
	SOCKET WelcomeSocket, ConnectionSocket;		//WelcomeSocket used by a MySocket object configured as a TCP / IP Server
									// 	ConnectionSocket used for client / server communications(both TCP and UDP)
	sockaddr_in SvrAddr;			//to store connection information
	SocketType mySocket;			//is CLIENT or SERVER
	std::string IPAddr;					//to hold the IPv4 IP Address string
	int Port;						//to hold the port number to be used
	ConnectionType connectionType;	// is TCP or UDP 
	bool bTCPConnect;				//flag to determine if a connection has been established or not
	int MaxSize;					//to store the maximum number of bytes the buffer is allocated to
	bool listenerOpen; //NEW: used in setIPAddr() logic to check if socket is already connected
	


	
public:
	MySocket(SocketType Sock, std::string, unsigned int, ConnectionType, unsigned int); /* A constructor
that configures the socket and connection types, sets the IP Address and Port Number and 
dynamically allocates 
memory for the Buffer.
 Note that the constructor should put servers in 
conditions to either accept conn
ections (if TCP), or to receive messages (if UDP
). 
o
NOTE:  If an invalid size is provided the 
DEFAULT_SIZE
 should be used.*/
	~MySocket();
	void ConnectTCP();					//Used to establish a TCP / IP socket connection(3 way handshake)
	void DisconnectTCP();				// Used to disconnect an established TCP/IP socket connection (4-way handshake)
	void SendData(const char*, int);	 //Used to transmit a block of RAW data, specified by the starting memory address and number of bytes, over the socket. This function should work with both TCP and UDP.
	int GetData(char*);					// – Used to  receive the last block of RAW data stored in the internal MySocket Buffer
						//TAKES A CHAR *, WHICH IS THEN DYNAMICALLY ALLOCATED IN FUNCTION BODY
										//After getting the received message into Buffer, this function will transfer its contents to 
										//the provided memory address and return the total number of bytes written.
										//This function should work with both TCP and UDP.

	std::string GetIPAddr();					//Returns the IP address configured within the MySocket object
	void SetIPAddr(std::string);		//Changes the default IP address within the MySocket object
										//This  method should return an error message if a connection has already been established

	void Set_Port(int);//NOTE: 'SetPort' identifier was already used by some other library (probably winsock) //Changes the default Port number within the MySocket object

										//This method should return an error if a connection has already been established

	int GetPort();						//Returns the Port number configured within the MySocket object

	SocketType GetType();				//Returns the default SocketType the MySocket object is configured as

	void SetType(SocketType);			//Changes the default SocketType within the MySocket object
	bool getConnected();
	int getMaxSize(); //Returns  the maximum number of bytes the buffer is allocated to

};

#endif