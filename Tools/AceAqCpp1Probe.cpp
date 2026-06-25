#include "ArhqenCognitionEngine/Aquarium/AceAqActions.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqBody.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqEnvironment.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqGrid.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqObjects.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqObservation.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace aq = ace::aquarium;

namespace
{
    int g_failures = 0;

    bool Near(double a, double b, double eps = 1.0e-6)
    {
        return std::fabs(a - b) <= eps;
    }

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
        if (ok)
        {
            Pass(name);
        }
        else
        {
            Fail(name, reason);
        }
    }

    bool HasFlag(const aq::AceAqLastActionResult& result, const std::string& flag)
    {
        for (const auto& item : result.eventFlags)
        {
            if (item == flag)
            {
                return true;
            }
        }
        return false;
    }
}

int main()
{
    {
        aq::AceAqBodyState ideal;
        Check("body_ideal_error", Near(aq::HomeostaticError(ideal), 0.0), "ideal body should have zero homeostatic error");
    }

    {
        aq::AceAqBodyState body;
        const auto decayed = aq::NaturalDecay(body);
        Check("body_decay", decayed.hydration < body.hydration && decayed.nutrition < body.nutrition, "hydration/nutrition should decay");
    }

    {
        aq::AceAqBodyState body;
        body.ApplyDelta({10.0, -10.0, 2.0, -2.0});
        Check("body_apply_delta_clamp", Near(body.hydration, 1.0) && Near(body.nutrition, 0.0) && Near(body.integrity, 1.0) && Near(body.temperature, 0.0), "body values not clamped");
    }

    {
        auto world = aq::AceAqGridWorld::FromAscii({"#####", "#>#.#", "#####"});
        aq::AceAqEnvironment env;
        env.Reset(world);
        auto result = env.Step(aq::AceAqAction::MoveForward);
        Check("wall_blocks", result.lastActionResult.blocked && HasFlag(result.lastActionResult, "move_blocked"), "wall should block move");
    }

    {
        auto world = aq::AceAqGridWorld::FromAscii({"#####", "#>..#", "#####"});
        aq::AceAqEnvironment env;
        env.Reset(world);
        auto result = env.Step(aq::AceAqAction::MoveForward);
        Check("empty_allows_move", result.lastActionResult.moved && env.World().AgentPosition() == aq::AceAqPoint{2, 1}, "empty cell should allow move");
    }

    {
        auto world = aq::AceAqGridWorld::FromAscii({"#####", "#>..#", "#####"});
        aq::AceAqEnvironment env;
        env.Reset(world);
        env.Step(aq::AceAqAction::TurnLeft);
        Check("turn_left_changes_direction", env.World().AgentDirection() == aq::AceAqDirection::North, "turn left from east should face north");
        env.Step(aq::AceAqAction::TurnRight);
        Check("turn_right_changes_direction", env.World().AgentDirection() == aq::AceAqDirection::East, "turn right from north should face east");
    }

    {
        auto world = aq::AceAqGridWorld::FromAscii({"#####", "#>X.#", "#####"});
        aq::AceAqEnvironment env;
        env.Reset(world);
        const double before = env.GetBodyState().integrity;
        auto result = env.Step(aq::AceAqAction::TouchFront);
        Check("touch_acid_integrity", env.GetBodyState().integrity < before && HasFlag(result.lastActionResult, "touch"), "touching acid should reduce integrity");
    }

    {
        auto world = aq::AceAqGridWorld::FromAscii({"#####", "#>W.#", "#####"});
        aq::AceAqBodyState body;
        body.hydration = 0.30;
        aq::AceAqEnvironment env;
        env.Reset(world, body);
        auto result = env.Step(aq::AceAqAction::ConsumeFront);
        const auto debug = env.GetDebugTruthJsonLike();
        Check("water_consume_hydration", env.GetBodyState().hydration > 0.30 && HasFlag(result.lastActionResult, "consume_success"), "water should increase hydration");
        Check("water_consume_cell_empty", debug.find("\"EMPTY\"") != std::string::npos && debug.find("\"WATER\"") == std::string::npos, "water cell should become empty");
    }

    {
        auto world = aq::AceAqGridWorld::FromAscii({"#####", "#>#.#", "#####"});
        aq::AceAqEnvironment env;
        env.Reset(world);
        auto result = env.Step(aq::AceAqAction::ConsumeFront);
        Check("consume_wall_fails", result.lastActionResult.blocked && HasFlag(result.lastActionResult, "consume_failed"), "wall consume should fail");
    }

    {
        auto world = aq::AceAqGridWorld::FromAscii({"######", "#>S..#", "######"});
        aq::AceAqEnvironment env;
        env.Reset(world);
        auto result = env.Step(aq::AceAqAction::PushFront);
        const auto debug = env.GetDebugTruthJsonLike();
        Check("push_stone_success", result.lastActionResult.pushed && HasFlag(result.lastActionResult, "push_success"), "stone should push into empty space");
        Check("push_stone_agent_stays", env.World().AgentPosition() == aq::AceAqPoint{1, 1}, "push should not move agent in M2");
        Check("push_stone_truth_has_stone", debug.find("\"STONE\"") != std::string::npos, "debug truth should still contain stone after push");
    }

    {
        auto world = aq::AceAqGridWorld::FromAscii({"#####", "#>W.#", "#####"});
        aq::AceAqEnvironment env;
        env.Reset(world);
        auto result = env.Step(aq::AceAqAction::ConsumeFront);
        Check("episode_memory_grows", env.GetEpisodeMemory().Size() == 1, "memory should have one episode after one step");
        Check("episode_actual_delta", result.episode.actualDelta.hydration > 0.0, "actual_delta should reflect hydration increase minus decay");
    }

    {
        auto world = aq::AceAqGridWorld::FromAscii({"#####", "#>W.#", "#####"});
        aq::AceAqEnvironment env;
        env.Reset(world);
        const auto obsJson = aq::ToJsonLikeString(env.GetObservation());
        Check("observation_privacy", !aq::ObservationJsonContainsWorldTruthLabels(obsJson), "observation contains world truth labels");
        const auto truthJson = env.GetDebugTruthJsonLike();
        Check("debug_truth_separate", truthJson.find("\"WATER\"") != std::string::npos, "debug truth should expose evaluator-side labels");
    }

    {
        const auto logPath = std::filesystem::path("Build/Logs/ace_aquarium_episodes_probe.jsonl");
        std::filesystem::remove(logPath);
        auto world = aq::AceAqGridWorld::FromAscii({"#####", "#>F.#", "#####"});
        aq::AceAqEnvironment env;
        env.Reset(world);
        env.EnableEpisodeLogging(logPath);
        env.Step(aq::AceAqAction::ConsumeFront);

        std::ifstream input(logPath);
        std::string line;
        std::getline(input, line);
        Check("episode_logger_jsonl", !line.empty() && line.front() == '{' && line.find("\"episode_id\"") != std::string::npos, "logger did not write valid-ish JSONL");
    }

    return g_failures == 0 ? 0 : 1;
}
