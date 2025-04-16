#include "PktDef.h"
#include <cstring>
#include <bitset>

//define default constructor
PktDef::PktDef() {
	cmdPacket.header.PktCount = 0;
	cmdPacket.header.cmdFlags.drive = 0;
	cmdPacket.header.cmdFlags.ack = 0;
	cmdPacket.header.cmdFlags.sleep = 0;
	cmdPacket.header.cmdFlags.status = 0;
	cmdPacket.header.cmdFlags.padding = 0;
	cmdPacket.length = 0;
	cmdPacket.Data = nullptr;
	cmdPacket.CRC = 0;
	RawBuffer = nullptr;
}

//overloaded constructor
PktDef::PktDef(unsigned char* rawData) {
	memcpy(&cmdPacket.header, rawData, HEADERSIZE);
	cmdPacket.length = rawData[HEADERSIZE];
	if (cmdPacket.length > 0) {
		cmdPacket.Data = new unsigned char[cmdPacket.length];
		memcpy(cmdPacket.Data, rawData + HEADERSIZE + 1, cmdPacket.length);
	}
	else {
		cmdPacket.Data = nullptr;
	}
	cmdPacket.CRC = rawData[HEADERSIZE + 1 + cmdPacket.length];
	RawBuffer = nullptr;
}

PktDef::~PktDef() {
	if (cmdPacket.Data != nullptr) {
		delete[] cmdPacket.Data;
	}
	if (RawBuffer != nullptr) {
		delete[] RawBuffer;
	}
}

void PktDef::setCMD(CMDType cmd) {
	cmdPacket.header.cmdFlags.drive = 0;
	cmdPacket.header.cmdFlags.status = 0;
	cmdPacket.header.cmdFlags.sleep = 0;

	switch (cmd) {
	case CMDType::DRIVE:
		cmdPacket.header.cmdFlags.drive = 1;
		break;
	case CMDType::SLEEP:
		cmdPacket.header.cmdFlags.sleep = 1;
		break;
	case CMDType::RESPONSE:
		cmdPacket.header.cmdFlags.status = 1;
		break;
	}
}

void PktDef::setBodyData(unsigned char* data, unsigned char size) {
	if (cmdPacket.Data != nullptr) {
		delete[] cmdPacket.Data;
	}
	cmdPacket.Data = new unsigned char[size];
	memcpy(cmdPacket.Data, data, size);
	cmdPacket.length = size;
}

void PktDef::setPktCount(unsigned short int size) {
	cmdPacket.header.PktCount = size;
}

CMDType PktDef::getCMD() {
	if (cmdPacket.header.cmdFlags.drive) return CMDType::DRIVE;
	if (cmdPacket.header.cmdFlags.sleep) return CMDType::SLEEP;
	if (cmdPacket.header.cmdFlags.status) return CMDType::RESPONSE;
	return CMDType::DRIVE;
}

bool PktDef::getAck() {
	return cmdPacket.header.cmdFlags.ack;
}

unsigned char PktDef::getLength() {
	return cmdPacket.length;
}

unsigned char* PktDef::getBodyData() {
	return cmdPacket.Data;
}

unsigned short int PktDef::getPktCount() {
	return cmdPacket.header.PktCount;
}

bool PktDef::checkCRC(unsigned char* buffer, unsigned char size) {
	unsigned char calculatedCRC = 0;
	for (unsigned char i = 0; i < size - 1; i++) {
		std::bitset<8> bits(buffer[i]);
		calculatedCRC += static_cast<unsigned char>(bits.count());
	}
	return calculatedCRC == buffer[size - 1];
}

void PktDef::calcCRC() {
	unsigned char calculatedCRC = 0;
	std::bitset<16> pktBits(cmdPacket.header.PktCount);
	calculatedCRC += static_cast<unsigned char>(pktBits.count());
	std::bitset<8> flagsBits(*reinterpret_cast<unsigned char*>(&cmdPacket.header.cmdFlags));
	calculatedCRC += static_cast<unsigned char>(flagsBits.count());
	std::bitset<8> lengthBits(cmdPacket.length);
	calculatedCRC += static_cast<unsigned char>(lengthBits.count());

	if (cmdPacket.Data != nullptr) {
		for (unsigned char i = 0; i < cmdPacket.length; i++) {
			std::bitset<8> dataBits(cmdPacket.Data[i]);
			calculatedCRC += static_cast<unsigned char>(dataBits.count());
		}
	}

	cmdPacket.CRC = calculatedCRC;
}

unsigned char* PktDef::genPacket() {
	if (RawBuffer != nullptr) {
		delete[] RawBuffer;
	}

	unsigned char totalSize = HEADERSIZE + 1 + cmdPacket.length + 1;
	RawBuffer = new unsigned char[totalSize];

	memcpy(RawBuffer, &cmdPacket.header, HEADERSIZE);
	RawBuffer[HEADERSIZE] = cmdPacket.length;

	if (cmdPacket.Data != nullptr) {
		memcpy(RawBuffer + HEADERSIZE + 1, cmdPacket.Data, cmdPacket.length);
	}

	calcCRC();
	RawBuffer[totalSize - 1] = cmdPacket.CRC;

	return RawBuffer;
}
