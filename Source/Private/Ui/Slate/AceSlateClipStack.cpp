#include "ArhqenCognitionEngine/Ui/Slate/AceSlateClipStack.h"

#include <algorithm>
#include <sstream>

namespace am::ui::slate
{
    void AceSlateClipStack::Reset(UiRect rootClip)
    {
        rootClip_ = rootClip;
        serial_ = 0;
        entries_.clear();
        stats_ = {};
    }

    void AceSlateClipStack::Push(UiRect rect, std::uint32_t layer, const char* debugName)
    {
        AceSlateClipEntry entry{};
        entry.rect = rect.empty() ? rootClip_ : Intersect(rect, CurrentClip());
        entry.layer = layer;
        entry.serial = ++serial_;
        entry.axisAligned = true;
        if (debugName)
        {
            entry.debugName = debugName;
        }
        entries_.push_back(std::move(entry));
        ++stats_.pushes;
        stats_.maxDepth = std::max<std::uint32_t>(stats_.maxDepth, static_cast<std::uint32_t>(entries_.size()));
    }

    void AceSlateClipStack::Pop()
    {
        if (!entries_.empty())
        {
            entries_.pop_back();
            ++stats_.pops;
        }
    }

    AceSlateClipDecision AceSlateClipStack::Resolve(UiRect bounds) const
    {
        ++stats_.queries;
        AceSlateClipDecision decision{};
        decision.finalClip = CurrentClip();
        decision.finalBounds = Intersect(bounds, decision.finalClip);
        decision.clipDepth = static_cast<std::uint32_t>(entries_.size());
        decision.visible = !bounds.empty() && !decision.finalBounds.empty();
        decision.clipped = decision.visible &&
            (decision.finalBounds.left != bounds.left || decision.finalBounds.top != bounds.top ||
             decision.finalBounds.right != bounds.right || decision.finalBounds.bottom != bounds.bottom);
        if (!decision.visible)
        {
            ++stats_.culled;
            ++stats_.emptyClipRejected;
            decision.reason = "culled_by_clip";
        }
        else if (decision.clipped)
        {
            ++stats_.clipped;
            decision.reason = "clipped";
        }
        else
        {
            decision.reason = "visible";
        }
        return decision;
    }

    AceSlateClipDecision AceSlateClipStack::ResolveForLayer(UiRect bounds, std::uint32_t layer) const
    {
        ++stats_.queries;
        UiRect clip = rootClip_;
        std::uint32_t depth = 0;
        for (const auto& entry : entries_)
        {
            if (entry.layer <= layer)
            {
                clip = Intersect(clip, entry.rect);
                ++depth;
            }
        }
        AceSlateClipDecision decision{};
        decision.finalClip = clip;
        decision.finalBounds = Intersect(bounds, clip);
        decision.clipDepth = depth;
        decision.visible = !bounds.empty() && !decision.finalBounds.empty();
        decision.clipped = decision.visible &&
            (decision.finalBounds.left != bounds.left || decision.finalBounds.top != bounds.top ||
             decision.finalBounds.right != bounds.right || decision.finalBounds.bottom != bounds.bottom);
        if (!decision.visible)
        {
            ++stats_.culled;
            decision.reason = "culled_by_layer_clip";
        }
        else if (decision.clipped)
        {
            ++stats_.clipped;
            decision.reason = "clipped_by_layer";
        }
        else
        {
            decision.reason = "visible_layer";
        }
        return decision;
    }

    UiRect AceSlateClipStack::CurrentClip() const
    {
        if (entries_.empty())
        {
            return rootClip_;
        }
        return entries_.back().rect;
    }

    UiRect AceSlateClipStack::Intersect(UiRect a, UiRect b) const
    {
        if (a.empty())
        {
            return b;
        }
        if (b.empty())
        {
            return a;
        }
        const float left = std::max(a.left, b.left);
        const float top = std::max(a.top, b.top);
        const float right = std::min(a.right, b.right);
        const float bottom = std::min(a.bottom, b.bottom);
        return makeUiRect(left, top, std::max(left, right), std::max(top, bottom));
    }

    std::string AceSlateClipStack::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "depth=" << entries_.size()
            << ";pushes=" << stats_.pushes
            << ";pops=" << stats_.pops
            << ";queries=" << stats_.queries
            << ";culled=" << stats_.culled
            << ";clipped=" << stats_.clipped
            << ";empty=" << stats_.emptyClipRejected
            << ";max_depth=" << stats_.maxDepth;
        if (!entries_.empty())
        {
            const auto& top = entries_.back();
            oss << ";top=" << top.debugName
                << "@" << top.rect.left << ',' << top.rect.top << ',' << top.rect.width() << 'x' << top.rect.height();
        }
        return oss.str();
    }

    std::wstring AceSlateClipStack::WideDiagnostics() const
    {
        const std::string text = Diagnostics();
        return std::wstring(text.begin(), text.end());
    }
}
