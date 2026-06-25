#include "ArhqenCognitionEngine/Ui/D2D/D2DGraphForceLayout.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace am::ui
{
    void D2DGraphForceLayout::apply(
        std::vector<am::core::AceUiGraphNode>& nodes,
        const std::vector<am::core::AceUiGraphEdge>& edges,
        D2DGraphForceLayoutOptions options)
    {
        if (nodes.size() < 2)
        {
            return;
        }

        struct Velocity
        {
            float x = 0.0f;
            float y = 0.0f;
        };

        std::unordered_map<std::uint64_t, Velocity> velocities;

        for (int iteration = 0; iteration < options.iterations; ++iteration)
        {
            for (std::size_t i = 0; i < nodes.size(); ++i)
            {
                for (std::size_t j = i + 1; j < nodes.size(); ++j)
                {
                    auto& a = nodes[i];
                    auto& b = nodes[j];

                    float dx = a.x - b.x;
                    float dy = a.y - b.y;
                    float distSq = dx * dx + dy * dy + 0.001f;
                    float dist = std::sqrt(distSq);
                    float force = options.repulsion / distSq;

                    dx /= dist;
                    dy /= dist;

                    velocities[a.id].x += dx * force;
                    velocities[a.id].y += dy * force;
                    velocities[b.id].x -= dx * force;
                    velocities[b.id].y -= dy * force;
                }
            }

            for (const auto& edge : edges)
            {
                auto* a = findNode(nodes, edge.from);
                auto* b = findNode(nodes, edge.to);

                if (!a || !b)
                {
                    continue;
                }

                float dx = b->x - a->x;
                float dy = b->y - a->y;
                float dist = std::sqrt(dx * dx + dy * dy + 0.001f);
                float force = (dist - 0.32f) * options.attraction;

                dx /= dist;
                dy /= dist;

                velocities[a->id].x += dx * force;
                velocities[a->id].y += dy * force;
                velocities[b->id].x -= dx * force;
                velocities[b->id].y -= dy * force;
            }

            for (auto& node : nodes)
            {
                velocities[node.id].x += (0.5f - node.x) * options.centerPull;
                velocities[node.id].y += (0.5f - node.y) * options.centerPull;

                node.x += velocities[node.id].x;
                node.y += velocities[node.id].y;

                node.x = std::clamp(node.x, 0.08f, 0.92f);
                node.y = std::clamp(node.y, 0.10f, 0.90f);

                velocities[node.id].x *= options.damping;
                velocities[node.id].y *= options.damping;
            }
        }
    }

    am::core::AceUiGraphNode* D2DGraphForceLayout::findNode(std::vector<am::core::AceUiGraphNode>& nodes, std::uint64_t id)
    {
        for (auto& node : nodes)
        {
            if (node.id == id)
            {
                return &node;
            }
        }

        return nullptr;
    }
}
