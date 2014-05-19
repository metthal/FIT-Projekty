#ifndef PACKET_H
#define PACKET_H

#include <vector>
#include <cstring>

#define PACKET_HEADER_SIZE      (sizeof(uint8_t) + sizeof(uint32_t))

enum PacketOpcode
{
    CMSG_HANDSHAKE_REQUEST      = 0,
    SMSG_HANDSHAKE_RESPONSE     = 1,
    CMSG_DOWNLOAD_REQUEST       = 2,
    SMSG_DOWNLOAD_RESPONSE      = 3,
    SMSG_DOWNLOAD_DATA          = 4,
    XMSG_FAREWELL               = 5,
};

class Packet
{
public:
    Packet() = delete;
    Packet(const Packet&) = delete;
    Packet(uint8_t opcode, uint32_t length) : m_readPos(PACKET_HEADER_SIZE), m_writePos(0), m_maxPacketLen(length + PACKET_HEADER_SIZE), m_buffer(m_maxPacketLen, 0)
    {
        Write<uint8_t>(opcode);
        Write<uint32_t>(length);
    }

    Packet(const uint8_t* buffer, uint32_t bufferSize) : m_readPos(PACKET_HEADER_SIZE), m_writePos(0)
    {
        if (bufferSize < PACKET_HEADER_SIZE)
            throw IPKException("Packet::Packet - size of buffer cannot be less than PACKET_HEADER_SIZE");

        m_maxPacketLen = *((uint32_t*)&buffer[1]) + PACKET_HEADER_SIZE;
        m_buffer.resize(m_maxPacketLen, 0);
        AppendBuffer(buffer, bufferSize);
    }

    ~Packet()
    {
        m_buffer.clear();
    }

    Packet& operator <<(const uint8_t& data)        {   Write<uint8_t>(data);    return *this;   }
    Packet& operator <<(const uint16_t& data)       {   Write<uint16_t>(data);   return *this;   }
    Packet& operator <<(const uint32_t& data)       {   Write<uint32_t>(data);   return *this;   }
    Packet& operator <<(const uint64_t& data)       {   Write<uint64_t>(data);   return *this;   }
    Packet& operator <<(const int8_t& data)         {   Write<int8_t>(data);     return *this;   }
    Packet& operator <<(const int16_t& data)        {   Write<int16_t>(data);    return *this;   }
    Packet& operator <<(const int32_t& data)        {   Write<int32_t>(data);    return *this;   }
    Packet& operator <<(const int64_t& data)        {   Write<int64_t>(data);    return *this;   }
    Packet& operator <<(const std::string& data)    {   WriteString(data);       return *this;   }

    Packet& operator >>(uint8_t& data)              {   Read<uint8_t>(data);     return *this;   }
    Packet& operator >>(uint16_t& data)             {   Read<uint16_t>(data);    return *this;   }
    Packet& operator >>(uint32_t& data)             {   Read<uint32_t>(data);    return *this;   }
    Packet& operator >>(uint64_t& data)             {   Read<uint64_t>(data);    return *this;   }
    Packet& operator >>(int8_t& data)               {   Read<int8_t>(data);      return *this;   }
    Packet& operator >>(int16_t& data)              {   Read<int16_t>(data);     return *this;   }
    Packet& operator >>(int32_t& data)              {   Read<int32_t>(data);     return *this;   }
    Packet& operator >>(int64_t& data)              {   Read<int64_t>(data);     return *this;   }
    Packet& operator >>(std::string& data)          {   ReadString(data);        return *this;   }

    void AppendBuffer(const uint8_t* buffer, uint32_t bufferSize)
    {
        uint32_t bytesToCopy = std::min(m_maxPacketLen - m_writePos, bufferSize);
        memcpy(&m_buffer[m_writePos], buffer, bytesToCopy);
        m_writePos += bytesToCopy;
    }

    uint32_t GetCurrentLength() const
    {
        return m_writePos;
    }

    uint32_t GetLength() const
    {
        return m_maxPacketLen;
    }

    uint32_t GetDataLength() const
    {
        return m_maxPacketLen - PACKET_HEADER_SIZE;
    }

    const uint8_t* GetBuffer() const
    {
        return (const uint8_t*)&m_buffer[0];
    }

    const uint8_t* GetDataBuffer() const
    {
        return (const uint8_t*)&m_buffer[PACKET_HEADER_SIZE];
    }

    bool IsValid() const
    {
        return (m_writePos == m_maxPacketLen);
    }

    uint8_t GetOpcode() const
    {
        return m_buffer[0];
    }

protected:
    template <typename T> void Write(const T& data)
    {
        size_t dataSize = sizeof(T);
        if (m_writePos + dataSize > m_maxPacketLen)
            return;

        memcpy(&m_buffer[m_writePos], &data, dataSize);
        m_writePos += dataSize;
    }

    template <typename T> void Read(T& data)
    {
        size_t dataSize = sizeof(T);
        if (m_readPos + dataSize > m_maxPacketLen)
        {
            data = T();
            return;
        }

        data = *((const T*)&m_buffer[m_readPos]);
        m_readPos += dataSize;
    }

    void WriteString(const std::string& data)
    {
        uint8_t strLen = data.length() > 255 ? 255 : data.length();
        if (m_writePos + strLen + 1 > m_maxPacketLen)
            return;

        Write<uint8_t>(strLen);
        memcpy(&m_buffer[m_writePos], data.c_str(), strLen);
        m_writePos += strLen;
    }

    void ReadString(std::string& data)
    {
        if (m_readPos + 1 > m_maxPacketLen)
        {
            data = "";
            return;
        }

        uint8_t strLen;
        Read<uint8_t>(strLen);

        if (m_readPos + strLen > m_maxPacketLen)
        {
            data = "";
            return;
        }

        data = std::string((const char*)&m_buffer[m_readPos], strLen);
    }

private:
    Packet& operator =(const Packet&);

    uint32_t m_readPos;
    uint32_t m_writePos;
    uint32_t m_maxPacketLen;
    std::vector<uint8_t> m_buffer;
};

#endif // PACKET_H
