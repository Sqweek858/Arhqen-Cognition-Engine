#include "ArhqenCognitionEngine/Renderer/Scene3DDrawList.h"

#include <algorithm>

namespace am::renderer
{
    void Scene3DDrawList::clear()
    {
        vertices_.clear();
    }

    Scene3DVertex Scene3DDrawList::makeVertex(Scene3DVec3 p, Scene3DColor color) const
    {
        Scene3DVertex v{};
        v.x = p.x;
        v.y = p.y;
        v.z = p.z;
        v.r = std::clamp(color.r, 0.0f, 1.0f);
        v.g = std::clamp(color.g, 0.0f, 1.0f);
        v.b = std::clamp(color.b, 0.0f, 1.0f);
        v.a = std::clamp(color.a, 0.0f, 1.0f);
        return v;
    }

    void Scene3DDrawList::addTriangle(Scene3DVec3 a, Scene3DVec3 b, Scene3DVec3 c, Scene3DColor color)
    {
        vertices_.push_back(makeVertex(a, color));
        vertices_.push_back(makeVertex(b, color));
        vertices_.push_back(makeVertex(c, color));
    }

    void Scene3DDrawList::addBox(Scene3DVec3 minCorner, Scene3DVec3 maxCorner, Scene3DColor color)
    {
        const float minX = std::min(minCorner.x, maxCorner.x);
        const float maxX = std::max(minCorner.x, maxCorner.x);
        const float minY = std::min(minCorner.y, maxCorner.y);
        const float maxY = std::max(minCorner.y, maxCorner.y);
        const float minZ = std::min(minCorner.z, maxCorner.z);
        const float maxZ = std::max(minCorner.z, maxCorner.z);

        const Scene3DVec3 p000{minX, minY, minZ};
        const Scene3DVec3 p001{minX, minY, maxZ};
        const Scene3DVec3 p010{minX, maxY, minZ};
        const Scene3DVec3 p011{minX, maxY, maxZ};
        const Scene3DVec3 p100{maxX, minY, minZ};
        const Scene3DVec3 p101{maxX, minY, maxZ};
        const Scene3DVec3 p110{maxX, maxY, minZ};
        const Scene3DVec3 p111{maxX, maxY, maxZ};

        // bottom/top
        addTriangle(p000, p100, p101, color); addTriangle(p000, p101, p001, color);
        addTriangle(p010, p011, p111, color); addTriangle(p010, p111, p110, color);
        // front/back
        addTriangle(p001, p101, p111, color); addTriangle(p001, p111, p011, color);
        addTriangle(p000, p010, p110, color); addTriangle(p000, p110, p100, color);
        // left/right
        addTriangle(p000, p001, p011, color); addTriangle(p000, p011, p010, color);
        addTriangle(p100, p110, p111, color); addTriangle(p100, p111, p101, color);
    }

    void Scene3DDrawList::addGroundGrid(float minX, float maxX, float minZ, float maxZ, float y, float thickness, Scene3DColor color)
    {
        const float t = std::max(0.01f, thickness);
        for (int x = static_cast<int>(minX); x <= static_cast<int>(maxX); ++x)
        {
            addBox({static_cast<float>(x) - t, y, minZ}, {static_cast<float>(x) + t, y + t, maxZ}, color);
        }
        for (int z = static_cast<int>(minZ); z <= static_cast<int>(maxZ); ++z)
        {
            addBox({minX, y, static_cast<float>(z) - t}, {maxX, y + t, static_cast<float>(z) + t}, color);
        }
    }

    const std::vector<Scene3DVertex>& Scene3DDrawList::vertices() const
    {
        return vertices_;
    }

    std::uint32_t Scene3DDrawList::vertexCount() const
    {
        return static_cast<std::uint32_t>(vertices_.size());
    }

    bool Scene3DDrawList::empty() const
    {
        return vertices_.empty();
    }
}
