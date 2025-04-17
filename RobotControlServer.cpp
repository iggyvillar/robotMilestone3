/*#define CROW_MAIN

#include "crow_all.h"
#include "PktDef.h"
#include "MySocket.h"
#include <fstream>
#include <sstream>
#include <iostream>
using namespace std;

// Utility function to serve files
void sendFile(crow::response& res, const string& path, const string& contentType) {
    ifstream in("/app/public/" + path, ifstream::in);
    if (in) {
        ostringstream contents;
        contents << in.rdbuf();
        in.close();
        res.set_header("Content-Type", contentType);
        res.write(contents.str());
    } else {
        res.code = 404;
        res.write("404 - Not found");
    }
    res.end();
}

// Specific file senders
void sendHtml(crow::response& res, const string& filename) {
    sendFile(res, filename + ".html", "text/html");
}

void sendScript(crow::response& res, const string& filename) {
    sendFile(res, "js/" + filename, "application/javascript");
}

void sendStyle(crow::response& res, const string& filename) {
    sendFile(res, "css/" + filename, "text/css");
}

int main() {
    crow::SimpleApp app;
    MySocket* connection = nullptr;
    PktDef* packet = nullptr;
    int packetCount = 0;

    // Serve the index.html
    CROW_ROUTE(app, "/")([](const crow::request&, crow::response& res) {
        sendHtml(res, "index");
    });

    // Route to serve CSS/JS
    CROW_ROUTE(app, "/css/<string>")([](const crow::request&, crow::response& res, string filename) {
        sendStyle(res, filename);
    });

    CROW_ROUTE(app, "/js/<string>")([](const crow::request&, crow::response& res, string filename) {
        sendScript(res, filename);
    });

    // Connect to the robot
    CROW_ROUTE(app, "/connect/<string>/<int>/<string>").methods(crow::HTTPMethod::Post)
    ([&connection](const crow::request&, crow::response& res, string ip, int port, string connType) {
        if (connType == "TCP") {
            connection = new MySocket(SocketType::CLIENT, ip, port, ConnectionType::TCP, 255);
        } else {
            connection = new MySocket(SocketType::CLIENT, ip, port, ConnectionType::UDP, 255);
        }
        std::cerr << "[SOCKET] Connected to " << ip << ":" << port << " using " << connType << endl;
        res.code = 201;
        res.write("Connected.");
        res.end();
    });

    // Telecommand
    CROW_ROUTE(app, "/telecommand/<string>/<string>/<int>/<int>").methods(crow::HTTPMethod::Post)
    ([&connection, &packet, &packetCount](const crow::request&, crow::response& res, string cmdStr, string dirStr, int duration, int speed) {
        if (!connection) {
            res.code = 500;
            res.write("Socket not connected.");
            res.end();
            return;
        }

        CMDType cmd;
        if (cmdStr == "DRIVE") cmd = CMDType::DRIVE;
        else if (cmdStr == "SLEEP") cmd = CMDType::SLEEP;
        else if (cmdStr == "RESPONSE") cmd = CMDType::RESPONSE;
        else {
            res.code = 400;
            res.write("Invalid command.");
            res.end();
            return;
        }

        unsigned char direction;
        if (dirStr == "FORWARD") direction = FORWARD;
        else if (dirStr == "BACKWARD") direction = BACKWARD;
        else if (dirStr == "LEFT") direction = LEFT;
        else if (dirStr == "RIGHT") direction = RIGHT;
        else direction = 0;

        if (packet) delete packet;
        packet = new PktDef();
        packet->setPktCount(++packetCount);
        packet->setCMD(cmd);

        driveBody body;
        body.direction = direction;
        body.duration = (unsigned char)duration;
        body.speed = (unsigned char)speed;
        packet->setBodyData((unsigned char*)&body, sizeof(body));
        unsigned char* raw = packet->genPacket();

        connection->SendData((const char*)raw, packet->getLength());

        if (cmd == CMDType::RESPONSE) {
            char buffer[1024];
            int received = connection->GetData(buffer);
            if (received > 0) {
                PktDef responsePacket((unsigned char*)buffer);
                if (responsePacket.checkCRC((unsigned char*)buffer, received)) {
                    telemetry* t = (telemetry*)responsePacket.getBodyData();
                    crow::json::wvalue json;
                    json["LastPktCounter"] = t->LastPktCounter;
                    json["CurrentGrade"] = t->CurrentGrade;
                    json["HitCount"] = t->HitCount;
                    json["LastCmd"] = t->LastCmd;
                    json["LastCmdValue"] = t->LastCmdValue;
                    json["LastCmdSpeed"] = t->LastCmdSpeed;
                    res.code = 200;
                    res.write(json.dump());
                } else {
                    res.code = 500;
                    res.write("Telemetry CRC check failed.");
                }
            } else {
                res.code = 500;
                res.write("No telemetry response received.");
            }
        } else {
            res.code = 200;
            res.write("Command sent.");
        }

        res.end();
    });

    app.port(23500).multithreaded().run();
    return 0;
}/*
/** */
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

// Send a packet to robot using your PktDef format
void sendPacket(CMDType cmd, unsigned char* data = nullptr, int size = 0) {
    PktDef pkt;
    pkt.setPktCount(++packetCount);
    pkt.setCMD(cmd);
    if (data && size > 0)
        pkt.setBodyData(data, size);
    pkt.calcCRC();
    unsigned char* buffer = pkt.genPacket();
    socketPtr->SendData((char*)buffer, pkt.getLength());
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

    // Serve CSS
    CROW_ROUTE(app, "/styles/<string>")([](string filename) {
        response res;
        res.set_header("Content-Type", "text/css");
        res.write(loadFile("../public/styles/" + filename));
        return res;
    });

    // Serve JS
    CROW_ROUTE(app, "/scripts/<string>")([](string filename) {
        response res;
        res.set_header("Content-Type", "application/javascript");
        res.write(loadFile("../public/scripts/" + filename));
        return res;
    });

    // Connect route
    CROW_ROUTE(app, "/connect").methods(HTTPMethod::Post)([](const request& req) {
        auto json = crow::json::load(req.body);
        if (!json || !json.has("ip") || !json.has("port"))
            return response(400, "Invalid connect payload");

        string ip = json["ip"].s();
        int port = json["port"].i();

        socketPtr = make_unique<MySocket>(SocketType::CLIENT, ip, port, ConnectionType::UDP, DEFAULT_SIZE);
        return response(200, "Connected successfully to robot");
    });

    // Telecommand (drive / sleep)
    CROW_ROUTE(app, "/telecommand").methods(HTTPMethod::Put)([](const request& req) {
        auto json = crow::json::load(req.body);
        if (!json || !json.has("command"))
            return response(400, "Missing command field");

        string command = json["command"].s();

        if (command == "drive") {
            if (!json.has("direction") || !json.has("duration") || !json.has("speed"))
                return response(400, "Missing drive parameters");

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
            return response(400, "Unsupported command");
        }

        return response(200, "Command sent");
    });

    // Telemetry request
    CROW_ROUTE(app, "/telemetry_request").methods(HTTPMethod::Get)([](const request&) {
        sendPacket(CMDType::RESPONSE);

        char raw[DEFAULT_SIZE];
        int received = socketPtr->GetData(raw);
        if (received <= 0)
            return response(500, "No telemetry response received");

        return response(parseTelemetry((unsigned char*)raw, received));
    });

    app.port(8080).multithreaded().run();
    return 0;
}
