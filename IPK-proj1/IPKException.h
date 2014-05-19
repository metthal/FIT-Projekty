/**
 * Project: IPK - Project 1 (2014) - FTP client
 * Author: Marek Milkovic <xmilko01@stud.fit.vutbr.cz>
 **/
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>

class IPKException : public std::exception
{
public:
    IPKException(const std::string& msg) : m_msg(msg) { }

    virtual const char* what() const noexcept
    {
        return m_msg.c_str();
    }

private:
    std::string m_msg;
};

#endif // EXCEPTIONS_H
