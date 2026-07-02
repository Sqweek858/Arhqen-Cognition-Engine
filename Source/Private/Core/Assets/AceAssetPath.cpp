#include "ArhqenCognitionEngine/Core/Assets/AceAssetPath.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <system_error>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace am::core::assets
{
    namespace
    {
        constexpr std::size_t MaxVirtualPathBytes = 1024;
        constexpr std::size_t MaxSegmentBytes = 255;

        void setError(AssetPathError* output, AssetPathError value) noexcept
        {
            if (output)
            {
                *output = value;
            }
        }

        [[nodiscard]] bool validUtf8(std::string_view text) noexcept
        {
            std::size_t index = 0;
            while (index < text.size())
            {
                const unsigned char first = static_cast<unsigned char>(text[index]);
                if (first <= 0x7fU)
                {
                    ++index;
                    continue;
                }

                std::size_t continuationCount = 0;
                std::uint32_t value = 0;
                if ((first & 0xe0U) == 0xc0U) { continuationCount = 1; value = first & 0x1fU; }
                else if ((first & 0xf0U) == 0xe0U) { continuationCount = 2; value = first & 0x0fU; }
                else if ((first & 0xf8U) == 0xf0U) { continuationCount = 3; value = first & 0x07U; }
                else { return false; }

                if (index + continuationCount >= text.size())
                {
                    return false;
                }
                for (std::size_t offset = 1; offset <= continuationCount; ++offset)
                {
                    const unsigned char next = static_cast<unsigned char>(text[index + offset]);
                    if ((next & 0xc0U) != 0x80U)
                    {
                        return false;
                    }
                    value = (value << 6) | (next & 0x3fU);
                }

                const bool overlong = (continuationCount == 1 && value < 0x80U) ||
                                      (continuationCount == 2 && value < 0x800U) ||
                                      (continuationCount == 3 && value < 0x10000U);
                if (overlong || value > 0x10ffffU || (value >= 0xd800U && value <= 0xdfffU))
                {
                    return false;
                }
                index += continuationCount + 1;
            }
            return true;
        }

        [[nodiscard]] std::string asciiLower(std::string_view value)
        {
            std::string output(value);
            std::transform(output.begin(), output.end(), output.begin(), [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });
            return output;
        }

#ifdef _WIN32
        [[nodiscard]] std::optional<std::wstring> utf8ToWide(std::string_view value)
        {
            if (value.empty()) return std::wstring{};
            const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                                   static_cast<int>(value.size()), nullptr, 0);
            if (length <= 0) return std::nullopt;
            std::wstring wide(static_cast<std::size_t>(length), L'\0');
            if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
                                    wide.data(), length) != length)
            {
                return std::nullopt;
            }
            return wide;
        }

        [[nodiscard]] std::optional<std::string> wideToUtf8(std::wstring_view value)
        {
            if (value.empty()) return std::string{};
            const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                                                   static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
            if (length <= 0) return std::nullopt;
            std::string utf8(static_cast<std::size_t>(length), '\0');
            if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
                                    utf8.data(), length, nullptr, nullptr) != length)
            {
                return std::nullopt;
            }
            return utf8;
        }

        [[nodiscard]] std::optional<std::string> normalizeUtf8Nfc(std::string_view value)
        {
            const auto wide = utf8ToWide(value);
            if (!wide) return std::nullopt;
            std::wstring normalized(wide->size() * 3 + 8, L'\0');
            int written = NormalizeString(NormalizationC, wide->data(), static_cast<int>(wide->size()),
                                          normalized.data(), static_cast<int>(normalized.size()));
            if (written < 0)
            {
                normalized.resize(static_cast<std::size_t>(-written));
                written = NormalizeString(NormalizationC, wide->data(), static_cast<int>(wide->size()),
                                          normalized.data(), static_cast<int>(normalized.size()));
            }
            if (written <= 0)
            {
                return std::nullopt;
            }
            normalized.resize(static_cast<std::size_t>(written));
            return wideToUtf8(normalized);
        }
#else
        [[nodiscard]] std::optional<std::string> normalizeUtf8Nfc(std::string_view value)
        {
            return std::string(value);
        }
#endif

        [[nodiscard]] bool reservedWindowsName(std::string_view segment)
        {
            const std::size_t dot = segment.find('.');
            const std::string base = asciiLower(segment.substr(0, dot));
            static constexpr std::array Reserved{"con", "prn", "aux", "nul", "clock$"};
            if (std::find(Reserved.begin(), Reserved.end(), base) != Reserved.end())
            {
                return true;
            }
            if (base.size() == 4 && (base.starts_with("com") || base.starts_with("lpt")) &&
                base[3] >= '1' && base[3] <= '9')
            {
                return true;
            }
            return false;
        }

        [[nodiscard]] bool validateSegment(std::string_view segment, AssetPathError* error)
        {
            if (segment.empty())
            {
                setError(error, AssetPathError::EmptySegment);
                return false;
            }
            if (segment == "." || segment == "..")
            {
                setError(error, AssetPathError::Traversal);
                return false;
            }
            if (segment.size() > MaxSegmentBytes)
            {
                setError(error, AssetPathError::TooLong);
                return false;
            }
            if (segment.back() == '.' || segment.back() == ' ')
            {
                setError(error, AssetPathError::InvalidEnding);
                return false;
            }
            for (const unsigned char ch : segment)
            {
                if (ch < 0x20U || ch == '<' || ch == '>' || ch == ':' || ch == '"' ||
                    ch == '|' || ch == '?' || ch == '*' || ch == '\\')
                {
                    setError(error, AssetPathError::InvalidCharacter);
                    return false;
                }
            }
            if (reservedWindowsName(segment))
            {
                setError(error, AssetPathError::ReservedName);
                return false;
            }
            return true;
        }

        [[nodiscard]] std::filesystem::path pathFromUtf8(std::string_view value)
        {
#ifdef _WIN32
            const auto wide = utf8ToWide(value);
            return wide ? std::filesystem::path(*wide) : std::filesystem::path{};
#else
            return std::filesystem::path(value);
#endif
        }

        [[nodiscard]] std::string pathToUtf8(const std::filesystem::path& value)
        {
#ifdef _WIN32
            const std::wstring wide = value.generic_wstring();
            const auto utf8 = wideToUtf8(wide);
            return utf8.value_or(std::string{});
#else
            return value.generic_string();
#endif
        }
    }

    std::optional<AssetPath> AssetPath::parse(std::string_view text, AssetPathError* error)
    {
        setError(error, AssetPathError::None);
        if (text.empty())
        {
            setError(error, AssetPathError::Empty);
            return std::nullopt;
        }
        if (text.size() > MaxVirtualPathBytes)
        {
            setError(error, AssetPathError::TooLong);
            return std::nullopt;
        }
        if (!validUtf8(text))
        {
            setError(error, AssetPathError::InvalidUtf8);
            return std::nullopt;
        }

        const auto nfc = normalizeUtf8Nfc(text);
        if (!nfc)
        {
            setError(error, AssetPathError::InvalidUtf8);
            return std::nullopt;
        }
        std::string normalized = *nfc;
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        while (normalized.size() > 1 && normalized.back() == '/') normalized.pop_back();
        if (normalized != "/Game" && !normalized.starts_with("/Game/"))
        {
            setError(error, AssetPathError::WrongMount);
            return std::nullopt;
        }

        std::size_t start = normalized == "/Game" ? normalized.size() : 6;
        while (start < normalized.size())
        {
            const std::size_t slash = normalized.find('/', start);
            const std::size_t end = slash == std::string::npos ? normalized.size() : slash;
            if (!validateSegment(std::string_view(normalized).substr(start, end - start), error))
            {
                return std::nullopt;
            }
            if (slash == std::string::npos) break;
            start = slash + 1;
        }
        return AssetPath(std::move(normalized));
    }

    std::optional<AssetPath> AssetPath::fromFilesystemPath(const std::filesystem::path& contentRoot,
                                                            const std::filesystem::path& path,
                                                            AssetPathError* error)
    {
        setError(error, AssetPathError::None);
        std::error_code ec;
        const std::filesystem::path root = std::filesystem::absolute(contentRoot, ec).lexically_normal();
        if (ec)
        {
            setError(error, AssetPathError::OutsideContentRoot);
            return std::nullopt;
        }
        const std::filesystem::path absolute = std::filesystem::absolute(path, ec).lexically_normal();
        if (ec)
        {
            setError(error, AssetPathError::OutsideContentRoot);
            return std::nullopt;
        }
        const std::filesystem::path relative = absolute.lexically_relative(root);
        if (relative.empty() && absolute != root)
        {
            setError(error, AssetPathError::OutsideContentRoot);
            return std::nullopt;
        }
        for (const auto& part : relative)
        {
            if (part == "..")
            {
                setError(error, AssetPathError::OutsideContentRoot);
                return std::nullopt;
            }
        }
        const std::string suffix = relative == "." ? std::string{} : pathToUtf8(relative);
        if (relative != "." && suffix.empty())
        {
            setError(error, AssetPathError::InvalidUtf8);
            return std::nullopt;
        }
        return parse(suffix.empty() ? "/Game" : "/Game/" + suffix, error);
    }

    std::string AssetPath::comparisonKey() const
    {
#ifdef _WIN32
        const auto wide = utf8ToWide(value_);
        if (!wide) return asciiLower(value_);
        const int length = LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE,
                                         wide->data(), static_cast<int>(wide->size()), nullptr, 0,
                                         nullptr, nullptr, 0);
        if (length <= 0) return asciiLower(value_);
        std::wstring lowered(static_cast<std::size_t>(length), L'\0');
        if (LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE,
                          wide->data(), static_cast<int>(wide->size()), lowered.data(), length,
                          nullptr, nullptr, 0) != length)
        {
            return asciiLower(value_);
        }
        const auto utf8 = wideToUtf8(lowered);
        return utf8.value_or(asciiLower(value_));
#else
        return asciiLower(value_);
#endif
    }

    std::string_view AssetPath::leafName() const noexcept
    {
        if (value_.empty()) return {};
        const std::size_t slash = value_.find_last_of('/');
        return slash == std::string::npos ? std::string_view(value_) : std::string_view(value_).substr(slash + 1);
    }

    AssetPath AssetPath::parent() const
    {
        if (empty() || isRoot()) return *this;
        const std::size_t slash = value_.find_last_of('/');
        return AssetPath(slash <= 5 ? "/Game" : value_.substr(0, slash));
    }

    std::optional<AssetPath> AssetPath::child(std::string_view segment, AssetPathError* error) const
    {
        if (empty() || !validateSegment(segment, error) || !validUtf8(segment))
        {
            if (!validUtf8(segment)) setError(error, AssetPathError::InvalidUtf8);
            return std::nullopt;
        }
        return parse(value_ + "/" + std::string(segment), error);
    }

    std::filesystem::path AssetPath::toFilesystemPath(const std::filesystem::path& contentRoot) const
    {
        if (empty()) return {};
        std::filesystem::path output = contentRoot;
        std::size_t start = isRoot() ? value_.size() : 6;
        while (start < value_.size())
        {
            const std::size_t slash = value_.find('/', start);
            const std::size_t end = slash == std::string::npos ? value_.size() : slash;
            output /= pathFromUtf8(std::string_view(value_).substr(start, end - start));
            if (slash == std::string::npos) break;
            start = slash + 1;
        }
        return output.lexically_normal();
    }

    std::string_view assetPathErrorText(AssetPathError error) noexcept
    {
        switch (error)
        {
        case AssetPathError::None: return "No error";
        case AssetPathError::Empty: return "Asset path is empty";
        case AssetPathError::TooLong: return "Asset path is too long";
        case AssetPathError::InvalidUtf8: return "Asset path is not valid UTF-8";
        case AssetPathError::WrongMount: return "Asset path must use the /Game mount";
        case AssetPathError::EmptySegment: return "Asset path contains an empty segment";
        case AssetPathError::Traversal: return "Asset path traversal is not allowed";
        case AssetPathError::InvalidCharacter: return "Asset path contains a character invalid on Windows";
        case AssetPathError::InvalidEnding: return "Asset path segment cannot end in a dot or space";
        case AssetPathError::ReservedName: return "Asset path uses a reserved Windows filename";
        case AssetPathError::OutsideContentRoot: return "Filesystem path is outside Content";
        }
        return "Invalid asset path";
    }
}
