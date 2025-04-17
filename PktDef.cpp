#include "PktDef.h"
#include <cstring>
#include <stdexcept>

PktDef::PktDef() {
    cmdPacket.header.PktCount = 0;
    cmdPacket.header.cmdFlags.drive = 0;
    cmdPacket.header.cmdFlags.status = 0;
    cmdPacket.header.cmdFlags.sleep = 0;
    cmdPacket.header.cmdFlags.ack = 0;
    cmdPacket.header.cmdFlags.padding = 0;
    cmdPacket.length = HEADERSIZE + 1; // Header + CRC (no body)
    cmdPacket.Data = nullptr;
    cmdPacket.CRC = 0;
    RawBuffer = nullptr;
}

PktDef::PktDef(unsigned char* rawData) {
    if (rawData == nullptr) {
        throw std::invalid_argument("Raw data buffer is null");
    }

    // Deserialize header
    memcpy(&cmdPacket.header, rawData, sizeof(Header));

    // Get length
    cmdPacket.length = rawData[sizeof(Header)];

    // Deserialize body if present
    unsigned char bodySize = cmdPacket.length - HEADERSIZE - 1; // Subtract header and CRC
    if (bodySize > 0) {
        cmdPacket.Data = new unsigned char[bodySize];
        memcpy(cmdPacket.Data, rawData + sizeof(Header) + 1, bodySize);
    }
    else {
        cmdPacket.Data = nullptr;
    }

    // Deserialize CRC
    cmdPacket.CRC = rawData[cmdPacket.length - 1];

    RawBuffer = nullptr;
}

void PktDef::setCMD(CMDType cmd) {
    cmdPacket.header.cmdFlags.drive = 0;
    cmdPacket.header.cmdFlags.status = 0;
    cmdPacket.header.cmdFlags.sleep = 0;
    cmdPacket.header.cmdFlags.ack = 0;

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

    if (size > 0 && data != nullptr) {
        cmdPacket.Data = new unsigned char[size];
        memcpy(cmdPacket.Data, data, size);
        cmdPacket.length = HEADERSIZE + 1 + size; // Header + CRC + body
    }
    else {
        cmdPacket.Data = nullptr;
        cmdPacket.length = HEADERSIZE + 1; // Header + CRC (no body)
    }
}

void PktDef::setPktCount(unsigned short int count) {
    cmdPacket.header.PktCount = count;
}

CMDType PktDef::getCMD() {
    if (cmdPacket.header.cmdFlags.drive) {
        return CMDType::DRIVE;
    }
    else if (cmdPacket.header.cmdFlags.sleep) {
        return CMDType::SLEEP;
    }
    else if (cmdPacket.header.cmdFlags.status) {
        return CMDType::RESPONSE;
    }
    throw std::runtime_error("No command flag set");
}

bool PktDef::getAck() {
    return cmdPacket.header.cmdFlags.ack == 1;
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
    if (buffer == nullptr || size == 0) {
        return false;
    }

    unsigned char calculatedCRC = 0;
    for (int i = 0; i < size - 1; i++) { // Exclude the CRC byte itself
        unsigned char byte = buffer[i];
        for (int j = 0; j < 8; j++) {
            calculatedCRC += (byte >> j) & 0x01;
        }
    }

    return calculatedCRC == buffer[size - 1];
}

void PktDef::calcCRC() {
    if (RawBuffer != nullptr) {
        delete[] RawBuffer;
    }

    // Generate the packet to calculate CRC
    genPacket();

    // Now calculate CRC based on RawBuffer (excluding the CRC byte itself)
    cmdPacket.CRC = 0;
    for (int i = 0; i < cmdPacket.length - 1; i++) {
        unsigned char byte = RawBuffer[i];
        for (int j = 0; j < 8; j++) {
            cmdPacket.CRC += (byte >> j) & 0x01;
        }
    }

    // Update the CRC in the RawBuffer
    RawBuffer[cmdPacket.length - 1] = cmdPacket.CRC;
}

unsigned char* PktDef::genPacket() {
    if (RawBuffer != nullptr) {
        delete[] RawBuffer;
    }

    RawBuffer = new unsigned char[cmdPacket.length];

    // Serialize header
    memcpy(RawBuffer, &cmdPacket.header, sizeof(Header));

    // Serialize length
    RawBuffer[sizeof(Header)] = cmdPacket.length;

    // Serialize body if present
    unsigned char bodySize = cmdPacket.length - HEADERSIZE - 1;
    if (bodySize > 0 && cmdPacket.Data != nullptr) {
        memcpy(RawBuffer + sizeof(Header) + 1, cmdPacket.Data, bodySize);
    }

    // Serialize CRC
    RawBuffer[cmdPacket.length - 1] = cmdPacket.CRC;

    return RawBuffer;
}

PktDef::~PktDef() {
    if (cmdPacket.Data != nullptr) {
        delete[] cmdPacket.Data;
    }
    if (RawBuffer != nullptr) {
        delete[] RawBuffer;
    }
}