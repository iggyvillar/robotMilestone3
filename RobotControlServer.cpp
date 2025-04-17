#include <crow.h>
#include <crow/middlewares/cors.h>
#include "MySocket.h"
#include "PktDef.h"
#include <thread>
#include <mutex>

std::mutex socketMutex;
MySocket* robotSocket = nullptr;
unsigned short pktCounter = 0;

void initializeWebServer(crow::App<crow::CORSHandler>& app) {
    CROW_ROUTE(app, "/")([]() {
        crow::mustache::context ctx;
        return crow::mustache::load("index.html").render();
        });

    CROW_ROUTE(app, "/connect").methods("POST"_method)
        ([](const crow::request& req) {
        auto x = crow::json::load(req.body);
        if (!x) {
            return crow::response(400, "Invalid JSON");
        }

        std::string ip = x["ip"].s();
        int port = x["port"].i();

        std::lock_guard<std::mutex> lock(socketMutex);
        if (robotSocket != nullptr) {
            delete robotSocket;
        }

        try {
            robotSocket = new MySocket(SocketType::CLIENT, ip, port, ConnectionType::UDP, 1024);
            return crow::response(200, "Connected to robot at " + ip + ":" + std::to_string(port));
        }
        catch (const std::exception& e) {
            return crow::response(500, std::string("Connection failed: ") + e.what());
        }
            });

    CROW_ROUTE(app, "/telecommand").methods("PUT"_method)
        ([](const crow::request& req) {
        if (robotSocket == nullptr) {
            return crow::response(400, "Not connected to robot");
        }

        auto x = crow::json::load(req.body);
        if (!x) {
            return crow::response(400, "Invalid JSON");
        }

        std::string command = x["command"].s();
        PktDef packet;

        try {
            if (command == "drive") {
                unsigned char direction = x["direction"].i();
                unsigned char duration = x["duration"].i();
                unsigned char speed = x["speed"].i();

                // Validate speed
                if (speed < 80) speed = 80;
                if (speed > 100) speed = 100;

                packet.setCMD(CMDType::DRIVE);
                packet.setPktCount(++pktCounter);

                driveBody body;
                body.direction = direction;
                body.duration = duration;
                body.speed = speed;

                packet.setBodyData(reinterpret_cast<unsigned char*>(&body), sizeof(driveBody));
            }
            else if (command == "sleep") {
                packet.setCMD(CMDType::SLEEP);
                packet.setPktCount(++pktCounter);
            }
            else {
                return crow::response(400, "Invalid command");
            }

            packet.calcCRC();
            unsigned char* rawPacket = packet.genPacket();

            std::lock_guard<std::mutex> lock(socketMutex);
            robotSocket->SendData(reinterpret_cast<const char*>(rawPacket), packet.getLength());

            // Wait for ACK
            char ackBuffer[1024];
            int bytesReceived = robotSocket->GetData(ackBuffer);
            if (bytesReceived > 0) {
                PktDef ackPacket(reinterpret_cast<unsigned char*>(ackBuffer));
                if (ackPacket.getAck() && ackPacket.checkCRC(ackBuffer, bytesReceived)) {
                    return crow::response(200, "Command acknowledged");
                }
            }
            return crow::response(500, "No ACK received from robot");
        }
        catch (const std::exception& e) {
            return crow::response(500, std::string("Command failed: ") + e.what());
        }
            });

    CROW_ROUTE(app, "/telemetry_request").methods("GET"_method)
        ([]() {
        if (robotSocket == nullptr) {
            return crow::response(400, "Not connected to robot");
        }

        try {
            PktDef packet;
            packet.setCMD(CMDType::RESPONSE);
            packet.setPktCount(++pktCounter);
            packet.calcCRC();
            unsigned char* rawPacket = packet.genPacket();

            std::lock_guard<std::mutex> lock(socketMutex);
            robotSocket->SendData(reinterpret_cast<const char*>(rawPacket), packet.getLength());

            // Wait for telemetry response
            char telemetryBuffer[1024];
            int bytesReceived = robotSocket->GetData(telemetryBuffer);
            if (bytesReceived > 0) {
                PktDef telemetryPacket(reinterpret_cast<unsigned char*>(telemetryBuffer));
                if (telemetryPacket.checkCRC(telemetryBuffer, bytesReceived)) {
                    telemetry* tm = reinterpret_cast<telemetry*>(telemetryPacket.getBodyData());

                    crow::json::wvalue response;
                    response["last_pkt_counter"] = tm->LastPktCounter;
                    response["current_grade"] = tm->CurrentGrade;
                    response["hit_count"] = tm->HitCount;
                    response["last_cmd"] = static_cast<int>(tm->LastCmd);
                    response["last_cmd_value"] = static_cast<int>(tm->LastCmdValue);
                    response["last_cmd_speed"] = static_cast<int>(tm->LastCmdSpeed);

                    return crow::response(response);
                }
            }
            return crow::response(500, "No telemetry received from robot");
        }
        catch (const std::exception& e) {
            return crow::response(500, std::string("Telemetry request failed: ") + e.what());
        }
            });
}

int main() {
    crow::App<crow::CORSHandler> app;

    // Configure CORS
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors
        .global()
        .headers("X-Custom-Header", "Upgrade-Insecure-Requests")
        .methods("POST"_method, "GET"_method, "PUT"_method)
        .prefix("/")
        .origin("*")
        .prefix("/connect")
        .origin("*")
        .prefix("/telecommand")
        .origin("*")
        .prefix("/telemetry_request")
        .origin("*");

    initializeWebServer(app);

    app.port(18080).multithreaded().run();
}