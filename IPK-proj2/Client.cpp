#include <iostream>
#include <fstream>
#include "Client.h"

Client::Client(const std::string& hostname, uint16_t port, const std::string& downloadFile) : Service(hostname, port), m_downloadFile(downloadFile) {}

Client::~Client() { }

void Client::Run()
{
    m_socket->Open();
    m_socket->Connect();
    m_socket->SetReusableAddress(true);

    Packet* packet = nullptr;

    SendMessage(m_socket, CMSG_HANDSHAKE_REQUEST, sizeof(uint16_t), (uint16_t)1337);

    packet = ReceiveMessage(m_socket);
    if (!HandleHandshakeResponse(m_socket, packet))
    {
        m_socket->Close();
        return;
    }

    packet = ReceiveMessage(m_socket);
    if (!HandleDownloadResponse(m_socket, packet))
    {
        m_socket->Close();
        return;
    }

    packet = ReceiveMessage(m_socket);
    if (!HandleFarewell(m_socket, packet))
    {
        m_socket->Close();
        return;
    }

    m_socket->Close();
}

bool Client::HandleHandshakeResponse(SocketPtr socket, Packet* packet)
{
    if (!packet)
        return false;

    if (packet->GetOpcode() != SMSG_HANDSHAKE_RESPONSE)
        return false;

    // TODO length of m_downloadFile can be > 255
    SendMessage(socket, CMSG_DOWNLOAD_REQUEST, m_downloadFile.length() + 1, m_downloadFile);
    return true;
}
/*
#include <list>
#include <ctime>
#include <utility>
*/
bool Client::HandleDownloadResponse(SocketPtr socket, Packet* packet)
{
    if (!packet)
        return false;

    if (packet->GetOpcode() != SMSG_DOWNLOAD_RESPONSE)
        return false;

    uint8_t result;
    *packet >> result;

    if (!result)
        return true;

    uint64_t fileSize;
    *packet >> fileSize;

    uint64_t bytesRecvd = 0;
    std::ofstream file(m_downloadFile, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
/*
    std::list<std::pair<uint64_t, uint64_t>> downloadHistory;
    downloadHistory.clear();*/

    while (bytesRecvd < fileSize)
    {
        Packet* dataPacket = ReceiveMessage(socket);
        if (!dataPacket)
            break;

        file.write((const char*)dataPacket->GetDataBuffer(), dataPacket->GetDataLength());
        bytesRecvd += dataPacket->GetDataLength();

        /*uint64_t actualTime = time(NULL);
        downloadHistory.push_back(std::pair<uint64_t, uint64_t>(actualTime, dataPacket->GetDataLength()));
        std::cout << "Donwloaded chuck of size " << dataPacket->GetDataLength() << std::endl;
        while (downloadHistory.front().first + 1 < actualTime)
            downloadHistory.pop_front();
        uint64_t speed = 0;
        for (auto itr = downloadHistory.begin(); itr != downloadHistory.end(); ++itr)
            speed += itr->second;
        std::cout << "Current speed is " << speed << " B/S" << std::endl;*/

        file.flush();
    }

    if (bytesRecvd != fileSize)
        return false;

    SendMessage(socket, XMSG_FAREWELL, 0);
    return true;
}

bool Client::HandleFarewell(SocketPtr socket, Packet* packet)
{
    (void)socket;

    if (!packet)
        return false;

    if (packet->GetOpcode() != XMSG_FAREWELL)
        return false;

    return true;
}
