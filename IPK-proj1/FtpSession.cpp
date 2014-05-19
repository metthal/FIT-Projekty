/**
 * Project: IPK - Project 1 (2014) - FTP client
 * Author: Marek Milkovic <xmilko01@stud.fit.vutbr.cz>
 **/
#include <sstream>
#include "FtpSession.h"
#include "IPKException.h"

FtpSession::FtpSession(const char* hostname, uint16_t port)
{
    m_cmdSocket = new Socket(hostname, port);
    m_cmdSocket->SetRecvTimeout(DEFAULT_TIMEOUT);
}

FtpSession::~FtpSession()
{
    delete m_cmdSocket;
}

void FtpSession::Connect(const char* username, const char* password)
{
    // if username set but password not, it is error
    if (username && !password)
        throw IPKException("FtpSession::Connect - username specified but no password");

    m_cmdSocket->Open();
    if (WaitForResponse() != FTP_RES_READY_TO_LOGIN)
        throw IPKException("FtpSession::Connect - unable to login, no challenge for username");

    SendCommand(FTP_CMD_USER, username ? username : "anonymous");
    uint16_t response = WaitForResponse();
    if (response == FTP_RES_SPECIFY_PASS)
    {
        SendCommand(FTP_CMD_PASS, password ? password : "anonymous");
        response = WaitForResponse();
    }

    if (response != FTP_RES_LOGIN_SUCCESSFUL)
        throw IPKException("FtpSession::Connect - unable to login, invalid user");
}

void FtpSession::Disconnect()
{
    SendCommand(FTP_CMD_QUIT);
    if (WaitForResponse() != FTP_RES_GOODBYE)
        throw IPKException("FtpSession::Disconnect - didn't receive goodbye message");

    m_cmdSocket->Close();
}

void FtpSession::ListDir(const char* dirPath, std::string& dirList)
{
    std::string dataIpAddr;
    uint16_t dataPort;

    EnterPassiveMode(dataIpAddr, dataPort);
    SendCommand(FTP_CMD_LIST, dirPath);

    Socket* dataSocket = new Socket(dataIpAddr.c_str(), dataPort);
    dataSocket->Open();

    if (WaitForResponse() != FTP_RES_OPEN_DATA_CONN)
        throw IPKException("FtpSession::ListDir - cannot initiate data connection");

    dataSocket->SetRecvTimeout(DEFAULT_TIMEOUT);

    do
    {
        dataSocket->Recv();

        // build the directory listing chunck by chunck
        std::vector<uint8_t> buffer = dataSocket->GetBuffer();
        dirList.append(std::string(buffer.begin(), buffer.begin() + dataSocket->GetBufferSize()));
        dataSocket->RewindBuffer(dataSocket->GetBufferSize());
    }
    while (dataSocket->IsReadyToRead(1));

    if (WaitForResponse() != FTP_RES_CLOSE_DATA_CONN)
        throw IPKException("FtpSession::ListDir - didn't receive end of data message");

    dataSocket->Close();
    delete dataSocket;
}

void FtpSession::ListCurrentDir(std::string& dirList)
{
    ListDir(".", dirList);
}

void FtpSession::SendCommand(FtpCommand command, const char* arg)
{
    std::stringstream dataBuffer;

    switch (command)
    {
        case FTP_CMD_USER:
            if (!arg)
                throw IPKException("FtpSession::SendCommand - FTP_CMD_USER - no arg specified");

            dataBuffer << "USER " << arg;
            break;
        case FTP_CMD_PASS:
            if (!arg)
                throw IPKException("FtpSession::SendCommand - FTP_CMD_PASS - no arg specified");

            dataBuffer << "PASS " << arg;
            break;
        case FTP_CMD_PASV:
            dataBuffer << "PASV";
            break;
        case FTP_CMD_LIST:
            dataBuffer << "LIST";
            if (arg)
                dataBuffer << " " << arg;
            break;
        case FTP_CMD_QUIT:
            dataBuffer << "QUIT";
            break;
        default:
            throw IPKException("FtpSession::SendCommand - unknown command");
    }

    dataBuffer << "\r\n";
    m_cmdSocket->Send(dataBuffer.str().c_str(), dataBuffer.str().length());
}

void FtpSession::EnterPassiveMode(std::string& ipAddr, uint16_t& port)
{
    std::vector<uint8_t> responseBuffer;

    SendCommand(FTP_CMD_PASV);
    if (WaitForResponse(&responseBuffer) != FTP_RES_PASSIVE_MODE)
        throw IPKException("FtpSession::EnterPassiveMode - unable to enter passive mode");

    ParseIPAddressAndPort(responseBuffer, ipAddr, port);
}

uint16_t FtpSession::WaitForResponse(std::vector<uint8_t>* responseBuffer)
{
    // there is possible message to parse, try no to receive new packets if we have something in buffer
    bool needRecv = m_cmdSocket->GetBufferSize() < 6 ? true : false;
    uint16_t responseCode = 0;

    while (!responseCode)
    {
        if (needRecv)
        {
            // Socket::Recv throws exceptions, but the WaitForResponse() conditions throw them too, so this can cause
            // unwanted termination, catch the exception from Socket::Recv here and then just return 0 so the superior
            // exceptions can be properly raised
            try
            {
                m_cmdSocket->Recv();
            }
            catch (const IPKException& ex)
            {
                return 0;
            }
        }

        std::vector<uint8_t> buffer = m_cmdSocket->GetBuffer();
        uint32_t bufferSize = m_cmdSocket->GetBufferSize();

        // we don't have enough data to parse out message
        if (bufferSize < 6)
        {
            needRecv = true;
            continue;
        }

        // not response message, find \r\n or end of buffer and throw this message out
        if (!isdigit(buffer[0]) || !isdigit(buffer[1]) || !isdigit(buffer[2]) || buffer[3] != ' ')
        {
            uint32_t rewindCount = bufferSize; // defaultly throw out the whole buffer
            for (uint32_t i = 4; i < bufferSize - 1; ++i)
            {
                // try to find \r\n and throw out only <beginning of buffer;position of \r\n>
                if (buffer[i] == '\r' && buffer[i + 1] == '\n')
                {
                    rewindCount = i + 2;
                    needRecv = ((i + 2) == bufferSize) ? true : false;
                    break;
                }
            }

            m_cmdSocket->RewindBuffer(rewindCount);
        }
        // response message, parse out response code and rewind buffer leaving after the response message there
        else
        {
            for (uint32_t i = 4; i < bufferSize - 1; ++i)
            {
                if (buffer[i] == '\r' && buffer[i + 1] == '\n')
                {
                    if (responseBuffer)
                    {
                        responseBuffer->resize(i + 2);
                        std::copy(buffer.begin(), buffer.begin() + i + 2, responseBuffer->begin());
                    }

                    m_cmdSocket->RewindBuffer(i + 2);
                    responseCode = ((buffer[0] - '0') * 100 + (buffer[1] - '0') * 10 + (buffer[2] - '0'));
                    break;
                }
            }
        }
    }

    return responseCode;
}

void FtpSession::ParseIPAddressAndPort(const std::vector<uint8_t>& buffer, std::string& ipAddr, uint16_t& port)
{
    std::stringstream ipAddrStr;
    uint32_t dotCount = 0;
    uint32_t index = 0;

    // find the opening '(' after which we can find the IP address
    while ((index < buffer.size()) && (buffer[index++] != '('));

    // it wasn't found
    if (index == buffer.size())
        throw IPKException("FtpSession::ParseIPAddressAndPort - malformed passive mode packet (no server info)");

    while (dotCount < 4)
    {
        // IP address is divided with commas
        if (buffer[index] == ',')
        {
            dotCount++;
            if (dotCount != 4)
                ipAddrStr << ".";
        }
        else if (isdigit(buffer[index]))
        {
            ipAddrStr << buffer[index];
        }
        else
            throw IPKException("FtpSession::ParseIPAddressAndPort - malformed passive mode packet (invalid IP address)");

        index++;
    }

    ipAddr = ipAddrStr.str();

    uint16_t portNibble = 0;
    // ')' ends the port info
    while (buffer[index] != ')')
    {
        // nibbles divided with commas
        if (buffer[index] == ',')
        {
            port = portNibble << 8;
            portNibble = 0;
        }
        else if (isdigit(buffer[index]))
        {
            portNibble = portNibble * 10 + (buffer[index] - '0');
        }
        else
            throw IPKException("FtpSession::ParseIPAddressAndPort - malformed passive mode packet (invalid port)");

        index++;
    }

    port |= portNibble;
}
