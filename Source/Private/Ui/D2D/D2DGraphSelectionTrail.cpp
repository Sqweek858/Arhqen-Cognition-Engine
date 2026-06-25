#include "ArhqenCognitionEngine/Ui/D2D/D2DGraphSelectionTrail.h"

namespace am::ui
{
    void D2DGraphSelectionTrail::setRect(UiRect rect)
    {
        rect_ = rect;
    }

    void D2DGraphSelectionTrail::push(am::core::AceUiSelection selection)
    {
        if (selection.id == 0 && selection.label.empty())
        {
            return;
        }

        if (!entries_.empty() && entries_.front().id == selection.id && entries_.front().label == selection.label)
        {
            return;
        }

        D2DGraphSelectionTrailEntry entry;
        entry.id = selection.id;
        entry.label = selection.label.empty() ? L"selection" : selection.label;
        entry.source = selection.source.empty() ? L"graph" : selection.source;

        entries_.push_front(std::move(entry));

        while (entries_.size() > maxEntries_)
        {
            entries_.pop_back();
        }
    }

    void D2DGraphSelectionTrail::clear()
    {
        entries_.clear();
    }

    std::size_t D2DGraphSelectionTrail::size() const
    {
        return entries_.size();
    }

    bool D2DGraphSelectionTrail::empty() const
    {
        return entries_.empty();
    }

    void D2DGraphSelectionTrail::render(D2DRenderContext& ctx)
    {
        if (rect_.empty() || entries_.empty())
        {
            return;
        }

        D2DWidgetUtils::fillRounded(ctx, rect_, 12.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"SELECTIONS", FontRole::Small, rect_.inset({8.0f, 6.0f, 8.0f, rect_.height() - 24.0f}), ctx.brushes.muted);

        float y = rect_.top + 28.0f;

        for (std::size_t i = 0; i < entries_.size(); ++i)
        {
            const auto& entry = entries_[i];
            UiRect row{rect_.left + 8.0f, y, rect_.right - 8.0f, y + 28.0f};

            D2DWidgetUtils::fillRounded(ctx, row, 9.0f, i == 0 ? ctx.brushes.panelSoft : ctx.brushes.panelElevated, nullptr, 0.0f);
            D2DWidgetUtils::drawTextEx(ctx, entry.label, FontRole::Small, row.inset({8.0f, 3.0f, 8.0f, 13.0f}), ctx.brushes.text);
            D2DWidgetUtils::drawTextEx(ctx, L"#" + std::to_wstring(entry.id) + L" • " + entry.source, FontRole::Small, row.inset({8.0f, 15.0f, 8.0f, 1.0f}), ctx.brushes.muted);

            y += 31.0f;
        }
    }
}
