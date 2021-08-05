
#include "Pkt_Def.h"
#include <iostream>
#include <string>//used for std::getline
#include "MySocket.h"
#include <mutex>
#include <iomanip>

//bool ExeComplete;

//generates a motorBody struct, from provided direction and duration, then passes back a char * to that struct for passing in to SetBody() etc
char* genMotoBdy(MotorBody * motoBdy, int direction, int duration) {

	//fake a char buffer to hold the packet's data (MotorBody is a global struct, so use that instead of allocating a char buffer):
	motoBdy->Direction = direction;
	motoBdy->Duration = duration;
	char * setPtr = (char *)((void*)motoBdy);//more pointer juggling (trying this syntax to see if we can do this instead of allocating a new char buffer)
	return setPtr;
}

void cmdThread(std::string ip, int portNum, bool *sleep) {
	//if this function sends a sleep packet, and the robot returns an ACK to that sleep, &sleep is set to true, killing both cmd and tele threads

	MySocket cmdSocket(CLIENT, ip, portNum, TCP, 9);//NEW: changed from 9 to 12... then back to 9 ( I now realize that the cmd socket will probably not be working with 12 byte telemetry packets, so changed back to 9)
	cmdSocket.ConnectTCP();




	while (!*sleep) {//run until sleep flag is set
		bool ack = false;
		unsigned int comType = 0, duration = 0, direction = 0;
		//if this function sends a sleep packet, and the robot returns an ACK to that sleep, &sleep is set to true, killing both cmd and tele threads

		//placeholder code if you want this thread to just run indefinitely (inner code does not run because sleep is 
		//if (*sleep) {//is never run
		//std::cout << "argh";
		//}

		//some sort of loop to get packets and send them.
		//loop that generates a packet, transmits it and waits for ackn
		//while (!sleep) {
		//while (ack == true) {
		PktDef cmdPkt2Go;//REMEMBER: default constructor sets packet length to 0--need to set it (and everything else) before transmission

		MotorBody motoBdy;
		//char * txBuff = cmdPkt2Go.GenPacket();//that's not how this works: set the packet before calling GenPacket() to serialize it into a buffer for transmission

		//collect info from user
		std::cout << "Enter the type of command: 1 - for Drive | 2 - Arm | 3 - Claw | 4 - Sleep" << std::endl;
		std::cin >> comType;
		std::cin.ignore(2000, '\n');
		bool okCmd = true;//validation is pretty much nonexistent except for this
		char * setPtr;
		switch (comType) {
		case 1:
			cmdPkt2Go.SetCmd(DRIVE);
			do {
				std::cout << "Which direction would you like to go?\n" << "1 - Forward, 2 - Backward, 3 - Right, 4 - Left" << std::endl;
				std::cin >> direction;
				std::cin.ignore(2000, '\n');
			} while (direction > 4 || direction < 1);
			std::cout << "How long (in seconds) would you like to continue moving in that direction? " << std::endl;
			std::cin >> duration;
			std::cin.ignore(2000, '\n');

			//see beginning of file for genMotoBdy definition; creates a motorbody struct, and returns a char * to the struct
			setPtr = genMotoBdy(&motoBdy, direction, duration);

			//NEED A WAY TO SET PACKET COUNT BEFORE SENDING OFF (MAKE PACKET COUNT A STATIC VARIABLE?)
			//generate body data, and set length of packet (SetBodyData will always set the header's Length property for you)
			cmdPkt2Go.SetBodyData(setPtr, sizeof(MotorBody));
			//std::cout << "placeholder";//delete me
			break;
			//}
			//else if (comType == 2) {
		case 2:
			cmdPkt2Go.SetCmd(ARM);
			std::cout << "What to do with the arm then? 1 - Up, 2 - Down" << std::endl;
			std::cin >> direction;
			if (direction == 1) {
				direction = 5;
			}
			else {
				direction = 6;
			}
			//see beginning of file for genMotoBdy definition; creates a motorbody struct, and returns a char * to the struct
			setPtr = genMotoBdy(&motoBdy, direction, duration);

			//NEED A WAY TO SET PACKET COUNT BEFORE SENDING OFF (MAKE PACKET COUNT A STATIC VARIABLE?)
			//generate body data, and set length of packet (SetBodyData will always set the header's Length property for you)
			cmdPkt2Go.SetBodyData(setPtr, sizeof(MotorBody));
			//}
			break;
			//	else {
		case 3:

			cmdPkt2Go.SetCmd(CLAW);
			std::cout << "Let me guess, now you want to operate the claw. Well then, what will it be? 1 - Open, 2 - Close" << std::endl;
			std::cin >> direction;
			if (direction == 1) {
				direction = 7;
			}
			else {
				direction = 8;
			}
			//see beginning of file for genMotoBdy definition; creates a motorbody struct, and returns a char * to the struct
			setPtr = genMotoBdy(&motoBdy, direction, duration);

			//NEED A WAY TO SET PACKET COUNT BEFORE SENDING OFF (MAKE PACKET COUNT A STATIC VARIABLE?)
			//generate body data, and set length of packet (SetBodyData will always set the header's Length property for you)
			cmdPkt2Go.SetBodyData(setPtr, sizeof(MotorBody));
			//}
			break;
		case 4://remember: sleep commands have a body length of 0 (which means that packet length should actually be set to  7 in the default constructor() [CHANGED]
			cmdPkt2Go.SetCmd(SLEEP);//also hit with the realization that CalcCRC should really be run ANY time packet info is changed [CHANGED setters in PktDef to call CalcCRC()]
			break;


		default:
			std::cout << "Types must be between 1 and 4" << std::endl;
			okCmd = false;
		}
		if (okCmd) {
			//MUSTAFA'S CODE GOES BELOW HERE
			do {
				//send the txBuffer by call cmdPkt2Go.GenPacket() function that retune txBuffer
				cmdSocket.SendData(cmdPkt2Go.GenPacket(), cmdPkt2Go.GetLength());

				//receice data to rxBuff
				char *rxBuff = new char[cmdSocket.getMaxSize()];
				//create a pointer with the max size of cmdSocket
				cmdSocket.GetData(rxBuff);
				//call GetDate that going to store the received data into rxBuff 
				PktDef rxPtk(rxBuff);
				//create PktDef using the received date by passing rxBuff to the constructor of PktDef

				if (rxPtk.getAckSleep())
					//chacking if rxPkt has SLEEP and ACK flags set to true
				{
					//to exit the (do while) loop 
					*sleep = true;
					ack = true;
				}
				else if (!rxPtk.getAckSleep() && rxPtk.GetAck())
					//checking if the rxPkt get ACK
				{
					//*sleep = false; //sleep flag is still false
					ack = true;
					std::cout << "\n\nGood the packet is ACK, please send the other one...\n\n" << std::endl;
				}
				else
					//checking if the rxPkt is NACK, it will keep sending the same packet until get ACK 
				{
					ack = false;
					std::cout << "\n\nSorry the packet is NACK, it is sending again...\n\n" << std::endl;
				}
			} while (!ack && !*sleep);//END (do while loop)
		}
	}//END ( while (!*sleep) loop)
	std::cout << "Sleeping (and smothering with pillow) threads and returning to Main." << std::endl;
//	cmdSocket.DisconnectTCP();
}

void teleThread(std::string ip, int portNum, bool *sleep) {

	MySocket teleSocket(CLIENT, ip, portNum, TCP, 12);//hard coded >:( telemetry packet is 12 bytes
	teleSocket.ConnectTCP();
	char rxBuff[12];//points to socket's Buffer member
	char * body = nullptr;
	int buffSize;//think my implementation of GetBodydata() and calcCRC gets/does stuff in a roundabout way--if it works it works, and if it doesn't, then make fun of me
	while (!*sleep) {
		buffSize = teleSocket.GetData(rxBuff);//
		PktDef telemPkt(rxBuff);//construct a new packet from the received char * buffer (use the power of the PktDef constructor we've already implemented)
		if (telemPkt.CheckCRC(rxBuff, buffSize) && telemPkt.getStatus()) {//if the packet's CRC is ok and status flag=1 (meaning it's a telemetry packet)
														//prof mentioned the robot may not care if we send it telemetry acks, because it will spit us a new packet every 3 seconds regardless

														//get address of packet body data:
			body = telemPkt.GetBodyData();

			//Telemetry extraction here
			
			//raw data:
			char * RAWDataPtr = rxBuff;
			std::cout << "--------------------Packet-----------------------" << std::endl;
			for (int i = 0; i < telemPkt.GetLength(); i++)
				//loop through the the bytes to dispay them
				std::cout << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << (unsigned int)*RAWDataPtr++ << " "; //print byte by byte to display the RAW data
			std::cout << std::endl;


			//extract sensor and arm position data:
			short sensor;
			short armPos;
			void * ptr;
			ptr = body;
			short* shortPtr = (short *)ptr;
			sensor = *shortPtr;
			shortPtr++;
			armPos = *shortPtr;
			shortPtr++;

			
			std::cout << std::dec << "Sensor: " << sensor << "     Arm Position: " << armPos << std::endl;
			std::cout << "Status Bits: " << std::endl;
			ptr = shortPtr;
			char * bytePtr = (char*)ptr;
			struct impotentRage {//my impotent rage has been vindicated (works)
				unsigned drive : 1;
				unsigned armUp : 1;
				unsigned armDown : 1;
				unsigned clawOpen : 1;
				unsigned clawClosed : 1;
				unsigned pad : 3;
			}argh;

			memcpy(&argh, bytePtr, 1);

			if ((bool)argh.drive) {
				std::cout << "Drive:   1" << std::endl;
			}
			 if (!(bool)argh.drive) {
				std::cout << "Drive:   0" << std::endl;
			}
			 if ((bool)argh.armUp) {
				 std::cout << "Arm is Up" << std::endl;
			 }
			 if ((bool)argh.armDown) {
				 std::cout << "Arm is Down" << std::endl;
			 }
			 if ((bool)argh.clawOpen) {
				 std::cout << "Claw is Open" << std::endl;
			 }
			 if ((bool)argh.clawClosed) {
				 std::cout << "Claw is Closed" << std::endl;
			 }
	/*		//display body flags
			char mask = 1;
			for (int i = 0; i < 5; i++) {

				if ((*bytePtr)& mask == 1) {
					switch (i) {
					case 0:
						std::cout << "Drive:   1" << std::endl;
						break;
					case 1:
						std::cout << "Arm is Up" << std::endl;
						break;
					case 3:
						std::cout << "Claw is Open" << std::endl;
						break;
					}
				}
				else
				{
					switch (i) {
					case 0:
						std::cout << "Drive:   0" << std::endl;
						break;
					case 2:
						std::cout << "Arm is Down" << std::endl;
						break;
					case 4:
						std::cout << "Claw is Closed" << std::endl;
						break;
					}
				}
				mask = mask << 1;
			}
*/

		}
		//std::cout << " ";
	}
	//std::cout << " ";
//	teleSocket.DisconnectTCP();
}




int main() {
	bool sleep = true;
	bool ExeComplete = false;//misinterpreted instructions on sleep--sleep=kill, so be lazy and set this based on sleep flag:
	//std::thread(, &sleep);//

	//std::thread(, &sleep)

	int hackCounter = 0;//a hack flag because sleep deprived me cannot do the simple task of getting Execomplete and sleep flags coordinated properly

	while (!ExeComplete) {
		//get user input to configure socket information (don't prompt for user input in threads unless you want to use a mutex, etc)--------------------------
		std::string ip;
		int portNumIn, port2NumIn;
		

		if (sleep && !ExeComplete) {
			//std::cout << "press enter to continue" << std::endl;
			//getchar();
			if (sleep && !ExeComplete && hackCounter==0) {
				std::cout << "\n\n\nEnter an IP address:  ";
				std::getline(std::cin, ip);
				//	std::cin.ignore(2000, '\n');
				std::cout << "Enter a port # for the send port:  ";
				std::cin >> portNumIn;
				std::cin.ignore(2000, '\n');

				sleep = false;//important to do this before threads detach, or they will die immediately

				std::cout << "Enter a port # for the receive port:  ";
				std::cin >> port2NumIn;
				std::cin.ignore(2000, '\n');

				//launch threads--------------------------------------------------------
				std::thread cmd(cmdThread, ip, portNumIn, &sleep);
				std::thread tele(teleThread, ip, port2NumIn, &sleep);
				cmd.detach();
				tele.detach();

				hackCounter++;
			}
		}
		if (sleep)
			ExeComplete = true;
	}

}



