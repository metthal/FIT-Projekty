/**
 * Project: IPK - Project 1 (2014) - FTP client
 * Author: Marek Milkovic <xmilko01@stud.fit.vutbr.cz>
 **/
#ifndef FTP_SESSION_H
#define FTP_SESSION_H

#include <cstdint>
#include <string>
#include <vector>
#include "Socket.h"

#define DEFAULT_FTP_PORT        21
#define DEFAULT_TIMEOUT         60

enum FtpCommand
{
    FTP_CMD_USER                = 0,
    FTP_CMD_PASS,
    FTP_CMD_TYPE,
    FTP_CMD_PASV,
    FTP_CMD_LIST,
    FTP_CMD_QUIT
};

enum FtpResult
{
    FTP_RES_OPEN_DATA_CONN      = 150,
    FTP_RES_ASCII_MODE          = 200,
    FTP_RES_READY_TO_LOGIN      = 220,
    FTP_RES_CLOSE_DATA_CONN     = 226,
    FTP_RES_PASSIVE_MODE        = 227,
    FTP_RES_LOGIN_SUCCESSFUL    = 230,
    FTP_RES_SPECIFY_PASS        = 331,
    FTP_RES_GOODBYE             = 221
};

class FtpSession
{
public:
    FtpSession(const char* hostname, uint16_t port = DEFAULT_FTP_PORT);
    ~FtpSession();

    void Connect(const char* username = NULL, const char* password = NULL);
    void Disconnect();

    void ListDir(const char* dirPath, std::string& dirList);
    void ListCurrentDir(std::string& dirList);

private:
    void     SendCommand(FtpCommand command, const char* arg = NULL);
    uint16_t WaitForResponse(std::vector<uint8_t>* responseBuffer = NULL);
    void     EnterPassiveMode(std::string& ipAddr, uint16_t& port);

    void ParseIPAddressAndPort(const std::vector<uint8_t>& buffer, std::string& ipAddr, unsigned short& port);

    Socket* m_cmdSocket;

};

#endif // FTP_SESSION_H
