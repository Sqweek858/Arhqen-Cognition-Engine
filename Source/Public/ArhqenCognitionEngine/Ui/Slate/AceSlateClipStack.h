#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateFramePrimitives.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace am::ui::slate
{
    struct AceSlateClipEntry
    {
        UiRect rect{};
        std::uint32_t layer = 0;
        std::uint64_t serial = 0;
        bool axisAligned = true;
        std::string debugName;
    };

    struct AceSlateClipDecision
    {
        bool visible = true;
        bool clipped = false;
        UiRect finalClip{};
        UiRect finalBounds{};
        std::uint32_t clipDepth = 0;
        std::string reason;
    };

    struct AceSlateClipStackStats
    {
        std::uint64_t pushes = 0;
        std::uint64_t pops = 0;
        std::uint64_t queries = 0;
        std::uint64_t culled = 0;
        std::uint64_t clipped = 0;
        std::uint64_t emptyClipRejected = 0;
        std::uint32_t maxDepth = 0;
    };

    class AceSlateClipStack
    {
    public:
        void Reset(UiRect rootClip);
        void Push(UiRect rect, std::uint32_t layer, const char* debugName = nullptr);
        void Pop();
        AceSlateClipDecision Resolve(UiRect bounds) const;
        AceSlateClipDecision ResolveForLayer(UiRect bounds, std::uint32_t layer) const;
        UiRect CurrentClip() const;
        bool Empty() const { return entries_.empty(); }
        std::uint32_t Depth() const { return static_cast<std::uint32_t>(entries_.size()); }
        const AceSlateClipStackStats& Stats() const { return stats_; }
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
    private:
        UiRect Intersect(UiRect a, UiRect b) const;
        UiRect rootClip_{};
        std::uint64_t serial_ = 0;
        mutable AceSlateClipStackStats stats_{};
        std::vector<AceSlateClipEntry> entries_{};
    };
}
