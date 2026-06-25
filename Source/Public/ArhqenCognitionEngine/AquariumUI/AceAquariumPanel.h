#pragma once

#include "ArhqenCognitionEngine/AquariumUI/AceAquariumRuntimeController.h"

#include <string>
#include <vector>

namespace ace::aquarium_ui
{
    class AceAquariumPanel
    {
    public:
        explicit AceAquariumPanel(AceAquariumRuntimeController* controller = nullptr);

        void SetController(AceAquariumRuntimeController* controller);
        AceAquariumRuntimeController* Controller() const;

        std::vector<std::string> BuildSummaryLines() const;

    private:
        AceAquariumRuntimeController* controller_ = nullptr;
    };
}
