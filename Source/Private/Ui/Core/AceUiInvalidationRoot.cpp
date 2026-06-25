#include "ArhqenCognitionEngine/Ui/Core/AceUiInvalidationRoot.h"

#include <algorithm>

namespace am::ui
{
    void AceUiInvalidationRoot::Mark(AceUiDirtyReason reason)
    {
        flags_ |= reason;
        ++markCount_;
    }

    void AceUiInvalidationRoot::MarkRect(UiRect rect, AceUiDirtyReason reason)
    {
        if (rect.empty())
        {
            Mark(reason);
            return;
        }

        flags_ |= reason;
        ++markCount_;

        for (auto& existing : rects_)
        {
            if (CanMerge(existing, rect))
            {
                existing = Merge(existing, rect);
                ++mergedRectCount_;
                return;
            }
        }

        rects_.push_back(rect);
    }

    void AceUiInvalidationRoot::Clear()
    {
        flags_ = AceUiDirtyReason::None;
        rects_.clear();
    }

    AceUiDirtySnapshot AceUiInvalidationRoot::Consume()
    {
        AceUiDirtySnapshot snapshot = Snapshot();
        ++consumeCount_;
        Clear();
        return snapshot;
    }

    AceUiDirtySnapshot AceUiInvalidationRoot::Snapshot() const
    {
        AceUiDirtySnapshot snapshot;
        snapshot.flags = flags_;
        snapshot.rects = rects_;
        snapshot.markCount = markCount_;
        snapshot.consumeCount = consumeCount_;
        snapshot.mergedRectCount = mergedRectCount_;
        return snapshot;
    }

    bool AceUiInvalidationRoot::HasAny() const
    {
        return flags_ != AceUiDirtyReason::None || !rects_.empty();
    }

    bool AceUiInvalidationRoot::Has(AceUiDirtyReason reason) const
    {
        return aceUiHasDirty(flags_, reason);
    }

    bool AceUiInvalidationRoot::CanMerge(UiRect a, UiRect b)
    {
        constexpr float kTouchPadding = 8.0f;
        return !(a.right + kTouchPadding < b.left || b.right + kTouchPadding < a.left ||
                 a.bottom + kTouchPadding < b.top || b.bottom + kTouchPadding < a.top);
    }

    UiRect AceUiInvalidationRoot::Merge(UiRect a, UiRect b)
    {
        return makeUiRect(
            std::min(a.left, b.left),
            std::min(a.top, b.top),
            std::max(a.right, b.right),
            std::max(a.bottom, b.bottom));
    }
}
