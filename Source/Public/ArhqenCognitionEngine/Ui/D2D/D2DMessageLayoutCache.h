#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

#include <unordered_map>

namespace am::ui
{
    struct MessageLayoutEntry
    {
        std::uint64_t id = 0;
        float y = 0.0f;
        float height = 0.0f;
        float bubbleWidth = 0.0f;
        std::size_t textLength = 0;
        bool valid = false;
    };

    struct VisibleMessageRange
    {
        std::size_t begin = 0;
        std::size_t end = 0;
        float totalHeight = 0.0f;
    };

    class D2DMessageLayoutCache
    {
    public:
        void clear();
        void invalidateMessage(std::uint64_t id);
        void invalidateAll();

        MessageLayoutEntry layoutMessage(D2DRenderContext& ctx, const ChatMessage& message, float y, float bubbleWidth);
        VisibleMessageRange computeVisibleRange(
            D2DRenderContext& ctx,
            const std::vector<ChatMessage>& messages,
            UiRect viewport,
            float scrollOffset,
            float bubbleWidth
        );

        float totalHeight(D2DRenderContext& ctx, const std::vector<ChatMessage>& messages, float bubbleWidth);
        std::size_t cachedCount() const;
        std::size_t hitCount() const;
        std::size_t missCount() const;

    private:
        float measureHeight(D2DRenderContext& ctx, const ChatMessage& message, float bubbleWidth);

        std::unordered_map<std::uint64_t, MessageLayoutEntry> entries_;
        std::size_t hits_ = 0;
        std::size_t misses_ = 0;
    };
}
