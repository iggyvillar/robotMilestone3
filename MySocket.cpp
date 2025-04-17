#include "MySocket.h"
#include <cstring>
#include <stdexcept>
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

MySocket::MySocket(SocketType socketType, std::string ipAddress, unsigned int port,
    ConnectionType connType, unsigned int bufferSize)
    : mySocket(socketType), IPAddr(ipAddress), Port(port),
    connectionType(connType), bTCPConnect(false) {

    if (bufferSize <= 0) {
        MaxSize = DEFAULT_SIZE;
    }
    else {
        MaxSize = bufferSize;
    }

    Buffer = new char[MaxSize];

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        throw std::runtime_error("WSAStartup failed");
    }
#endif

    if (connectionType == ConnectionType::TCP && socketType == SocketType::SERVER) {
        welcomeSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (welcomeSocket == INVALID_SOCKET) {
            throw std::runtime_error("Failed to create welcome socket");
        }

        SvrAddr.sin_family = AF_INET;
        SvrAddr.sin_addr.s_addr = INADDR_ANY;
        SvrAddr.sin_port = htons(port);

        if (bind(welcomeSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr)) == SOCKET_ERROR) {
            throw std::runtime_error("Failed to bind welcome socket");
        }

        if (listen(welcomeSocket, 1) == SOCKET_ERROR) {
            throw std::runtime_error("Failed to listen on welcome socket");
        }
    }
    else {
        connectionSocket = socket(AF_INET,
            (connectionType == ConnectionType::TCP) ? SOCK_STREAM : SOCK_DGRAM,
            (connectionType == ConnectionType::TCP) ? IPPROTO_TCP : IPPROTO_UDP);
        if (connectionSocket == INVALID_SOCKET) {
            throw std::runtime_error("Failed to create connection socket");
        }

        SvrAddr.sin_family = AF_INET;
        SvrAddr.sin_port = htons(port);

        if (ipAddress.empty() || ipAddress == "0.0.0.0") {
            SvrAddr.sin_addr.s_addr = INADDR_ANY;
        }
        else {
            if (inet_pton(AF_INET, ipAddress.c_str(), &SvrAddr.sin_addr) <= 0) {
                throw std::runtime_error("Invalid IP address");
            }
        }

        if (socketType == SocketType::SERVER && connectionType == ConnectionType::UDP) {
            if (bind(connectionSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr)) == SOCKET_ERROR) {
                throw std::runtime_error("Failed to bind UDP socket");
            }
        }
    }
}

MySocket::~MySocket() {
    if (bTCPConnect) {
        DisconnectTCP();
    }

    if (connectionType == ConnectionType::TCP && mySocket == SocketType::SERVER) {
#ifdef _WIN32
        closesocket(welcomeSocket);
#else
        close(welcomeSocket);
#endif
    }

#ifdef _WIN32
    closesocket(connectionSocket);
    WSACleanup();
#else
    close(connectionSocket);
#endif

    delete[] Buffer;
}

void MySocket::ConnectTCP() {
    if (connectionType != ConnectionType::TCP) {
        throw std::runtime_error("Cannot perform TCP operations on UDP socket");
    }

    if (mySocket == SocketType::SERVER) {
        sockaddr_in CltAddr;
        int addrLen = sizeof(CltAddr);
        connectionSocket = accept(welcomeSocket, (struct sockaddr*)&CltAddr, &addrLen);
        if (connectionSocket == INVALID_SOCKET) {
            throw std::runtime_error("Failed to accept connection");
        }
    }
    else {
        if (connect(connectionSocket, (struct sockaddr*)&SvrAddr, sizeof(SvrAddr)) == SOCKET_ERROR) {
            throw std::runtime_error("Failed to connect to server");
        }
    }
    bTCPConnect = true;
}

void MySocket::DisconnectTCP() {
    if (connectionType != ConnectionType::TCP) {
        throw std::runtime_error("Cannot perform TCP operations on UDP socket");
    }

    if (bTCPConnect) {
#ifdef _WIN32
        closesocket(connectionSocket);
#else
        close(connectionSocket);
#endif
        bTCPConnect = false;
    }
}

void MySocket::SendData(const char* rawData, int bytesToSend) {
    if (bytesToSend <= 0 || bytesToSend > MaxSize) {
        throw std::runtime_error("Invalid data size");
    }

    if (connectionType == ConnectionType::TCP && !bTCPConnect) {
        throw std::runtime_error("TCP connection not established");
    }

    int bytesSent = 0;
    if (connectionType == ConnectionType::TCP) {
        bytesSent = send(connectionSocket, rawData, bytesToSend, 0);
    }
    else {
        bytesSent = sendto(connectionSocket, rawData, bytesToSend, 0,
            (struct sockaddr*)&SvrAddr, sizeof(SvrAddr));
    }

    if (bytesSent == SOCKET_ERROR) {
        throw std::runtime_error("Failed to send data");
    }
}

int MySocket::GetData(char* destination) {
    if (connectionType == ConnectionType::TCP && !bTCPConnect) {
        throw std::runtime_error("TCP connection not established");
    }

    int bytesReceived = 0;
    if (connectionType == ConnectionType::TCP) {
        bytesReceived = recv(connectionSocket, Buffer, MaxSize, 0);
    }
    else {
        sockaddr_in senderAddr;
        socklen_t senderAddrSize = sizeof(senderAddr);
        bytesReceived = recvfrom(connectionSocket, Buffer, MaxSize, 0,
            (struct sockaddr*)&senderAddr, &senderAddrSize);
    }

    if (bytesReceived == SOCKET_ERROR) {
        throw std::runtime_error("Failed to receive data");
    }

    if (bytesReceived > 0) {
        memcpy(destination, Buffer, bytesReceived);
    }

    return bytesReceived;
}

std::string MySocket::GetIPAddr() const {
    return IPAddr;
}

void MySocket::SetIPAddr(std::string ipAddress) {
    if (bTCPConnect || (connectionType == ConnectionType::TCP && mySocket == SocketType::SERVER && welcomeSocket != INVALID_SOCKET)) {
        throw std::runtime_error("Cannot change IP address after connection is established");
    }
    IPAddr = ipAddress;
}

void MySocket::SetPort(int port) {
    if (bTCPConnect || (connectionType == ConnectionType::TCP && mySocket == SocketType::SERVER && welcomeSocket != INVALID_SOCKET)) {
        throw std::runtime_error("Cannot change port after connection is established");
    }
    Port = port;
}

int MySocket::GetPort() const {
    return Port;
}

SocketType MySocket::GetType() const {
    return mySocket;
}

void MySocket::SetType(SocketType socketType) {
    if (bTCPConnect || (connectionType == ConnectionType::TCP && mySocket == SocketType::SERVER && welcomeSocket != INVALID_SOCKET)) {
        throw std::runtime_error("Cannot change socket type after connection is established");
    }
    mySocket = socketType;
}