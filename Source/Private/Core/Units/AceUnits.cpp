#include "ArhqenCognitionEngine/Core/Units/AceUnits.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cwctype>
#include <iomanip>
#include <iterator>
#include <limits>
#include <locale>
#include <sstream>
#include <string>

namespace am::core::units
{
    namespace
    {
        constexpr double Pi = 3.141592653589793238462643383279502884;

        constexpr std::array Definitions{
            Definition{Unit::Multiplier, Dimension::Percentage, L"x", L"x|multiplier|scalar|ratio", 1.0, 0.0},

            Definition{Unit::Micrometer, Dimension::Length, L"um", L"um|micrometer|micrometers|micrometre|micrometres", 1.0e-6, 0.0},
            Definition{Unit::Millimeter, Dimension::Length, L"mm", L"mm|millimeter|millimeters|millimetre|millimetres", 1.0e-3, 0.0},
            Definition{Unit::Centimeter, Dimension::Length, L"cm", L"cm|centimeter|centimeters|centimetre|centimetres", 1.0e-2, 0.0},
            Definition{Unit::Decimeter, Dimension::Length, L"dm", L"dm|decimeter|decimeters|decimetre|decimetres", 1.0e-1, 0.0},
            Definition{Unit::Meter, Dimension::Length, L"m", L"m|meter|meters|metre|metres", 1.0, 0.0},
            Definition{Unit::Kilometer, Dimension::Length, L"km", L"km|kilometer|kilometers|kilometre|kilometres", 1.0e3, 0.0},
            Definition{Unit::Inch, Dimension::Length, L"in", L"in|inch|inches|\"", 0.0254, 0.0},
            Definition{Unit::Foot, Dimension::Length, L"ft", L"ft|foot|feet|'", 0.3048, 0.0},
            Definition{Unit::Yard, Dimension::Length, L"yd", L"yd|yard|yards", 0.9144, 0.0},
            Definition{Unit::Mile, Dimension::Length, L"mi", L"mi|mile|miles", 1609.344, 0.0},

            Definition{Unit::SquareMillimeter, Dimension::Area, L"mm2", L"mm2|squaremillimeter|squaremillimeters", 1.0e-6, 0.0},
            Definition{Unit::SquareCentimeter, Dimension::Area, L"cm2", L"cm2|squarecentimeter|squarecentimeters", 1.0e-4, 0.0},
            Definition{Unit::SquareMeter, Dimension::Area, L"m2", L"m2|squaremeter|squaremeters", 1.0, 0.0},
            Definition{Unit::SquareKilometer, Dimension::Area, L"km2", L"km2|squarekilometer|squarekilometers", 1.0e6, 0.0},
            Definition{Unit::SquareInch, Dimension::Area, L"in2", L"in2|squareinch|squareinches", 0.00064516, 0.0},
            Definition{Unit::SquareFoot, Dimension::Area, L"ft2", L"ft2|squarefoot|squarefeet", 0.09290304, 0.0},
            Definition{Unit::Acre, Dimension::Area, L"acre", L"acre|acres", 4046.8564224, 0.0},
            Definition{Unit::Hectare, Dimension::Area, L"ha", L"ha|hectare|hectares", 10000.0, 0.0},

            Definition{Unit::CubicCentimeter, Dimension::Volume, L"cm3", L"cm3|cubiccentimeter|cubiccentimeters|cc", 1.0e-6, 0.0},
            Definition{Unit::CubicMeter, Dimension::Volume, L"m3", L"m3|cubicmeter|cubicmeters", 1.0, 0.0},
            Definition{Unit::Milliliter, Dimension::Volume, L"mL", L"ml|milliliter|milliliters|millilitre|millilitres", 1.0e-6, 0.0},
            Definition{Unit::Liter, Dimension::Volume, L"L", L"l|liter|liters|litre|litres", 1.0e-3, 0.0},
            Definition{Unit::CubicInch, Dimension::Volume, L"in3", L"in3|cubicinch|cubicinches", 1.6387064e-5, 0.0},
            Definition{Unit::CubicFoot, Dimension::Volume, L"ft3", L"ft3|cubicfoot|cubicfeet", 0.028316846592, 0.0},
            Definition{Unit::UsGallon, Dimension::Volume, L"gal", L"gal|gallon|gallons|usgallon|usgallons", 0.003785411784, 0.0},

            Definition{Unit::Radian, Dimension::Angle, L"rad", L"rad|radian|radians", 1.0, 0.0},
            Definition{Unit::Degree, Dimension::Angle, L"deg", L"deg|degree|degrees", Pi / 180.0, 0.0},

            Definition{Unit::Nanosecond, Dimension::Time, L"ns", L"ns|nanosecond|nanoseconds", 1.0e-9, 0.0},
            Definition{Unit::Microsecond, Dimension::Time, L"us", L"us|microsecond|microseconds", 1.0e-6, 0.0},
            Definition{Unit::Millisecond, Dimension::Time, L"ms", L"ms|millisecond|milliseconds", 1.0e-3, 0.0},
            Definition{Unit::Second, Dimension::Time, L"s", L"s|sec|second|seconds", 1.0, 0.0},
            Definition{Unit::Minute, Dimension::Time, L"min", L"min|minute|minutes", 60.0, 0.0},
            Definition{Unit::Hour, Dimension::Time, L"h", L"h|hr|hour|hours", 3600.0, 0.0},
            Definition{Unit::Day, Dimension::Time, L"d", L"d|day|days", 86400.0, 0.0},

            Definition{Unit::CentimetersPerSecond, Dimension::Speed, L"cm/s", L"cm/s|cmps|centimeterspersecond", 0.01, 0.0},
            Definition{Unit::MetersPerSecond, Dimension::Speed, L"m/s", L"m/s|mps|meterspersecond", 1.0, 0.0},
            Definition{Unit::KilometersPerHour, Dimension::Speed, L"km/h", L"km/h|kph|kmh|kilometersperhour", 1.0 / 3.6, 0.0},
            Definition{Unit::FeetPerSecond, Dimension::Speed, L"ft/s", L"ft/s|fps|feetpersecond", 0.3048, 0.0},
            Definition{Unit::MilesPerHour, Dimension::Speed, L"mph", L"mph|milesperhour", 0.44704, 0.0},

            Definition{Unit::RadiansPerSecond, Dimension::AngularSpeed, L"rad/s", L"rad/s|rps|radianspersecond", 1.0, 0.0},
            Definition{Unit::DegreesPerSecond, Dimension::AngularSpeed, L"deg/s", L"deg/s|dps|degreespersecond", Pi / 180.0, 0.0},

            Definition{Unit::CentimetersPerSecondSquared, Dimension::Acceleration, L"cm/s2", L"cm/s2|cmps2|centimeterspersecondsquared", 0.01, 0.0},
            Definition{Unit::MetersPerSecondSquared, Dimension::Acceleration, L"m/s2", L"m/s2|mps2|meterspersecondsquared", 1.0, 0.0},
            Definition{Unit::StandardGravity, Dimension::Acceleration, L"g0", L"g0|standardgravity|gee", 9.80665, 0.0},

            Definition{Unit::Milligram, Dimension::Mass, L"mg", L"mg|milligram|milligrams", 1.0e-6, 0.0},
            Definition{Unit::Gram, Dimension::Mass, L"g", L"g|gram|grams", 1.0e-3, 0.0},
            Definition{Unit::Kilogram, Dimension::Mass, L"kg", L"kg|kilogram|kilograms", 1.0, 0.0},
            Definition{Unit::MetricTonne, Dimension::Mass, L"t", L"t|tonne|tonnes|metricton|metrictons", 1000.0, 0.0},
            Definition{Unit::Ounce, Dimension::Mass, L"oz", L"oz|ounce|ounces", 0.028349523125, 0.0},
            Definition{Unit::Pound, Dimension::Mass, L"lb", L"lb|lbs|pound|pounds", 0.45359237, 0.0},
            Definition{Unit::Stone, Dimension::Mass, L"st", L"st|stone|stones", 6.35029318, 0.0},

            Definition{Unit::KilogramsPerCubicMeter, Dimension::Density, L"kg/m3", L"kg/m3|kgpm3|kilogramspercubicmeter", 1.0, 0.0},
            Definition{Unit::GramsPerCubicCentimeter, Dimension::Density, L"g/cm3", L"g/cm3|gpc3|gramspercubiccentimeter", 1000.0, 0.0},

            Definition{Unit::Newton, Dimension::Force, L"N", L"n|newton|newtons", 1.0, 0.0},
            Definition{Unit::Kilonewton, Dimension::Force, L"kN", L"kn|kilonewton|kilonewtons", 1000.0, 0.0},
            Definition{Unit::PoundForce, Dimension::Force, L"lbf", L"lbf|poundforce|poundsforce", 4.4482216152605, 0.0},
            Definition{Unit::NewtonMeter, Dimension::Torque, L"N*m", L"n*m|nm|newtonmeter|newtonmeters", 1.0, 0.0},
            Definition{Unit::PoundFoot, Dimension::Torque, L"lb*ft", L"lb*ft|lbft|poundfoot|poundfeet", 1.3558179483314, 0.0},
            Definition{Unit::NewtonSecond, Dimension::Impulse, L"N*s", L"n*s|nsimpulse|newtonsecond|newtonseconds", 1.0, 0.0},

            Definition{Unit::Kelvin, Dimension::Temperature, L"K", L"k|kelvin", 1.0, 0.0},
            Definition{Unit::Celsius, Dimension::Temperature, L"C", L"c|celsius", 1.0, 273.15},
            Definition{Unit::Fahrenheit, Dimension::Temperature, L"F", L"f|fahrenheit", 5.0 / 9.0, 255.3722222222222},

            Definition{Unit::Hertz, Dimension::Frequency, L"Hz", L"hz|hertz", 1.0, 0.0},
            Definition{Unit::Kilohertz, Dimension::Frequency, L"kHz", L"khz|kilohertz", 1.0e3, 0.0},
            Definition{Unit::Megahertz, Dimension::Frequency, L"MHz", L"mhz|megahertz", 1.0e6, 0.0},
            Definition{Unit::Gigahertz, Dimension::Frequency, L"GHz", L"ghz|gigahertz", 1.0e9, 0.0},
            Definition{Unit::RevolutionsPerMinute, Dimension::Frequency, L"rpm", L"rpm|revolutionsperminute", 1.0 / 60.0, 0.0},

            Definition{Unit::Pascal, Dimension::Pressure, L"Pa", L"pa|pascal|pascals", 1.0, 0.0},
            Definition{Unit::Kilopascal, Dimension::Pressure, L"kPa", L"kpa|kilopascal|kilopascals", 1.0e3, 0.0},
            Definition{Unit::Megapascal, Dimension::Pressure, L"MPa", L"mpa|megapascal|megapascals", 1.0e6, 0.0},
            Definition{Unit::Gigapascal, Dimension::Pressure, L"GPa", L"gpa|gigapascal|gigapascals", 1.0e9, 0.0},
            Definition{Unit::Bar, Dimension::Pressure, L"bar", L"bar|bars", 1.0e5, 0.0},
            Definition{Unit::Psi, Dimension::Pressure, L"psi", L"psi|poundspersquareinch", 6894.757293168, 0.0},

            Definition{Unit::Byte, Dimension::DataSize, L"B", L"b|byte|bytes", 1.0, 0.0},
            Definition{Unit::Kibibyte, Dimension::DataSize, L"KiB", L"kib|kb|kibibyte|kibibytes|kilobyte|kilobytes", 1024.0, 0.0},
            Definition{Unit::Mebibyte, Dimension::DataSize, L"MiB", L"mib|mb|mebibyte|mebibytes|megabyte|megabytes", 1048576.0, 0.0},
            Definition{Unit::Gibibyte, Dimension::DataSize, L"GiB", L"gib|gb|gibibyte|gibibytes|gigabyte|gigabytes", 1073741824.0, 0.0},
            Definition{Unit::Tebibyte, Dimension::DataSize, L"TiB", L"tib|tb|tebibyte|tebibytes|terabyte|terabytes", 1099511627776.0, 0.0},

            Definition{Unit::Lumen, Dimension::LuminousFlux, L"lm", L"lm|lumen|lumens", 1.0, 0.0},
            Definition{Unit::Candela, Dimension::LuminousIntensity, L"cd", L"cd|candela|candelas", 1.0, 0.0},
            Definition{Unit::Lux, Dimension::Illuminance, L"lx", L"lx|lux", 1.0, 0.0},
            Definition{Unit::Nit, Dimension::Luminance, L"nit", L"nit|nits|cd/m2|candelapersquaremeter", 1.0, 0.0},

            Definition{Unit::Percent, Dimension::Percentage, L"%", L"%|percent|percentage", 0.01, 0.0}
        };

        [[nodiscard]] std::wstring_view trim(std::wstring_view value) noexcept
        {
            while (!value.empty() && std::iswspace(value.front()) != 0)
            {
                value.remove_prefix(1);
            }
            while (!value.empty() && std::iswspace(value.back()) != 0)
            {
                value.remove_suffix(1);
            }
            return value;
        }

        [[nodiscard]] std::wstring normalizeUnitToken(std::wstring_view token)
        {
            std::wstring normalized;
            normalized.reserve(token.size());
            for (wchar_t ch : trim(token))
            {
                if (std::iswspace(ch) != 0 || ch == L'_')
                {
                    continue;
                }
                if (ch == L'\u00B0')
                {
                    continue;
                }
                if (ch == L'\u00B2')
                {
                    normalized.push_back(L'2');
                    continue;
                }
                if (ch == L'\u00B3')
                {
                    normalized.push_back(L'3');
                    continue;
                }
                if (ch == L'\u00B5' || ch == L'\u03BC')
                {
                    normalized.push_back(L'u');
                    continue;
                }
                if (ch == L'\u00B7')
                {
                    normalized.push_back(L'*');
                    continue;
                }
                normalized.push_back(static_cast<wchar_t>(std::towlower(ch)));
            }
            return normalized;
        }

        [[nodiscard]] bool aliasContains(std::wstring_view aliases, std::wstring_view normalizedToken)
        {
            std::size_t start = 0;
            while (start <= aliases.size())
            {
                const std::size_t separator = aliases.find(L'|', start);
                const std::size_t end = separator == std::wstring_view::npos ? aliases.size() : separator;
                if (normalizeUnitToken(aliases.substr(start, end - start)) == normalizedToken)
                {
                    return true;
                }
                if (separator == std::wstring_view::npos)
                {
                    break;
                }
                start = separator + 1;
            }
            return false;
        }

        [[nodiscard]] std::optional<double> parseNumber(std::wstring_view text, std::size_t& consumed) noexcept
        {
            std::string ascii;
            ascii.reserve(text.size());
            for (wchar_t ch : text)
            {
                if (ch > 127)
                {
                    break;
                }
                ascii.push_back(static_cast<char>(ch));
            }

            double value = 0.0;
            const char* begin = ascii.data();
            const char* end = begin + ascii.size();
            const bool explicitPositive = begin != end && *begin == '+';
            const char* parseBegin = explicitPositive ? begin + 1 : begin;
            const auto result = std::from_chars(parseBegin, end, value, std::chars_format::general);
            consumed = static_cast<std::size_t>(result.ptr - begin);
            if (result.ec != std::errc{} || consumed == 0)
            {
                return std::nullopt;
            }
            return value;
        }

        [[nodiscard]] Unit selectBest(double canonicalValue, const Unit* units, std::size_t count) noexcept
        {
            if (count == 0 || !std::isfinite(canonicalValue))
            {
                return Unit::Unspecified;
            }
            if (canonicalValue == 0.0)
            {
                return units[0];
            }

            Unit best = units[0];
            double bestScore = std::numeric_limits<double>::infinity();
            for (std::size_t index = 0; index < count; ++index)
            {
                const auto converted = UnitConversion::fromCanonical(canonicalValue, units[index]);
                if (!converted || *converted == 0.0)
                {
                    continue;
                }
                const double magnitude = std::abs(*converted);
                const double score = magnitude >= 1.0 && magnitude < 1000.0
                    ? std::abs(std::log10(magnitude) - 1.0)
                    : 100.0 + std::abs(std::log10(magnitude));
                if (score < bestScore)
                {
                    bestScore = score;
                    best = units[index];
                }
            }
            return best;
        }
    }

    const Definition* UnitConversion::definition(Unit unit) noexcept
    {
        const auto found = std::find_if(Definitions.begin(), Definitions.end(),
            [unit](const Definition& candidate) { return candidate.unit == unit; });
        return found == Definitions.end() ? nullptr : &*found;
    }

    Unit UnitConversion::canonicalUnit(Dimension dimension) noexcept
    {
        switch (dimension)
        {
        case Dimension::Scalar: return Unit::Unspecified;
        case Dimension::Length: return Unit::Meter;
        case Dimension::Area: return Unit::SquareMeter;
        case Dimension::Volume: return Unit::CubicMeter;
        case Dimension::Angle: return Unit::Radian;
        case Dimension::Time: return Unit::Second;
        case Dimension::Speed: return Unit::MetersPerSecond;
        case Dimension::AngularSpeed: return Unit::RadiansPerSecond;
        case Dimension::Acceleration: return Unit::MetersPerSecondSquared;
        case Dimension::Mass: return Unit::Kilogram;
        case Dimension::Density: return Unit::KilogramsPerCubicMeter;
        case Dimension::Force: return Unit::Newton;
        case Dimension::Torque: return Unit::NewtonMeter;
        case Dimension::Impulse: return Unit::NewtonSecond;
        case Dimension::Temperature: return Unit::Kelvin;
        case Dimension::Frequency: return Unit::Hertz;
        case Dimension::Pressure: return Unit::Pascal;
        case Dimension::DataSize: return Unit::Byte;
        case Dimension::LuminousFlux: return Unit::Lumen;
        case Dimension::LuminousIntensity: return Unit::Candela;
        case Dimension::Illuminance: return Unit::Lux;
        case Dimension::Luminance: return Unit::Nit;
        case Dimension::Percentage: return Unit::Multiplier;
        }
        return Unit::Unspecified;
    }

    bool UnitConversion::compatible(Unit from, Unit to) noexcept
    {
        const Definition* fromDefinition = definition(from);
        const Definition* toDefinition = definition(to);
        return fromDefinition && toDefinition && fromDefinition->dimension == toDefinition->dimension;
    }

    std::optional<double> UnitConversion::toCanonical(double value, Unit from) noexcept
    {
        const Definition* source = definition(from);
        if (!source || !std::isfinite(value))
        {
            return std::nullopt;
        }
        const double converted = value * source->scaleToCanonical + source->offsetToCanonical;
        return std::isfinite(converted) ? std::optional<double>{converted} : std::nullopt;
    }

    std::optional<double> UnitConversion::fromCanonical(double canonicalValue, Unit to) noexcept
    {
        const Definition* destination = definition(to);
        if (!destination || !std::isfinite(canonicalValue) || destination->scaleToCanonical == 0.0)
        {
            return std::nullopt;
        }
        const double converted = (canonicalValue - destination->offsetToCanonical) / destination->scaleToCanonical;
        return std::isfinite(converted) ? std::optional<double>{converted} : std::nullopt;
    }

    std::optional<double> UnitConversion::convert(double value, Unit from, Unit to) noexcept
    {
        if (!compatible(from, to))
        {
            return std::nullopt;
        }
        const auto canonical = toCanonical(value, from);
        return canonical ? fromCanonical(*canonical, to) : std::nullopt;
    }

    std::optional<Unit> UnitConversion::findUnit(std::wstring_view token,
                                                  std::optional<Dimension> expectedDimension) noexcept
    {
        const std::wstring normalized = normalizeUnitToken(token);
        if (normalized.empty())
        {
            return std::nullopt;
        }

        for (const Definition& candidate : Definitions)
        {
            if (expectedDimension && candidate.dimension != *expectedDimension)
            {
                continue;
            }
            if (normalizeUnitToken(candidate.symbol) == normalized || aliasContains(candidate.aliases, normalized))
            {
                return candidate.unit;
            }
        }
        return std::nullopt;
    }

    ParseResult UnitConversion::parse(std::wstring_view text,
                                      Dimension expectedDimension,
                                      Unit defaultUnit) noexcept
    {
        ParseResult result;
        const std::wstring_view source = trim(text);
        if (source.empty())
        {
            result.error = ParseError::Empty;
            return result;
        }

        std::size_t numberLength = 0;
        const auto value = parseNumber(source, numberLength);
        if (!value)
        {
            result.error = ParseError::InvalidNumber;
            return result;
        }
        if (!std::isfinite(*value))
        {
            result.error = ParseError::NonFinite;
            return result;
        }

        const std::wstring_view suffix = trim(source.substr(numberLength));
        Unit parsedUnit = defaultUnit;
        if (!suffix.empty())
        {
            const auto anyUnit = findUnit(suffix);
            if (!anyUnit)
            {
                result.error = ParseError::UnknownUnit;
                result.errorOffset = numberLength;
                return result;
            }
            parsedUnit = *anyUnit;
        }
        else if (parsedUnit == Unit::Unspecified)
        {
            result.error = ParseError::MissingUnit;
            result.errorOffset = numberLength;
            return result;
        }

        const Definition* parsedDefinition = definition(parsedUnit);
        if (!parsedDefinition || parsedDefinition->dimension != expectedDimension)
        {
            result.error = ParseError::IncompatibleUnit;
            result.errorOffset = numberLength;
            return result;
        }

        const auto canonical = toCanonical(*value, parsedUnit);
        if (!canonical)
        {
            result.error = ParseError::NonFinite;
            result.errorOffset = numberLength;
            return result;
        }

        result.ok = true;
        result.canonicalValue = *canonical;
        result.sourceUnit = parsedUnit;
        return result;
    }

    Unit UnitConversion::bestDisplayUnit(double canonicalValue,
                                         Dimension dimension,
                                         UnitSystem system) noexcept
    {
        static constexpr Unit MetricLength[]{Unit::Millimeter, Unit::Centimeter, Unit::Meter, Unit::Kilometer};
        static constexpr Unit ImperialLength[]{Unit::Inch, Unit::Foot, Unit::Yard, Unit::Mile};
        static constexpr Unit MetricArea[]{Unit::SquareMillimeter, Unit::SquareCentimeter, Unit::SquareMeter, Unit::Hectare, Unit::SquareKilometer};
        static constexpr Unit ImperialArea[]{Unit::SquareInch, Unit::SquareFoot, Unit::Acre};
        static constexpr Unit MetricVolume[]{Unit::Milliliter, Unit::Liter, Unit::CubicMeter};
        static constexpr Unit ImperialVolume[]{Unit::CubicInch, Unit::CubicFoot, Unit::UsGallon};
        static constexpr Unit MetricMass[]{Unit::Milligram, Unit::Gram, Unit::Kilogram, Unit::MetricTonne};
        static constexpr Unit ImperialMass[]{Unit::Ounce, Unit::Pound, Unit::Stone};
        static constexpr Unit TimeUnits[]{Unit::Nanosecond, Unit::Microsecond, Unit::Millisecond, Unit::Second, Unit::Minute, Unit::Hour, Unit::Day};
        static constexpr Unit FrequencyUnits[]{Unit::Hertz, Unit::Kilohertz, Unit::Megahertz, Unit::Gigahertz};
        static constexpr Unit DataUnits[]{Unit::Byte, Unit::Kibibyte, Unit::Mebibyte, Unit::Gibibyte, Unit::Tebibyte};
        static constexpr Unit PressureUnits[]{Unit::Pascal, Unit::Kilopascal, Unit::Megapascal, Unit::Gigapascal};

        switch (dimension)
        {
        case Dimension::Length:
            return system == UnitSystem::Metric
                ? selectBest(canonicalValue, MetricLength, std::size(MetricLength))
                : selectBest(canonicalValue, ImperialLength, std::size(ImperialLength));
        case Dimension::Area:
            return system == UnitSystem::Metric
                ? selectBest(canonicalValue, MetricArea, std::size(MetricArea))
                : selectBest(canonicalValue, ImperialArea, std::size(ImperialArea));
        case Dimension::Volume:
            return system == UnitSystem::Metric
                ? selectBest(canonicalValue, MetricVolume, std::size(MetricVolume))
                : selectBest(canonicalValue, ImperialVolume, std::size(ImperialVolume));
        case Dimension::Mass:
            return system == UnitSystem::Metric
                ? selectBest(canonicalValue, MetricMass, std::size(MetricMass))
                : selectBest(canonicalValue, ImperialMass, std::size(ImperialMass));
        case Dimension::Time: return selectBest(canonicalValue, TimeUnits, std::size(TimeUnits));
        case Dimension::Frequency: return selectBest(canonicalValue, FrequencyUnits, std::size(FrequencyUnits));
        case Dimension::DataSize: return selectBest(canonicalValue, DataUnits, std::size(DataUnits));
        case Dimension::Pressure: return selectBest(canonicalValue, PressureUnits, std::size(PressureUnits));
        default: return canonicalUnit(dimension);
        }
    }

    std::wstring UnitConversion::format(double canonicalValue,
                                        Unit displayUnit,
                                        const FormatOptions& options)
    {
        const auto converted = fromCanonical(canonicalValue, displayUnit);
        if (!converted)
        {
            return {};
        }

        const int precision = std::clamp(options.maxFractionDigits, 0, 15);
        std::wostringstream stream;
        stream.imbue(std::locale::classic());
        stream << std::fixed << std::setprecision(precision) << (*converted == 0.0 ? 0.0 : *converted);
        std::wstring output = stream.str();

        if (options.trimTrailingZeros && precision > 0)
        {
            const std::size_t decimal = output.find(L'.');
            if (decimal != std::wstring::npos)
            {
                while (!output.empty() && output.back() == L'0')
                {
                    output.pop_back();
                }
                if (!output.empty() && output.back() == L'.')
                {
                    output.pop_back();
                }
            }
        }

        if (options.includeUnit)
        {
            const std::wstring_view unitSymbol = symbol(displayUnit);
            if (!unitSymbol.empty())
            {
                if (options.spaceBeforeUnit)
                {
                    output.push_back(L' ');
                }
                output.append(unitSymbol);
            }
        }
        return output;
    }

    std::wstring_view UnitConversion::symbol(Unit unit) noexcept
    {
        const Definition* found = definition(unit);
        return found ? found->symbol : std::wstring_view{};
    }

    std::wstring_view UnitConversion::parseErrorText(ParseError error) noexcept
    {
        switch (error)
        {
        case ParseError::None: return L"No error";
        case ParseError::Empty: return L"Enter a value";
        case ParseError::InvalidNumber: return L"Invalid number";
        case ParseError::NonFinite: return L"Value must be finite";
        case ParseError::MissingUnit: return L"A unit is required";
        case ParseError::UnknownUnit: return L"Unknown unit";
        case ParseError::IncompatibleUnit: return L"Unit has the wrong physical dimension";
        case ParseError::TrailingCharacters: return L"Unexpected trailing characters";
        }
        return L"Invalid value";
    }
}
