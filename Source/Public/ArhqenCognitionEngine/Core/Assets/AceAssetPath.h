#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace am::core::assets
{
    enum class AssetPathError
    {
        None,
        Empty,
        TooLong,
        InvalidUtf8,
        WrongMount,
        EmptySegment,
        Traversal,
        InvalidCharacter,
        InvalidEnding,
        ReservedName,
        OutsideContentRoot
    };

    class AssetPath final
    {
    public:
        AssetPath() = default;

        [[nodiscard]] static std::optional<AssetPath> parse(std::string_view text,
                                                            AssetPathError* error = nullptr);
        [[nodiscard]] static std::optional<AssetPath> fromFilesystemPath(const std::filesystem::path& contentRoot,
                                                                         const std::filesystem::path& path,
                                                                         AssetPathError* error = nullptr);

        [[nodiscard]] bool empty() const noexcept { return value_.empty(); }
        [[nodiscard]] bool isRoot() const noexcept { return value_ == "/Game"; }
        [[nodiscard]] const std::string& string() const noexcept { return value_; }
        [[nodiscard]] std::string comparisonKey() const;
        [[nodiscard]] std::string_view leafName() const noexcept;
        [[nodiscard]] AssetPath parent() const;
        [[nodiscard]] std::optional<AssetPath> child(std::string_view segment,
                                                     AssetPathError* error = nullptr) const;
        [[nodiscard]] std::filesystem::path toFilesystemPath(const std::filesystem::path& contentRoot) const;

        friend bool operator==(const AssetPath&, const AssetPath&) = default;

    private:
        explicit AssetPath(std::string value) : value_(std::move(value)) {}
        std::string value_;
    };

    [[nodiscard]] std::string_view assetPathErrorText(AssetPathError error) noexcept;
}
