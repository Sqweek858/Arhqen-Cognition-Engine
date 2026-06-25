#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DTextLayoutFoundation.h"

#include <cstdint>
#include <string>
#include <vector>

namespace am::ui
{
    enum class D2DDrawCommandKind
    {
        PushClip,
        PopClip,
        Rect,
        RoundedRect,
        Text,
        Line
    };

    struct D2DDrawCommand
    {
        D2DDrawCommandKind kind = D2DDrawCommandKind::Rect;
        UiRect rect{};
        UiRect rect2{};
        std::wstring text;
        FontRole role = FontRole::Body;
        int layer = 0;
        float radius = 0.0f;
        float thickness = 1.0f;
        D2DTextOverflowMode overflow = D2DTextOverflowMode::Ellipsis;
    };

    struct D2DDrawCommandStats
    {
        std::uint64_t frameCount = 0;
        std::uint64_t commandCount = 0;
        std::uint64_t maxCommandCount = 0;
        std::uint64_t clipCount = 0;
        std::uint64_t textCount = 0;
    };

    class D2DDrawCommandBuffer
    {
    public:
        void BeginFrame();
        void PushClip(UiRect rect, int layer = 0);
        void PopClip(int layer = 0);
        void Rect(UiRect rect, int layer = 0);
        void RoundedRect(UiRect rect, float radius, int layer = 0);
        void Text(std::wstring text, UiRect rect, FontRole role = FontRole::Body, int layer = 0, D2DTextOverflowMode overflow = D2DTextOverflowMode::Ellipsis);
        void Line(UiRect endpoints, float thickness, int layer = 0);
        const std::vector<D2DDrawCommand>& Commands() const;
        D2DDrawCommandStats Stats() const;

    private:
        void Add(D2DDrawCommand command);

        std::vector<D2DDrawCommand> commands_;
        D2DDrawCommandStats stats_{};
    };
}
