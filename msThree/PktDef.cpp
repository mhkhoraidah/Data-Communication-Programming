#include "Pkt_Def.h"

unsigned int PktDef::staticPktCount = 0;
//tester code at end of spec has no namespace, so assuming global

PktDef::PktDef() {//note: identifier for CmdPkt is 'cmdPkt', declared in header
	//set Header ints to 0:
	cmdPkt.cmdHeader.PktCount = 0;
	cmdPkt.cmdHeader.Length = 7;//CHANGED in MS3 (realized that sleep commands have no body, and since SetBody() and PktDef are the only function that set Length the default of 7 is needed here
	//set bitfield=0
	char* setPtr;
	setPtr= (char *) &cmdPkt.cmdHeader;
	setPtr +=sizeof(int);//set pointer to start of our bitfield
	*setPtr = 0;//4 bytes?

	//set body pointer
	cmdPkt.Data = nullptr;
	cmdPkt.CRC = 0;
	RawBuffer = nullptr;
	staticPktCount++;
	SetPktCount(staticPktCount);
}


PktDef::PktDef(char * rxBuff) {//remember: HEADERSIZE IS USED TO ITERATE THROUGH CHAR BUFFERS--sizeof() header is 12 bytes because it is allocated weirdly
	char * temp = rxBuff;
	RawBuffer = nullptr;
	//copy over header:----
	memcpy(&cmdPkt.cmdHeader, temp, HEADERSIZE);//header in cmdPkt has 3bytes garbage after bitfield, and 3bytes garbage after char length
	memcpy(&cmdPkt.cmdHeader, temp, (sizeof(int) + 1));//copy over PktCount and bitfield
	temp += (sizeof(int) + 1);
	memcpy(&cmdPkt.cmdHeader.Length, temp, sizeof(unsigned char));//copy over Length
	temp += sizeof(unsigned char);//advance past Length in packet's header (that's what we took last memcpy)
	//--------------done copying header
	//copy over the body (allocate dynamic memory)----------------
	int bodySize = (cmdPkt.cmdHeader.Length - HEADERSIZE) - sizeof(cmdPkt.CRC);//get the size of the body
	if (bodySize == 0) {
		cmdPkt.Data = nullptr;
	}
	else {
		cmdPkt.Data = new char[bodySize];
		memcpy(cmdPkt.Data, temp, bodySize);
	}
	
	//copy over the trailer
	temp += bodySize;//if no body, this will be +=0
	memcpy(&cmdPkt.CRC, temp, sizeof(unsigned char));
	temp = nullptr;
	//counting logic below is poorly executed for ensuring sync between received packet count and the actual value packet count should be at
	staticPktCount++;
	//SetPktCount(staticPktCount);//no. this would be cheating...
}



void PktDef::SetCmd(CmdType type)//sets the packet's header bitfield flags based on the enum value passed in (ie use switch-case)
{
	//char * ptr = (char*)&cmdPkt.cmdHeader.PktCount;
	//ptr += sizeof(cmdPkt.cmdHeader.PktCount);
	switch (type)
	{
	case DRIVE:
		cmdPkt.cmdHeader.Drive = true;
		break;
	case SLEEP:
		cmdPkt.cmdHeader.Sleep = true;
		break;
	case ARM:
		cmdPkt.cmdHeader.Arm = true;
		break;
	case CLAW:
		cmdPkt.cmdHeader.Claw = true;
		break;
	case ACK:
		cmdPkt.cmdHeader.Ack = true;
		break;
	default:
		break;
	}
	CalcCRC();//NEW: as of MS3 realized any change of the packet contents should call CalcCRC for safety
}


/*checks the CmdPkt's header bitfields for the command type.
TODO: function will break if not used properly:
DOES NOT ACCOUNT FOR NACK
check how to define NACK (the enum does not account for NACK headers)

*/
CmdType PktDef::GetCmd() {
	char* hdPtr = (char*) &cmdPkt.cmdHeader;
	char mask = 1;
	CmdType returnMe=ACK;//default behaviour--just set it as an ACK even if no accompanying flags (although an ACK must have accompanying
						//command flags set, if the return packet has corrupted header flags it's unclear if the command was executed, so don't
						//potentially execute it twice (assume ACK is true, even if it isn't)


	if (!cmdPkt.cmdHeader.Drive && !cmdPkt.cmdHeader.Sleep && !cmdPkt.cmdHeader.Arm && !cmdPkt.cmdHeader.Claw && !cmdPkt.cmdHeader.Ack)
		return NACK;//if all flags are set to zero or false GetCmd() function will return NACK command 

	for (int i = 0; i < 6; i++) {//<6 because we don't care about padding contents

		if ((*hdPtr) & mask == 1) {//because this will repeatedly change returnMe, and ACK is last,
								//returnMe will be set appropriately for ACK responses, which
								//have both the original command flag, and the ack flag set.
			switch (i) {
			case 0:
				returnMe = DRIVE;
				break;
			case 1://ignore status flag/bitfield
				break;
			case 2:
				returnMe = SLEEP;
				break;
			case 3:
				returnMe = ARM;
				break;
			case 4:
				returnMe = CLAW;
				break;
			case 5://Changed in MS3: we have getAck() to tell if there's an ack, and if we get an ACK, we want it to have the accompanying command type (and thus want to see it)
			//	returnMe = ACK;
				break;
				//consider throwing an exception in default: and catching it whenever getCmd is called?	
			}
		}
		mask = mask << 1;
	}
	return returnMe;
}
void PktDef::CalcCRC() {
	char * ptr;
	void * hack;//intermediary pointer to allow memory contents to be treated as a char* through ptr (can't assign unsigned int* to char * but can  to void *)
	int foundCRC=0;//the calculated total CRC based on number of bits set to 1 in packet's header and body

	hack = &cmdPkt.cmdHeader.PktCount;//set to a void pointer
	ptr = (char *)hack;//set char * to address of void pointer so we now can read contents by the byte
	//char temp = *ptr;//copy first byte into temp field so bitshifting it doesn't ruin contents
	
	//calculate crc up to and including first bitfield
	for (int i = 0; i < (sizeof(cmdPkt.cmdHeader.PktCount) + 1); i++) {//packet's pktCount plus 1 byte bitfield * 8 bits in a byte
		for (int j = 0; j < 8; j++) {
			foundCRC += (*ptr >> j) & 0x01;
		}
		ptr += 1;
	}
	
	//get the rest of the header
	hack = &cmdPkt.cmdHeader.Length;
	ptr = (char *)hack;
	for (int i = 0; i < sizeof(cmdPkt.cmdHeader.Length); i++) {
		for (int j = 0; j < 8; j++) {
			foundCRC += (*ptr >> j) & 0x01;
		}
		ptr += 1;
	}

	//check if there's a body to calculate the CRC on and calc if it exists:
	int bodySize = ((int)cmdPkt.cmdHeader.Length - HEADERSIZE - sizeof(cmdPkt.CRC));//TODO: check this syntax
	if ( bodySize > 0) {
		ptr = cmdPkt.Data;
		for (int i = 0; i < (bodySize); i++) {
			for (int j = 0; j < 8; j++) {
				foundCRC += (*ptr >> j) & 0x01;
			}
			ptr += 1;
		}
	}
	ptr = nullptr;
	hack = nullptr;
	cmdPkt.CRC = foundCRC;
}

//works
bool PktDef::CheckCRC(char* buff, int size) {

	char* ptr;
	ptr = buff;
	char mask = 1;
	int foundCRC= 0;
	int countMax = (size - 1);//-1 to omit CRC trailer
	countMax = countMax * 8;
	/*for (int i = 0; i < countMax; i++) {
			
			 foundCRC += (*ptr >> i) & mask;
			 std::cout << std::endl;
	}*/
	char temp=*ptr;//make copies of each byte, and bitshift (destroy) those contents instead of original
	for (int i = 0; i<(size-1); i++) {
		for (int j = 0; j<8; j++) {
			foundCRC += (temp) & mask;
			temp = temp >> 1;
			//*ptr = *ptr >> 1;//bitshift data at address (destroys contents)
			//mask = mask << 1;(shifting just the mask breaks because we are no longer adding & of 1, we are doing it with progressively higher binary digits (instead of consistent +1, it becomes +1, +2, +4, etc...)
		}
		ptr += 1;
		temp = *ptr;//copy the next byte to mangle/explore
	}
	
	//ptr = (buff + (8 * (size - 1)));
	ptr = &buff[size - 1];
	int sentCRC = *ptr;
	
	ptr = nullptr;
	return (foundCRC == sentCRC);
}


void PktDef::SetPktCount(int pktCount)
{
	cmdPkt.cmdHeader.PktCount = pktCount;
	CalcCRC();
}

int PktDef::GetPktCount()
{
	return cmdPkt.cmdHeader.PktCount;
}

char * PktDef::GetBodyData()
{
	return cmdPkt.Data;
}
//TODO: how many places do we need to set Length in the header? OTHER FUNCTIONS ARE DEPENDENT ON LENGTH BEING ACCURATE TO THE PACKET'S CONTENTS
//Takes a char* buffer to set the body with, and an int size to specify the #bytes to allocate for the packet's data portion. also sets header's Length value
void PktDef::SetBodyData(char* buff, int size) {
	//char * ptr = buffer;

	cmdPkt.Data = new char[size];
	memcpy(cmdPkt.Data, buff, size);
	cmdPkt.cmdHeader.Length = HEADERSIZE + sizeof(cmdPkt.CRC) + size;
	CalcCRC();

	//ptr = nullptr;
	//std::cout << "done copying body" << std::endl;
}
//clears RawBuffer and reallocates it to the size of the current packet when serialized, then serializes packet data to the new buffer.
//returns the char pointer address to the newly allocated and populated buffer that holds the serialized packet for transmission
char* PktDef::GenPacket()
{
	delete[] RawBuffer;
	RawBuffer = new char[cmdPkt.cmdHeader.Length];//is length of a SERIALIZED packet
	char * ptr = RawBuffer;
	std::memcpy(ptr, &cmdPkt.cmdHeader.PktCount, sizeof(unsigned int) + 1);
	ptr += sizeof(unsigned int) + 1;
	std::memcpy(ptr, &cmdPkt.cmdHeader.Length, sizeof(unsigned char));
	ptr += sizeof(unsigned char);
	if (GetBodyData() != nullptr)
	{
		std::memcpy(ptr, cmdPkt.Data, getBodySize());//Attention: updated to use new getBodySize() function instead of sizeof(MotorBody)
		ptr += getBodySize();//Attention: updated to use new getBodySize() function instead of sizeof(MotorBody)
	}
	std::memcpy(ptr, &cmdPkt.CRC, sizeof(char));

	ptr = nullptr;
	return RawBuffer;
}
//returns the length of the packet when serialized into a buffer (ie better than using sizeof(cmdPkt))
int PktDef::GetLength() {
	return cmdPkt.cmdHeader.Length;
}

bool PktDef::GetAck() {
	
	return (bool)cmdPkt.cmdHeader.Ack;

}

bool PktDef::getStatus() {//NEW FROM MS3
	return (bool)cmdPkt.cmdHeader.Status;
}

int PktDef::getBodySize() {//returns the size (hopefully) of the body of the packet by checking the header's Length value, and subtracting HEADERSIZE, and the tail's size
	int bodySize= ((int)cmdPkt.cmdHeader.Length - HEADERSIZE - sizeof(cmdPkt.CRC));
	return bodySize;
}

bool PktDef::getAckSleep() {//NEW FROM MS3 check if the packet that sent has ACK and SLEEP flags set to TRUE, so return TRUE
	return ((bool)cmdPkt.cmdHeader.Sleep && (bool)cmdPkt.cmdHeader.Ack) ? true : false;
}