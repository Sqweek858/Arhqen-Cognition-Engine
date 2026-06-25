#include "ArhqenCognitionEngine/AquariumRender/AceAquariumRenderPrimitive.h"
#include "ArhqenCognitionEngine/AquariumRender/AceAquariumSceneAdapter.h"
#include "ArhqenCognitionEngine/AquariumRender/AceAquariumViewport.h"
#include "ArhqenCognitionEngine/AquariumUI/AceAquariumRuntimeController.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using ace::aquarium_render::AceAqRenderPrimitiveKind;

namespace
{
    int failures = 0;

    void pass(const std::string& name)
    {
        std::cout << "PASS|" << name << "\n";
    }

    void fail(const std::string& name, const std::string& detail = {})
    {
        ++failures;
        std::cout << "FAIL|" << name;
        if (!detail.empty())
        {
            std::cout << "|" << detail;
        }
        std::cout << "\n";
    }

    bool hasKind(const std::vector<ace::aquarium_render::AceAqRenderPrimitive>& primitives, AceAqRenderPrimitiveKind kind)
    {
        return std::any_of(primitives.begin(), primitives.end(), [kind](const auto& p) { return p.Kind == kind; });
    }

    std::size_t countKind(const std::vector<ace::aquarium_render::AceAqRenderPrimitive>& primitives, AceAqRenderPrimitiveKind kind)
    {
        return static_cast<std::size_t>(std::count_if(primitives.begin(), primitives.end(), [kind](const auto& p) { return p.Kind == kind; }));
    }

    bool hasTruthLabel(const std::vector<ace::aquarium_render::AceAqRenderPrimitive>& primitives)
    {
        return std::any_of(primitives.begin(), primitives.end(), [](const auto& p)
        {
            return ace::aquarium_render::PrimitiveLabelContainsObjectKindTruth(p);
        });
    }

    bool hasDebugWarning(const std::vector<ace::aquarium_render::AceAqRenderPrimitive>& primitives)
    {
        return std::any_of(primitives.begin(), primitives.end(), [](const auto& p)
        {
            return p.Kind == AceAqRenderPrimitiveKind::DebugLabel && p.Label.find("DEBUG TRUTH - NOT AGENT INPUT") != std::string::npos;
        });
    }
}

int main()
{
    ace::aquarium_ui::AceAquariumRuntimeController controller;
    if (!controller.Initialize())
    {
        fail("viewport_model_initializes", "controller Initialize failed");
        return 1;
    }

    ace::aquarium_render::AceAquariumSceneAdapter adapter;
    ace::aquarium_render::AceAquariumViewport viewport;
    ace::aquarium_render::AceAquariumCamera camera{};

    const auto initial = adapter.BuildPrimitives(controller, false);

    if (countKind(initial, AceAqRenderPrimitiveKind::GridLine) > 1) pass("scene_adapter_builds_grid"); else fail("scene_adapter_builds_grid");
    if (hasKind(initial, AceAqRenderPrimitiveKind::Agent)) pass("scene_adapter_builds_agent"); else fail("scene_adapter_builds_agent");
    if (hasKind(initial, AceAqRenderPrimitiveKind::DirectionArrow)) pass("scene_adapter_builds_direction_arrow"); else fail("scene_adapter_builds_direction_arrow");
    if (hasKind(initial, AceAqRenderPrimitiveKind::Highlight)) pass("scene_adapter_builds_front_highlight"); else fail("scene_adapter_builds_front_highlight");
    if (countKind(initial, AceAqRenderPrimitiveKind::Block) > 0 || countKind(initial, AceAqRenderPrimitiveKind::Tile) > 0) pass("scene_adapter_builds_objects"); else fail("scene_adapter_builds_objects");

    const int stepBefore = controller.StepIndex();
    controller.StepOnce();
    const auto afterStep = adapter.BuildPrimitives(controller, false);
    if (controller.StepIndex() == stepBefore + 1 && !afterStep.empty()) pass("scene_adapter_updates_after_step"); else fail("scene_adapter_updates_after_step");

    controller.ResetScenario(controller.CurrentScenarioName(), 123);
    const auto afterReset = adapter.BuildPrimitives(controller, false);
    if (controller.StepIndex() == 0 && !afterReset.empty()) pass("scene_adapter_updates_after_reset"); else fail("scene_adapter_updates_after_reset");

    if (!hasTruthLabel(afterReset)) pass("scene_adapter_debug_off_no_objectkind_labels"); else fail("scene_adapter_debug_off_no_objectkind_labels");

    const auto debugOn = adapter.BuildPrimitives(controller, true);
    if (hasDebugWarning(debugOn) && hasTruthLabel(debugOn)) pass("scene_adapter_debug_on_allows_debug_labels"); else fail("scene_adapter_debug_on_allows_debug_labels");

    const auto model = viewport.BuildViewportModel(afterReset, camera, 640.0f, 360.0f);
    if (!model.empty()) pass("viewport_model_initializes"); else fail("viewport_model_initializes");
    if (model.size() >= afterReset.size() / 2) pass("viewport_model_accepts_runtime_snapshot"); else fail("viewport_model_accepts_runtime_snapshot");

    if (hasDebugWarning(debugOn)) pass("debug_truth_warning_present"); else fail("debug_truth_warning_present");

    return failures == 0 ? 0 : 1;
}
