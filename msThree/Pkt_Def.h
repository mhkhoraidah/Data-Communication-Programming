#include <iostream>

#ifndef _PKT_DEF_H
#define _PKT_DEF_H

enum CmdType { DRIVE, SLEEP, ARM, CLAW, ACK, NACK };

const int FORWARD = 1;
const int BACKWARD = 2;
const int RIGHT = 3;
const int LEFT = 4;
const int UP = 5;
const int DOWN = 6;
const int OPEN = 7;
const int CLOSE = 8;
const int HEADERSIZE = 6;//what size SHOULD be -- USE ONLY WITH CHAR BUFFERS (don't use for iterating through structs)
						 //const int HEADERSIZE = 12;

struct MotorBody {//defines behaviour for drive, arm, and claw--see spec for header context specific values for drive vs claw, etc
	unsigned char Direction;//these store numerical values (unsigned char for 1 byte size)
	unsigned char Duration;//spec diagram says this is 1 byte, spec description says this is an unsigned int, which is 4 bytes.....
};

struct Header {//TODO: [POST SUBMISSION] consider refactoring to char type bitfields? seems to be without this dumb offset in BitManipulation class examples
	unsigned int PktCount;//says that this is the nth packet that has been sent/received
						  //bitfields say what the body will contain (and thus determine how the packet should be processed)
	unsigned Drive : 1;
	unsigned Status : 1;
	unsigned Sleep : 1;
	unsigned Arm : 1;
	unsigned Claw : 1;
	unsigned Ack : 1;
	unsigned Padding : 2;//I assume there isn't an offset here because the datatype next to the bitfield is a char(1 byte), not an int(4 bytes)
						 //so the bitfield shouldn't be followed by an extra 3 bytes like it was in lab 6

	unsigned char Length;//1 byte of data that says the total number of bytes in the entire serialized packet that is sent over the connection
};



class PktDef {

	//remember: unless otherwise spec'd numerical value is in order from 
	static unsigned int staticPktCount;//increment every time a packet is sent (and received)
	char* RawBuffer;//used to serialize data for transmission (remember: CmdPacket uses dynamic memory for its body)

	//---structs----------
	
	//**DIRECTIONS**see const int definitions above. [ARM AND CLAW DURATION IS ALWAYS 0]
	//directions must coincide with appropriate header bitfield values
	
	 struct CmdPacket {//
		Header cmdHeader;//our Header struct is the packet header
		char * Data; //a char buffer that holds the body of our packet (ie MotorBody)
		char CRC;//the CRC register tail
	}cmdPkt;
	 //--------function declarations---------
public:
	 PktDef();
	 PktDef(char *);//will take a raw char buffer and parse it to populate the header, body and CRC members of PktDef
	 void SetCmd(CmdType);//sets the packet's header bitfield flags based on the enum value passed in (ie use switch-case)
	 void SetBodyData(char*, int);//takes a char* buffer address and int number of bytes and copies the contents of the char buffer to a body struct
	 void SetPktCount(int);//setter for CmdPacket's Header's PktCount
	 CmdType GetCmd();//returns the CmdType based on the header's flags
	 bool getStatus();
	 bool GetAck();//returns true/false depending on whether the header's Ack bitfield was set
	 int GetLength();//returns the packet length in bytes (body or entire packet?)
	 char* GetBodyData();//returns a pointer to the object's Body Field
	 int GetPktCount();//returns PktCount
	 bool CheckCRC(char*, int);//takes address of raw data buffer, and the size of buffer in bytes, and calculates the CRC.
								//If the calculated CRC matches the CRC of the packet in the buffer the function returns TRUE, otherwise FALSE
	 void CalcCRC();//calcs the crc and sets char CRC in CmdPacket to the resulting value
	 char* GenPacket();//serializes data into a  Raw Buffer for transmission and returns the address of the buffer
	 int getBodySize(); //returns the size (hopefully) of the body of the packet by checking the header's Length value, and subtracting HEADERSIZE, and the tail's size
	 bool getAckSleep(); //return True if SLEEP and ACK cmd are set to true
};


//NOTES:
/*

MEMORY STRUCTURE OF cmdPkt: 
header: 4bytes: PktCount, 1byte: bitfield, 3bytes: bitfield garbage,
+1byte: char Length, +3 bytes random garbage...=12
//size of cmdPkt.cmdHeader is 12 bytes

*****packet header LENGTH value specifies  the size in bytes of the entire packet
	 HEADERSIZE const stores the serialized header size (in a char buffer) NOT the struct's size.
--getCmd() currently CANNOT HANDLE NACK--the definition of the enum does not account for this
--ACK packets have a head and tail, but no body-- remember to refer to the spec on a NACK (rejected) packet as well
--NACK- all bitfields in the header are set to 0

--CRC calculator: need to create a bit manipulation function to count the number of bits set to 1

*/

#endif