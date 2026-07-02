#include "ArhqenCognitionEngine/Core/Serialization/AceArchive.h"
#include "ArhqenCognitionEngine/Core/Serialization/AceAtomicFile.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace { int failures = 0; void check(bool v, std::string_view n) { std::cout << (v ? "PASS|" : "FAIL|") << n << '\n'; if (!v) ++failures; } }

int main()
{
    using namespace am::core;
    using namespace am::core::serialization;
    const Guid schema = *Guid::parse("4f87844c-cc10-4aaf-963f-3257963fc19d");
    const Guid object = Guid::create();
    ArchiveWriter writer(schema, 3);
    writer.writeU32(42); writer.writeI64(-99); writer.writeDouble(3.25); writer.writeBool(true); writer.writeGuid(object);
    check(writer.writeString("Material/Piatr\xC4\x83"), "writer_accepts_utf8");
    check(!writer.writeString(std::string("bad") + static_cast<char>(0xc0)), "writer_rejects_invalid_utf8");
    const auto bytes = writer.finish();

    ArchiveReader reader;
    check(reader.open(bytes, schema, 1, 3), "archive_opens_with_compatible_schema_version");
    check(reader.objectVersion() == 3, "archive_preserves_object_version");
    std::uint32_t u32 = 0; std::int64_t i64 = 0; double d = 0; bool b = false; Guid id; std::string text;
    check(reader.readU32(u32) && u32 == 42, "archive_u32_round_trip");
    check(reader.readI64(i64) && i64 == -99, "archive_i64_round_trip");
    check(reader.readDouble(d) && d == 3.25, "archive_double_round_trip");
    check(reader.readBool(b) && b, "archive_bool_round_trip");
    check(reader.readGuid(id) && id == object, "archive_guid_round_trip");
    check(reader.readString(text) && text == "Material/Piatr\xC4\x83", "archive_utf8_string_round_trip");
    check(reader.atEnd(), "archive_reader_reaches_exact_end");
    std::uint8_t finalByte = 0;
    check(!reader.readU8(finalByte) && reader.error() == ArchiveError::EndOfPayload, "archive_end_is_sticky_error");

    ArchiveReader wrong;
    check(!wrong.open(bytes, Guid::create(), 1, 3) && wrong.error() == ArchiveError::SchemaMismatch, "schema_mismatch_rejected");
    check(!wrong.open(bytes, schema, 4, 8) && wrong.error() == ArchiveError::ObjectVersionTooOld, "old_version_rejected");
    check(!wrong.open(bytes, schema, 1, 2) && wrong.error() == ArchiveError::ObjectVersionTooNew, "new_version_rejected");
    check(!wrong.open(bytes, schema, 1, 3, 1) && wrong.error() == ArchiveError::PayloadTooLarge, "payload_limit_enforced");

    auto corrupted = bytes; corrupted.back() ^= 0xffU;
    check(!wrong.open(corrupted, schema, 1, 3) && wrong.error() == ArchiveError::ChecksumMismatch, "payload_corruption_detected");
    auto truncated = bytes; truncated.pop_back();
    check(!wrong.open(truncated, schema, 1, 3) && wrong.error() == ArchiveError::Truncated, "truncation_detected");
    auto trailing = bytes; trailing.push_back(0);
    check(!wrong.open(trailing, schema, 1, 3) && wrong.error() == ArchiveError::Truncated, "trailing_bytes_detected");
    auto badMagic = bytes; badMagic[0] = 'X';
    check(!wrong.open(badMagic, schema, 1, 3) && wrong.error() == ArchiveError::InvalidMagic, "bad_magic_rejected");

    const auto dir = std::filesystem::current_path() / "Build" / "ACE-ARCHIVE" / "atomic";
    const auto file = dir / "test.acebin";
    std::string error; std::vector<std::uint8_t> loaded;
    check(AtomicFile::write(file, bytes, &error), "atomic_initial_write");
    check(AtomicFile::read(file, loaded, bytes.size(), &error) && loaded == bytes, "atomic_read_round_trip");
    check(AtomicFile::write(file, corrupted, &error), "atomic_replace_existing");
    check(AtomicFile::read(file, loaded, corrupted.size(), &error) && loaded == corrupted, "atomic_replacement_visible");
    check(!AtomicFile::read(file, loaded, 1, &error), "atomic_read_limit_enforced");
    std::size_t temporaryCount = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) if (entry.path().filename().wstring().find(L".tmp.") != std::wstring::npos) ++temporaryCount;
    check(temporaryCount == 0, "atomic_write_leaves_no_temporary_files");
    std::error_code ec; std::filesystem::remove_all(dir, ec);

    check(archiveErrorText(ArchiveError::ChecksumMismatch) == "Archive checksum does not match", "archive_error_text_stable");
    if (failures) { std::cout << "FAIL|ace_archive_probe|count=" << failures << '\n'; return 1; }
    std::cout << "PASS|ace_archive_probe\n"; return 0;
}
