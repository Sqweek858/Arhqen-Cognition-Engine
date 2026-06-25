#include "ArhqenCognitionEngine/AquariumRender/AceAquariumRenderPrimitive.h"
#include "ArhqenCognitionEngine/AquariumRender/AceAquariumSceneAdapter.h"
#include "ArhqenCognitionEngine/AquariumRender/AceAquariumViewport.h"
#include "ArhqenCognitionEngine/AquariumUI/AceAquariumRuntimeController.h"
#include "ArhqenCognitionEngine/AquariumUI/AceEnvironmentWorkspace.h"

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
    ace::aquarium_ui::AceEnvironmentWorkspace workspace;
    const auto layout = workspace.Compute(1440.0f, 860.0f, 58.0f);

    if (layout.viewport.Width() >= 800.0f && layout.viewport.Height() >= 450.0f && workspace.ViewportIsLargeEnough(layout, 1440.0f, 860.0f))
    {
        pass("viewport_rect_large_enough");
    }
    else
    {
        fail("viewport_rect_large_enough", "computed viewport too small");
    }

    const auto resized = workspace.Compute(1280.0f, 760.0f, 58.0f);
    if (resized.viewport.Width() != layout.viewport.Width() && resized.viewport.Height() != layout.viewport.Height())
    {
        pass("viewport_rect_updates_on_resize");
    }
    else
    {
        fail("viewport_rect_updates_on_resize");
    }

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

    const auto model = viewport.BuildViewportModel(afterReset, camera, layout.viewport.Width(), layout.viewport.Height());
    if (!model.empty()) pass("embedded_workspace_scene_model_accepts_primitives"); else fail("embedded_workspace_scene_model_accepts_primitives");

    return failures == 0 ? 0 : 1;
}
