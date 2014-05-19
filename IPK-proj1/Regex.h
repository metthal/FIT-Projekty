/**
 * Project: IPK - Project 1 (2014) - FTP client
 * Author: Marek Milkovic <xmilko01@stud.fit.vutbr.cz>
 **/
#ifndef REGEX_H
#define REGEX_H

#include <sys/types.h>
#include <regex.h>
#include <vector>
#include <utility>

typedef std::vector<std::string> MatchList;

class Regex
{
public:
    Regex(const char* pattern)
    {
        regcomp(&m_regex, pattern, REG_EXTENDED);
    }

    ~Regex()
    {
        regfree(&m_regex);
    }

    bool Match(const char* str)
    {
        if (regexec(&m_regex, str, 0, NULL, 0) == REG_NOMATCH)
            return false;

        return true;
    }

    bool Match(const char* str, uint32_t maxMatches, MatchList& matches)
    {
        // don't allow 0 maxMatches
        if (maxMatches == 0)
            maxMatches = 1;

        regmatch_t regMatches[maxMatches];
        if (regexec(&m_regex, str, maxMatches, regMatches, 0) == REG_NOMATCH)
            return false;

        matches.clear();
        uint32_t matchIdx = 0;
        while (matchIdx < maxMatches)
        {
            // match group is empty
            if (regMatches[matchIdx].rm_so == -1)
                matches.push_back("");
            else
                matches.push_back(std::string(str + regMatches[matchIdx].rm_so, regMatches[matchIdx].rm_eo - regMatches[matchIdx].rm_so));

            ++matchIdx;
        }

        return true;
    }

private:
    regex_t m_regex;
};

#endif // REGEX_H
