#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace am::core::serialization
{
    class AtomicFile final
    {
    public:
        static bool write(const std::filesystem::path& destination,
                          std::span<const std::uint8_t> bytes,
                          std::string* error = nullptr);
        static bool read(const std::filesystem::path& source,
                         std::vector<std::uint8_t>& bytes,
                         std::size_t maximumBytes,
                         std::string* error = nullptr);
    };
}
