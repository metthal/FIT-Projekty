#ifndef IPK_EXCEPTION_H
#define IPK_EXCEPTION_H

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

#endif // IPK_EXCEPTION_H
