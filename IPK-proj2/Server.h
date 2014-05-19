#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <atomic>
#include <chrono>
#include <memory>
#include <cstdint>
#include "Service.h"
#include "Socket.h"

#define DATA_SEND_DELAY     10
#define IN_KILOBYTES        1000
#define IN_MILLISECONDS     1000

typedef std::chrono::duration<uint64_t, std::milli> MsDelay;

class Server : public Service
{
public:
    Server() = delete;
    Server(const Server&) = delete;
    Server(const std::string& hostname, uint16_t port, uint64_t speedLimit);

    ~Server();

    void Run();
    void Stop();

    void ProcessSession(SocketPtr socket);

protected:
    bool HandleHandshakeRequest(SocketPtr socket, Packet* packet);
    bool HandleDownloadRequest(SocketPtr socket, Packet* packet);
    bool HandleFarewell(SocketPtr socket, Packet* packet);

private:
    Server& operator =(const Server&);

    std::atomic_bool m_running;
    std::atomic_uint m_sessionCount;
    uint64_t m_speedLimit;
};

#endif // SERVER_H
