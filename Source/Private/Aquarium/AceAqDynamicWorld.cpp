#include "ArhqenCognitionEngine/Aquarium/AceAqDynamicWorld.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <cmath>
#include <sstream>
#include <utility>

namespace ace::aquarium
{
    namespace
    {
        int Sign(int value)
        {
            if (value < 0) return -1;
            if (value > 0) return 1;
            return 0;
        }

        bool IsEmpty(const AceAqGridWorld& world, AceAqPoint point)
        {
            return world.InBounds(point) && world.GetCell(point).kind == AceAqObjectKind::Empty;
        }

        bool Adjacent(AceAqPoint a, AceAqPoint b)
        {
            return std::abs(a.x - b.x) + std::abs(a.y - b.y) == 1;
        }
    }

    AceAqDynamicWorldSystem::AceAqDynamicWorldSystem(AceAqDynamicWorldConfig config)
        : config_(config)
        , state_(config.seed == 0 ? 1u : config.seed)
    {
    }

    void AceAqDynamicWorldSystem::Reset(AceAqDynamicWorldConfig config)
    {
        config_ = config;
        state_ = config.seed == 0 ? 1u : config.seed;
    }

    std::uint32_t AceAqDynamicWorldSystem::NextRandom()
    {
        state_ = state_ * 1664525u + 1013904223u;
        return state_;
    }

    std::vector<AceAqWorldEvent> AceAqDynamicWorldSystem::Tick(int step, AceAqGridWorld& world, AceAqBodyState& body)
    {
        std::vector<AceAqWorldEvent> events;
        if (!config_.enabled)
        {
            return events;
        }

        if (config_.enableMovingHazards)
        {
            bool movedOne = false;
            for (int y = 0; y < world.Height() && !movedOne; ++y)
            {
                for (int x = 0; x < world.Width() && !movedOne; ++x)
                {
                    const AceAqPoint pos{x, y};
                    if (world.GetCell(pos).kind != AceAqObjectKind::MovingHazard)
                    {
                        continue;
                    }

                    const auto agent = world.AgentPosition();
                    AceAqPoint target = pos;
                    if (pos.y == agent.y)
                    {
                        target.x += Sign(agent.x - pos.x);
                    }
                    else
                    {
                        target.x += (NextRandom() % 2u) == 0u ? 1 : -1;
                    }

                    AceAqWorldEvent event;
                    event.eventType = "dynamic_hazard_move";
                    event.step = step;
                    event.positionBefore = pos;
                    event.positionAfter = target;
                    event.objectBefore = "moving_hazard";
                    event.objectAfter = "moving_hazard";
                    event.external = true;
                    event.eventFlags.push_back("dynamic_hazard_moved");
                    event.description = "moving hazard ticked";

                    if (target == agent || Adjacent(pos, agent))
                    {
                        event.bodyDelta = {0.0, 0.0, -0.20, 0.0};
                        event.eventFlags.push_back("external_event");
                        event.eventFlags.push_back("external_hazard_damage");
                        body.ApplyDelta(event.bodyDelta);
                    }
                    else if (IsEmpty(world, target))
                    {
                        world.SetCell(target, AceAqObjectKind::MovingHazard);
                        world.SetCell(pos, AceAqObjectKind::Empty);
                    }
                    else
                    {
                        event.positionAfter = pos;
                    }

                    events.push_back(event);
                    movedOne = true;
                }
            }
        }

        if (config_.enableSpreadingAcid && step > 0 && step % 2 == 0)
        {
            bool spread = false;
            for (int y = 0; y < world.Height() && !spread; ++y)
            {
                for (int x = 0; x < world.Width() && !spread; ++x)
                {
                    const AceAqPoint pos{x, y};
                    const auto kind = world.GetCell(pos).kind;
                    if (kind != AceAqObjectKind::SpreadingAcid && kind != AceAqObjectKind::Acid)
                    {
                        continue;
                    }

                    const AceAqPoint candidates[] = {{x + 1, y}, {x - 1, y}, {x, y + 1}, {x, y - 1}};
                    for (const auto candidate : candidates)
                    {
                        if (candidate == world.AgentPosition())
                        {
                            AceAqWorldEvent event;
                            event.eventType = "dynamic_spread_damage";
                            event.step = step;
                            event.positionBefore = pos;
                            event.positionAfter = candidate;
                            event.objectBefore = "spreading_acid";
                            event.objectAfter = "spreading_acid";
                            event.bodyDelta = {0.0, 0.0, -0.15, 0.0};
                            event.external = true;
                            event.eventFlags = {"dynamic_spread", "external_event", "external_acid_damage"};
                            event.description = "spreading acid reached agent";
                            body.ApplyDelta(event.bodyDelta);
                            events.push_back(event);
                            spread = true;
                            break;
                        }

                        if (IsEmpty(world, candidate))
                        {
                            world.SetCell(candidate, AceAqObjectKind::Acid);

                            AceAqWorldEvent event;
                            event.eventType = "dynamic_acid_spread";
                            event.step = step;
                            event.positionBefore = pos;
                            event.positionAfter = candidate;
                            event.objectBefore = "spreading_acid";
                            event.objectAfter = "acid";
                            event.external = true;
                            event.eventFlags = {"dynamic_spread"};
                            event.description = "acid spread into empty cell";
                            events.push_back(event);
                            spread = true;
                            break;
                        }
                    }
                }
            }
        }

        if (config_.enableFoodDecay && step >= config_.foodDecayStep)
        {
            bool decayed = false;
            for (int y = 0; y < world.Height() && !decayed; ++y)
            {
                for (int x = 0; x < world.Width() && !decayed; ++x)
                {
                    const AceAqPoint pos{x, y};
                    if (world.GetCell(pos).kind == AceAqObjectKind::Food)
                    {
                        world.SetCell(pos, AceAqObjectKind::PoisonFood);
                        AceAqWorldEvent event;
                        event.eventType = "dynamic_food_decay";
                        event.step = step;
                        event.positionBefore = pos;
                        event.positionAfter = pos;
                        event.objectBefore = "food";
                        event.objectAfter = "poison_food";
                        event.external = true;
                        event.eventFlags = {"dynamic_food_decay"};
                        event.description = "food decayed into poison food";
                        events.push_back(event);
                        decayed = true;
                    }
                }
            }
        }

        return events;
    }

    std::string AceAqDynamicWorldSystem::ToJsonLikeString() const
    {
        return ace::aquarium::ToJsonLikeString(config_);
    }

    std::string ToJsonLikeString(const AceAqDynamicWorldConfig& config)
    {
        return "{" +
            std::string("\"enabled\":") + JsonBool(config.enabled) + "," +
            "\"seed\":" + std::to_string(config.seed) + "," +
            "\"enable_moving_hazards\":" + JsonBool(config.enableMovingHazards) + "," +
            "\"enable_spreading_acid\":" + JsonBool(config.enableSpreadingAcid) + "," +
            "\"enable_food_decay\":" + JsonBool(config.enableFoodDecay) + "," +
            "\"food_decay_step\":" + std::to_string(config.foodDecayStep) +
            "}";
    }
}
