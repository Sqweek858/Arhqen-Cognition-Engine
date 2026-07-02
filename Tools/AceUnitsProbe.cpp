#include "ArhqenCognitionEngine/Core/Units/AceUnits.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

namespace
{
    int Failures = 0;

    void check(bool condition, std::string_view name)
    {
        if (condition)
        {
            std::cout << "PASS|" << name << '\n';
        }
        else
        {
            std::cout << "FAIL|" << name << '\n';
            ++Failures;
        }
    }

    bool close(double left, double right, double epsilon = 1.0e-9)
    {
        return std::abs(left - right) <= epsilon;
    }
}

int main()
{
    using namespace am::core::units;

    check(UnitConversion::canonicalUnit(Dimension::Length) == Unit::Meter, "canonical_length_meter");
    check(UnitConversion::canonicalUnit(Dimension::Angle) == Unit::Radian, "canonical_angle_radian");
    check(UnitConversion::canonicalUnit(Dimension::Mass) == Unit::Kilogram, "canonical_mass_kilogram");

    const auto inches = UnitConversion::convert(1.0, Unit::Meter, Unit::Inch);
    check(inches && close(*inches, 39.37007874015748, 1.0e-10), "meter_to_inches");
    const auto mph = UnitConversion::convert(10.0, Unit::MetersPerSecond, Unit::MilesPerHour);
    check(mph && close(*mph, 22.369362920544, 1.0e-9), "meters_per_second_to_mph");
    const auto force = UnitConversion::convert(100.0, Unit::Newton, Unit::PoundForce);
    check(force && close(*force, 22.480894309972, 1.0e-9), "newton_to_pound_force");
    const auto density = UnitConversion::convert(1.0, Unit::GramsPerCubicCentimeter, Unit::KilogramsPerCubicMeter);
    check(density && close(*density, 1000.0), "density_conversion");

    const auto freezingF = UnitConversion::convert(0.0, Unit::Celsius, Unit::Fahrenheit);
    const auto boilingK = UnitConversion::convert(212.0, Unit::Fahrenheit, Unit::Kelvin);
    check(freezingF && close(*freezingF, 32.0, 1.0e-10), "temperature_celsius_to_fahrenheit");
    check(boilingK && close(*boilingK, 373.15, 1.0e-10), "temperature_fahrenheit_to_kelvin");

    check(!UnitConversion::convert(1.0, Unit::Meter, Unit::Second), "incompatible_conversion_rejected");
    check(!UnitConversion::toCanonical(std::numeric_limits<double>::infinity(), Unit::Meter), "non_finite_conversion_rejected");

    const ParseResult centimeters = UnitConversion::parse(L" 250 cm ", Dimension::Length);
    check(centimeters.ok && close(centimeters.canonicalValue, 2.5) && centimeters.sourceUnit == Unit::Centimeter,
          "parse_explicit_metric_length");

    const ParseResult defaultMeters = UnitConversion::parse(L"-1.25e2", Dimension::Length, Unit::Meter);
    check(defaultMeters.ok && close(defaultMeters.canonicalValue, -125.0), "parse_default_unit_and_exponent");

    const ParseResult explicitPositive = UnitConversion::parse(L"+12.5 m", Dimension::Length);
    check(explicitPositive.ok && close(explicitPositive.canonicalValue, 12.5), "parse_explicit_positive_sign");

    const ParseResult unicodeUnits = UnitConversion::parse(L"90\u00B0", Dimension::Angle, Unit::Degree);
    check(!unicodeUnits.ok && unicodeUnits.error == ParseError::UnknownUnit, "bare_degree_symbol_is_not_silently_accepted");

    const ParseResult celsius = UnitConversion::parse(L"20 \u00B0C", Dimension::Temperature);
    check(celsius.ok && close(celsius.canonicalValue, 293.15), "parse_unicode_degree_celsius");

    const ParseResult square = UnitConversion::parse(L"12.5 m\u00B2", Dimension::Area);
    check(square.ok && close(square.canonicalValue, 12.5), "parse_superscript_area");

    const ParseResult missing = UnitConversion::parse(L"42", Dimension::Length);
    check(!missing.ok && missing.error == ParseError::MissingUnit, "missing_unit_rejected_without_default");

    const ParseResult incompatible = UnitConversion::parse(L"1 s", Dimension::Length);
    check(!incompatible.ok && incompatible.error == ParseError::IncompatibleUnit, "parse_incompatible_dimension_rejected");

    const ParseResult unknown = UnitConversion::parse(L"1 banana", Dimension::Length);
    check(!unknown.ok && unknown.error == ParseError::UnknownUnit, "unknown_unit_rejected");

    const ParseResult comma = UnitConversion::parse(L"1,5 m", Dimension::Length);
    check(!comma.ok, "locale_ambiguous_decimal_rejected");

    const ParseResult percent = UnitConversion::parse(L"50%", Dimension::Percentage);
    check(percent.ok && close(percent.canonicalValue, 0.5), "percent_parses_to_canonical_ratio");
    check(UnitConversion::format(0.5, Unit::Multiplier) == L"0.5 x", "canonical_ratio_formats_as_multiplier");

    check(UnitConversion::bestDisplayUnit(0.012, Dimension::Length) == Unit::Millimeter,
          "best_metric_length_millimeter");
    check(UnitConversion::bestDisplayUnit(1609.344, Dimension::Length, UnitSystem::Imperial) == Unit::Mile,
          "best_imperial_length_mile");
    check(UnitConversion::bestDisplayUnit(1048576.0, Dimension::DataSize) == Unit::Mebibyte,
          "best_binary_data_unit");

    check(UnitConversion::format(1.25, Unit::Centimeter) == L"125 cm", "format_trimmed_centimeters");
    check(UnitConversion::format(273.15, Unit::Celsius) == L"0 C", "format_affine_temperature");

    FormatOptions exact;
    exact.maxFractionDigits = 4;
    exact.trimTrailingZeros = false;
    exact.spaceBeforeUnit = false;
    check(UnitConversion::format(1.0, Unit::Meter, exact) == L"1.0000m", "format_options_respected");

    const ParseResult roundTrip = UnitConversion::parse(UnitConversion::format(3.14159, Unit::Foot), Dimension::Length);
    check(roundTrip.ok && close(roundTrip.canonicalValue, 3.14159, 0.0002), "format_parse_round_trip_with_display_precision");

    check(UnitConversion::parseErrorText(ParseError::UnknownUnit) == L"Unknown unit", "stable_parse_error_text");

    if (Failures != 0)
    {
        std::cout << "FAIL|ace_units_probe|count=" << Failures << '\n';
        return 1;
    }
    std::cout << "PASS|ace_units_probe\n";
    return 0;
}
