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
        if (!detail.empty()) { std::cout << "|" << detail; }
        std::cout << "\n";
    }

    bool hasKind(const std::vector<ace::aquarium_render::AceAqRenderPrimitive>& primitives, AceAqRenderPrimitiveKind kind)
    {
        return std::any_of(primitives.begin(), primitives.end(), [kind](const auto& p) { return p.Kind == kind; });
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
        fail("scene_adapter_builds_grid", "controller initialize failed");
        return 1;
    }

    ace::aquarium_render::AceAquariumSceneAdapter adapter;
    ace::aquarium_render::AceAquariumViewport viewport;
    ace::aquarium_render::AceAquariumCamera camera{};

    const auto initial = adapter.BuildPrimitives(controller, false);

    if (hasKind(initial, AceAqRenderPrimitiveKind::GridLine)) pass("scene_adapter_builds_grid"); else fail("scene_adapter_builds_grid");
    if (hasKind(initial, AceAqRenderPrimitiveKind::Agent)) pass("scene_adapter_builds_agent"); else fail("scene_adapter_builds_agent");
    if (hasKind(initial, AceAqRenderPrimitiveKind::DirectionArrow)) pass("scene_adapter_builds_direction_arrow"); else fail("scene_adapter_builds_direction_arrow");
    if (hasKind(initial, AceAqRenderPrimitiveKind::Highlight)) pass("scene_adapter_builds_front_highlight"); else fail("scene_adapter_builds_front_highlight");

    const int before = controller.StepIndex();
    controller.StepOnce();
    const auto afterStep = adapter.BuildPrimitives(controller, false);
    if (controller.StepIndex() == before + 1 && !afterStep.empty()) pass("scene_adapter_updates_after_step"); else fail("scene_adapter_updates_after_step");

    controller.ResetScenario(controller.CurrentScenarioName(), 123);
    const auto afterReset = adapter.BuildPrimitives(controller, false);
    if (controller.StepIndex() == 0 && !afterReset.empty()) pass("scene_adapter_updates_after_reset"); else fail("scene_adapter_updates_after_reset");

    if (!hasTruthLabel(afterReset)) pass("debug_off_no_objectkind_labels"); else fail("debug_off_no_objectkind_labels");

    const auto debugOn = adapter.BuildPrimitives(controller, true);
    if (hasDebugWarning(debugOn)) pass("debug_on_debug_truth_warning_present"); else fail("debug_on_debug_truth_warning_present");

    const auto model = viewport.BuildViewportModel(afterReset, camera, 960.0f, 520.0f);
    if (!model.empty()) pass("embedded_viewport_scene_model_accepts_primitives"); else fail("embedded_viewport_scene_model_accepts_primitives");

    return failures == 0 ? 0 : 1;
}
