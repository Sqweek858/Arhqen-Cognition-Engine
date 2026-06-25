#include "ArhqenCognitionEngine/AquariumUI/AceAquariumRuntimeController.h"
#include "ArhqenCognitionEngine/AquariumUI/AceAquariumPanel.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace aq = ace::aquarium;
namespace ui = ace::aquarium_ui;

namespace
{
    int g_failures = 0;

    void Pass(const std::string& name)
    {
        std::cout << "PASS|" << name << "\n";
    }

    void Fail(const std::string& name, const std::string& reason)
    {
        ++g_failures;
        std::cout << "FAIL|" << name << "|" << reason << "\n";
    }

    void Check(const std::string& name, bool ok, const std::string& reason)
    {
        ok ? Pass(name) : Fail(name, reason);
    }

    bool HasLine(const std::vector<std::string>& lines)
    {
        return !lines.empty();
    }
}

int main()
{
    ui::AceAquariumRuntimeController controller;

    Check("runtime_controller_initializes", controller.Initialize(), "initialize failed");
    Check("runtime_controller_lists_scenarios", !controller.ScenarioNames().empty(), "scenario list empty");
    Check("runtime_controller_reset_water_front", controller.ResetScenario("water_front", 123) && controller.CurrentScenarioName() == "water_front", "reset water_front failed");
    Check("runtime_controller_set_planner_safe", controller.SetPlanner("safe") && controller.CurrentPlannerName() == "safe", "safe planner failed");
    Check("runtime_controller_set_planner_counterfactual", controller.SetPlanner("counterfactual") && controller.CurrentPlannerName() == "counterfactual", "counterfactual planner failed");

    const int beforeStep = controller.StepIndex();
    Check("runtime_controller_step_once_increments_step", controller.StepOnce() && controller.StepIndex() == beforeStep + 1, "step once did not advance");

    Check("runtime_controller_manual_action_forward", controller.StepManual(aq::AceAqAction::MoveForward) && controller.StepIndex() == beforeStep + 2, "manual forward failed");

    controller.SetRunning(false);
    const int pausedStep = controller.StepIndex();
    controller.Tick(10.0);
    Check("runtime_controller_tick_does_not_advance_when_paused", controller.StepIndex() == pausedStep, "paused tick advanced");

    controller.SetRunning(true);
    controller.Tick(1.0);
    Check("runtime_controller_run_tick_advances_when_running", controller.StepIndex() > pausedStep, "running tick did not advance");
    controller.SetRunning(false);

    auto snapshot = controller.BuildSnapshot();

    Check("runtime_snapshot_contains_body_state", snapshot.body.hydration >= 0.0 && snapshot.homeostaticError >= 0.0, "body missing");
    Check("runtime_snapshot_contains_observation_lines", HasLine(snapshot.observationLines), "observation lines missing");
    Check("runtime_snapshot_contains_planner_lines", HasLine(snapshot.decisionTraceLines), "planner lines missing");
    Check("runtime_snapshot_contains_counterfactual_lines", HasLine(snapshot.counterfactualTraceLines), "counterfactual lines missing");
    Check("runtime_snapshot_contains_delayed_dynamic_lines", HasLine(snapshot.delayedEffectLines) && HasLine(snapshot.worldEventLines), "delayed/dynamic lines missing");

    const auto& logs = controller.GetRecentLogs();
    const bool hasReset = std::any_of(logs.begin(), logs.end(), [](const auto& entry) { return entry.message.find("reset") != std::string::npos; });
    const bool hasStep = std::any_of(logs.begin(), logs.end(), [](const auto& entry) { return entry.message.find("planner") != std::string::npos || entry.message.find("manual") != std::string::npos; });
    Check("runtime_logs_record_reset", hasReset, "reset log missing");
    Check("runtime_logs_record_step", hasStep, "step log missing");

    controller.SetDebugTruthEnabled(false);
    auto noTruth = controller.BuildSnapshot();
    Check("debug_truth_toggle_separates_truth", noTruth.debugTruthLines.empty(), "debug truth visible while off");
    Check("privacy_no_objectkind_when_debug_off", !ui::SnapshotAgentFacingTextContainsTruthLabels(noTruth), "truth label leaked");

    controller.SetDebugTruthEnabled(true);
    auto truth = controller.BuildSnapshot();
    Check("debug_truth_visible_when_enabled", !truth.debugTruthLines.empty() && truth.debugTruthLines.front().find("DEBUG TRUTH") != std::string::npos, "debug truth missing");

    ui::AceAquariumPanel panel(&controller);
    const auto summary = panel.BuildSummaryLines();
    Check("runtime_panel_summary_uses_controller", summary.size() >= 3, "panel summary missing");

    return g_failures == 0 ? 0 : 1;
}
