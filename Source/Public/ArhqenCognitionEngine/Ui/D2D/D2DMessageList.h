#pragma once

#include "ArhqenCognitionEngine/Core/AceUiModel.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DMessageLayoutCache.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DScrollController.h"

#include <optional>

namespace am::ui
{
    class D2DMessageList
    {
    public:
        void setRect(UiRect rect);
        void addMessage(ChatMessage message);
        void setMessages(std::vector<ChatMessage> messages);
        const std::vector<ChatMessage>& messages() const;

        void clear();
        std::size_t size() const;

        void scrollBy(float delta);
        void scrollToBottom();
        void update(float dtSeconds);
        bool animating() const;

        std::size_t layoutCacheSize() const;
        std::size_t layoutCacheHits() const;
        std::size_t layoutCacheMisses() const;

        std::optional<am::core::AceUiSelection> hitTest(D2DRenderContext& ctx, float x, float y);
        const ChatMessage* findMessage(std::uint64_t id) const;

        void render(D2DRenderContext& ctx);

    private:
        float estimateBubbleHeight(D2DRenderContext& ctx, const ChatMessage& message, float bubbleWidth) const;
        UiRect bubbleRectFor(D2DRenderContext& ctx, const ChatMessage& message, float y, float height) const;

        UiRect rect_{};
        std::vector<ChatMessage> messages_;
        D2DMessageLayoutCache layoutCache_;
        D2DScrollController scroll_;
        bool stickToBottom_ = true;
    };
}
