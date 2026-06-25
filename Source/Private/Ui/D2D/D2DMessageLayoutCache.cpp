#include "ArhqenCognitionEngine/Ui/D2D/D2DMessageLayoutCache.h"

namespace am::ui
{
    void D2DMessageLayoutCache::clear()
    {
        entries_.clear();
        hits_ = 0;
        misses_ = 0;
    }

    void D2DMessageLayoutCache::invalidateMessage(std::uint64_t id)
    {
        entries_.erase(id);
    }

    void D2DMessageLayoutCache::invalidateAll()
    {
        entries_.clear();
    }

    MessageLayoutEntry D2DMessageLayoutCache::layoutMessage(D2DRenderContext& ctx, const ChatMessage& message, float y, float bubbleWidth)
    {
        const auto found = entries_.find(message.id);

        if (found != entries_.end() &&
            found->second.valid &&
            found->second.bubbleWidth == bubbleWidth &&
            found->second.textLength == message.text.size())
        {
            ++hits_;
            auto entry = found->second;
            entry.y = y;
            return entry;
        }

        ++misses_;

        MessageLayoutEntry entry;
        entry.id = message.id;
        entry.y = y;
        entry.height = measureHeight(ctx, message, bubbleWidth);
        entry.bubbleWidth = bubbleWidth;
        entry.textLength = message.text.size();
        entry.valid = true;

        entries_[message.id] = entry;
        return entry;
    }

    VisibleMessageRange D2DMessageLayoutCache::computeVisibleRange(
        D2DRenderContext& ctx,
        const std::vector<ChatMessage>& messages,
        UiRect viewport,
        float scrollOffset,
        float bubbleWidth)
    {
        VisibleMessageRange range;
        range.begin = 0;
        range.end = messages.size();

        float total = 0.0f;
        std::vector<float> heights;
        heights.reserve(messages.size());

        for (const auto& message : messages)
        {
            const auto entry = layoutMessage(ctx, message, 0.0f, bubbleWidth);
            heights.push_back(entry.height);
            total += entry.height + 12.0f;
        }

        range.totalHeight = total;

        const float visibleTop = std::max(0.0f, total - viewport.height() - scrollOffset);
        const float visibleBottom = visibleTop + viewport.height();

        float y = 0.0f;
        bool foundBegin = false;

        for (std::size_t i = 0; i < heights.size(); ++i)
        {
            const float rowTop = y;
            const float rowBottom = y + heights[i];

            if (!foundBegin && rowBottom >= visibleTop)
            {
                range.begin = i;
                foundBegin = true;
            }

            if (foundBegin && rowTop > visibleBottom)
            {
                range.end = i;
                return range;
            }

            y += heights[i] + 12.0f;
        }

        range.end = messages.size();
        return range;
    }

    float D2DMessageLayoutCache::totalHeight(D2DRenderContext& ctx, const std::vector<ChatMessage>& messages, float bubbleWidth)
    {
        float total = 0.0f;

        for (const auto& message : messages)
        {
            total += layoutMessage(ctx, message, 0.0f, bubbleWidth).height + 12.0f;
        }

        return total;
    }

    std::size_t D2DMessageLayoutCache::cachedCount() const
    {
        return entries_.size();
    }

    std::size_t D2DMessageLayoutCache::hitCount() const
    {
        return hits_;
    }

    std::size_t D2DMessageLayoutCache::missCount() const
    {
        return misses_;
    }

    float D2DMessageLayoutCache::measureHeight(D2DRenderContext& ctx, const ChatMessage& message, float bubbleWidth)
    {
        UiRect measureRect{0.0f, 0.0f, bubbleWidth - 32.0f, 480.0f};
        const auto metrics = D2DWidgetUtils::measureText(ctx, message.text, FontRole::Body, measureRect);

        float metadataHeight = message.metadata.empty() ? 0.0f : 22.0f;

        if (metrics.height > 0.0f)
        {
            return std::clamp(48.0f + metrics.height + 14.0f + metadataHeight, 72.0f, 220.0f);
        }

        const float charsPerLine = std::max(24.0f, bubbleWidth / 9.5f);
        const float lines = std::max(1.0f, static_cast<float>(message.text.size()) / charsPerLine + 1.0f);
        return std::clamp(48.0f + lines * 23.0f + metadataHeight, 72.0f, 220.0f);
    }
}
