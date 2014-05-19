/**
 * Project: IPK - Project 1 (2014) - FTP client
 * Author: Marek Milkovic <xmilko01@stud.fit.vutbr.cz>
 **/
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include "Socket.h"
#include "IPKException.h"

Socket::Socket(const char* hostname, uint16_t port) : m_hostname(hostname), m_port(port)
{
    m_socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    m_buffer.resize(DEFAULT_BUFFER_SIZE);
    m_bufferSize = 0;
}

Socket::~Socket() { }

void Socket::Open()
{
    if (m_socketHandle == INVALID_SOCKET)
        throw IPKException("Socket::Open - unable to create socket handle");

    // resolve hostname -> IP address
    hostent* hostInfo = gethostbyname(m_hostname.c_str());
    if (!hostInfo)
        throw IPKException("Socket::Open - cannot resolve hostname to IP");

    sockaddr_in remotePoint;
    memset(&remotePoint, 0, sizeof(sockaddr_in));

    remotePoint.sin_family = AF_INET;
    remotePoint.sin_port = htons(m_port);
    memcpy(&(remotePoint.sin_addr), hostInfo->h_addr, hostInfo->h_length);

    if (connect(m_socketHandle, (const sockaddr*)&remotePoint, sizeof(sockaddr_in)) == INVALID_SOCKET)
        throw IPKException("Socket::Open - unable to connect to the endpoint");
}

void Socket::Close()
{
    if (m_socketHandle == INVALID_SOCKET)
        return;

    if (shutdown(m_socketHandle, SHUT_RDWR) != 0)
        throw IPKException("Socket::Close - shutdown of socket failed");

    if (close(m_socketHandle) != 0)
        throw IPKException("Socket::Close - close on socket failed");
}

void Socket::SetRecvTimeout(uint32_t secs)
{
    if (m_socketHandle == INVALID_SOCKET)
        return;

    timeval timeout;
    timeout.tv_sec = secs;
    timeout.tv_usec = 0;

    if (setsockopt(m_socketHandle, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0)
        throw IPKException("Socket::SetRecvTimeout - failed to set recv timeout");
}

void Socket::Send(const char* buffer, uint32_t bufferSize)
{
    uint32_t bytesSent = 0;

    // send() can send the message in chuncks
    while (bytesSent < bufferSize)
        bytesSent += send(m_socketHandle, buffer, bufferSize, 0);
}

void Socket::Recv()
{
    int32_t bytesRead = 0;

    while ((bytesRead = recv(m_socketHandle, &m_buffer[m_bufferSize], DEFAULT_BUFFER_SIZE - m_bufferSize, 0)) > 0)
    {
        m_bufferSize += bytesRead;

        // if we don't have any data, cancel receiving and let the caller to decide what to do
        if (!IsReadyToRead(0))
            return;
    }

    // recv() timed out
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        throw IPKException("Socket::Recv - connection timed out");

    // error on the socket
    if (bytesRead == -1)
        throw IPKException("Socket::Recv - error while receiving occured");
}

bool Socket::IsReadyToRead(uint32_t timeoutSecs)
{
    timeval timeout;
    timeout.tv_sec = timeoutSecs;
    timeout.tv_usec = 0;
    fd_set readFd;

    FD_ZERO(&readFd);
    FD_SET(m_socketHandle, &readFd);

    // wait 'timeoutSecs' if anything appears on the socket to read
    if (select(m_socketHandle + 1, &readFd, NULL, NULL, &timeout) == 1)
    {
        int32_t bytes;
        ioctl(m_socketHandle, FIONREAD, &bytes);
        return bytes > 0;
    }

    return false;
}

void Socket::RewindBuffer(uint32_t count)
{
    // cut 'count' elements from the beginning by moving the 'count'th element at the beginning
    memmove(&m_buffer[0], &m_buffer[count], DEFAULT_BUFFER_SIZE - count);
    m_bufferSize = std::max((int32_t)(m_bufferSize - count), 0);
}

std::vector<uint8_t> Socket::GetBuffer()
{
    return m_buffer;
}

uint32_t Socket::GetBufferSize() const
{
    return m_bufferSize;
}
