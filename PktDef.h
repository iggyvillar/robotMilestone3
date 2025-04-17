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

const unsigned char HEADERSIZE = 4;		//updated

//header
struct Header {
	unsigned short int PktCount;
	struct {
		uint8_t drive : 1;
		uint8_t status : 1;
		uint8_t sleep : 1;
		uint8_t ack : 1;
		uint8_t padding : 4;
	} cmdFlags;
	uint8_t length;
};

// drivebody struct
struct driveBody {
	uint8_t direction;
	uint8_t duration;
	uint8_t speed;
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
		unsigned char* Data;
		uint8_t CRC;
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