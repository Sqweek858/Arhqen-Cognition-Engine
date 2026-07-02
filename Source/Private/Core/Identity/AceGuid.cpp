#include "ArhqenCognitionEngine/Core/Identity/AceGuid.h"

#include <chrono>
#include <random>
#include <thread>

namespace am::core
{
    namespace
    {
        [[nodiscard]] constexpr int hexValue(char value) noexcept
        {
            if (value >= '0' && value <= '9') return value - '0';
            if (value >= 'a' && value <= 'f') return value - 'a' + 10;
            if (value >= 'A' && value <= 'F') return value - 'A' + 10;
            return -1;
        }
    }

    Guid Guid::create()
    {
        thread_local std::mt19937_64 generator([]
        {
            std::random_device source;
            const auto now = static_cast<std::uint64_t>(
                std::chrono::high_resolution_clock::now().time_since_epoch().count());
            const auto thread = static_cast<std::uint64_t>(
                std::hash<std::thread::id>{}(std::this_thread::get_id()));
            std::seed_seq seed{
                source(), source(), source(), source(),
                static_cast<std::uint32_t>(now), static_cast<std::uint32_t>(now >> 32),
                static_cast<std::uint32_t>(thread), static_cast<std::uint32_t>(thread >> 32)};
            return std::mt19937_64(seed);
        }());

        Bytes bytes{};
        for (std::size_t offset = 0; offset < bytes.size(); offset += sizeof(std::uint64_t))
        {
            const std::uint64_t value = generator();
            for (std::size_t byte = 0; byte < sizeof(value); ++byte)
            {
                bytes[offset + byte] = static_cast<std::uint8_t>(value >> (byte * 8));
            }
        }

        // RFC 4122 variant and version 4. These bits also make the all-zero value impossible.
        bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0fU) | 0x40U);
        bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3fU) | 0x80U);
        return Guid(bytes);
    }

    std::optional<Guid> Guid::parse(std::string_view text) noexcept
    {
        if (text.size() != 36 || text[8] != '-' || text[13] != '-' || text[18] != '-' || text[23] != '-')
        {
            return std::nullopt;
        }

        Bytes bytes{};
        std::size_t output = 0;
        for (std::size_t input = 0; input < text.size();)
        {
            if (text[input] == '-')
            {
                ++input;
                continue;
            }
            if (input + 1 >= text.size() || output >= bytes.size())
            {
                return std::nullopt;
            }
            const int high = hexValue(text[input]);
            const int low = hexValue(text[input + 1]);
            if (high < 0 || low < 0)
            {
                return std::nullopt;
            }
            bytes[output++] = static_cast<std::uint8_t>((high << 4) | low);
            input += 2;
        }
        return output == bytes.size() ? std::optional<Guid>{Guid(bytes)} : std::nullopt;
    }

    std::string Guid::toString() const
    {
        static constexpr char Hex[] = "0123456789abcdef";
        std::string result;
        result.reserve(36);
        for (std::size_t index = 0; index < bytes_.size(); ++index)
        {
            if (index == 4 || index == 6 || index == 8 || index == 10)
            {
                result.push_back('-');
            }
            result.push_back(Hex[bytes_[index] >> 4]);
            result.push_back(Hex[bytes_[index] & 0x0fU]);
        }
        return result;
    }

    std::size_t GuidHash::operator()(const Guid& guid) const noexcept
    {
        // FNV-1a gives stable behavior across 32/64-bit standard-library implementations.
        std::size_t hash = sizeof(std::size_t) == 8
            ? static_cast<std::size_t>(14695981039346656037ull)
            : static_cast<std::size_t>(2166136261u);
        const std::size_t prime = sizeof(std::size_t) == 8
            ? static_cast<std::size_t>(1099511628211ull)
            : static_cast<std::size_t>(16777619u);
        for (const std::uint8_t byte : guid.bytes())
        {
            hash ^= byte;
            hash *= prime;
        }
        return hash;
    }
}
