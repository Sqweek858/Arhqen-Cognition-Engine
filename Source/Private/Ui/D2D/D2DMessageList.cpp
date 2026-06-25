#include "ArhqenCognitionEngine/Ui/D2D/D2DMessageList.h"

#include "ArhqenCognitionEngine/Ui/D2D/D2DGlassEffects.h"

#include <utility>

namespace am::ui
{
    namespace
    {
        std::wstring sanitizeVisibleText(std::wstring text)
        {
            std::wstring out;
            out.reserve(text.size());

            for (wchar_t ch : text)
            {
                // Strip common mojibake lead/continuation characters without embedding
                // the corrupted byte-rendered strings in source again.
                if (ch == 0x00C2 || ch == 0x00C3 || ch == 0x00E2 || ch == 0x20AC || ch == 0x00A2)
                {
                    continue;
                }

                out.push_back(ch);
            }

            return out;
        }
    }

    void D2DMessageList::setRect(UiRect rect)
    {
        rect_ = rect;
        scroll_.setViewport(rect_.inset({16.0f, 16.0f, 16.0f, 16.0f}).height());
    }

    void D2DMessageList::addMessage(ChatMessage message)
    {
        messages_.push_back(std::move(message));
        layoutCache_.invalidateAll();

        if (stickToBottom_)
        {
            scrollToBottom();
        }
    }

    void D2DMessageList::setMessages(std::vector<ChatMessage> messages)
    {
        messages_ = std::move(messages);
        layoutCache_.invalidateAll();
        scrollToBottom();
    }

    const std::vector<ChatMessage>& D2DMessageList::messages() const
    {
        return messages_;
    }

    void D2DMessageList::clear()
    {
        messages_.clear();
        layoutCache_.clear();
        scrollToBottom();
    }

    std::size_t D2DMessageList::size() const
    {
        return messages_.size();
    }

    void D2DMessageList::scrollBy(float delta)
    {
        stickToBottom_ = false;
        scroll_.scrollBy(delta);

        if (scroll_.targetOffset() >= scroll_.maxOffset() - 5.0f)
        {
            stickToBottom_ = true;
            scroll_.scrollToBottom();
        }
    }

    void D2DMessageList::scrollToBottom()
    {
        stickToBottom_ = true;
        scroll_.scrollToBottom();
    }

    void D2DMessageList::update(float dtSeconds)
    {
        scroll_.update(dtSeconds);
    }

    bool D2DMessageList::animating() const
    {
        return scroll_.animating();
    }

    std::size_t D2DMessageList::layoutCacheSize() const
    {
        return layoutCache_.cachedCount();
    }

    std::size_t D2DMessageList::layoutCacheHits() const
    {
        return layoutCache_.hitCount();
    }

    std::size_t D2DMessageList::layoutCacheMisses() const
    {
        return layoutCache_.missCount();
    }


    std::optional<am::core::AceUiSelection> D2DMessageList::hitTest(D2DRenderContext& ctx, float x, float y)
    {
        if (!rect_.contains(x, y))
        {
            return std::nullopt;
        }

        const UiRect clipRect = rect_.inset({16.0f, 16.0f, 16.0f, 16.0f});
        const float bubbleWidth = std::min(760.0f, clipRect.width() * 0.78f);
        const float totalHeight = layoutCache_.totalHeight(ctx, messages_, bubbleWidth);
        (void)totalHeight;
        float rowY = clipRect.top - scroll_.offset();

        for (std::size_t i = 0; i < messages_.size(); ++i)
        {
            const auto& message = messages_[i];
            const auto layout = layoutCache_.layoutMessage(ctx, message, rowY, bubbleWidth);
            const auto bubble = bubbleRectFor(ctx, message, rowY, layout.height);

            if (bubble.contains(x, y))
            {
                return am::core::AceUiSelection{
                    am::core::AceUiItemKind::Message,
                    message.id,
                    message.author,
                    L"message_list"
                };
            }

            rowY += layout.height + 12.0f;
        }

        return std::nullopt;
    }

    const ChatMessage* D2DMessageList::findMessage(std::uint64_t id) const
    {
        for (const auto& message : messages_)
        {
            if (message.id == id)
            {
                return &message;
            }
        }

        return nullptr;
    }


    void D2DMessageList::render(D2DRenderContext& ctx)
    {
        D2DGlassMaterial listGlass;
        listGlass.radius = 18.0f;
        listGlass.fillAlpha = messages_.empty() ? 0.16f : 0.24f;
        listGlass.borderAlpha = messages_.empty() ? 0.34f : 0.54f;
        listGlass.highlightAlpha = 0.16f;
        listGlass.glowAlpha = messages_.empty() ? 0.10f : 0.18f;
        listGlass.shadowAlpha = messages_.empty() ? 0.06f : 0.14f;
        listGlass.blurFallbackAlpha = messages_.empty() ? 0.04f : 0.08f;
        listGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, rect_, listGlass);

        const UiRect clipRect = rect_.inset({16.0f, 16.0f, 16.0f, 16.0f});
        const float bubbleWidth = std::min(760.0f, clipRect.width() * 0.78f);
        const float totalHeight = layoutCache_.totalHeight(ctx, messages_, bubbleWidth);

        (void)totalHeight;
        scroll_.setViewport(clipRect.height());
        scroll_.setContentHeight(totalHeight);

        if (stickToBottom_)
        {
            scroll_.scrollToBottom();
        }

        const auto visibleRange = layoutCache_.computeVisibleRange(ctx, messages_, clipRect, scroll_.offset(), bubbleWidth);

        ctx.target->PushAxisAlignedClip(clipRect.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        float y = clipRect.top - scroll_.offset();

        for (std::size_t i = 0; i < messages_.size(); ++i)
        {
            const auto& message = messages_[i];
            const auto layout = layoutCache_.layoutMessage(ctx, message, y, bubbleWidth);
            const float bubbleHeight = layout.height;

            if (i >= visibleRange.begin && i < visibleRange.end)
            {
                const UiRect bubble = bubbleRectFor(ctx, message, y, bubbleHeight);
                ID2D1Brush* fill = message.system ? ctx.brushes.systemBubble : (message.fromUser ? ctx.brushes.userBubble : ctx.brushes.assistantBubble);
                ID2D1Brush* authorBrush = message.fromUser ? ctx.brushes.text : (message.system ? ctx.brushes.accentWarm : ctx.brushes.accent);

                if (message.kind == ChatMessageKind::Warning)
                {
                    fill = ctx.brushes.systemBubble;
                    authorBrush = ctx.brushes.danger;
                }
                else if (message.kind == ChatMessageKind::Tool)
                {
                    fill = ctx.brushes.panelElevated;
                    authorBrush = ctx.brushes.accentBlue;
                }

                D2DWidgetUtils::fillRounded(ctx, bubble, 16.0f, fill, ctx.brushes.borderDim, 1.0f);

                std::wstring authorLine = message.author;
                if (!message.timestamp.empty())
                {
                    authorLine += L"  -  " + message.timestamp;
                }

                D2DWidgetUtils::drawTextEx(ctx, authorLine, FontRole::Small, bubble.inset({16.0f, 10.0f, 16.0f, bubble.height() - 30.0f}), authorBrush);
                D2DWidgetUtils::drawTextEx(ctx, sanitizeVisibleText(message.text), FontRole::Body, bubble.inset({16.0f, 32.0f, 16.0f, message.metadata.empty() ? 10.0f : 28.0f}), ctx.brushes.text);

                if (!message.metadata.empty())
                {
                    D2DWidgetUtils::drawTextEx(ctx, sanitizeVisibleText(message.metadata), FontRole::Small, bubble.inset({16.0f, bubble.height() - 24.0f, 16.0f, 8.0f}), ctx.brushes.muted);
                }
            }

            y += bubbleHeight + 12.0f;
        }

        ctx.target->PopAxisAlignedClip();

        if (totalHeight > clipRect.height())
        {
            const float trackHeight = clipRect.height() - 8.0f;
            const float thumbHeight = std::max(44.0f, trackHeight * (clipRect.height() / totalHeight));
            const float normalized = scroll_.maxOffset() <= 0.0f ? 0.0f : (scroll_.offset() / scroll_.maxOffset());
            const float thumbTop = clipRect.top + 4.0f + (trackHeight - thumbHeight) * normalized;
            UiRect thumb{clipRect.right - 6.0f, thumbTop, clipRect.right - 2.0f, thumbTop + thumbHeight};
            D2DWidgetUtils::fillRounded(ctx, thumb, 2.0f, ctx.brushes.border);
        }
    }

    float D2DMessageList::estimateBubbleHeight(D2DRenderContext& ctx, const ChatMessage& message, float bubbleWidth) const
    {
        UiRect measureRect{0.0f, 0.0f, bubbleWidth - 32.0f, 420.0f};
        const auto metrics = D2DWidgetUtils::measureText(ctx, sanitizeVisibleText(message.text), FontRole::Body, measureRect);

        float metadataHeight = message.metadata.empty() ? 0.0f : 22.0f;

        if (metrics.height > 0.0f)
        {
            return std::clamp(48.0f + metrics.height + 14.0f + metadataHeight, 72.0f, 220.0f);
        }

        const float charsPerLine = std::max(24.0f, bubbleWidth / 9.5f);
        const float lines = std::max(1.0f, static_cast<float>(message.text.size()) / charsPerLine + 1.0f);
        return std::clamp(48.0f + lines * 23.0f + metadataHeight, 72.0f, 220.0f);
    }

    UiRect D2DMessageList::bubbleRectFor(D2DRenderContext&, const ChatMessage& message, float y, float height) const
    {
        const UiRect clipRect = rect_.inset({16.0f, 16.0f, 16.0f, 16.0f});
        const float bubbleWidth = std::min(message.system ? 820.0f : 760.0f, clipRect.width() * (message.system ? 0.90f : 0.78f));

        if (message.system)
        {
            const float x = clipRect.left + (clipRect.width() - bubbleWidth) * 0.5f;
            return {x, y, x + bubbleWidth, y + height};
        }

        if (message.fromUser)
        {
            return {clipRect.right - bubbleWidth - 10.0f, y, clipRect.right - 10.0f, y + height};
        }

        return {clipRect.left + 10.0f, y, clipRect.left + bubbleWidth + 10.0f, y + height};
    }
}
