#pragma once
#include <cstdint>

//enumerated CMDType with specified command types
enum class CMDType {
	DRIVE,
	SLEEP,
	RESPONSE
};

//constants
const unsigned char FORWARD = 1;
const unsigned char BACKWARD = 2;
const unsigned char RIGHT = 3;
const unsigned char LEFT = 4;
const unsigned char HEADERSIZE = 3;		//headersize = PktCount(2 bytes) + 1 byte flag = 3

//header
struct Header {
	unsigned short int PktCount;
	struct {
		unsigned char drive : 1;
		unsigned char status : 1;
		unsigned char sleep : 1;
		unsigned char ack : 1;
		unsigned char padding : 4;
	} cmdFlags;
};

// drivebody struct
struct driveBody {
	unsigned char direction;
	unsigned char duration;
	unsigned char speed;
};

// telemetry struct
struct telemetry {
	unsigned short int LastPktCounter;
	unsigned short int CurrentGrade;
	unsigned short int HitCount;
	unsigned char LastCmd;
	unsigned char LastCmdValue;
	unsigned char LastCmdSpeed;
};

class PktDef {
private:
	struct CmdPacket {
		Header header;
		unsigned char length;  
		unsigned char* Data;
		unsigned char CRC;
	}cmdPacket;

	unsigned char* RawBuffer;

public:
	PktDef();
	PktDef(unsigned char* rawData);
	void setCMD(CMDType cmd);
	void setBodyData(unsigned char* data, unsigned char size);
	void setPktCount(unsigned short int);
	CMDType getCMD();
	bool getAck();
	unsigned char getLength();
	unsigned char* getBodyData();
	unsigned short int getPktCount();
	bool checkCRC(unsigned char* buffer, unsigned char size);
	void calcCRC();				//counting number of 1s
	unsigned char* genPacket();
	~PktDef();
};