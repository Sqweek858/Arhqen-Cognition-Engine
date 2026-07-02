#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"
#include <cstdint>

namespace am::ui
{
    enum class PanelResizeEdge : std::uint8_t { None=0, Left=1, Right=2, Top=4, Bottom=8 };
    constexpr PanelResizeEdge operator|(PanelResizeEdge a,PanelResizeEdge b) noexcept{return static_cast<PanelResizeEdge>(static_cast<unsigned>(a)|static_cast<unsigned>(b));}
    constexpr bool hasEdge(PanelResizeEdge value,PanelResizeEdge edge) noexcept{return (static_cast<unsigned>(value)&static_cast<unsigned>(edge))!=0;}
    enum class PanelResizeCursor { Arrow, Horizontal, Vertical, DiagonalNorthWestSouthEast, DiagonalNorthEastSouthWest };

    class PanelResizePolicy final
    {
    public:
        static PanelResizeEdge hitTest(UiRect panel,float x,float y,PanelResizeEdge allowed,float grabWidth=7.0f) noexcept;
        static UiRect apply(UiRect start,PanelResizeEdge edges,float deltaX,float deltaY,UiRect bounds,float minimumWidth,float minimumHeight) noexcept;
        static PanelResizeCursor cursor(PanelResizeEdge edges) noexcept;
    };
}
