#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <utility>

namespace Parser
{
    using List = std::vector<std::pair<std::size_t, std::size_t>>;
    using Block = std::pair<std::pair<std::size_t, std::size_t>, std::u32string>;
    class Parser
    {
    private:
        static std::u32string const whitespace;
        List list;
        std::u32string const& text;
        std::size_t current;

        bool IsWhitespace(char32_t const c)
        {
            for (auto const item : whitespace)
            {
                if (c == item)
                    return true;
            }
            return false;
        }

        bool IsValid(char32_t const c)
        {
            return (text[current] == c) &&
                (((current + 1) == text.size()) || (IsWhitespace(text[current + 1])));
        }

        std::size_t FindNextToken(char32_t const c)
        {
            current = text.find_first_not_of(whitespace, current);
            while (current != std::u32string::npos)
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
            while (current != std::u32string::npos)
            {
                auto start = FindNextToken('[');
                auto end = FindNextToken(']');
                list.push_back(std::make_pair(start, end));
            }
        }

    public:

        Parser(std::u32string const& text) : text{ text }, current{ 0 }, list{} {}
        Parser(Parser const&) = default;
        Parser(Parser&&) = default;
        Parser& operator=(Parser const&) = default;
        Parser& operator=(Parser&&) = default;
        ~Parser() = default;

        List GetTokens()
        {
            FindTokens();
            if (list.size() > 0)
            {
                if (list.back().second == std::u32string::npos)
                {
                    list.pop_back();
                }
            }
            List out = std::move(list);
            list.clear();
            current = 0;
            return out;
        }
    };

    inline std::u32string const Parser::whitespace{ U" \t\n" };

    class Transformator
    {
    private:
        std::vector<Block> blocks;
        std::u32string const& text;

        static std::u32string const masks;
    public:
        Transformator(std::u32string const& text) : text{ text }, blocks{} {}
        Transformator(Transformator const&) = default;
        Transformator(Transformator&&) = default;
        Transformator& operator=(Transformator const&) = default;
        Transformator& operator=(Transformator&&) = default;
        ~Transformator() = default;

        std::u32string Transform(List&& list)
        {
            std::u32string temp{};
            std::size_t current{ 0 };
            for (auto&& item : list)
            {
                temp.append(text.substr(current, item.first - current));
                temp.append(masks);
                blocks.push_back(
                    std::make_pair(
                        std::make_pair(temp.size() - masks.size(), temp.size() - 1),
                        text.substr(item.first + 2, item.second - (item.first + 3))));
                current = item.second + 1;
            }
            temp.append(text.substr(current, std::u32string::npos));
            return temp;
        }

        std::vector<Block> Get()
        {
            std::vector<Block> temp{ std::move(blocks) };
            blocks.clear();
            return temp;
        }
    };

    inline std::u32string const Transformator::masks{ U"******" };
}

#endif