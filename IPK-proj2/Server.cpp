#include <iostream>
#include <thread>
#include <fstream>
#include "Server.h"
#include "IPKException.h"

Server::Server(const std::string& hostname, uint16_t port, uint64_t speedLimit) : Service(hostname, port), m_running(false), m_sessionCount(0), m_speedLimit(speedLimit)
{
}

Server::~Server() { }

void Server::Run()
{
    m_socket->Open();
    m_socket->Bind();
    m_socket->Listen();
    m_socket->SetReusableAddress(true);

    m_running = true;
    while (m_running)
    {
        SocketPtr sessionSocket = m_socket->Accept(3);

        if (!m_running)
            break;

        if (!sessionSocket)
            continue;

        m_sessionCount++;
        std::thread sessionThread(&Server::ProcessSession, this, sessionSocket);
        sessionThread.detach();
    }
}

void Server::ProcessSession(SocketPtr socket)
{
    try
    {
        socket->SetRecvTimeout(3, 0);
        Packet* packet = nullptr;

        packet = ReceiveMessage(socket);
        if (!HandleHandshakeRequest(socket, packet))
        {
            socket->Close();
            return;
        }

        packet = ReceiveMessage(socket);
        if (!HandleDownloadRequest(socket, packet))
        {
            socket->Close();
            return;
        }

        packet = ReceiveMessage(socket);
        if (!HandleFarewell(socket, packet))
        {
            socket->Close();
            return;
        }

        socket->Close();
    }
    catch (const IPKException& ex)
    {
        socket->Close();
    }
}

bool Server::HandleHandshakeRequest(SocketPtr socket, Packet* packet)
{
    if (!packet)
        return false;

    if (packet->GetOpcode() != CMSG_HANDSHAKE_REQUEST)
        return false;

    uint16_t magic;
    *packet >> magic;
    // TODO: check magic?

    SendMessage(socket, SMSG_HANDSHAKE_RESPONSE, sizeof(uint16_t), (uint16_t)42);
    return true;
}

bool Server::HandleDownloadRequest(SocketPtr socket, Packet* packet)
{
    if (!packet)
        return false;

    if (packet->GetOpcode() != CMSG_DOWNLOAD_REQUEST)
        return false;

    std::string filePath;
    *packet >> filePath;
    // TODO: check?

    std::ifstream file(filePath, std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
    bool result = file.good();

    uint64_t fileSize = 0;
    if (result)
    {
        fileSize = file.tellg();
        file.seekg(0, std::ios_base::beg);
    }

    SendMessage(socket, SMSG_DOWNLOAD_RESPONSE, sizeof(uint8_t) + sizeof(uint64_t), (uint8_t)result, fileSize);

    if (!result)
        return true;

    uint64_t bytesSent = 0;
    uint64_t chunkSize = ((m_speedLimit * IN_KILOBYTES) * ((double)DATA_SEND_DELAY / IN_MILLISECONDS) + 0.5);
    MsDelay delay(DATA_SEND_DELAY);
    char* buffer = new char[chunkSize];
    while (bytesSent < fileSize)
    {
        uint32_t bytes = std::min(fileSize - bytesSent, chunkSize);
        file.read(buffer, bytes);

        Packet packet(SMSG_DOWNLOAD_DATA, bytes);
        packet.AppendBuffer((const uint8_t*)buffer, bytes);
        SendMessage(socket, &packet);

        bytesSent += bytes;

        std::this_thread::sleep_for(delay);
    }

    return true;
}

bool Server::HandleFarewell(SocketPtr socket, Packet* packet)
{
    if (!packet)
        return false;

    if (packet->GetOpcode() != XMSG_FAREWELL)
        return false;

    SendMessage(socket, XMSG_FAREWELL, 0);
    return true;
}
