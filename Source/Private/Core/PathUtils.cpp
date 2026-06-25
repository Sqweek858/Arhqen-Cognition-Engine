#include "ArhqenCognitionEngine/Core/PathUtils.h"

namespace am::core
{
    std::optional<std::filesystem::path> findRepoRootFrom(const std::filesystem::path& start)
    {
        std::filesystem::path current = std::filesystem::absolute(start);

        if (std::filesystem::is_regular_file(current))
        {
            current = current.parent_path();
        }

        for (int depth = 0; depth < 12; ++depth)
        {
            if (std::filesystem::exists(current / "Config" / "app.amconfig"))
            {
                return current;
            }

            if (!current.has_parent_path() || current == current.parent_path())
            {
                break;
            }

            current = current.parent_path();
        }

        return std::nullopt;
    }

    std::filesystem::path configPathFromRepoRoot(const std::filesystem::path& repoRoot)
    {
        return repoRoot / "Config" / "app.amconfig";
    }
}
