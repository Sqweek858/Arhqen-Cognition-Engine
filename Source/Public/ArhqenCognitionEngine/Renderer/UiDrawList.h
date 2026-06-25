#pragma once

#include <cstdint>
#include <vector>

namespace am::renderer
{
    struct UiColor
    {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
    };

    struct UiRect
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    struct UiVertex
    {
        float x = 0.0f;
        float y = 0.0f;
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
    };

    class UiDrawList
    {
    public:
        void clear();
        void addRectPixels(const UiRect& rect, UiColor color, float surfaceWidth, float surfaceHeight);

        const std::vector<UiVertex>& vertices() const;
        std::uint32_t vertexCount() const;
        bool empty() const;

    private:
        void addTriangle(UiVertex a, UiVertex b, UiVertex c);

        std::vector<UiVertex> vertices_;
    };
}
