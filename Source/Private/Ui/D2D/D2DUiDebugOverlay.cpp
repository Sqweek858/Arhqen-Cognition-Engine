#include "ArhqenCognitionEngine/Ui/D2D/D2DUiDebugOverlay.h"

#include "ArhqenCognitionEngine/Ui/D2D/D2DGlassEffects.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DTextLayoutFoundation.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

#include <algorithm>
#include <utility>

namespace am::ui
{
    void D2DUiDebugOverlay::SetVisible(bool visible)
    {
        snapshot_.visible = visible;
    }

    void D2DUiDebugOverlay::Toggle()
    {
        snapshot_.visible = !snapshot_.visible;
    }

    bool D2DUiDebugOverlay::Visible() const
    {
        return snapshot_.visible;
    }

    void D2DUiDebugOverlay::SetSnapshot(D2DUiDebugOverlaySnapshot snapshot)
    {
        const bool visible = snapshot_.visible;
        snapshot_ = std::move(snapshot);
        snapshot_.visible = visible;
    }

    void D2DUiDebugOverlay::Render(D2DRenderContext& ctx)
    {
        if (!snapshot_.visible || !ctx.target)
        {
            return;
        }

        for (const auto& item : snapshot_.rects)
        {
            if (item.rect.empty())
            {
                continue;
            }
            ID2D1Brush* brush = item.hot ? ctx.brushes.accentWarm : ctx.brushes.border;
            ctx.target->DrawRectangle(item.rect.d2d(), brush, item.hot ? 2.0f : 1.0f);
            D2DTextLayoutFoundation::Draw(ctx, item.label, FontRole::Small, item.rect.inset({4.0f, 2.0f, 4.0f, 2.0f}), brush, D2DTextOverflowMode::Ellipsis);
        }

        const float width = std::min(520.0f, std::max(360.0f, ctx.width * 0.34f));
        const UiRect panel = makeUiRect(ctx.width - width - 18.0f, 58.0f, ctx.width - 18.0f, 338.0f);
        D2DGlassMaterial material;
        material.radius = 14.0f;
        material.fillAlpha = 0.62f;
        material.borderAlpha = 0.56f;
        material.glowAlpha = 0.18f;
        material.blurFallbackAlpha = 0.16f;
        D2DGlassEffects::drawGlassPanel(ctx, panel, material);

        float y = panel.top + 12.0f;
        auto row = [&](const std::wstring& text, ID2D1Brush* brush = nullptr)
        {
            const UiRect r = makeUiRect(panel.left + 14.0f, y, panel.right - 14.0f, y + 18.0f);
            D2DTextLayoutFoundation::Draw(ctx, text, FontRole::Small, r, brush ? brush : ctx.brushes.text, D2DTextOverflowMode::Ellipsis);
            y += 19.0f;
        };

        row(snapshot_.title, ctx.brushes.accent);
        row(L"layout passes=" + std::to_wstring(snapshot_.layoutStats.arrangePasses) + L" nodes=" + std::to_wstring(snapshot_.layoutStats.nodeCount));
        row(L"draw commands=" + std::to_wstring(snapshot_.drawStats.commandCount) + L" max=" + std::to_wstring(snapshot_.drawStats.maxCommandCount));
        row(L"text draws=" + std::to_wstring(snapshot_.textStats.drawCount) + L" ellipsis=" + std::to_wstring(snapshot_.textStats.ellipsisCount));
        row(L"dirty marks=" + std::to_wstring(snapshot_.dirty.markCount) + L" rects=" + std::to_wstring(snapshot_.dirty.rects.size()));
        row(L"paint=" + std::to_wstring(snapshot_.paintCount) + L" repaint=" + std::to_wstring(snapshot_.repaintCount));
        row(L"hover=" + (snapshot_.hoverId.empty() ? L"none" : snapshot_.hoverId));
        row(L"focus=" + (snapshot_.focusId.empty() ? L"none" : snapshot_.focusId));
        row(L"F9 toggles this overlay", ctx.brushes.muted);
    }
}
