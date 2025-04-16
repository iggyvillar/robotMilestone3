#include "MySocket.h"
#include <iostream>

MySocket::MySocket(SocketType type, std::string ip, unsigned int port, ConnectionType conn, unsigned int size)
    : mySocket(type), IPAddr(ip), Port(port), connectionType(conn), bTCPConnect(false)
{
    MaxSize = (size > 0) ? size : DEFAULT_SIZE;
    Buffer = new char[MaxSize];
    memset(Buffer, 0, MaxSize);

    WSAData wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (connectionType == ConnectionType::TCP)
        ConnectionSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    else
        ConnectionSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    SvrAddr.sin_family = AF_INET;
    SvrAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &SvrAddr.sin_addr);

    if (type == SocketType::SERVER && connectionType == ConnectionType::TCP) {
        WelcomeSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        bind(WelcomeSocket, (SOCKADDR*)&SvrAddr, sizeof(SvrAddr));
        listen(WelcomeSocket, SOMAXCONN);
    }
}

MySocket::~MySocket() {
    delete[] Buffer;
    closesocket(ConnectionSocket);
    if (mySocket == SocketType::SERVER && connectionType == ConnectionType::TCP)
        closesocket(WelcomeSocket);
    WSACleanup();
}

void MySocket::ConnectTCP() {
    if (connectionType != ConnectionType::TCP) return;

    if (mySocket == SocketType::CLIENT) {
        connect(ConnectionSocket, (SOCKADDR*)&SvrAddr, sizeof(SvrAddr));
        bTCPConnect = true;
    }
    else {
        int size = sizeof(SOCKADDR);
        ConnectionSocket = accept(WelcomeSocket, (SOCKADDR*)&SvrAddr, &size);
        bTCPConnect = true;
    }
}

void MySocket::DisconnectTCP() {
    closesocket(ConnectionSocket);
    bTCPConnect = false;
}

void MySocket::SendData(const char* data, int size) {
    if (connectionType == ConnectionType::TCP) {
        send(ConnectionSocket, data, size, 0);
    }
    else {
        sendto(ConnectionSocket, data, size, 0, (SOCKADDR*)&SvrAddr, sizeof(SvrAddr));
    }
}

int MySocket::GetData(char* dest) {
    int bytes = 0;
    if (connectionType == ConnectionType::TCP) {
        bytes = recv(ConnectionSocket, Buffer, MaxSize, 0);
    }
    else {
        int addrLen = sizeof(SvrAddr);
        bytes = recvfrom(ConnectionSocket, Buffer, MaxSize, 0, (SOCKADDR*)&SvrAddr, &addrLen);
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
