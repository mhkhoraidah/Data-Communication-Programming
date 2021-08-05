#include "MySocket.h"



MySocket::MySocket(SocketType Sock, std::string ipa, unsigned int portNumber, ConnectionType conType, unsigned int max)
{

	listenerOpen = false;
	//configures the socket and connection types,
	this->mySocket = Sock;

	this->connectionType = conType;

	//sets the IP Address and Port Number 
	this->IPAddr = ipa;

	this->Port = portNumber;

	//dynamically allocates memory for the Buffer
	if (max > 0) {
		Buffer = new char[max];
		MaxSize = max;
	}
	else {
		Buffer = new char[DEFAULT_SIZE];
		MaxSize = DEFAULT_SIZE;
	}

	bTCPConnect = false;//NEW (used to check if connected in setIPAddr(), and is set to 1 in ConnectTCP
	
						  //common to both TCP AND UDP (duh)
	WSADATA wsaData;
	if ((WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
		 throw ("wsa startup failed!");//TODO: remember to catch in main() when implementing
	}
	//setup if server:-------------
	if (mySocket == SERVER) {
		if (connectionType == TCP)
			WelcomeSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		else
			WelcomeSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		//bind arguments are common for both TCP and UDP
		SvrAddr.sin_family = AF_INET;
		SvrAddr.sin_addr.s_addr = inet_addr(IPAddr.c_str());
		SvrAddr.sin_port = htons(Port);
		if (bind(WelcomeSocket, (struct sockaddr *)&SvrAddr, sizeof(SvrAddr)) == SOCKET_ERROR) {
			closesocket(WelcomeSocket);
			WSACleanup();
			throw("server socket binding failed.");
		}
		else {
			listenerOpen = true;
		}
		//UDP SERVER SETUP IS NOW DONE-----

		//listen (TCP only)
		if (connectionType == TCP) {
			if (listen(WelcomeSocket, 1) == SOCKET_ERROR) {
				closesocket(WelcomeSocket);
				WSACleanup();
				throw("server listener failed");
			}

		}
		//Accept goes here based on implementation (could also be put in connectTCP, but is not called this way in tester file)
		if (mySocket == SERVER) {
			//ConnectionSocket is the communications socket for mySocket
			ConnectionSocket = SOCKET_ERROR;
			if ((ConnectionSocket = accept(WelcomeSocket, NULL, NULL)) == SOCKET_ERROR) {
				closesocket(WelcomeSocket);
				WSACleanup();
				exit(0);
			}
		}
	}
	//setup if client (is just the socket creation since we connect in connectTCP())-------------
	//make a socket
	else if (mySocket == CLIENT) {
		if (connectionType == TCP)
			ConnectionSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		else if (connectionType == UDP)
			ConnectionSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);//again: client socket is connectionSocket
		else
			throw ("invalid connection type passed to MySocket");//not sure this is even possible	
	}
	else
		throw("invalid mySocket type (somehow set to somethign other than CLIENT or SERVER)");

}


MySocket::~MySocket()//used to both prevent Buffer memory leak, and to ensure that any connections are closed
{
	delete[] Buffer;
	Buffer = nullptr;
	if (connectionType == TCP)
		DisconnectTCP();
	else {
		if (mySocket == SERVER) {
			closesocket(WelcomeSocket);
			listenerOpen = false;//probably not necessary here
		}
		if (mySocket == CLIENT) {
			closesocket(ConnectionSocket);
		}

	}
}
void MySocket::ConnectTCP() {
	//consider doing a connection loop?
	if (mySocket == CLIENT && connectionType == TCP) {//this function is only applicable for TCP client sockets
		SvrAddr.sin_family = AF_INET;									//address family type internet
		SvrAddr.sin_port = htons(this->Port);							//port (host to network conversion)
		SvrAddr.sin_addr.s_addr = inet_addr(this->IPAddr.c_str());		//IP Address 
		if (connect(this->ConnectionSocket, (struct sockaddr *)&SvrAddr, sizeof(SvrAddr)) == SOCKET_ERROR) {
			closesocket(this->ConnectionSocket);
			WSACleanup();
			exit(0);
		}
		else
			bTCPConnect = true;//NEW
	}
	else {//or consider throwing
		std::cout << "Only Client TCP sockets can use connect()" << std::endl;
	}
}

void MySocket::DisconnectTCP() {
	//check for type of socket
	if (connectionType == TCP) {
		if (mySocket == CLIENT) {
			closesocket(this->ConnectionSocket);
		}
		else {  //prevents having an else if and default else right? Yep. and you were right to come back and differentiate between client and server.
			closesocket(this->ConnectionSocket);
			closesocket(this->WelcomeSocket);
			listenerOpen = false;
		}
		WSACleanup();
		bTCPConnect = 0;
	}
}

void MySocket::SendData(const char* Buff, int numBytes) {//was updated to include UDP--note that GetData won't function properly if SendData sends more
														//data than the receiving side's current value of MaxSize

	//send(this->ConnectionSocket, Buff, numBytes, 0);
	if (connectionType == TCP)
	{
		send(ConnectionSocket, Buff, numBytes, 0);
	}
	else
	{
		sendto(ConnectionSocket, Buff, numBytes, 0, (struct sockaddr *)&SvrAddr, sizeof(SvrAddr));
	}

}


int MySocket::GetData(char* provMemAdd) {
	std::memset(Buffer, '\0', MaxSize);	//set all the bytes in RxBuffer to nullbyte--assumes MaxSize is correct and current

	if (connectionType == TCP) {
		recv(ConnectionSocket, Buffer, MaxSize, 0);
	}
	else {//assume contents of connectionType aren't bad--assumes UDP
		int addr_len = sizeof(SvrAddr);
		recvfrom(ConnectionSocket, Buffer, MaxSize, 0, (struct sockaddr *)&SvrAddr, &addr_len);
	}

	memcpy(provMemAdd, Buffer, MaxSize);//why!?! why do we care?????? it's in Buffer

	//is this return correct? doesn't make a whole lot of sense to return the MaxSize, which
	//is what this function operates off of for determining amount to receive and size of Buffer.
	//the smart handling of buffer contents is done in MS1 where we take this char buffer and look at the
	//header to see how much data we actually got
	return MaxSize;
}

std::string MySocket::GetIPAddr() {
	return IPAddr;

}

void MySocket::SetIPAddr(std::string newIPAdd) {

	if (!newIPAdd.empty() && !bTCPConnect && !listenerOpen) {//UDP: bTCPConnect is set to false once in constructor, then not touched
		this->IPAddr = newIPAdd;
	}
	else//consider replacing with throw?
		std::cout << ("Did not change IP Address: A listener and/or connection socket is connected.") << std::endl;

}


void MySocket::Set_Port(int aPortNum) {
	//Changes the	Port number within the MySocket object--
	if (!bTCPConnect && !listenerOpen) {
		if (aPortNum > 0) {
			this->Port = aPortNum;
		}
	}
	else
		std::cout << ("Did not change port #: A listener and/or connection socket is connected.") << std::endl;
}

int MySocket::GetPort() {
	return Port;
}


SocketType MySocket::GetType() {
	return mySocket;
}

void MySocket::SetType(SocketType sType) {
	//not sure about the direction to go on this one.... so I didn't go any further
	//ConnectionSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//how about this intsead, so we can just call ConnectTCP(), which will take this parameter:
	mySocket = sType;

	//the current problem is that while we can call Connect, we do not currently handle the setup
	//of a listener socket. 

}
bool MySocket::getConnected() {
	return  bTCPConnect;
}

int MySocket::getMaxSize() {
	return MaxSize;
}
