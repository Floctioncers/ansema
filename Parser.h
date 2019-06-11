#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <utility>

namespace Parser
{
    class Parser
    {
    private:
        static std::string const whitespace;
        std::vector<std::pair<std::size_t, std::size_t>> list;
        std::string const& text;
        std::size_t current;

        bool IsWhitespace(char const c)
        {
            for(auto const item : whitespace)
            {
                if (c == item)
                return true;
            }
            return false;
        }

        bool IsValid(char const c)
        {
            return (text[current] == c) &&
                (((current + 1) == text.size()) || (IsWhitespace(text[current + 1])));
        } 

        std::size_t FindNextToken(char const c)
        {
            current = text.find_first_not_of(whitespace, current);
            while(current != std::string::npos)
            {
                if (IsValid(c))
                {
                    break;
                }
                current = text.find_first_of(whitespace, current);
                current = text.find_first_not_of(whitespace, current);
            }
            return current;
        }

        void FindTokens()
        {
            while(current != std::string::npos)
            {
                auto start = FindNextToken('[');
                auto end = FindNextToken(']');
                list.push_back(std::make_pair(start, end));
            }
        }

    public:

        Parser(std::string const &text) : text{ text }, current{ 0 }, list{} {}
        Parser(Parser const&) = default;
        Parser(Parser&&) = default;
        Parser& operator=(Parser const &) = default;
        Parser& operator=(Parser&&) = default;
        ~Parser() = default;
         
        std::vector<std::pair<std::size_t, std::size_t>> GetTokens()
        {
            FindTokens();
            if (list.size() > 0)
            {
                if (list.back().second == std::string::npos)
                {
                    list.pop_back();
                }
            }
            std::vector<std::pair<std::size_t, std::size_t>> out = std::move(list);
            list.clear();
            current = 0;
            return out;
        }    

    };
}

#endif