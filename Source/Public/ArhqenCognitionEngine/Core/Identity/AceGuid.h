#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace am::core
{
    class Guid final
    {
    public:
        using Bytes = std::array<std::uint8_t, 16>;

        constexpr Guid() noexcept = default;
        explicit constexpr Guid(Bytes bytes) noexcept : bytes_(bytes) {}

        [[nodiscard]] static Guid create();
        [[nodiscard]] static std::optional<Guid> parse(std::string_view text) noexcept;
        [[nodiscard]] static constexpr Guid fromBytes(Bytes bytes) noexcept { return Guid(bytes); }

        [[nodiscard]] constexpr const Bytes& bytes() const noexcept { return bytes_; }
        [[nodiscard]] constexpr bool isValid() const noexcept
        {
            for (const std::uint8_t byte : bytes_)
            {
                if (byte != 0)
                {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] std::string toString() const;

        friend constexpr bool operator==(const Guid&, const Guid&) noexcept = default;
        friend constexpr auto operator<=>(const Guid&, const Guid&) noexcept = default;

    private:
        Bytes bytes_{};
    };

    struct GuidHash
    {
        [[nodiscard]] std::size_t operator()(const Guid& guid) const noexcept;
    };
}
