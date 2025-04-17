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
            cmdPacket.header.length = HEADERSIZE + 1;  // header + CRC
            break;
        case CMDType::RESPONSE:
            cmdPacket.header.cmdFlags.status = 1;
            cmdPacket.header.length = HEADERSIZE + 1;  // header + CRC
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
    int size = cmdPacket.header.length - 1; // exclude the CRC byte
    unsigned char* temp = new unsigned char[size];

    // copy header
    memcpy(temp, &cmdPacket.header, HEADERSIZE);

    // copy body
    if (cmdPacket.Data != nullptr) {
        memcpy(temp + HEADERSIZE, cmdPacket.Data, cmdPacket.header.length - HEADERSIZE - 1);
    }

    uint8_t crc = 0;
    for (int i = 0; i < size; ++i) {
        std::bitset<8> bits(temp[i]);
        crc += bits.count();
    }

    delete[] temp;
    cmdPacket.CRC = crc;
    std::cout << "[calcCRC()] Final CRC = " << (int)crc << std::endl;
}


unsigned char* PktDef::genPacket() {
    if (RawBuffer != nullptr)
        delete[] RawBuffer;

    int totalSize = cmdPacket.header.length;
    if (totalSize < HEADERSIZE + 1)  
        totalSize = HEADERSIZE + 1;

    RawBuffer = new unsigned char[totalSize];

    // header
    memcpy(RawBuffer, &cmdPacket.header, HEADERSIZE);

    if (cmdPacket.Data != nullptr && totalSize > HEADERSIZE + 1) {
        memcpy(RawBuffer + HEADERSIZE, cmdPacket.Data, totalSize - HEADERSIZE - 1);
    }

    // calc and write CRC
    calcCRC();
    RawBuffer[totalSize - 1] = cmdPacket.CRC;

    return RawBuffer;
}

