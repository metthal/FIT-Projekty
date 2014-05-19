#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <cstdint>
#include "Service.h"
#include "Socket.h"

class Client : public Service
{
public:
    Client() = delete;
    Client(const Client&) = delete;
    Client(const std::string& hostname, uint16_t port, const std::string& downloadFile);

    ~Client();

    void Run();

protected:
    bool HandleHandshakeResponse(SocketPtr socket, Packet* packet);
    bool HandleDownloadResponse(SocketPtr socket, Packet* packet);
    bool HandleFarewell(SocketPtr socket, Packet* packet);

private:
    std::string m_downloadFile;
};

#endif // CLIENT_H
