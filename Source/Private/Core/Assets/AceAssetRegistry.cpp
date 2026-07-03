#include "ArhqenCognitionEngine/Core/Assets/AceAssetRegistry.h"

#include "ArhqenCognitionEngine/Core/Serialization/AceArchive.h"
#include "ArhqenCognitionEngine/Core/Serialization/AceAtomicFile.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <system_error>
#include <unordered_set>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace am::core::assets
{
    namespace
    {
        using am::core::serialization::ArchiveReader;
        using am::core::serialization::ArchiveWriter;
        using am::core::serialization::AtomicFile;

        constexpr std::uint32_t kRegistryVersion = 1;
        constexpr std::size_t kMaximumRegistryBytes = 64ull * 1024ull * 1024ull;
        constexpr std::uint32_t kMaximumAssets = 250000;
        constexpr Guid kRegistrySchema = Guid::fromBytes({
            0x62, 0x4f, 0x2c, 0xa1, 0x0f, 0x8d, 0x46, 0xf2,
            0xa2, 0x4a, 0x1d, 0xe4, 0x77, 0x91, 0x33, 0xb8});

        void fail(std::string* error, std::string message)
        {
            if (error) *error = std::move(message);
        }

        std::string lower(std::string_view value)
        {
            std::string result(value);
            std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });
            return result;
        }

        std::string extensionUtf8(const std::filesystem::path& path)
        {
#ifdef _WIN32
            const std::wstring wide = path.extension().wstring();
            if (wide.empty()) return {};
            const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(),
                static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
            if (size <= 0) return {};
            std::string utf8(static_cast<std::size_t>(size), '\0');
            if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide.data(),
                static_cast<int>(wide.size()), utf8.data(), size, nullptr, nullptr) != size) return {};
            return lower(utf8);
#else
            return lower(path.extension().string());
#endif
        }

        bool hiddenOrInternal(const std::filesystem::path& path)
        {
            const auto name = path.filename().wstring();
            if (name.empty() || name.front() == L'.' || name == L"desktop.ini" || name == L"Thumbs.db")
                return true;
#ifdef _WIN32
            const DWORD attributes = GetFileAttributesW(path.c_str());
            return attributes != INVALID_FILE_ATTRIBUTES && (attributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) != 0;
#else
            return false;
#endif
        }

        bool isWithin(const std::filesystem::path& root, const std::filesystem::path& candidate)
        {
            const auto relative = candidate.lexically_relative(root);
            if (relative.empty()) return candidate == root;
            for (const auto& part : relative) if (part == "..") return false;
            return true;
        }
    }

    std::optional<AssetType> AssetRegistry::typeFromExtension(std::string_view extension) noexcept
    {
        const std::string ext = lower(extension);
        if (ext == ".acematerial" || ext == ".acemat") return AssetType::Material;
        if (ext == ".acematerialfunction" || ext == ".acematfn") return AssetType::MaterialFunction;
        if (ext == ".acemesh") return AssetType::StaticMesh;
        if (ext == ".fbx" || ext == ".obj" || ext == ".gltf" || ext == ".glb") return AssetType::MeshSource;
        if (ext == ".acetexture" || ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
            ext == ".tga" || ext == ".dds" || ext == ".hdr" || ext == ".exr") return AssetType::Texture;
        if (ext == ".acescene") return AssetType::Scene;
        if (ext == ".acelandscape") return AssetType::Landscape;
        if (ext == ".acephysmat") return AssetType::PhysicsMaterial;
        return std::nullopt;
    }

    std::string_view AssetRegistry::typeName(AssetType type) noexcept
    {
        switch (type)
        {
        case AssetType::Material: return "Material";
        case AssetType::MaterialFunction: return "Material Function";
        case AssetType::StaticMesh: return "Static Mesh";
        case AssetType::MeshSource: return "Mesh Source";
        case AssetType::Texture: return "Texture";
        case AssetType::Scene: return "Scene";
        case AssetType::Landscape: return "Landscape";
        case AssetType::PhysicsMaterial: return "Physics Material";
        }
        return "Unknown";
    }

    bool AssetRegistry::validateRoots(std::string* error)
    {
        std::error_code ec;
        std::filesystem::create_directories(contentRoot_, ec);
        if (ec)
        {
            fail(error, "Could not create Content root: " + ec.message());
            return false;
        }
        contentRoot_ = std::filesystem::weakly_canonical(contentRoot_, ec);
        if (ec || contentRoot_.empty())
        {
            fail(error, "Could not canonicalize Content root");
            return false;
        }
        stateFile_ = std::filesystem::absolute(stateFile_, ec).lexically_normal();
        if (ec || stateFile_.empty() || isWithin(contentRoot_, stateFile_))
        {
            fail(error, "Asset registry state file must be outside Content root");
            return false;
        }
        return true;
    }

    bool AssetRegistry::initialize(std::filesystem::path contentRoot,
                                   std::filesystem::path stateFile,
                                   std::string* error)
    {
        if (error) error->clear();
        initialized_ = false;
        snapshot_ = {};
        lastDelta_ = {};
        stableIdsByPathKey_.clear();
        assetIndexByPathKey_.clear();
        assetIndexById_.clear();
        lastWarning_.clear();
        contentRoot_ = std::move(contentRoot);
        stateFile_ = std::move(stateFile);
        if (contentRoot_.empty() || stateFile_.empty() || !validateRoots(error)) return false;

        std::string loadError;
        if (!loadStableIds(&loadError))
        {
            // Corrupt cache-like metadata must not make user Content unavailable.
            // Recover with fresh IDs, preserve a diagnostic, then atomically replace it.
            stableIdsByPathKey_.clear();
            lastWarning_ = std::move(loadError);
        }
        initialized_ = true;
        return rescan(error, true);
    }

    bool AssetRegistry::loadStableIds(std::string* error)
    {
        stableIdsByPathKey_.clear();
        std::error_code existsError;
        if (!std::filesystem::exists(stateFile_, existsError)) return !existsError;
        if (existsError)
        {
            fail(error, "Could not inspect asset registry state: " + existsError.message());
            return false;
        }

        std::vector<std::uint8_t> bytes;
        if (!AtomicFile::read(stateFile_, bytes, kMaximumRegistryBytes, error)) return false;
        ArchiveReader reader;
        if (!reader.open(bytes, kRegistrySchema, kRegistryVersion, kRegistryVersion, kMaximumRegistryBytes))
        {
            fail(error, "Asset registry state archive was rejected");
            return false;
        }
        std::uint32_t count = 0;
        if (!reader.readU32(count) || count > kMaximumAssets)
        {
            fail(error, "Asset registry state count is invalid");
            return false;
        }
        std::unordered_set<Guid, GuidHash> ids;
        for (std::uint32_t index = 0; index < count; ++index)
        {
            std::string pathText;
            Guid id;
            if (!reader.readString(pathText, 1024) || !reader.readGuid(id) || !id.isValid())
            {
                fail(error, "Asset registry state entry is malformed");
                return false;
            }
            const auto path = AssetPath::parse(pathText);
            if (!path || path->isRoot() || !ids.emplace(id).second ||
                !stableIdsByPathKey_.emplace(path->comparisonKey(), id).second)
            {
                fail(error, "Asset registry state contains an invalid or duplicate path");
                return false;
            }
        }
        if (!reader.atEnd())
        {
            fail(error, "Asset registry state has trailing payload");
            return false;
        }
        return true;
    }

    bool AssetRegistry::saveStableIds(std::string* error) const
    {
        ArchiveWriter writer(kRegistrySchema, kRegistryVersion);
        if (snapshot_.assets.size() > kMaximumAssets)
        {
            fail(error, "Asset registry exceeds supported asset count");
            return false;
        }
        writer.writeU32(static_cast<std::uint32_t>(snapshot_.assets.size()));
        for (const auto& asset : snapshot_.assets)
        {
            if (!writer.writeString(asset.path.string()))
            {
                fail(error, "Asset registry path is invalid UTF-8");
                return false;
            }
            writer.writeGuid(asset.id);
        }
        return AtomicFile::write(stateFile_, writer.finish(), error);
    }

    bool AssetRegistry::rescan(std::string* error, bool fullRescan)
    {
        if (error) error->clear();
        if (!initialized_)
        {
            fail(error, "Asset registry is not initialized");
            return false;
        }

        AssetRegistrySnapshot next;
        next.generation = snapshot_.generation + 1;
        const auto rootPath = AssetPath::parse("/Game");
        if (!rootPath)
        {
            fail(error, "Canonical /Game mount is unavailable");
            return false;
        }
        next.folders.push_back({*rootPath, 0, 0});

        std::error_code ec;
        std::filesystem::recursive_directory_iterator iterator(contentRoot_,
            std::filesystem::directory_options::skip_permission_denied, ec);
        const std::filesystem::recursive_directory_iterator end;
        if (ec)
        {
            fail(error, "Could not enumerate Content root: " + ec.message());
            return false;
        }

        for (; iterator != end; iterator.increment(ec))
        {
            if (ec)
            {
                ++next.skippedUnsafeEntries;
                ec.clear();
                continue;
            }
            const auto& entry = *iterator;
            const auto path = entry.path();
            std::error_code statusError;
            const auto status = entry.symlink_status(statusError);
            if (statusError || std::filesystem::is_symlink(status) || hiddenOrInternal(path))
            {
                ++next.skippedUnsafeEntries;
                if (entry.is_directory(statusError)) iterator.disable_recursion_pending();
                continue;
            }

            const auto virtualPath = AssetPath::fromFilesystemPath(contentRoot_, path);
            if (!virtualPath)
            {
                ++next.skippedUnsafeEntries;
                if (entry.is_directory(statusError)) iterator.disable_recursion_pending();
                continue;
            }

            if (entry.is_directory(statusError))
            {
                next.folders.push_back({*virtualPath, 0, 0});
                continue;
            }
            if (!entry.is_regular_file(statusError))
            {
                ++next.skippedUnsafeEntries;
                continue;
            }
            if (next.assets.size() >= kMaximumAssets)
            {
                fail(error, "Content root exceeds supported asset count");
                return false;
            }

            const std::string extension = extensionUtf8(path);
            const auto type = typeFromExtension(extension);
            if (!type)
            {
                ++next.skippedUnsupportedFiles;
                continue;
            }

            const std::string key = virtualPath->comparisonKey();
            Guid id;
            if (const auto existing = stableIdsByPathKey_.find(key); existing != stableIdsByPathKey_.end()) id = existing->second;
            else id = Guid::create();

            const auto size = entry.file_size(statusError);
            if (statusError)
            {
                ++next.skippedUnsafeEntries;
                continue;
            }
            const auto writeTime = entry.last_write_time(statusError);
            if (statusError)
            {
                ++next.skippedUnsafeEntries;
                continue;
            }
            next.assets.push_back({id, *virtualPath, *type, size,
                static_cast<std::int64_t>(writeTime.time_since_epoch().count()), extension});
        }

        auto byPath = [](const auto& lhs, const auto& rhs)
        {
            return lhs.path.comparisonKey() < rhs.path.comparisonKey();
        };
        std::sort(next.assets.begin(), next.assets.end(), byPath);
        std::sort(next.folders.begin(), next.folders.end(), byPath);

        std::unordered_map<std::string, std::size_t> folderIndexes;
        for (std::size_t index = 0; index < next.folders.size(); ++index)
            folderIndexes.emplace(next.folders[index].path.comparisonKey(), index);
        for (const auto& asset : next.assets)
        {
            AssetPath folder = asset.path.parent();
            if (const auto direct = folderIndexes.find(folder.comparisonKey()); direct != folderIndexes.end())
                ++next.folders[direct->second].directAssetCount;
            while (!folder.empty())
            {
                if (const auto ancestor = folderIndexes.find(folder.comparisonKey()); ancestor != folderIndexes.end())
                    ++next.folders[ancestor->second].descendantAssetCount;
                if (folder.isRoot()) break;
                folder = folder.parent();
            }
        }

        AssetRegistryDelta delta;
        delta.generation = next.generation;
        delta.fullRescan = fullRescan;
        std::unordered_map<std::string, const AssetRecord*> previousByPath;
        previousByPath.reserve(snapshot_.assets.size());
        for (const auto& asset : snapshot_.assets)
            previousByPath.emplace(asset.path.comparisonKey(), &asset);
        for (const auto& asset : next.assets)
        {
            const auto previous = previousByPath.find(asset.path.comparisonKey());
            if (previous == previousByPath.end())
            {
                delta.added.push_back(asset.id);
                continue;
            }
            const AssetRecord& old = *previous->second;
            if (old.type != asset.type || old.fileSize != asset.fileSize ||
                old.lastWriteStamp != asset.lastWriteStamp || old.extension != asset.extension)
                delta.modified.push_back(asset.id);
            previousByPath.erase(previous);
        }
        for (const auto& [key, asset] : previousByPath)
        {
            (void)key;
            delta.removed.push_back(asset->id);
        }
        auto sortGuids = [](std::vector<Guid>& values) { std::sort(values.begin(), values.end()); };
        sortGuids(delta.added);
        sortGuids(delta.modified);
        sortGuids(delta.removed);

        std::unordered_map<std::string, Guid> nextStableIds;
        std::unordered_map<std::string, std::size_t> nextPathIndexes;
        std::unordered_map<Guid, std::size_t, GuidHash> nextIdIndexes;
        nextStableIds.reserve(next.assets.size());
        nextPathIndexes.reserve(next.assets.size());
        nextIdIndexes.reserve(next.assets.size());
        for (std::size_t index = 0; index < next.assets.size(); ++index)
        {
            const auto& asset = next.assets[index];
            const std::string key = asset.path.comparisonKey();
            if (!nextStableIds.emplace(key, asset.id).second ||
                !nextPathIndexes.emplace(key, index).second ||
                !nextIdIndexes.emplace(asset.id, index).second)
            {
                fail(error, "Content root contains duplicate asset identity or path");
                return false;
            }
        }

        snapshot_ = std::move(next);
        lastDelta_ = std::move(delta);
        stableIdsByPathKey_ = std::move(nextStableIds);
        assetIndexByPathKey_ = std::move(nextPathIndexes);
        assetIndexById_ = std::move(nextIdIndexes);
        return saveStableIds(error);
    }

    const AssetRecord* AssetRegistry::findByPath(std::string_view virtualPath) const
    {
        const auto path = AssetPath::parse(virtualPath);
        if (!path) return nullptr;
        const auto found = assetIndexByPathKey_.find(path->comparisonKey());
        return found == assetIndexByPathKey_.end() ? nullptr : &snapshot_.assets[found->second];
    }

    const AssetRecord* AssetRegistry::findById(const Guid& id) const
    {
        const auto found = assetIndexById_.find(id);
        return found == assetIndexById_.end() ? nullptr : &snapshot_.assets[found->second];
    }

    bool AssetRegistry::remapPath(const AssetPath& oldPath, const AssetPath& newPath, std::string* error)
    {
        if (error) error->clear();
        if (!initialized_ || oldPath.empty() || newPath.empty() || oldPath.isRoot() || newPath.isRoot() ||
            oldPath == newPath)
        {
            fail(error, "Asset registry path move is invalid");
            return false;
        }

        const std::string oldKey = oldPath.comparisonKey();
        const std::string newKey = newPath.comparisonKey();
        auto matchesPrefix = [](std::string_view value, std::string_view prefix)
        {
            return value == prefix || (value.size() > prefix.size() && value.starts_with(prefix) && value[prefix.size()] == '/');
        };
        if (matchesPrefix(newKey, oldKey))
        {
            fail(error, "Asset registry path cannot move into its own subtree");
            return false;
        }

        std::string actualOldRoot;
        for (const auto& folder : snapshot_.folders)
            if (folder.path.comparisonKey() == oldKey) { actualOldRoot = folder.path.string(); break; }
        if (actualOldRoot.empty())
            for (const auto& asset : snapshot_.assets)
                if (asset.path.comparisonKey() == oldKey) { actualOldRoot = asset.path.string(); break; }
        if (actualOldRoot.empty())
        {
            fail(error, "Asset registry move source does not exist");
            return false;
        }

        AssetRegistrySnapshot next = snapshot_;
        std::vector<AssetRegistryDelta::Move> moves;
        auto remap = [&](AssetPath& path, const Guid* id) -> bool
        {
            const std::string key = path.comparisonKey();
            if (!matchesPrefix(key, oldKey)) return true;
            if (path.string().size() < actualOldRoot.size()) return false;
            const std::string candidateText = newPath.string() + path.string().substr(actualOldRoot.size());
            const auto candidate = AssetPath::parse(candidateText);
            if (!candidate) return false;
            if (id) moves.push_back({*id, path, *candidate});
            path = *candidate;
            return true;
        };
        for (auto& asset : next.assets) if (!remap(asset.path, &asset.id))
        {
            fail(error, "Asset registry could not construct moved asset path");
            return false;
        }
        for (auto& folder : next.folders) if (!remap(folder.path, nullptr))
        {
            fail(error, "Asset registry could not construct moved folder path");
            return false;
        }

        std::unordered_set<std::string> nextKeys;
        for (const auto& asset : next.assets)
        {
            const std::string key = asset.path.comparisonKey();
            if (!nextKeys.emplace(key).second)
            {
                fail(error, "Asset registry move collides with another asset");
                return false;
            }
        }
        for (const auto& folder : next.folders)
        {
            if (!nextKeys.emplace(folder.path.comparisonKey()).second)
            {
                fail(error, "Asset registry move collides with another folder");
                return false;
            }
        }

        auto byPath = [](const auto& lhs, const auto& rhs) { return lhs.path.comparisonKey() < rhs.path.comparisonKey(); };
        std::sort(next.assets.begin(), next.assets.end(), byPath);
        std::sort(next.folders.begin(), next.folders.end(), byPath);
        next.generation = snapshot_.generation + 1;

        std::unordered_map<std::string, Guid> nextStable;
        std::unordered_map<std::string, std::size_t> nextPathIndex;
        std::unordered_map<Guid, std::size_t, GuidHash> nextIdIndex;
        for (std::size_t index = 0; index < next.assets.size(); ++index)
        {
            const auto& asset = next.assets[index];
            const std::string key = asset.path.comparisonKey();
            if (!nextStable.emplace(key, asset.id).second ||
                !nextPathIndex.emplace(key, index).second ||
                !nextIdIndex.emplace(asset.id, index).second)
            {
                fail(error, "Asset registry move produced duplicate identity");
                return false;
            }
        }

        const auto oldSnapshot = snapshot_;
        const auto oldDelta = lastDelta_;
        auto oldStable = stableIdsByPathKey_;
        auto oldPathIndex = assetIndexByPathKey_;
        auto oldIdIndex = assetIndexById_;
        snapshot_ = std::move(next);
        stableIdsByPathKey_ = std::move(nextStable);
        assetIndexByPathKey_ = std::move(nextPathIndex);
        assetIndexById_ = std::move(nextIdIndex);
        lastDelta_ = {};
        lastDelta_.generation = snapshot_.generation;
        lastDelta_.moved = std::move(moves);
        if (!saveStableIds(error))
        {
            snapshot_ = oldSnapshot;
            lastDelta_ = oldDelta;
            stableIdsByPathKey_ = std::move(oldStable);
            assetIndexByPathKey_ = std::move(oldPathIndex);
            assetIndexById_ = std::move(oldIdIndex);
            return false;
        }
        return true;
    }
}
