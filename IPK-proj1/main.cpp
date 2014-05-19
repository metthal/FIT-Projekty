/**
 * Project: IPK - Project 1 (2014) - FTP client
 * Author: Marek Milkovic <xmilko01@stud.fit.vutbr.cz>
 **/
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include "FtpSession.h"
#include "IPKException.h"
#include "Regex.h"

#define MATCH_USERPASS      2
#define MATCH_USERNAME      3
#define MATCH_PASSWORD      4
#define MATCH_HOST          5
#define MATCH_PORT          7
#define MATCH_PATH          8

void printHelpClause(const char* left, const char* right)
{
    std::cout << std::setw(10) << std::setfill(' ') << left;
    std::cout << std::setw(15 + strlen(right)) << std::setfill(' ') << right << std::endl;
}

void printHelp()
{
    std::cout << "IPK - Project 1 (2014) - FTP client" << std::endl;
    std::cout << "Author: Marek Milkovic <xmilko01@stud.fit.vutbr.cz>" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;

    const char* usage = "ftpclient --help | URL";
    std::cout << std::setw(10 + strlen(usage)) << std::setfill(' ') << usage << std::endl;
    std::cout << std::endl;
    printHelpClause("--help", "Prints help");
    printHelpClause("URL", "URL of the FTP server in format [ftp://[username:password@]]hostname[:port][/path][/]");
}

int main(int argc, char** argv)
{
    try
    {
        // only one parameter allowed
        if (argc != 2)
            throw IPKException("Invalid parameters");

        if (strcmp(argv[1], "--help") == 0)
        {
            printHelp();
            return 0;
        }

        // magic regular expressions for parsing the URL
        // DON'T TOUCH IT!!!
        std::string pattern = R"(^(ftp://(([^[:space:]@:]*):([^[:space:]@:]*)@)?)?([a-zA-Z0-9.-]+)(:([0-9]+))?((/[^/[:space:]]+)*(/?))$)";
        Regex regex(pattern.c_str());
        MatchList matches;
        if (!regex.Match(argv[1], 10, matches))
            throw IPKException("Invalid URL format specified");

        // default are anonymous credentials
        const char* username = NULL, *password = NULL;
        if (matches[MATCH_USERPASS] == ":@") // is this error? (empty username and password specified)
        {
            throw IPKException("Invalid URL format specified");
        }
        else if (matches[MATCH_USERPASS] != "") // any username or password specified
        {
            username = matches[MATCH_USERNAME].c_str();
            password = matches[MATCH_PASSWORD].c_str();
        }

        uint16_t portNum = DEFAULT_FTP_PORT;
        if (matches[MATCH_PORT] != "")
        {
            // parse out the number from port string
            std::istringstream iss(matches[MATCH_PORT]);
            iss >> portNum;
        }

        // initiate connection
        FtpSession session(matches[MATCH_HOST].c_str(), portNum);
        session.Connect(username, password);

        // print the directory list
        std::string dir;
        session.ListDir(matches[MATCH_PATH].c_str(), dir);
        std::cout << dir;

        // disconnect
        session.Disconnect();
    }
    catch (const IPKException& ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
