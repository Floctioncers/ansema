#ifndef AES_TRANSFORMATOR_H
#define AES_TRANSFORMATOR_H

#include "thread_pool.h"

#include <string>
#include <array>
#include <vector>
#include <cryptopp/cryptlib.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>
#include <cryptopp/hkdf.h>

namespace AesTransformator
{
	template<typename T>
	using Future = ThreadPool::Future<T>;

    std::array<unsigned char, 32> GenerateKey(std::array<unsigned char, 32> const &salt, std::string const &password)
    {
        std::string const information{ "Generating 256bit key" };
        std::array<unsigned char, 32> derived{};
        CryptoPP::HKDF<CryptoPP::SHA256> hkdf{};
        hkdf.DeriveKey(
            reinterpret_cast<CryptoPP::byte*>(derived.data()), derived.size(),
            reinterpret_cast<CryptoPP::byte const*>(password.data()), password.size(),
            reinterpret_cast<CryptoPP::byte const*>(salt.data()), salt.size(),
            reinterpret_cast<CryptoPP::byte const*>(information.data()), information.size());
        return derived;
    }

    std::array<unsigned char, 32> GenerateSalt()
    {
        CryptoPP::AutoSeededX917RNG<CryptoPP::AES> rng;
        std::array<unsigned char, 32> salt{};
        rng.GenerateBlock(reinterpret_cast<CryptoPP::byte*>(salt.data()), salt.size());
        return salt;
    }

    class AesTransformator
    {
    private:
        using AES = CryptoPP::ECB_Mode<CryptoPP::AES>;
        using Block = CryptoPP::SecByteBlock;
        using Sink = CryptoPP::StringSink;
        using Filter = CryptoPP::StreamTransformationFilter;

        Block key;

    public:
        AesTransformator() : key{ 32 } {}
        AesTransformator(AesTransformator const&) = default;
        AesTransformator(AesTransformator&&) = default;
        AesTransformator& operator=(AesTransformator const&) = default;
        AesTransformator& operator=(AesTransformator&&) = default;
        ~AesTransformator() = default;

        std::string Encrypt(std::string const& plain)
        {
            AES::Encryption e{};
            e.SetKey(key, key.size());
            std::string out{};
            auto encryptor = CryptoPP::StringSource{ plain, true, new Filter{ e, new Sink{ out } } };
            return out;
        }

        std::string Decrypt(std::string const& encrypted)
        {
            AES::Decryption d{};
            d.SetKey(key, key.size());
            std::string out{};
            auto decryptor = CryptoPP::StringSource{ encrypted, true, new Filter{ d, new Sink{ out } } };
            return out;
        }

        void SetKey(std::array<unsigned char, 32> &&key)
        {
            Block temp{ 32 };
            temp.Assign(key.data(), key.size());
            this->key = std::move(temp);
        }

        void SetKey(std::array<unsigned char, 32> const &key)
        {
            Block temp{ 32 };
            temp.Assign(key.data(), key.size());
            this->key = std::move(temp);
        }

        void SetKey(std::array<unsigned char, 32> const &salt, std::string const &password)
        {
            SetKey(GenerateKey(salt, password));
        }
    };

    class AesFile
    {
    private:
        using IOStream = ThreadPool::ThreadStream<unsigned char>;
        AesTransformator transformator;
        std::string data;
		std::array<unsigned char, 32> salt;
		std::string key;
        IOStream stream;

    public:
        AesFile(std::string const &key) : transformator{}, data{}, stream{}, salt{ GenerateSalt() }, key{ key }
        {
            transformator.SetKey(salt, key);
        }
        AesFile(AesFile const&) = default;
        AesFile(AesFile&&) = default;
        AesFile& operator=(AesFile const&) = default;
        AesFile& operator=(AesFile&&) = default;
        ~AesFile() = default;

        void Append(std::string const &text)
        {
			data.append(text);
        }

		void Append(std::string &&text)
		{
			data.append(std::move(text));
		}

		std::string Get()
		{
			std::string out{ std::move(data) };
			data.clear();
			return out;
		}

        void Write(std::filesystem::path const &path)
        {
            auto temp{ transformator.Encrypt(data) };

            std::vector<unsigned char> out{};
            out.reserve(temp.size() + salt.size());
			for (auto const& item : salt)
			{
				out.push_back(item);
			}

            for (auto &&item : temp)
                out.push_back(std::move(item));

            auto p{ path };
            stream.Write(std::move(p), std::move(out));
        }

		void Read(std::filesystem::path const &path)
		{
			auto p{ path };
			Future<std::vector<unsigned char>> in{};
			stream.Read(std::move(p), in);

			while (in.IsEmpty())
			{
				std::this_thread::yield();
			}

			std::optional<std::vector<unsigned char>> read{ in.Get() };
			if (!read.has_value())
				return;
			std::vector<unsigned char> value{ read.value() };
			for (std::size_t i = 0; i < salt.size(); ++i)
			{
				salt[i] = value[i];
			}

			std::string encrypted{};
			encrypted.reserve(value.size() - 32);
			for (std::size_t i = salt.size(); i < value.size(); ++i)
			{
				encrypted.push_back(value[i]);
			}

			transformator.SetKey(salt, key);
            try 
            {
                data = transformator.Decrypt(encrypted);
            }
            catch (CryptoPP::InvalidCiphertext &ex)
            {
                data = std::string{ ex.GetWhat() };
            }
		}
    };
}

#endif