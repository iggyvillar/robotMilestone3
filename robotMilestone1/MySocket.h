#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

enum class SocketType { CLIENT, SERVER };
enum class ConnectionType { TCP, UDP };

const int DEFAULT_SIZE = 1024;

class MySocket {
private:
    char* Buffer;
    SOCKET WelcomeSocket;
    SOCKET ConnectionSocket;
    sockaddr_in SvrAddr;
    SocketType mySocket;
    std::string IPAddr;
    int Port;
    ConnectionType connectionType;
    bool bTCPConnect;
    int MaxSize;

public:
    MySocket(SocketType, std::string, unsigned int, ConnectionType, unsigned int);
    ~MySocket();

    void ConnectTCP();
    void DisconnectTCP();
    void SendData(const char*, int);
    int GetData(char*);

    std::string GetIPAddr() const;
    void SetIPAddr(std::string);
    void SetPort(int);
    int GetPort() const;
    SocketType GetType() const;
    void SetType(SocketType);
};
