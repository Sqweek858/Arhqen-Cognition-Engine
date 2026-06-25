#include "ArhqenCognitionEngine/Ui/D2D/D2DDrawCommandBuffer.h"

#include <algorithm>
#include <utility>

namespace am::ui
{
    void D2DDrawCommandBuffer::BeginFrame()
    {
        ++stats_.frameCount;
        commands_.clear();
    }

    void D2DDrawCommandBuffer::PushClip(UiRect rect, int layer)
    {
        D2DDrawCommand c;
        c.kind = D2DDrawCommandKind::PushClip;
        c.rect = rect;
        c.layer = layer;
        Add(std::move(c));
        ++stats_.clipCount;
    }

    void D2DDrawCommandBuffer::PopClip(int layer)
    {
        D2DDrawCommand c;
        c.kind = D2DDrawCommandKind::PopClip;
        c.layer = layer;
        Add(std::move(c));
        ++stats_.clipCount;
    }

    void D2DDrawCommandBuffer::Rect(UiRect rect, int layer)
    {
        D2DDrawCommand c;
        c.kind = D2DDrawCommandKind::Rect;
        c.rect = rect;
        c.layer = layer;
        Add(std::move(c));
    }

    void D2DDrawCommandBuffer::RoundedRect(UiRect rect, float radius, int layer)
    {
        D2DDrawCommand c;
        c.kind = D2DDrawCommandKind::RoundedRect;
        c.rect = rect;
        c.radius = radius;
        c.layer = layer;
        Add(std::move(c));
    }

    void D2DDrawCommandBuffer::Text(std::wstring text, UiRect rect, FontRole role, int layer, D2DTextOverflowMode overflow)
    {
        D2DDrawCommand c;
        c.kind = D2DDrawCommandKind::Text;
        c.text = std::move(text);
        c.rect = rect;
        c.role = role;
        c.layer = layer;
        c.overflow = overflow;
        Add(std::move(c));
        ++stats_.textCount;
    }

    void D2DDrawCommandBuffer::Line(UiRect endpoints, float thickness, int layer)
    {
        D2DDrawCommand c;
        c.kind = D2DDrawCommandKind::Line;
        c.rect = endpoints;
        c.thickness = thickness;
        c.layer = layer;
        Add(std::move(c));
    }

    const std::vector<D2DDrawCommand>& D2DDrawCommandBuffer::Commands() const
    {
        return commands_;
    }

    D2DDrawCommandStats D2DDrawCommandBuffer::Stats() const
    {
        D2DDrawCommandStats s = stats_;
        s.commandCount = commands_.size();
        return s;
    }

    void D2DDrawCommandBuffer::Add(D2DDrawCommand command)
    {
        commands_.push_back(std::move(command));
        stats_.commandCount = commands_.size();
        stats_.maxCommandCount = std::max<std::uint64_t>(stats_.maxCommandCount, commands_.size());
    }
}
