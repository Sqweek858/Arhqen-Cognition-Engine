#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"

#include <cstdint>
#include <vector>

namespace am::ui
{
    enum class AceUiDirtyReason : std::uint32_t
    {
        None = 0,
        Paint = 1u << 0,
        Layout = 1u << 1,
        Text = 1u << 2,
        Effect = 1u << 3,
        Viewport = 1u << 4,
        Input = 1u << 5,
        All = (1u << 0) | (1u << 1) | (1u << 2) | (1u << 3) | (1u << 4) | (1u << 5)
    };

    inline AceUiDirtyReason operator|(AceUiDirtyReason a, AceUiDirtyReason b)
    {
        return static_cast<AceUiDirtyReason>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
    }

    inline AceUiDirtyReason& operator|=(AceUiDirtyReason& a, AceUiDirtyReason b)
    {
        a = a | b;
        return a;
    }

    inline bool aceUiHasDirty(AceUiDirtyReason flags, AceUiDirtyReason test)
    {
        return (static_cast<std::uint32_t>(flags) & static_cast<std::uint32_t>(test)) != 0;
    }

    struct AceUiDirtySnapshot
    {
        AceUiDirtyReason flags = AceUiDirtyReason::None;
        std::vector<UiRect> rects;
        std::uint64_t markCount = 0;
        std::uint64_t consumeCount = 0;
        std::uint64_t mergedRectCount = 0;
    };

    class AceUiInvalidationRoot
    {
    public:
        void Mark(AceUiDirtyReason reason);
        void MarkRect(UiRect rect, AceUiDirtyReason reason = AceUiDirtyReason::Paint);
        void Clear();
        AceUiDirtySnapshot Consume();
        AceUiDirtySnapshot Snapshot() const;
        bool HasAny() const;
        bool Has(AceUiDirtyReason reason) const;

    private:
        static bool CanMerge(UiRect a, UiRect b);
        static UiRect Merge(UiRect a, UiRect b);

        AceUiDirtyReason flags_ = AceUiDirtyReason::None;
        std::vector<UiRect> rects_;
        std::uint64_t markCount_ = 0;
        std::uint64_t consumeCount_ = 0;
        std::uint64_t mergedRectCount_ = 0;
    };
}
