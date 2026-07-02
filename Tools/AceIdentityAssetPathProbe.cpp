#include "ArhqenCognitionEngine/Core/Assets/AceAssetPath.h"
#include "ArhqenCognitionEngine/Core/Identity/AceGuid.h"

#include <filesystem>
#include <iostream>
#include <string_view>
#include <unordered_set>

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
}

int main()
{
    using am::core::Guid;
    using am::core::GuidHash;
    using am::core::assets::AssetPath;
    using am::core::assets::AssetPathError;

    const Guid invalid;
    check(!invalid.isValid(), "zero_guid_is_invalid");

    const Guid generated = Guid::create();
    check(generated.isValid(), "generated_guid_is_valid");
    check((generated.bytes()[6] & 0xf0U) == 0x40U, "generated_guid_is_rfc_v4");
    check((generated.bytes()[8] & 0xc0U) == 0x80U, "generated_guid_has_rfc_variant");

    const std::string text = generated.toString();
    check(text.size() == 36 && text[8] == '-' && text[13] == '-' && text[18] == '-' && text[23] == '-',
          "guid_uses_canonical_hyphenated_format");
    const auto parsed = Guid::parse(text);
    check(parsed && *parsed == generated, "guid_round_trip");
    check(Guid::parse("12345678-8765-4321-1234-567887654321").has_value(), "guid_known_value_parses");
    check(!Guid::parse("12345678-8765-4321-1234-56788765432z"), "guid_rejects_non_hex");
    check(!Guid::parse("1234"), "guid_rejects_wrong_length");
    check(!Guid::parse("{12345678-8765-4321-1234-567887654321}"), "guid_parser_is_strict");

    std::unordered_set<Guid, GuidHash> unique;
    for (int index = 0; index < 4096; ++index)
    {
        unique.insert(Guid::create());
    }
    check(unique.size() == 4096, "guid_generation_smoke_has_no_collisions");

    AssetPathError error = AssetPathError::None;
    const auto root = AssetPath::parse("/Game", &error);
    check(root && root->isRoot() && error == AssetPathError::None, "game_root_is_valid");

    const auto material = AssetPath::parse("/Game/Materials/M_Rock.acemat", &error);
    check(material && material->leafName() == "M_Rock.acemat", "asset_path_and_leaf_parse");
    check(material && material->parent().string() == "/Game/Materials", "asset_parent");
    check(material && material->comparisonKey() == "/game/materials/m_rock.acemat", "asset_ascii_case_key");

    const auto unicode = AssetPath::parse("/Game/Materiale/Piatr\xC4\x83.acemat", &error);
    check(unicode && unicode->string() == "/Game/Materiale/Piatr\xC4\x83.acemat", "asset_utf8_name_supported");

    const auto unicodeUpper = AssetPath::parse("/Game/MATERIALE/PIATR\xC4\x82.ACEMAT", &error);
    check(unicode && unicodeUpper && unicode->comparisonKey() == unicodeUpper->comparisonKey(),
          "asset_unicode_case_key_is_stable");

    const auto decomposed = AssetPath::parse("/Game/Materiale/Piatra\xCC\x86.acemat", &error);
    check(decomposed && unicode && decomposed->comparisonKey() == unicode->comparisonKey(),
          "asset_unicode_is_normalized_to_nfc");

    const auto normalizedSlashes = AssetPath::parse("/Game\\Meshes\\SM_Box.acemesh", &error);
    check(normalizedSlashes && normalizedSlashes->string() == "/Game/Meshes/SM_Box.acemesh",
          "asset_backslashes_normalize_at_boundary");

    check(!AssetPath::parse("/Engine/Private", &error) && error == AssetPathError::WrongMount,
          "only_game_mount_is_visible");
    check(!AssetPath::parse("/Game/../Secrets", &error) && error == AssetPathError::Traversal,
          "asset_traversal_rejected");
    check(!AssetPath::parse("/Game//Broken", &error) && error == AssetPathError::EmptySegment,
          "empty_asset_segment_rejected");
    check(!AssetPath::parse("/Game/Bad?.asset", &error) && error == AssetPathError::InvalidCharacter,
          "windows_invalid_character_rejected");
    check(!AssetPath::parse("/Game/CON.txt", &error) && error == AssetPathError::ReservedName,
          "windows_reserved_name_rejected_with_extension");
    check(!AssetPath::parse("/Game/Trailing. ", &error) && error == AssetPathError::InvalidEnding,
          "trailing_space_rejected");

    const std::string invalidUtf8 = std::string("/Game/") + static_cast<char>(0xc0) + static_cast<char>(0xaf);
    check(!AssetPath::parse(invalidUtf8, &error) && error == AssetPathError::InvalidUtf8,
          "overlong_utf8_rejected");

    const auto child = root ? root->child("Meshes", &error) : std::nullopt;
    check(child && child->string() == "/Game/Meshes", "safe_child_path");
    check(root && !root->child("..", &error) && error == AssetPathError::Traversal, "unsafe_child_rejected");

    const std::filesystem::path content = std::filesystem::current_path() / "Content";
    const std::filesystem::path file = content / "Meshes" / "SM_Box.acemesh";
    const auto fromFile = AssetPath::fromFilesystemPath(content, file, &error);
    check(fromFile && fromFile->string() == "/Game/Meshes/SM_Box.acemesh", "filesystem_to_virtual_path");
    check(fromFile && fromFile->toFilesystemPath(content).lexically_normal() == file.lexically_normal(),
          "virtual_to_filesystem_round_trip");

    const std::filesystem::path outside = content.parent_path() / "Source" / "Private.cpp";
    check(!AssetPath::fromFilesystemPath(content, outside, &error) && error == AssetPathError::OutsideContentRoot,
          "filesystem_escape_rejected");

    check(am::core::assets::assetPathErrorText(AssetPathError::Traversal) == "Asset path traversal is not allowed",
          "stable_asset_path_error_text");

    if (Failures != 0)
    {
        std::cout << "FAIL|ace_identity_asset_path_probe|count=" << Failures << '\n';
        return 1;
    }
    std::cout << "PASS|ace_identity_asset_path_probe\n";
    return 0;
}
