#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqActions.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqObjects.h"

#include <string>
#include <vector>

namespace ace::aquarium
{
    class AceAqGridWorld
    {
    public:
        AceAqGridWorld() = default;
        AceAqGridWorld(int width, int height);

        static AceAqGridWorld DefaultWorld();
        static AceAqGridWorld FromAscii(const std::vector<std::string>& rows);

        int Width() const { return width_; }
        int Height() const { return height_; }

        bool InBounds(AceAqPoint point) const;
        AceAqWorldObject GetCell(AceAqPoint point) const;
        void SetCell(AceAqPoint point, AceAqObjectKind kind);

        AceAqPoint AgentPosition() const { return agentPosition_; }
        AceAqDirection AgentDirection() const { return agentDirection_; }
        void SetAgentPosition(AceAqPoint point) { agentPosition_ = point; }
        void SetAgentDirection(AceAqDirection direction) { agentDirection_ = direction; }

        AceAqPoint FrontPosition() const;
        AceAqPoint LeftPosition() const;
        AceAqPoint RightPosition() const;
        AceAqPoint OffsetFromAgent(int localX, int localY) const;

        void TurnAgentLeft();
        void TurnAgentRight();

        bool MoveAgentForward();
        bool PushFrontObject(std::string* reason);

        std::string DebugTruthJsonLike() const;

    private:
        int Index(AceAqPoint point) const;

        int width_ = 0;
        int height_ = 0;
        std::vector<AceAqObjectKind> cells_;
        AceAqPoint agentPosition_{1, 1};
        AceAqDirection agentDirection_ = AceAqDirection::East;
    };
}
