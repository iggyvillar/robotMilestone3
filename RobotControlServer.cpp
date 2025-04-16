#include "crow_all.h"
#include "PktDef.h"
#include "MySocket.h"
#include <string>
#include <memory>
#include <filesystem>

class RobotControlServer {
private:
    crow::SimpleApp app;
    std::unique_ptr<MySocket> robotSocket;
    std::string robotIP;
    int robotPort;
    bool isConnected;

public:
    RobotControlServer() : isConnected(false) {
        setupRoutes();
    }

    void setupRoutes() {
        // Serve static files from the public directory
        CROW_ROUTE(app, "/public/<path>")
        ([](const crow::request& req, crow::response& res, std::string path) {
            std::string full_path = "public/" + path;
            if (std::filesystem::exists(full_path)) {
                res.set_static_file_info(full_path);
                res.end();
            } else {
                res.code = 404;
                res.end("File not found");
            }
        });

        // Root route - serves the main GUI
        CROW_ROUTE(app, "/")
        ([]() {
            return crow::mustache::load("index.html").render();
        });

        // Connect route
        CROW_ROUTE(app, "/connect")
            .methods("POST"_method)
        ([this](const crow::request& req) {
            auto x = crow::json::load(req.body);
            if (!x) {
                return crow::response(400, "Invalid JSON");
            }

            robotIP = x["ip"].s();
            robotPort = x["port"].i();

            try {
                robotSocket = std::make_unique<MySocket>(
                    SocketType::CLIENT, robotIP, robotPort, ConnectionType::TCP);                
                robotSocket->ConnectTCP();
                isConnected = true;
                return crow::response(200, "Connected successfully");
            } catch (const std::exception& e) {
                return crow::response(500, "Connection failed: " + std::string(e.what()));
            }
        });

        // Telecommand route
        CROW_ROUTE(app, "/telecommand")
            .methods("PUT"_method)
        ([this](const crow::request& req) {
            if (!isConnected) {
                return crow::response(400, "Not connected to robot");
            }

            auto x = crow::json::load(req.body);
            if (!x) {
                return crow::response(400, "Invalid JSON");
            }

            // Create and send command packet
            PktDef cmdPacket;
            if (x["command"].s() == "drive") {
                cmdPacket.setCMD(CMDType::DRIVE);
                driveBody body;
                body.direction = x["direction"].i();
                body.duration = x["duration"].i();
                body.speed = x["speed"].i();
                cmdPacket.setBodyData(reinterpret_cast<unsigned char*>(&body), sizeof(driveBody));
            } else if (x["command"].s() == "sleep") {
                cmdPacket.setCMD(CMDType::SLEEP);
            }

            cmdPacket.calcCRC();
            unsigned char* packet = cmdPacket.genPacket();
            robotSocket->SendData(packet, cmdPacket.getLength() + HEADERSIZE + 1);
            delete[] packet;

            return crow::response(200, "Command sent successfully");
        });

        // Telemetry request route
        CROW_ROUTE(app, "/telemetry_request")
            .methods("GET"_method)
        ([this]() {
            if (!isConnected) {
                return crow::response(400, "Not connected to robot");
            }

            PktDef cmdPacket;
            cmdPacket.setCMD(CMDType::RESPONSE);
            cmdPacket.calcCRC();
            unsigned char* packet = cmdPacket.genPacket();
            robotSocket->SendData(packet, cmdPacket.getLength() + HEADERSIZE + 1);
            delete[] packet;

            // Receive response
            unsigned char buffer[1024];
            int bytesReceived = robotSocket->GetData(buffer, 1024);
            if (bytesReceived > 0) {
                PktDef responsePacket(buffer);
                if (responsePacket.checkCRC(buffer, bytesReceived)) {
                    telemetry* telemetryData = reinterpret_cast<telemetry*>(responsePacket.getBodyData());
                    crow::json::wvalue response;
                    response["last_pkt_counter"] = telemetryData->LastPktCounter;
                    response["current_grade"] = telemetryData->CurrentGrade;
                    response["hit_count"] = telemetryData->HitCount;
                    response["last_cmd"] = telemetryData->LastCmd;
                    response["last_cmd_value"] = telemetryData->LastCmdValue;
                    response["last_cmd_speed"] = telemetryData->LastCmdSpeed;
                    return crow::response(200, response);
                }
            }
            return crow::response(500, "Failed to get telemetry");
        });
    }

    void run(int port = 8080) {
        app.port(port).multithreaded().run();
    }
};

int main() {
    RobotControlServer server;
    server.run();
    return 0;
} 