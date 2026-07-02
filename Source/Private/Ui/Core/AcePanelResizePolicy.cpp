#include "ArhqenCognitionEngine/Ui/Core/AcePanelResizePolicy.h"
#include <algorithm>

namespace am::ui
{
    PanelResizeEdge PanelResizePolicy::hitTest(UiRect p,float x,float y,PanelResizeEdge allowed,float g) noexcept
    {
        if(p.empty()||g<=0)return PanelResizeEdge::None;PanelResizeEdge out=PanelResizeEdge::None;
        if(y>=p.top-g&&y<=p.bottom+g){if(hasEdge(allowed,PanelResizeEdge::Left)&&x>=p.left-g&&x<=p.left+g)out=out|PanelResizeEdge::Left;if(hasEdge(allowed,PanelResizeEdge::Right)&&x>=p.right-g&&x<=p.right+g)out=out|PanelResizeEdge::Right;}
        if(x>=p.left-g&&x<=p.right+g){if(hasEdge(allowed,PanelResizeEdge::Top)&&y>=p.top-g&&y<=p.top+g)out=out|PanelResizeEdge::Top;if(hasEdge(allowed,PanelResizeEdge::Bottom)&&y>=p.bottom-g&&y<=p.bottom+g)out=out|PanelResizeEdge::Bottom;}
        return out;
    }
    UiRect PanelResizePolicy::apply(UiRect r,PanelResizeEdge e,float dx,float dy,UiRect b,float minW,float minH) noexcept
    {
        minW=std::max(1.0f,minW);minH=std::max(1.0f,minH);
        if(hasEdge(e,PanelResizeEdge::Left))r.left=std::clamp(r.left+dx,b.left,std::min(r.right-minW,b.right));
        if(hasEdge(e,PanelResizeEdge::Right))r.right=std::clamp(r.right+dx,std::max(r.left+minW,b.left),b.right);
        if(hasEdge(e,PanelResizeEdge::Top))r.top=std::clamp(r.top+dy,b.top,std::min(r.bottom-minH,b.bottom));
        if(hasEdge(e,PanelResizeEdge::Bottom))r.bottom=std::clamp(r.bottom+dy,std::max(r.top+minH,b.top),b.bottom);
        return r;
    }
    PanelResizeCursor PanelResizePolicy::cursor(PanelResizeEdge e) noexcept
    {
        const bool h=hasEdge(e,PanelResizeEdge::Left)||hasEdge(e,PanelResizeEdge::Right),v=hasEdge(e,PanelResizeEdge::Top)||hasEdge(e,PanelResizeEdge::Bottom);
        if(h&&v){const bool same=hasEdge(e,PanelResizeEdge::Left)==hasEdge(e,PanelResizeEdge::Top);return same?PanelResizeCursor::DiagonalNorthWestSouthEast:PanelResizeCursor::DiagonalNorthEastSouthWest;}
        if(h)return PanelResizeCursor::Horizontal;if(v)return PanelResizeCursor::Vertical;return PanelResizeCursor::Arrow;
    }
}
