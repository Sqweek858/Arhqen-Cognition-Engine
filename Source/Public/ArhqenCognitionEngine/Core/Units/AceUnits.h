#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace am::core::units
{
    enum class Dimension
    {
        Scalar,
        Length,
        Area,
        Volume,
        Angle,
        Time,
        Speed,
        AngularSpeed,
        Acceleration,
        Mass,
        Density,
        Force,
        Torque,
        Impulse,
        Temperature,
        Frequency,
        Pressure,
        DataSize,
        LuminousFlux,
        LuminousIntensity,
        Illuminance,
        Luminance,
        Percentage
    };

    enum class Unit
    {
        Unspecified,
        Multiplier,

        Micrometer, Millimeter, Centimeter, Decimeter, Meter, Kilometer,
        Inch, Foot, Yard, Mile,

        SquareMillimeter, SquareCentimeter, SquareMeter, SquareKilometer,
        SquareInch, SquareFoot, Acre, Hectare,

        CubicCentimeter, CubicMeter, Milliliter, Liter,
        CubicInch, CubicFoot, UsGallon,

        Radian, Degree,

        Nanosecond, Microsecond, Millisecond, Second, Minute, Hour, Day,

        CentimetersPerSecond, MetersPerSecond, KilometersPerHour,
        FeetPerSecond, MilesPerHour,

        RadiansPerSecond, DegreesPerSecond,

        CentimetersPerSecondSquared, MetersPerSecondSquared, StandardGravity,

        Milligram, Gram, Kilogram, MetricTonne, Ounce, Pound, Stone,

        KilogramsPerCubicMeter, GramsPerCubicCentimeter,

        Newton, Kilonewton, PoundForce,
        NewtonMeter, PoundFoot,
        NewtonSecond,

        Kelvin, Celsius, Fahrenheit,

        Hertz, Kilohertz, Megahertz, Gigahertz, RevolutionsPerMinute,

        Pascal, Kilopascal, Megapascal, Gigapascal, Bar, Psi,

        Byte, Kibibyte, Mebibyte, Gibibyte, Tebibyte,

        Lumen, Candela, Lux, Nit,

        Percent
    };

    enum class UnitSystem
    {
        Metric,
        Imperial
    };

    struct Definition
    {
        Unit unit = Unit::Unspecified;
        Dimension dimension = Dimension::Scalar;
        std::wstring_view symbol;
        std::wstring_view aliases;
        double scaleToCanonical = 1.0;
        double offsetToCanonical = 0.0;
    };

    enum class ParseError
    {
        None,
        Empty,
        InvalidNumber,
        NonFinite,
        MissingUnit,
        UnknownUnit,
        IncompatibleUnit,
        TrailingCharacters
    };

    struct ParseResult
    {
        bool ok = false;
        double canonicalValue = 0.0;
        Unit sourceUnit = Unit::Unspecified;
        ParseError error = ParseError::None;
        std::size_t errorOffset = 0;
    };

    struct FormatOptions
    {
        int maxFractionDigits = 3;
        bool trimTrailingZeros = true;
        bool includeUnit = true;
        bool spaceBeforeUnit = true;
    };

    class UnitConversion final
    {
    public:
        [[nodiscard]] static const Definition* definition(Unit unit) noexcept;
        [[nodiscard]] static Unit canonicalUnit(Dimension dimension) noexcept;
        [[nodiscard]] static bool compatible(Unit from, Unit to) noexcept;
        [[nodiscard]] static std::optional<double> toCanonical(double value, Unit from) noexcept;
        [[nodiscard]] static std::optional<double> fromCanonical(double canonicalValue, Unit to) noexcept;
        [[nodiscard]] static std::optional<double> convert(double value, Unit from, Unit to) noexcept;
        [[nodiscard]] static std::optional<Unit> findUnit(std::wstring_view token,
                                                          std::optional<Dimension> expectedDimension = std::nullopt) noexcept;
        [[nodiscard]] static ParseResult parse(std::wstring_view text,
                                               Dimension expectedDimension,
                                               Unit defaultUnit = Unit::Unspecified) noexcept;
        [[nodiscard]] static Unit bestDisplayUnit(double canonicalValue,
                                                  Dimension dimension,
                                                  UnitSystem system = UnitSystem::Metric) noexcept;
        [[nodiscard]] static std::wstring format(double canonicalValue,
                                                 Unit displayUnit,
                                                 const FormatOptions& options = {});
        [[nodiscard]] static std::wstring_view symbol(Unit unit) noexcept;
        [[nodiscard]] static std::wstring_view parseErrorText(ParseError error) noexcept;
    };
}
