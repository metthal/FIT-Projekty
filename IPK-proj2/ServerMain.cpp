#include <iostream>
#include <sstream>
#include "Server.h"
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
        if (argc != 5)
            throw IPKException("main - invalid count of parameters");

        uint32_t portArg, speedLimitArg;
        if (strcmp(argv[1], "-p") == 0)
        {
            if (strcmp(argv[3], "-d") == 0)
            {
                portArg = 2;
                speedLimitArg = 4;
            }
            else
                throw IPKException("main - invalid parameters");
        }
        else if (strcmp (argv[1], "-d") == 0)
        {
            if (strcmp(argv[3], "-p") == 0)
            {
                portArg = 4;
                speedLimitArg = 2;
            }
            else
                throw IPKException("main - invalid parameters");
        }
        else
            throw IPKException("main - invalid parameters");

        std::stringstream portStream(argv[portArg]);
        std::stringstream speedLimitStream(argv[speedLimitArg]);
        uint16_t port;
        uint64_t speedLimit;
        portStream >> port;
        speedLimitStream >> speedLimit;

        Server server("0.0.0.0", port, speedLimit);
        server.Run();
    }
    catch(const IPKException& ex)
    {
        std::cerr << ex.what() << std::endl;
    }

    return 0;
}
