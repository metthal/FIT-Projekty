#ifndef SOCKET_H
#define SOCKET_H

#include <string>
#include <memory>
#include <queue>
#include <cstdint>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include "IPKException.h"
#include "Packet.h"

#define INVALID_SOCKET          -1
#define DEFAULT_BUFFER_SIZE     4096

class Socket;
typedef std::shared_ptr<Socket> SocketPtr;
typedef std::weak_ptr<Socket> SocketPtrw;

class Socket
{
public:
    Socket() = delete;
    Socket(const Socket&) = delete;
    Socket(const std::string& hostname, uint16_t port) : m_socketFd(INVALID_SOCKET), m_socketAddr(nullptr), m_hostname(hostname), m_port(port)
    {
        memset(m_buffer, 0, DEFAULT_BUFFER_SIZE);
        m_bufferBytesRead = 0;
        m_pendingPacket = nullptr;
    }

    Socket(int socketFd, sockaddr_in* socketAddr) : m_socketFd(socketFd), m_socketAddr(new sockaddr_in), m_port(ntohs(socketAddr->sin_port))
    {
        char ipAddr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(socketAddr->sin_addr), ipAddr, INET_ADDRSTRLEN);
        m_hostname = ipAddr;

        memcpy(m_socketAddr, socketAddr, sizeof(sockaddr_in));
        memset(m_buffer, 0, DEFAULT_BUFFER_SIZE);
        m_bufferBytesRead = 0;
        m_pendingPacket = nullptr;
    }

    ~Socket()
    {
        if (m_socketAddr)
            delete m_socketAddr;
    }

    void Open()
    {
        m_socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_socketFd == INVALID_SOCKET)
            throw IPKException("Socket::Open - unable to initialize socket");

        hostent* hostInfo = gethostbyname(m_hostname.c_str());
        if (!hostInfo)
            throw IPKException("Socket::Open - cannot resolve hostname");

        m_socketAddr = new sockaddr_in;
        m_socketAddr->sin_family = AF_INET;
        m_socketAddr->sin_port = htons(m_port);
        memcpy(&(m_socketAddr->sin_addr), hostInfo->h_addr_list[0], hostInfo->h_length);
    }

    void Close()
    {
        shutdown(m_socketFd, SHUT_RDWR);
        close(m_socketFd);
    }

    void Connect()
    {
        if (connect(m_socketFd, (const sockaddr*)m_socketAddr, sizeof(sockaddr_in)) == INVALID_SOCKET)
            throw IPKException("Socket::Connect - unable to connect to the remote endpoint");
    }

    void Bind()
    {
        if (bind(m_socketFd, (const sockaddr*)m_socketAddr, sizeof(sockaddr_in)) == INVALID_SOCKET)
            throw IPKException("Socket::Bind - unable to bind to the selected address and port");
    }

    void Listen()
    {
        if (listen(m_socketFd, SOMAXCONN) == INVALID_SOCKET)
            throw IPKException("Socket::Listen - unable to start listening");
    }

    SocketPtr Accept(uint32_t timeoutSec)
    {
        fd_set acceptSet;
        FD_ZERO(&acceptSet);
        FD_SET(m_socketFd, &acceptSet);

        timeval timeout;
        timeout.tv_sec = timeoutSec;
        timeout.tv_usec = 0;

        if (select(m_socketFd + 1, &acceptSet, NULL, NULL, &timeout) == 0)
            return nullptr;

        if (!FD_ISSET(m_socketFd, &acceptSet))
            return nullptr;

        sockaddr_in sessionAddress;
        socklen_t sessionAddressLen = 0;
        int sessionSocket = accept(m_socketFd, (sockaddr*)&sessionAddress, &sessionAddressLen);
        if (sessionSocket == INVALID_SOCKET)
            throw IPKException("Socket::Accept - error occured during accept");

        return SocketPtr(new Socket(sessionSocket, &sessionAddress));
    }

    int GetSocketId() const
    {
        return m_socketFd;
    }

    void Send(const Packet& packet)
    {
        int64_t res;
        uint64_t bytesSent = 0;
        uint64_t bytesToSend = packet.GetLength();
        const uint8_t* buffer = packet.GetBuffer();

        while (bytesSent < bytesToSend)
        {
            res = send(m_socketFd, buffer + bytesSent, bytesToSend - bytesSent, 0);
            if (res == -1)
                throw IPKException("Socket::Send - error occured during transimission");

            bytesSent += res;
        }
    }

    bool Recv(uint32_t maxTimeoutCount = 1)
    {
        uint32_t timeoutCount = 0;
        uint32_t timeoutSecs, timeoutUsecs;
        GetRecvTimeout(timeoutSecs, timeoutUsecs);

        while (timeoutCount < maxTimeoutCount)
        {
            int64_t bytesRecvd = recv(m_socketFd, m_buffer + m_bufferBytesRead, DEFAULT_BUFFER_SIZE - m_bufferBytesRead, 0);

            if (bytesRecvd == 0) // remote endpoint disconnected
                return false;
            else if (bytesRecvd == -1)
            {
                // recv timed out
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    //if (m_pendingPacket)
                    //    throw IPKException("Socket::Recv - error occured during transmission with pending packet");
                    //else
                    {
                        timeoutCount++;
                        continue;
                    }
                }

                throw IPKException("Socket::Recv - error occured during transmission");
            }
            else if (bytesRecvd + m_bufferBytesRead < (int64_t)(PACKET_HEADER_SIZE))
            {
                m_bufferBytesRead += bytesRecvd;
            }
            else
            {
                bytesRecvd += m_bufferBytesRead;
                while (bytesRecvd >= (int64_t)(PACKET_HEADER_SIZE))
                {
                    uint32_t movePos = 0;
                    if (!m_pendingPacket)
                    {
                        m_pendingPacket = new Packet(m_buffer, bytesRecvd);
                        movePos = m_pendingPacket->GetCurrentLength();
                    }
                    else
                    {
                        movePos = m_pendingPacket->GetCurrentLength();
                        m_pendingPacket->AppendBuffer(m_buffer, bytesRecvd);
                        movePos = m_pendingPacket->GetCurrentLength() - movePos;
                    }

                    memmove(m_buffer, m_buffer + movePos, bytesRecvd - movePos);
                    bytesRecvd -= movePos;

                    if (m_pendingPacket->IsValid())
                    {
                        m_recvPacketQueue.push(m_pendingPacket);
                        m_pendingPacket = nullptr;
                        timeoutCount = 0;
                        maxTimeoutCount = 20;
                        SetRecvTimeout(0, 500);
                    }
                    else
                        break;
                }
                m_bufferBytesRead = bytesRecvd;
            }
        }

        SetRecvTimeout(timeoutSecs, timeoutUsecs);
        return true;
    }

    void SetRecvTimeout(uint32_t timeoutSecs, uint32_t timeoutUsecs)
    {
        if (m_socketFd == INVALID_SOCKET)
            return;

        timeval timeout;
        timeout.tv_sec = timeoutSecs;
        timeout.tv_usec = timeoutUsecs;

        if (setsockopt(m_socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0)
            throw IPKException("Socket::SetRecvTimeout - failed to set recv timeout");
    }

    void SetReusableAddress(bool reusable)
    {
        int reusableInt = reusable;
        if (setsockopt(m_socketFd, SOL_SOCKET, SO_REUSEADDR, &reusableInt, sizeof(reusableInt)) != 0)
            throw IPKException("Socket::SetReusableAddress - failed to set reusable address");
    }

    void GetRecvTimeout(uint32_t& timeoutSecs, uint32_t& timeoutUsecs) const
    {
        if (m_socketFd == INVALID_SOCKET)
            return;

        timeval timeout;
        socklen_t timeoutSize = sizeof(timeval);

        if (getsockopt(m_socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, &timeoutSize) != 0)
            throw IPKException("Socket::GetRecvTimeout - failed to get recv timeout");

        timeoutSecs = timeout.tv_sec;
        timeoutUsecs = timeout.tv_usec;
    }

    Packet* GetReceivedPacket()
    {
        if (m_recvPacketQueue.empty())
            return nullptr;

        Packet* packet = m_recvPacketQueue.front();
        m_recvPacketQueue.pop();
        return packet;
    }

    std::string GetHostname() const
    {
        return m_hostname;
    }

    uint16_t GetPort() const
    {
        return m_port;
    }

private:
    Socket& operator =(const Socket&);

    int m_socketFd;
    sockaddr_in* m_socketAddr;
    std::string m_hostname;
    uint16_t m_port;
    uint8_t m_buffer[DEFAULT_BUFFER_SIZE];
    uint32_t m_bufferBytesRead;
    Packet* m_pendingPacket;
    std::queue<Packet*> m_recvPacketQueue;
};

#endif // SOCKET_H
