#include "crow_all.h"
#include "PktDef.h"
#include "MySocket.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>

using namespace std;
using namespace crow;

unique_ptr<MySocket> socketPtr;
int packetCount = 0;

// Utility to load static files
string loadFile(const string& path) {
    ifstream file(path);
    if (file) {
        stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
    return "404 - File Not Found";
}

void sendPacket(CMDType cmd, unsigned char* data = nullptr, int size = 0) {
    PktDef pkt;
    pkt.setPktCount(++packetCount);
    pkt.setCMD(cmd);

    if (data && size > 0)
        pkt.setBodyData(data, size);

    unsigned char* buffer = pkt.genPacket();


    int totalSize = pkt.getLength();
    /* ignore debugging purposes
    cout << "[SENDING] Command: " << (int)cmd
         << ", Total Size: " << totalSize
         << ", CRC: " << (int)buffer[totalSize - 1] << endl;

    cout << "Packet Bytes: ";
    for (int i = 0; i < totalSize; i++) {
        printf("%02X ", buffer[i]);
    }
    cout << endl;
    */
    socketPtr->SendData((char*)buffer, totalSize);
}



// Convert telemetry packet to JSON
json::wvalue parseTelemetry(unsigned char* buffer, int length) {
    json::wvalue json;
    PktDef pkt(buffer);

    if (!pkt.checkCRC(buffer, length)) {
        json["error"] = "CRC validation failed";
        return json;
    }

    if (pkt.getCMD() != CMDType::RESPONSE || !pkt.getBodyData()) {
        json["error"] = "Invalid response packet";
        return json;
    }

    telemetry* data = (telemetry*)pkt.getBodyData();
    json["LastPktCounter"] = data->LastPktCounter;
    json["CurrentGrade"] = data->CurrentGrade;
    json["HitCount"] = data->HitCount;
    json["LastCmd"] = data->LastCmd;
    json["LastCmdValue"] = data->LastCmdValue;
    json["LastCmdSpeed"] = data->LastCmdSpeed;
    return json;
}

int main() {
    crow::SimpleApp app;

    // Serve HTML
    CROW_ROUTE(app, "/")([] {
        return loadFile("../public/index.html");
    });
    // Connect route
    CROW_ROUTE(app, "/connect").methods(HTTPMethod::Post)([](const request& req) {
        auto json = crow::json::load(req.body);
        if (!json || !json.has("ip") || !json.has("port"))
            return response(400, "invalid");

        string ip = json["ip"].s();
        int port = json["port"].i();

        socketPtr = make_unique<MySocket>(SocketType::CLIENT, ip, port, ConnectionType::UDP, DEFAULT_SIZE);
        return response(200, "connected successfully to robot");
    });

    // Telecommand (drive / sleep)
    CROW_ROUTE(app, "/telecommand").methods(HTTPMethod::Put)([](const request& req) {
        auto json = crow::json::load(req.body);
        if (!json || !json.has("command"))
            return response(400, "missing command ");

        string command = json["command"].s();

        if (command == "drive") {
            if (!json.has("direction") || !json.has("duration") || !json.has("speed"))
                return response(400, "missing drive params");

            unsigned char payload[3];
            payload[0] = (unsigned char)json["direction"].i();
            payload[1] = (unsigned char)json["duration"].i();
            payload[2] = (unsigned char)json["speed"].i();

            sendPacket(CMDType::DRIVE, payload, 3);
        }
        else if (command == "sleep") {
            sendPacket(CMDType::SLEEP);
        }
        else {
            return response(400, "command not supported");
        }

        return response(200, "command sent");
    });

    // telemetry req
    CROW_ROUTE(app, "/telemetry_request").methods(HTTPMethod::Get)([](const request&) {
        sendPacket(CMDType::RESPONSE);

        char raw[DEFAULT_SIZE];
        int received = socketPtr->GetData(raw);
        if (received <= 0)
            return response(500, "no telemetry response received");

        return response(parseTelemetry((unsigned char*)raw, received));
    });

    app.port(8080).multithreaded().run();
    return 0;
}
