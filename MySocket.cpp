#include "MySocket.h"
#include <iostream>
#include <cstring>
#include <unistd.h> 

MySocket::MySocket(SocketType type, std::string ip, unsigned int port, ConnectionType conn, unsigned int size)
    : mySocket(type), IPAddr(ip), Port(port), connectionType(conn), bTCPConnect(false)
{
    MaxSize = (size > 0) ? size : DEFAULT_SIZE;
    Buffer = new char[MaxSize];
    memset(Buffer, 0, MaxSize);

    if (connectionType == ConnectionType::TCP)
        connectionSocket = socket(AF_INET, SOCK_STREAM, 0);
    else
        connectionSocket = socket(AF_INET, SOCK_DGRAM, 0);

    SvrAddr.sin_family = AF_INET;
    SvrAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &SvrAddr.sin_addr);

    if (type == SocketType::SERVER && connectionType == ConnectionType::TCP) {
        welcomeSocket = socket(AF_INET, SOCK_STREAM, 0);
        bind(welcomeSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr));
        listen(welcomeSocket, SOMAXCONN);
    }
}

MySocket::~MySocket() {
    delete[] Buffer;
    close(connectionSocket);
    if (mySocket == SocketType::SERVER && connectionType == ConnectionType::TCP)
        close(welcomeSocket);
}

void MySocket::ConnectTCP() {
    if (connectionType != ConnectionType::TCP) return;

    if (mySocket == SocketType::CLIENT) {
        connect(connectionSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr));
        bTCPConnect = true;
    }
    else {
        socklen_t size = sizeof(SvrAddr);
        connectionSocket = accept(welcomeSocket, (struct sockaddr*)&SvrAddr, &size);
        bTCPConnect = true;
    }
}

void MySocket::DisconnectTCP() {
    close(connectionSocket);
    bTCPConnect = false;
}

void MySocket::SendData(const char* data, int size) {
    if (connectionType == ConnectionType::TCP) {
        send(connectionSocket, data, size, 0);
    }
    else {
        sendto(connectionSocket, data, size, 0, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr));
    }
}

int MySocket::GetData(char* dest) {
    int bytes = 0;
    if (connectionType == ConnectionType::TCP) {
        bytes = recv(connectionSocket, Buffer, MaxSize, 0);
    }
    else {
        socklen_t addrLen = sizeof(SvrAddr);
        bytes = recvfrom(connectionSocket, Buffer, MaxSize, 0, (struct sockaddr*)&SvrAddr, &addrLen);
    }
    memcpy(dest, Buffer, bytes);
    return bytes;
}

std::string MySocket::GetIPAddr() const {
    return IPAddr;
}

void MySocket::SetIPAddr(std::string ip) {
    if (bTCPConnect) return;
    IPAddr = ip;
    inet_pton(AF_INET, ip.c_str(), &SvrAddr.sin_addr);
}

void MySocket::SetPort(int port) {
    if (bTCPConnect) return;
    Port = port;
    SvrAddr.sin_port = htons(port);
}

int MySocket::GetPort() const {
    return Port;
}

SocketType MySocket::GetType() const {
    return mySocket;
}

void MySocket::SetType(SocketType type) {
    if (bTCPConnect) return;
    mySocket = type;
}

