#include <iostream>
#include <sstream>
#include "Client.h"
#include "Regex.h"
#include "IPKException.h"

#include <signal.h>
void dummySigPipe(int)
{
}

int main(int argc, char** argv)
{
    signal(SIGPIPE, &dummySigPipe);

    try
    {
        if (argc != 2)
            throw IPKException("main - invalid count of parameters");

        Regex addressRegex(R"(^([^:/]+):([0-9]+)/([^/]+)$)");

        MatchList matches;
        if (!addressRegex.Match(argv[1], 4, matches))
            throw IPKException("main - invalid parameter");

        std::string host = matches[1];
        std::string downloadFile = matches[3];
        std::stringstream portStream(matches[2]);
        uint16_t port;
        portStream >> port;

        Client client(host, port, downloadFile);
        client.Run();
    }
    catch(const IPKException& ex)
    {
        std::cerr << ex.what() << std::endl;
    }
}
