# UE 5.7 Reference Map

UE root: `C:\Users\Sqweek\Documents\UE_5.7\Engine\Source`

Populate during implementation with:

- ACE subsystem/decision.
- UE files/modules inspected.
- Reusable architectural idea.
- What is intentionally omitted.
- ThirdParty dependency/licensing notes.
- ACE implementation files and tests.

## M1.1 — units and numeric editor fields

- `Runtime/Core/Public/Math/UnitConversion.h`: separate unit identity, physical dimension, compatibility and display settings.
- `Runtime/Core/Public/Math/UnitConversion.inl`: convert through a shared unit per dimension and quantize for readable presentation.
- `Runtime/Core/Private/Math/UnitConversion.cpp`: centralized aliases, symbols, scale factors, affine temperature conversion and display-unit ranges.
- `Runtime/Core/Tests/Math/UnitConversionTests.cpp`: table-driven conversion and parsing coverage.
- `Editor/DetailCustomizations/Public/Customizations/MathStructProxyCustomizations.h`: details fields attach a unit-aware numeric interface instead of embedding conversion rules in widgets.

ACE adaptation: canonical storage is strict SI (meters, radians, seconds, kilograms, kelvin, newtons and derived SI), unlike UE's historical centimeter world scale. Editor parsing/formatting is centralized in `Core/Units`; UI widgets will consume it without owning conversion policy.

## M1.2a — stable identity and mounted asset paths

- `Runtime/Core/Public/Misc/Guid.h` and `Runtime/Core/Tests/Misc/GuidTest.cpp`: GUID is a small value type with explicit validity, canonical parse/format and archive-friendly bytes.
- `Runtime/CoreUObject/Public/Misc/PackagePath.h`: package identity is a mounted virtual name distinct from a local filesystem path; validation and conversion happen at the mount boundary.

ACE adaptation: `Guid` persists independently from asset names, while `AssetPath` exposes only `/Game/...` and maps it to the confirmed `Content/` root. ACE additionally normalizes UTF-8 to NFC and creates a Unicode case-insensitive key because the current target filesystem is Windows.
