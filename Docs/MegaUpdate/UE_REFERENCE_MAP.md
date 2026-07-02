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

## M1.2b — versioned serialization and atomic files

- `Runtime/Core/Public/Serialization/Archive.h`: serialization owns explicit loading/saving state, versions and bounded typed operations rather than raw struct dumps.
- `Runtime/Core/Tests/Serialization/CompactBinaryWriterTest.cpp`: round-trip and malformed-input tests are part of the format contract.
- `Runtime/Core/Tests/Misc/FileTest.cpp`: file behavior is tested independently from object serialization.

ACE adaptation: the initial archive is deliberately smaller than UE's archive stack but preserves the essential contract: fixed endianness, independent container/schema/object versions, bounded payloads, corruption detection and typed reads. Persistence uses a flushed temporary file followed by same-directory atomic replacement; higher asset layers must first resolve paths through the `/Game` sandbox.

## M1.3 — transactions

- `Editor/UnrealEd/Public/ScopedTransaction.h`: RAII defines a deliberate transaction boundary and supports cancellation.
- `Editor/UnrealEd/Private/EditorTransaction.cpp`: grouped records undo in reverse order, redo forward, and custom changes avoid unnecessary whole-object snapshots.

ACE adaptation: a compact operation-based transaction manager provides grouping, scoped commit/cancel, redo-branch invalidation and explicit memory/entry budgets. Property, transform, graph, asset and landscape layers will supply typed operations to this single manager.
