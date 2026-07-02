#include "ArhqenCognitionEngine/Core/Serialization/AceArchive.h"

#include <algorithm>
#include <bit>
#include <iterator>
#include <limits>

namespace am::core::serialization
{
    namespace
    {
        constexpr std::uint8_t Magic[]{'A', 'C', 'E', 'B', 'I', 'N', 1, 0};
        constexpr std::uint32_t ContainerVersion = 1;
        constexpr std::size_t HeaderBytes = 48;

        void appendU32(std::vector<std::uint8_t>& output, std::uint32_t value)
        {
            for (int shift = 0; shift < 32; shift += 8) output.push_back(static_cast<std::uint8_t>(value >> shift));
        }

        void appendU64(std::vector<std::uint8_t>& output, std::uint64_t value)
        {
            for (int shift = 0; shift < 64; shift += 8) output.push_back(static_cast<std::uint8_t>(value >> shift));
        }

        [[nodiscard]] std::uint32_t decodeU32(std::span<const std::uint8_t> bytes) noexcept
        {
            std::uint32_t value = 0;
            for (std::size_t i = 0; i < 4; ++i) value |= static_cast<std::uint32_t>(bytes[i]) << (i * 8);
            return value;
        }

        [[nodiscard]] std::uint64_t decodeU64(std::span<const std::uint8_t> bytes) noexcept
        {
            std::uint64_t value = 0;
            for (std::size_t i = 0; i < 8; ++i) value |= static_cast<std::uint64_t>(bytes[i]) << (i * 8);
            return value;
        }

        [[nodiscard]] std::uint64_t checksum(std::span<const std::uint8_t> bytes) noexcept
        {
            std::uint64_t hash = 14695981039346656037ull;
            for (const std::uint8_t byte : bytes) { hash ^= byte; hash *= 1099511628211ull; }
            return hash;
        }

        [[nodiscard]] bool validUtf8(std::string_view text) noexcept
        {
            std::size_t i = 0;
            while (i < text.size())
            {
                const auto first = static_cast<unsigned char>(text[i]);
                if (first < 0x80U) { ++i; continue; }
                std::size_t count = 0; std::uint32_t value = 0;
                if ((first & 0xe0U) == 0xc0U) { count = 1; value = first & 0x1fU; }
                else if ((first & 0xf0U) == 0xe0U) { count = 2; value = first & 0x0fU; }
                else if ((first & 0xf8U) == 0xf0U) { count = 3; value = first & 0x07U; }
                else return false;
                if (i + count >= text.size()) return false;
                for (std::size_t j = 1; j <= count; ++j)
                {
                    const auto next = static_cast<unsigned char>(text[i + j]);
                    if ((next & 0xc0U) != 0x80U) return false;
                    value = (value << 6) | (next & 0x3fU);
                }
                if ((count == 1 && value < 0x80U) || (count == 2 && value < 0x800U) ||
                    (count == 3 && value < 0x10000U) || value > 0x10ffffU ||
                    (value >= 0xd800U && value <= 0xdfffU)) return false;
                i += count + 1;
            }
            return true;
        }
    }

    ArchiveWriter::ArchiveWriter(Guid schema, std::uint32_t objectVersion)
        : schema_(schema), objectVersion_(objectVersion) {}

    void ArchiveWriter::writeU8(std::uint8_t value) { payload_.push_back(value); }
    void ArchiveWriter::writeBool(bool value) { writeU8(value ? 1U : 0U); }
    void ArchiveWriter::writeU32(std::uint32_t value) { appendU32(payload_, value); }
    void ArchiveWriter::writeU64(std::uint64_t value) { appendU64(payload_, value); }
    void ArchiveWriter::writeI64(std::int64_t value) { writeU64(std::bit_cast<std::uint64_t>(value)); }
    void ArchiveWriter::writeDouble(double value) { writeU64(std::bit_cast<std::uint64_t>(value)); }
    void ArchiveWriter::writeGuid(const Guid& value) { payload_.insert(payload_.end(), value.bytes().begin(), value.bytes().end()); }

    bool ArchiveWriter::writeString(std::string_view value)
    {
        if (value.size() > std::numeric_limits<std::uint32_t>::max() || !validUtf8(value)) return false;
        writeU32(static_cast<std::uint32_t>(value.size()));
        payload_.insert(payload_.end(), value.begin(), value.end());
        return true;
    }

    std::vector<std::uint8_t> ArchiveWriter::finish() const
    {
        std::vector<std::uint8_t> output;
        output.reserve(HeaderBytes + payload_.size());
        output.insert(output.end(), std::begin(Magic), std::end(Magic));
        appendU32(output, ContainerVersion);
        output.insert(output.end(), schema_.bytes().begin(), schema_.bytes().end());
        appendU32(output, objectVersion_);
        appendU64(output, static_cast<std::uint64_t>(payload_.size()));
        appendU64(output, checksum(payload_));
        output.insert(output.end(), payload_.begin(), payload_.end());
        return output;
    }

    bool ArchiveReader::open(std::span<const std::uint8_t> bytes, const Guid& expectedSchema,
                             std::uint32_t minimumObjectVersion, std::uint32_t maximumObjectVersion,
                             std::size_t maximumPayloadBytes) noexcept
    {
        payload_ = {}; offset_ = 0; objectVersion_ = 0; error_ = ArchiveError::None;
        if (bytes.size() < HeaderBytes) { error_ = ArchiveError::TooSmall; return false; }
        if (!std::equal(std::begin(Magic), std::end(Magic), bytes.begin())) { error_ = ArchiveError::InvalidMagic; return false; }
        if (decodeU32(bytes.subspan(8, 4)) != ContainerVersion) { error_ = ArchiveError::UnsupportedContainerVersion; return false; }

        Guid::Bytes schemaBytes{};
        std::copy_n(bytes.begin() + 12, schemaBytes.size(), schemaBytes.begin());
        if (Guid::fromBytes(schemaBytes) != expectedSchema) { error_ = ArchiveError::SchemaMismatch; return false; }
        objectVersion_ = decodeU32(bytes.subspan(28, 4));
        if (objectVersion_ < minimumObjectVersion) { error_ = ArchiveError::ObjectVersionTooOld; return false; }
        if (objectVersion_ > maximumObjectVersion) { error_ = ArchiveError::ObjectVersionTooNew; return false; }

        const std::uint64_t payloadLength = decodeU64(bytes.subspan(32, 8));
        if (payloadLength > maximumPayloadBytes || payloadLength > std::numeric_limits<std::size_t>::max())
        { error_ = ArchiveError::PayloadTooLarge; return false; }
        if (payloadLength != bytes.size() - HeaderBytes) { error_ = ArchiveError::Truncated; return false; }
        payload_ = bytes.subspan(HeaderBytes, static_cast<std::size_t>(payloadLength));
        if (checksum(payload_) != decodeU64(bytes.subspan(40, 8))) { payload_ = {}; error_ = ArchiveError::ChecksumMismatch; return false; }
        return true;
    }

    bool ArchiveReader::take(std::size_t count, std::span<const std::uint8_t>& output) noexcept
    {
        if (error_ != ArchiveError::None) return false;
        if (count > remaining()) { error_ = ArchiveError::EndOfPayload; return false; }
        output = payload_.subspan(offset_, count); offset_ += count; return true;
    }

    bool ArchiveReader::readU8(std::uint8_t& value) noexcept
    { std::span<const std::uint8_t> bytes; if (!take(1, bytes)) return false; value = bytes[0]; return true; }
    bool ArchiveReader::readBool(bool& value) noexcept
    { std::uint8_t raw = 0; if (!readU8(raw)) return false; if (raw > 1) { error_ = ArchiveError::InvalidBoolean; return false; } value = raw != 0; return true; }
    bool ArchiveReader::readU32(std::uint32_t& value) noexcept
    { std::span<const std::uint8_t> bytes; if (!take(4, bytes)) return false; value = decodeU32(bytes); return true; }
    bool ArchiveReader::readU64(std::uint64_t& value) noexcept
    { std::span<const std::uint8_t> bytes; if (!take(8, bytes)) return false; value = decodeU64(bytes); return true; }
    bool ArchiveReader::readI64(std::int64_t& value) noexcept
    { std::uint64_t raw = 0; if (!readU64(raw)) return false; value = std::bit_cast<std::int64_t>(raw); return true; }
    bool ArchiveReader::readDouble(double& value) noexcept
    { std::uint64_t raw = 0; if (!readU64(raw)) return false; value = std::bit_cast<double>(raw); return true; }
    bool ArchiveReader::readGuid(Guid& value) noexcept
    { std::span<const std::uint8_t> bytes; if (!take(16, bytes)) return false; Guid::Bytes raw{}; std::copy(bytes.begin(), bytes.end(), raw.begin()); value = Guid::fromBytes(raw); return true; }

    bool ArchiveReader::readString(std::string& value, std::size_t maximumBytes)
    {
        std::uint32_t length = 0; if (!readU32(length)) return false;
        if (length > maximumBytes) { error_ = ArchiveError::InvalidString; return false; }
        std::span<const std::uint8_t> bytes; if (!take(length, bytes)) return false;
        std::string candidate(bytes.begin(), bytes.end());
        if (!validUtf8(candidate)) { error_ = ArchiveError::InvalidString; return false; }
        value = std::move(candidate); return true;
    }

    std::string_view archiveErrorText(ArchiveError error) noexcept
    {
        switch (error)
        {
        case ArchiveError::None: return "No error"; case ArchiveError::TooSmall: return "Archive is too small";
        case ArchiveError::InvalidMagic: return "Archive magic is invalid"; case ArchiveError::UnsupportedContainerVersion: return "Archive container version is unsupported";
        case ArchiveError::SchemaMismatch: return "Archive schema does not match"; case ArchiveError::ObjectVersionTooOld: return "Object version is too old";
        case ArchiveError::ObjectVersionTooNew: return "Object version is too new"; case ArchiveError::PayloadTooLarge: return "Archive payload exceeds its limit";
        case ArchiveError::Truncated: return "Archive is truncated or has trailing bytes"; case ArchiveError::ChecksumMismatch: return "Archive checksum does not match";
        case ArchiveError::InvalidBoolean: return "Archive boolean is invalid"; case ArchiveError::InvalidString: return "Archive string is invalid";
        case ArchiveError::EndOfPayload: return "Archive payload ended unexpectedly";
        }
        return "Archive error";
    }
}
