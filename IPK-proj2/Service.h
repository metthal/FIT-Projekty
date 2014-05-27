#ifndef SERVICE_H
#define SERVICE_H

#include <cstdint>
#include "Socket.h"
#include "Packet.h"

class Service
{
public:
    Service() = delete;
    Service(const Service&) = delete;
    Service(const std::string& hostname, uint16_t port) : m_socket(new Socket(hostname, port)) { }
    ~Service() { }

    virtual void Run() = 0;

protected:
    Service& operator =(const Service&);

    void SendMessage(SocketPtrw socket, Packet* packet)
    {
        if (packet->IsValid())
            socket.lock()->Send(*packet);
        else
            throw IPKException("Service::SendMessage - try to send invalid packet");
    }

    template <typename T, typename... Args> void SendMessage(SocketPtrw socket, Packet* packet, const T& data, const Args&... dataArgs)
    {
        *packet << data;
        SendMessage(socket, packet, dataArgs...);
    }

    template <typename... Args> void SendMessage(SocketPtrw socket, uint8_t opcode, uint8_t dataLength, const Args&... dataArgs)
    {
        Packet* packet = new Packet(opcode, dataLength);
        SendMessage(socket, packet, dataArgs...);
    }

    Packet* ReceiveMessage(SocketPtr socket)
    {
        Packet* packet = socket->GetReceivedPacket();

        if (!packet)
        {
            bool active = socket->Recv(50);
            packet = socket->GetReceivedPacket();
            if (!active && !packet)
                return nullptr;
        }

        return packet;
    }

    SocketPtr m_socket;
};

#endif
