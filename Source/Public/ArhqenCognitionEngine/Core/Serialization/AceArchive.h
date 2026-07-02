#pragma once

#include "ArhqenCognitionEngine/Core/Identity/AceGuid.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace am::core::serialization
{
    enum class ArchiveError
    {
        None, TooSmall, InvalidMagic, UnsupportedContainerVersion, SchemaMismatch,
        ObjectVersionTooOld, ObjectVersionTooNew, PayloadTooLarge, Truncated,
        ChecksumMismatch, InvalidBoolean, InvalidString, EndOfPayload
    };

    class ArchiveWriter final
    {
    public:
        ArchiveWriter(Guid schema, std::uint32_t objectVersion);

        void writeU8(std::uint8_t value);
        void writeBool(bool value);
        void writeU32(std::uint32_t value);
        void writeU64(std::uint64_t value);
        void writeI64(std::int64_t value);
        void writeDouble(double value);
        void writeGuid(const Guid& value);
        bool writeString(std::string_view value);

        [[nodiscard]] const std::vector<std::uint8_t>& payload() const noexcept { return payload_; }
        [[nodiscard]] std::vector<std::uint8_t> finish() const;

    private:
        Guid schema_;
        std::uint32_t objectVersion_ = 0;
        std::vector<std::uint8_t> payload_;
    };

    class ArchiveReader final
    {
    public:
        bool open(std::span<const std::uint8_t> bytes, const Guid& expectedSchema,
                  std::uint32_t minimumObjectVersion, std::uint32_t maximumObjectVersion,
                  std::size_t maximumPayloadBytes = 512ull * 1024ull * 1024ull) noexcept;

        bool readU8(std::uint8_t& value) noexcept;
        bool readBool(bool& value) noexcept;
        bool readU32(std::uint32_t& value) noexcept;
        bool readU64(std::uint64_t& value) noexcept;
        bool readI64(std::int64_t& value) noexcept;
        bool readDouble(double& value) noexcept;
        bool readGuid(Guid& value) noexcept;
        bool readString(std::string& value, std::size_t maximumBytes = 16ull * 1024ull * 1024ull);

        [[nodiscard]] bool atEnd() const noexcept { return offset_ == payload_.size(); }
        [[nodiscard]] ArchiveError error() const noexcept { return error_; }
        [[nodiscard]] std::uint32_t objectVersion() const noexcept { return objectVersion_; }
        [[nodiscard]] std::size_t remaining() const noexcept { return payload_.size() - offset_; }

    private:
        bool take(std::size_t count, std::span<const std::uint8_t>& output) noexcept;
        std::span<const std::uint8_t> payload_;
        std::size_t offset_ = 0;
        std::uint32_t objectVersion_ = 0;
        ArchiveError error_ = ArchiveError::None;
    };

    [[nodiscard]] std::string_view archiveErrorText(ArchiveError error) noexcept;
}
