#include "ArhqenCognitionEngine/AquariumUI/AceAquariumPanel.h"

namespace ace::aquarium_ui
{
    AceAquariumPanel::AceAquariumPanel(AceAquariumRuntimeController* controller)
        : controller_(controller)
    {
    }

    void AceAquariumPanel::SetController(AceAquariumRuntimeController* controller)
    {
        controller_ = controller;
    }

    AceAquariumRuntimeController* AceAquariumPanel::Controller() const
    {
        return controller_;
    }

    std::vector<std::string> AceAquariumPanel::BuildSummaryLines() const
    {
        if (!controller_)
        {
            return {"Cognitive Environment Control Panel", "No Aquarium runtime controller attached."};
        }

        const auto snapshot = controller_->BuildSnapshot();
        return {
            "Cognitive Environment Control Panel",
            "3D viewport is not implemented yet.",
            "Headless Aquarium C++ runtime is active.",
            "scenario=" + snapshot.scenarioName,
            "planner=" + snapshot.plannerName,
            "step=" + std::to_string(snapshot.step)
        };
    }
}
