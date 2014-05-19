/**
 * Project: IPK - Project 1 (2014) - FTP client
 * Author: Marek Milkovic <xmilko01@stud.fit.vutbr.cz>
 **/
#ifndef SOCKET_H
#define SOCKET_H

#include <vector>
#include <string>

#define INVALID_SOCKET      -1
#define DEFAULT_BUFFER_SIZE 4096

class Socket
{
public:
    Socket(const char* hostname, uint16_t port);
    ~Socket();

    void Open();
    void Close();

    std::vector<uint8_t> GetBuffer();
    uint32_t GetBufferSize() const;
    void RewindBuffer(uint32_t count);

    void Send(const char* buffer, uint32_t bufferSize);
    void Recv();

    void SetRecvTimeout(uint32_t secs);
    bool IsReadyToRead(uint32_t timeoutSecs);

private:

    int m_socketHandle;
    std::string m_hostname;
    uint16_t m_port;
    std::vector<uint8_t> m_buffer;
    uint32_t m_bufferSize;
};

#endif // SOCKET_H
