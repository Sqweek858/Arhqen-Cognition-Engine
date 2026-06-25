#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"

#include <algorithm>

namespace am::ui
{
    struct UiThickness
    {
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;
    };

    inline UiThickness uiUniform(float value)
    {
        return {value, value, value, value};
    }

    inline UiRect uiInset(UiRect rect, UiThickness thickness)
    {
        return makeUiRect(rect.left + thickness.left, rect.top + thickness.top, rect.right - thickness.right, rect.bottom - thickness.bottom);
    }

    inline UiRect uiCenterBox(UiRect outer, float width, float height)
    {
        const float w = std::min(width, outer.width());
        const float h = std::min(height, outer.height());
        const float left = outer.left + (outer.width() - w) * 0.5f;
        const float top = outer.top + (outer.height() - h) * 0.5f;
        return makeUiRect(left, top, left + w, top + h);
    }

    inline UiRect uiMaxWidth(UiRect outer, float maxWidth)
    {
        const float w = std::min(maxWidth, outer.width());
        const float left = outer.left + (outer.width() - w) * 0.5f;
        return makeUiRect(left, outer.top, left + w, outer.bottom);
    }

    inline UiRect uiTakeTop(UiRect& rect, float height)
    {
        const float h = std::clamp(height, 0.0f, rect.height());
        UiRect result = makeUiRect(rect.left, rect.top, rect.right, rect.top + h);
        rect.top += h;
        return result;
    }

    inline UiRect uiTakeBottom(UiRect& rect, float height)
    {
        const float h = std::clamp(height, 0.0f, rect.height());
        UiRect result = makeUiRect(rect.left, rect.bottom - h, rect.right, rect.bottom);
        rect.bottom -= h;
        return result;
    }

    inline UiRect uiTakeLeft(UiRect& rect, float width)
    {
        const float w = std::clamp(width, 0.0f, rect.width());
        UiRect result = makeUiRect(rect.left, rect.top, rect.left + w, rect.bottom);
        rect.left += w;
        return result;
    }

    inline UiRect uiTakeRight(UiRect& rect, float width)
    {
        const float w = std::clamp(width, 0.0f, rect.width());
        UiRect result = makeUiRect(rect.right - w, rect.top, rect.right, rect.bottom);
        rect.right -= w;
        return result;
    }

    struct UiRowLayout
    {
        UiRect rect {};
        float gap = 8.0f;
        float cursor = 0.0f;

        explicit UiRowLayout(UiRect inRect, float inGap = 8.0f)
            : rect(inRect)
            , gap(inGap)
            , cursor(inRect.left)
        {
        }

        UiRect next(float width)
        {
            const float left = cursor;
            const float right = std::min(rect.right, left + width);
            cursor = right + gap;
            return makeUiRect(left, rect.top, right, rect.bottom);
        }
    };

    struct UiColumnLayout
    {
        UiRect rect {};
        float gap = 8.0f;
        float cursor = 0.0f;

        explicit UiColumnLayout(UiRect inRect, float inGap = 8.0f)
            : rect(inRect)
            , gap(inGap)
            , cursor(inRect.top)
        {
        }

        UiRect next(float height)
        {
            const float top = cursor;
            const float bottom = std::min(rect.bottom, top + height);
            cursor = bottom + gap;
            return makeUiRect(rect.left, top, rect.right, bottom);
        }
    };
}
