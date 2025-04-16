#include "pch.h"
#include "CppUnitTest.h"
#include "../robotMilestone1/PktDef.h"
#include "../robotMilestone1/MySocket.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft {
    namespace VisualStudio {
        namespace CppUnitTestFramework {

            template<> static std::wstring ToString<SocketType>(const SocketType& st) {
                switch (st) {
                case SocketType::CLIENT: return L"CLIENT";
                case SocketType::SERVER: return L"SERVER";
                default: return L"UNKNOWN";
                }
            }

            template<> static std::wstring ToString<ConnectionType>(const ConnectionType& ct) {
                switch (ct) {
                case ConnectionType::TCP: return L"TCP";
                case ConnectionType::UDP: return L"UDP";
                default: return L"UNKNOWN";
                }
            }

        }
    }
}

namespace PktDefTests
{
    TEST_CLASS(PktDefTests)
    {
    public:
        TEST_METHOD(defaultConstructorTest)
        {
            PktDef packet;
            Assert::AreEqual((unsigned short int)0, packet.getPktCount());
            Assert::IsFalse(packet.getAck());
            Assert::AreEqual((unsigned char)0, packet.getLength());
            Assert::IsNull(packet.getBodyData());
        }

        TEST_METHOD(genPacketTest)
        {
            PktDef packet;
            packet.setPktCount(1);
            packet.setCMD(CMDType::DRIVE);
            unsigned char testData[] = "lebron";
            packet.setBodyData(testData, 6);

            unsigned char* rawPacket = packet.genPacket();
            Assert::IsNotNull(rawPacket);

            unsigned short int pktCount = *reinterpret_cast<unsigned short int*>(rawPacket);
            Assert::AreEqual((unsigned short int)1, pktCount);

            unsigned char flags = rawPacket[2];
            Assert::IsTrue(flags & 0x01); // DRIVE flag
        }

        TEST_METHOD(emptyPacketTest)
        {
            PktDef packet;
            packet.setPktCount(1);
            packet.setCMD(CMDType::DRIVE);

            unsigned char* rawPacket = packet.genPacket();
            Assert::IsNotNull(rawPacket);
            Assert::AreEqual((unsigned char)0, packet.getLength());
            Assert::IsNull(packet.getBodyData());
        }

        TEST_METHOD(setBodyDataTest)
        {
            PktDef packet;
            unsigned char testData[] = "lebron";
            packet.setBodyData(testData, 6);
            Assert::AreEqual((unsigned char)6, packet.getLength());
            Assert::IsNotNull(packet.getBodyData());
        }

        TEST_METHOD(setCMDTest)
        {
            PktDef packet;
            packet.setCMD(CMDType::DRIVE);
            Assert::IsTrue(packet.getCMD() == CMDType::DRIVE);

            packet.setCMD(CMDType::SLEEP);
            Assert::IsTrue(packet.getCMD() == CMDType::SLEEP);

            packet.setCMD(CMDType::RESPONSE);
            Assert::IsTrue(packet.getCMD() == CMDType::RESPONSE);
        }

        TEST_METHOD(setPktCountTest)
        {
            PktDef packet;
            packet.setPktCount(12);
            Assert::AreEqual((unsigned short int)12, packet.getPktCount());
        }

        TEST_METHOD(crcTest)
        {
            PktDef packet;
            unsigned char testData[] = "lebron";
            packet.setBodyData(testData, 6);
            packet.calcCRC();

            unsigned char* rawPacket = packet.genPacket();
            Assert::IsTrue(packet.checkCRC(rawPacket, HEADERSIZE + 1 + 6 + 1));
        }

        TEST_METHOD(invalidCRCTest)
        {
            PktDef packet;
            unsigned char testData[] = "lebron";
            packet.setBodyData(testData, 6);
            packet.calcCRC();

            unsigned char* rawPacket = packet.genPacket();
            rawPacket[HEADERSIZE + 1 + 6] = 0xFF; // corrupt the CRC
            Assert::IsFalse(packet.checkCRC(rawPacket, HEADERSIZE + 1 + 6 + 1));
        }

        TEST_METHOD(directionConstantsTest)
        {
            Assert::AreEqual((unsigned char)1, FORWARD);
            Assert::AreEqual((unsigned char)2, BACKWARD);
            Assert::AreEqual((unsigned char)3, RIGHT);
            Assert::AreEqual((unsigned char)4, LEFT);
        }

        TEST_METHOD(telemetryTest)
        {
            PktDef packet;
            packet.setCMD(CMDType::RESPONSE);

            telemetry telemetryData = { 123, 45, 5, 1, 100, 50 };
            packet.setBodyData(reinterpret_cast<unsigned char*>(&telemetryData), sizeof(telemetry));

            Assert::AreEqual((unsigned char)sizeof(telemetry), packet.getLength());

            unsigned char* rawPacket = packet.genPacket();
            Assert::IsNotNull(rawPacket);
            Assert::IsTrue(packet.checkCRC(rawPacket, HEADERSIZE + 1 + sizeof(telemetry) + 1));
        }

        TEST_METHOD(MySocketConstructorTest)
        {
            MySocket sock(SocketType::CLIENT, "127.0.0.1", 8080, ConnectionType::UDP, 512);
            Assert::AreEqual(std::string("127.0.0.1"), sock.GetIPAddr());
            Assert::AreEqual(8080, sock.GetPort());
            Assert::AreEqual(SocketType::CLIENT, sock.GetType());
        }

        TEST_METHOD(MySocketTCPConnectionTest)
        {
            MySocket server(SocketType::SERVER, "127.0.0.1", 9000, ConnectionType::TCP, 512);
            MySocket client(SocketType::CLIENT, "127.0.0.1", 9000, ConnectionType::TCP, 512);
            server.ConnectTCP();
            client.ConnectTCP();
            client.DisconnectTCP();
        }

        TEST_METHOD(MySocketSendReceiveUDP)
        {
            MySocket receiver(SocketType::SERVER, "127.0.0.1", 8500, ConnectionType::UDP, 512);
            MySocket sender(SocketType::CLIENT, "127.0.0.1", 8500, ConnectionType::UDP, 512);

            const char* msg = "Hello";
            sender.SendData(msg, 5);

            char buffer[512] = { 0 };
            int bytes = receiver.GetData(buffer);

            Assert::AreEqual(5, bytes);
            Assert::AreEqual(std::string("Hello"), std::string(buffer, 5));
        }

        TEST_METHOD(overloadedConstructorTest)
        {
            PktDef original;
            original.setPktCount(777);
            original.setCMD(CMDType::SLEEP);
            unsigned char testData[] = { 0xAA, 0xBB, 0xCC };
            original.setBodyData(testData, 3);
            unsigned char* raw = original.genPacket();

            PktDef parsed(raw);

            Assert::AreEqual((unsigned short)777, parsed.getPktCount());
            Assert::IsTrue(parsed.getCMD() == CMDType::SLEEP);
            Assert::AreEqual((unsigned char)3, parsed.getLength());
            Assert::AreEqual(testData[0], parsed.getBodyData()[0]);
            Assert::IsTrue(parsed.checkCRC(raw, HEADERSIZE + 1 + 3 + 1));
        }

        TEST_METHOD(setAckFlagTest)
        {
            PktDef packet;
            packet.setCMD(CMDType::DRIVE);

            // Simulate a response with ACK
            unsigned char* raw = packet.genPacket();
            raw[2] |= 0x08; // Set ack flag manually in raw data

            PktDef parsed(raw);
            Assert::IsTrue(parsed.getAck());
        }

        TEST_METHOD(setBodyData_ReallocateTest)
        {
            PktDef packet;
            unsigned char first[] = { 1, 2, 3 };
            unsigned char second[] = { 9, 9 };

            packet.setBodyData(first, 3);
            packet.setBodyData(second, 2);

            Assert::AreEqual((unsigned char)2, packet.getLength());
            Assert::AreEqual((unsigned char)9, packet.getBodyData()[0]);
        }

        TEST_METHOD(emptyPacketCRCTest)
        {
            PktDef packet;
            packet.setPktCount(0);
            packet.setCMD(CMDType::SLEEP);
            unsigned char* raw = packet.genPacket();

            Assert::IsTrue(packet.checkCRC(raw, HEADERSIZE + 1 + 0 + 1));
        }

        TEST_METHOD(MySocketSetIPAfterConnectTCP)
        {
            MySocket server(SocketType::SERVER, "127.0.0.1", 9100, ConnectionType::TCP, 512);
            MySocket client(SocketType::CLIENT, "127.0.0.1", 9100, ConnectionType::TCP, 512);

            server.ConnectTCP();
            client.ConnectTCP();

            client.SetIPAddr("192.168.1.100"); // should be ignored if connection is active

            Assert::AreEqual(std::string("127.0.0.1"), client.GetIPAddr());

            client.DisconnectTCP();
        }

        TEST_METHOD(MySocketBinaryDataTest)
        {
            MySocket receiver(SocketType::SERVER, "127.0.0.1", 8700, ConnectionType::UDP, 512);
            MySocket sender(SocketType::CLIENT, "127.0.0.1", 8700, ConnectionType::UDP, 512);

            char binary[] = { (char)0xFF, (char)0x00, (char)0xAB };
            sender.SendData(binary, 3);

            char recv[512] = {};
            int received = receiver.GetData(recv);

            Assert::AreEqual(3, received);
            Assert::AreEqual(binary[0], recv[0]);
        }

    };
}
