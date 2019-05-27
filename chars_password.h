#ifndef CHARS_PASSWORD_H
#define CHARS_PASSWORD_H

#include <string>
#include <unordered_map>
#include <cryptopp/cryptlib.h>
#include <cryptopp/osrng.h>
#include <optional>

namespace CharsPassword
{
    std::unordered_map<char, std::string> const GenerateCharMap()
    {
        std::unordered_map<char, std::string> map{};
        map['n'] = "0123456789";
        map['a'] = "qwertyuiopasdfghjklzxcvbnm";
        map['A'] = "QWERTYUIOPASDFGHJKLZXCVBNM";
        map['-'] = "-";
        map['.'] = ".";
        map['*'] = "*";
        map['_'] = "_";
        map['x'] = ",.!?;:";
        map['X'] = "@#$%^&*";
        
        return map;
    }

    class PasswordGenerator
    {
    private:
        std::unordered_map<char, std::string> map;
    public:
        PasswordGenerator() : map{ GenerateCharMap() } { }
        PasswordGenerator(PasswordGenerator const&) = default;
        PasswordGenerator(PasswordGenerator&&) = default;
        PasswordGenerator& operator=(PasswordGenerator const&) = default;
        PasswordGenerator& operator=(PasswordGenerator&&) = default;
        ~PasswordGenerator() = default;

        std::optional<char> Generate(char c) const
        {
            CryptoPP::AutoSeededX917RNG<CryptoPP::AES> rng;
            if (map.find(c) != map.cend())
            {
                auto const ref{ map.at(c) };
                uint32_t const size = static_cast<uint32_t>(ref.size());
                CryptoPP::Integer const max{ CryptoPP::Integer::Sign::POSITIVE, size - 1 };
                CryptoPP::Integer ith{ rng, CryptoPP::Integer::Zero(), max };
                return std::make_optional<char>(ref[static_cast<std::size_t>(ith.ConvertToLong())]);
            }
            else
            {
                return std::nullopt;
            }
        }

        std::optional<std::string> Generate(std::string const &str) const 
        {
            std::string out{};
            for (auto const item : str)
            {
                auto const c = Generate(item);
                if (!c.has_value())
                {
                    return std::nullopt;
                }
                else
                {
                    out.push_back(c.value());
                }
            }
            return std::make_optional(out);
        }

        std::optional<std::string> Generate(std::optional<std::string> const &str) const
        {
            if (str.has_value())
            {
                auto const &temp = str.value();
                return Generate(temp);
            }
            return std::nullopt;
        }

        bool Check(char c) const
        {
            return map.find(c) != map.cend();
        }

        bool Check(std::string str)
        {
            bool out{ true };
            for (auto const item : str)
            {
                if (!Check(item))
                {
                    out = false;
                    break;
                }
            }
            return out;
        }
    };
}

#endif // !

