#include "ArhqenCognitionEngine/Aquarium/AceAqGrid.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <sstream>
#include <stdexcept>

namespace ace::aquarium
{
    AceAqGridWorld::AceAqGridWorld(int width, int height)
        : width_(width)
        , height_(height)
        , cells_(static_cast<std::size_t>(width * height), AceAqObjectKind::Empty)
    {
        if (width_ <= 0 || height_ <= 0)
        {
            throw std::invalid_argument("AceAqGridWorld requires positive dimensions.");
        }
    }

    AceAqGridWorld AceAqGridWorld::DefaultWorld()
    {
        return FromAscii({
            "#####",
            "#...#",
            "#.W.#",
            "#...#",
            "#####",
        });
    }

    AceAqGridWorld AceAqGridWorld::FromAscii(const std::vector<std::string>& rows)
    {
        if (rows.empty())
        {
            throw std::invalid_argument("AceAqGridWorld::FromAscii requires at least one row.");
        }

        const int height = static_cast<int>(rows.size());
        const int width = static_cast<int>(rows.front().size());
        if (width <= 0)
        {
            throw std::invalid_argument("AceAqGridWorld::FromAscii requires non-empty rows.");
        }

        AceAqGridWorld world(width, height);
        for (int y = 0; y < height; ++y)
        {
            if (static_cast<int>(rows[static_cast<std::size_t>(y)].size()) != width)
            {
                throw std::invalid_argument("AceAqGridWorld::FromAscii requires rectangular rows.");
            }

            for (int x = 0; x < width; ++x)
            {
                const char ch = rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
                if (ch == '>')
                {
                    world.SetAgentPosition({x, y});
                    world.SetAgentDirection(AceAqDirection::East);
                    world.SetCell({x, y}, AceAqObjectKind::Empty);
                }
                else if (ch == '<')
                {
                    world.SetAgentPosition({x, y});
                    world.SetAgentDirection(AceAqDirection::West);
                    world.SetCell({x, y}, AceAqObjectKind::Empty);
                }
                else if (ch == '^')
                {
                    world.SetAgentPosition({x, y});
                    world.SetAgentDirection(AceAqDirection::North);
                    world.SetCell({x, y}, AceAqObjectKind::Empty);
                }
                else if (ch == 'v')
                {
                    world.SetAgentPosition({x, y});
                    world.SetAgentDirection(AceAqDirection::South);
                    world.SetCell({x, y}, AceAqObjectKind::Empty);
                }
                else
                {
                    world.SetCell({x, y}, ObjectKindFromAscii(ch));
                }
            }
        }

        return world;
    }

    bool AceAqGridWorld::InBounds(AceAqPoint point) const
    {
        return point.x >= 0 && point.y >= 0 && point.x < width_ && point.y < height_;
    }

    int AceAqGridWorld::Index(AceAqPoint point) const
    {
        return point.y * width_ + point.x;
    }

    AceAqWorldObject AceAqGridWorld::GetCell(AceAqPoint point) const
    {
        if (!InBounds(point))
        {
            return MakeObject(AceAqObjectKind::Wall);
        }

        return MakeObject(cells_[static_cast<std::size_t>(Index(point))]);
    }

    void AceAqGridWorld::SetCell(AceAqPoint point, AceAqObjectKind kind)
    {
        if (!InBounds(point))
        {
            return;
        }

        cells_[static_cast<std::size_t>(Index(point))] = kind;
    }

    AceAqPoint AceAqGridWorld::FrontPosition() const
    {
        const auto delta = DirectionDelta(agentDirection_);
        return {agentPosition_.x + delta.x, agentPosition_.y + delta.y};
    }

    AceAqPoint AceAqGridWorld::LeftPosition() const
    {
        const auto delta = DirectionDelta(TurnLeft(agentDirection_));
        return {agentPosition_.x + delta.x, agentPosition_.y + delta.y};
    }

    AceAqPoint AceAqGridWorld::RightPosition() const
    {
        const auto delta = DirectionDelta(TurnRight(agentDirection_));
        return {agentPosition_.x + delta.x, agentPosition_.y + delta.y};
    }

    AceAqPoint AceAqGridWorld::OffsetFromAgent(int localX, int localY) const
    {
        const auto forward = DirectionDelta(agentDirection_);
        const auto right = DirectionDelta(TurnRight(agentDirection_));

        return {
            agentPosition_.x + right.x * localX + forward.x * localY,
            agentPosition_.y + right.y * localX + forward.y * localY,
        };
    }

    void AceAqGridWorld::TurnAgentLeft()
    {
        agentDirection_ = TurnLeft(agentDirection_);
    }

    void AceAqGridWorld::TurnAgentRight()
    {
        agentDirection_ = TurnRight(agentDirection_);
    }

    bool AceAqGridWorld::MoveAgentForward()
    {
        const auto target = FrontPosition();
        const auto object = GetCell(target);
        if (object.blocksMovement)
        {
            return false;
        }

        agentPosition_ = target;
        return true;
    }

    bool AceAqGridWorld::PushFrontObject(std::string* reason)
    {
        const auto target = FrontPosition();
        const auto object = GetCell(target);
        if (!object.pushable)
        {
            if (reason)
            {
                *reason = "front object is not pushable";
            }
            return false;
        }

        const auto delta = DirectionDelta(agentDirection_);
        const AceAqPoint behind{target.x + delta.x, target.y + delta.y};
        const auto behindObject = GetCell(behind);
        if (behindObject.kind != AceAqObjectKind::Empty || behindObject.blocksMovement)
        {
            if (reason)
            {
                *reason = "push target space is blocked";
            }
            return false;
        }

        SetCell(behind, object.kind);
        SetCell(target, AceAqObjectKind::Empty);
        if (reason)
        {
            *reason = "push succeeded";
        }
        return true;
    }

    std::string AceAqGridWorld::DebugTruthJsonLike() const
    {
        std::ostringstream out;
        out << "{";
        out << "\"width\":" << width_ << ",";
        out << "\"height\":" << height_ << ",";
        out << "\"agent_position\":{\"x\":" << agentPosition_.x << ",\"y\":" << agentPosition_.y << "},";
        out << "\"agent_direction\":" << JsonString(ToString(agentDirection_)) << ",";
        out << "\"cells\":[";
        for (int y = 0; y < height_; ++y)
        {
            if (y > 0)
            {
                out << ",";
            }

            out << "[";
            for (int x = 0; x < width_; ++x)
            {
                if (x > 0)
                {
                    out << ",";
                }

                out << JsonString(ToString(cells_[static_cast<std::size_t>(Index({x, y}))]));
            }
            out << "]";
        }
        out << "]}";
        return out.str();
    }
}
