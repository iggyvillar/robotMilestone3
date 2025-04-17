#include "PktDef.h"
#include <cstring>
#include <bitset>
#include <iostream>

//define default constructor
PktDef::PktDef() {
	cmdPacket.header.PktCount = 0;
	cmdPacket.header.cmdFlags.drive = 0;
	cmdPacket.header.cmdFlags.ack = 0;
	cmdPacket.header.cmdFlags.sleep = 0;
	cmdPacket.header.cmdFlags.status = 0;
	cmdPacket.header.cmdFlags.padding = 0;
	cmdPacket.header.length = 0;
	cmdPacket.Data = nullptr;
	cmdPacket.CRC = 0;
	RawBuffer = nullptr;
}

//overloaded constructor
PktDef::PktDef(unsigned char* rawData) {
	memcpy(&cmdPacket.header, rawData, HEADERSIZE);
	cmdPacket.header.length = rawData[HEADERSIZE];
	if (cmdPacket.header.length > 0) {
		cmdPacket.Data = new unsigned char[cmdPacket.header.length];
		memcpy(cmdPacket.Data, rawData + HEADERSIZE + 1, cmdPacket.header.length);
	}
	else {
		cmdPacket.Data = nullptr;
	}
	cmdPacket.CRC = rawData[HEADERSIZE + 1 + cmdPacket.header.length];
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

    cmdPacket.header.length = HEADERSIZE + size + 1;  // body + header + CRC
}


/*
void PktDef::setBodyData(unsigned char* data, unsigned char size) {
	if (cmdPacket.Data != nullptr) {
		delete[] cmdPacket.Data;
	}
	cmdPacket.Data = new unsigned char[size];
	memcpy(cmdPacket.Data, data, size);
	cmdPacket.header.length = size;
}*/

void PktDef::setPktCount(unsigned short int size) {
	cmdPacket.header.PktCount = size + 1;
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
	return cmdPacket.header.length;
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
    unsigned char crc = 0;

    unsigned char* headerBytes = (unsigned char*)&cmdPacket.header;
    for (int i = 0; i < HEADERSIZE; i++) {
        crc += std::bitset<8>(headerBytes[i]).count();
    }

    if (cmdPacket.Data != nullptr) {
        for (int i = 0; i < cmdPacket.header.length; i++) {
            crc += std::bitset<8>(cmdPacket.Data[i]).count();
        }
    }

    cmdPacket.CRC = crc;

    // Debug output to confirm it's being called
    std::cout << "[calcCRC()] Final CRC = " << (int)crc << std::endl;
}


unsigned char* PktDef::genPacket() {
    if (RawBuffer != nullptr)
        delete[] RawBuffer;

    int totalSize = cmdPacket.header.length;
    RawBuffer = new unsigned char[totalSize];

    // Write header first
    memcpy(RawBuffer, &cmdPacket.header, HEADERSIZE);

    // Then the body
    if (cmdPacket.Data != nullptr) {
        memcpy(RawBuffer + HEADERSIZE, cmdPacket.Data, totalSize - HEADERSIZE - 1);
    }

    // Now calculate CRC AFTER body is copied
    calcCRC();

    // ✅ Now insert CRC at very end
    RawBuffer[totalSize - 1] = cmdPacket.CRC;

    return RawBuffer;
}

