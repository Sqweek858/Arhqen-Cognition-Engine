#pragma once

#include <filesystem>
#include <optional>

namespace am::core
{
    std::optional<std::filesystem::path> findRepoRootFrom(const std::filesystem::path& start);
    std::filesystem::path configPathFromRepoRoot(const std::filesystem::path& repoRoot);
}
