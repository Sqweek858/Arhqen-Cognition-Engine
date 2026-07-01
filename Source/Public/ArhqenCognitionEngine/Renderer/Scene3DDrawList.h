#pragma once

#include <cstdint>
#include <vector>

namespace am::renderer
{
    struct Scene3DColor
    {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
    };

    struct Scene3DVec3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct Scene3DVertex
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
    };

    struct Scene3DConstants
    {
        float viewProjection[16] = {};
    };

    class Scene3DDrawList
    {
    public:
        void clear();
        void addTriangle(Scene3DVec3 a, Scene3DVec3 b, Scene3DVec3 c, Scene3DColor color);
        void addBox(Scene3DVec3 minCorner, Scene3DVec3 maxCorner, Scene3DColor color);
        void addGroundGrid(float minX, float maxX, float minZ, float maxZ, float y, float thickness, Scene3DColor color);

        const std::vector<Scene3DVertex>& vertices() const;
        std::uint32_t vertexCount() const;
        bool empty() const;

    private:
        Scene3DVertex makeVertex(Scene3DVec3 p, Scene3DColor color) const;
        std::vector<Scene3DVertex> vertices_;
    };
}
