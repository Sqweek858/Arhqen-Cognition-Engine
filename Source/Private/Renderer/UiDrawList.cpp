#include "ArhqenCognitionEngine/Renderer/UiDrawList.h"

namespace am::renderer
{
    namespace
    {
        float pixelXToNdc(float x, float surfaceWidth)
        {
            return (x / surfaceWidth) * 2.0f - 1.0f;
        }

        float pixelYToNdc(float y, float surfaceHeight)
        {
            return 1.0f - (y / surfaceHeight) * 2.0f;
        }

        UiVertex makeVertex(float x, float y, UiColor color)
        {
            UiVertex v;
            v.x = x;
            v.y = y;
            v.r = color.r;
            v.g = color.g;
            v.b = color.b;
            v.a = color.a;
            return v;
        }
    }

    void UiDrawList::clear()
    {
        vertices_.clear();
    }

    void UiDrawList::addRectPixels(const UiRect& rect, UiColor color, float surfaceWidth, float surfaceHeight)
    {
        if (rect.width <= 0.0f || rect.height <= 0.0f || surfaceWidth <= 0.0f || surfaceHeight <= 0.0f)
        {
            return;
        }

        const float left = pixelXToNdc(rect.x, surfaceWidth);
        const float right = pixelXToNdc(rect.x + rect.width, surfaceWidth);
        const float top = pixelYToNdc(rect.y, surfaceHeight);
        const float bottom = pixelYToNdc(rect.y + rect.height, surfaceHeight);

        const UiVertex topLeft = makeVertex(left, top, color);
        const UiVertex topRight = makeVertex(right, top, color);
        const UiVertex bottomLeft = makeVertex(left, bottom, color);
        const UiVertex bottomRight = makeVertex(right, bottom, color);

        addTriangle(topLeft, bottomLeft, topRight);
        addTriangle(topRight, bottomLeft, bottomRight);
    }

    const std::vector<UiVertex>& UiDrawList::vertices() const
    {
        return vertices_;
    }

    std::uint32_t UiDrawList::vertexCount() const
    {
        return static_cast<std::uint32_t>(vertices_.size());
    }

    bool UiDrawList::empty() const
    {
        return vertices_.empty();
    }

    void UiDrawList::addTriangle(UiVertex a, UiVertex b, UiVertex c)
    {
        vertices_.push_back(a);
        vertices_.push_back(b);
        vertices_.push_back(c);
    }
}
