#include "ArhqenCognitionEngine/AquariumUI/AceAquariumRuntimeController.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

namespace ace::aquarium_ui
{
    namespace aq = ace::aquarium;

    namespace
    {
        std::string Fixed(double value, int precision = 3)
        {
            std::ostringstream out;
            out << std::fixed << std::setprecision(precision) << value;
            return out.str();
        }

        std::string BodyCompact(const aq::AceAqBodyState& body)
        {
            return "H=" + Fixed(body.hydration, 2) +
                   " N=" + Fixed(body.nutrition, 2) +
                   " I=" + Fixed(body.integrity, 2) +
                   " T=" + Fixed(body.temperature, 2);
        }

        std::string CellLine(const std::string& label, const aq::AceAqCellObservation& cell)
        {
            return label +
                ": solid=" + std::string(cell.solidHint ? "1" : "0") +
                " liquid=" + std::string(cell.liquidLikeHint ? "1" : "0") +
                " color=(" + std::to_string(cell.colorRgb.r) + "," + std::to_string(cell.colorRgb.g) + "," + std::to_string(cell.colorRgb.b) + ")" +
                " smell=" + Fixed(cell.smellSignal, 2) +
                " temp=" + Fixed(cell.temperatureSignal, 2);
        }

        bool HasFlag(const aq::AceAqLastActionResult& result, const std::string& flag)
        {
            return std::find(result.eventFlags.begin(), result.eventFlags.end(), flag) != result.eventFlags.end();
        }

        std::string FlagsLine(const std::vector<std::string>& flags)
        {
            if (flags.empty())
            {
                return "flags: none";
            }

            std::ostringstream out;
            out << "flags:";
            for (const auto& flag : flags)
            {
                out << " " << flag;
            }
            return out.str();
        }

        std::uint32_t NextRandom(std::uint32_t& state)
        {
            state = state * 1664525u + 1013904223u;
            return state;
        }
    }

    bool AceAquariumRuntimeController::Initialize()
    {
        registry_ = aq::DefaultAceAqScenarioRegistry();
        initialized_ = true;
        scenarioName_ = registry_.HasScenario("moving_hazard") ? "moving_hazard" : "water_front";
        plannerName_ = "counterfactual";
        seed_ = 123;
        randomState_ = seed_;
        return ResetScenario(scenarioName_, seed_);
    }

    bool AceAquariumRuntimeController::ResetScenario(const std::string& scenarioName, std::uint32_t seed)
    {
        if (!initialized_)
        {
            registry_ = aq::DefaultAceAqScenarioRegistry();
            initialized_ = true;
        }

        if (!registry_.HasScenario(scenarioName))
        {
            AddLog(AceAquariumLogType::Error, "unknown scenario: " + scenarioName);
            return false;
        }

        scenarioName_ = scenarioName;
        seed_ = seed;
        randomState_ = seed == 0 ? 1u : seed;
        running_ = false;
        tickAccumulatorSeconds_ = 0.0;
        stepIndex_ = 0;
        worldModel_ = {};
        selfModel_.Clear();
        protoConcepts_.clear();
        lastDecisionTrace_ = {};
        lastCounterfactualTrace_ = {};
        lastEpisode_ = {};

        auto build = registry_.Build(scenarioName_, seed_);
        environment_ = std::move(build.environment);
        logModel_.Clear();
        AddLog(AceAquariumLogType::Sys, "reset scenario=" + scenarioName_ + " seed=" + std::to_string(seed_));
        return true;
    }

    bool AceAquariumRuntimeController::SetPlanner(const std::string& plannerName)
    {
        if (plannerName != "random" && plannerName != "safe" && plannerName != "counterfactual")
        {
            AddLog(AceAquariumLogType::Error, "unknown planner: " + plannerName);
            return false;
        }

        plannerName_ = plannerName;
        AddLog(AceAquariumLogType::Plan, "planner=" + plannerName_);
        return true;
    }

    void AceAquariumRuntimeController::SetRunning(bool running)
    {
        running_ = running;
        AddLog(AceAquariumLogType::Sys, running_ ? "run" : "pause");
    }

    bool AceAquariumRuntimeController::IsRunning() const
    {
        return running_;
    }

    void AceAquariumRuntimeController::SetDebugTruthEnabled(bool enabled)
    {
        debugTruthEnabled_ = enabled;
        AddLog(AceAquariumLogType::Sys, enabled ? "debug truth enabled" : "debug truth disabled");
    }

    bool AceAquariumRuntimeController::DebugTruthEnabled() const
    {
        return debugTruthEnabled_;
    }

    aq::AceAqAction AceAquariumRuntimeController::ChoosePlannerAction()
    {
        if (plannerName_ == "random")
        {
            const auto actions = aq::AllAceAqActions();
            return actions[NextRandom(randomState_) % actions.size()];
        }

        const auto observation = environment_.GetObservation();
        const auto body = environment_.GetBodyState();

        if (plannerName_ == "safe")
        {
            lastDecisionTrace_ = safePlanner_.ChooseAction(body, observation, worldModel_);
            AddLog(AceAquariumLogType::Plan, "safe chose " + aq::ToString(lastDecisionTrace_.chosenAction));
            return lastDecisionTrace_.chosenAction;
        }

        lastCounterfactualTrace_ = counterfactualPlanner_.ChoosePlan(body, observation, worldModel_);
        lastDecisionTrace_ = safePlanner_.ChooseAction(body, observation, worldModel_);
        AddLog(AceAquariumLogType::Counterfactual, "counterfactual chose " + aq::ToString(lastCounterfactualTrace_.chosenAction));
        return lastCounterfactualTrace_.chosenAction;
    }

    bool AceAquariumRuntimeController::StepOnce()
    {
        return ApplyStep(ChoosePlannerAction(), false);
    }

    bool AceAquariumRuntimeController::StepManual(aq::AceAqAction action)
    {
        return ApplyStep(action, true);
    }

    void AceAquariumRuntimeController::Tick(double deltaSeconds)
    {
        if (!running_)
        {
            return;
        }

        const double stepInterval = 1.0 / std::max(0.25, maxStepsPerSecond_);
        tickAccumulatorSeconds_ += std::max(0.0, deltaSeconds);

        int guard = 0;
        while (tickAccumulatorSeconds_ >= stepInterval && guard < 4)
        {
            tickAccumulatorSeconds_ -= stepInterval;
            StepOnce();
            ++guard;
        }
    }

    bool AceAquariumRuntimeController::ApplyStep(aq::AceAqAction action, bool manual)
    {
        const auto result = environment_.Step(action);
        lastEpisode_ = result.episode;
        worldModel_.LearnFromEpisode(lastEpisode_);
        selfModel_.ObserveEpisode(lastEpisode_);
        RebuildConcepts();

        stepIndex_ += 1;

        AddLog(AceAquariumLogType::Step, std::string(manual ? "manual " : "planner ") + aq::ToString(action));
        AddEpisodeDerivedLogs(lastEpisode_, action, manual);

        if (result.terminated)
        {
            running_ = false;
            AddLog(AceAquariumLogType::Error, "episode terminated");
        }

        return true;
    }

    void AceAquariumRuntimeController::RebuildConcepts()
    {
        protoConcepts_ = protoMiner_.Mine(environment_.GetEpisodeMemory());
        symbolTable_ = {};
        int bindingBudget = 0;
        for (const auto& proto : protoConcepts_)
        {
            if (bindingBudget >= 8)
            {
                break;
            }
            symbolTable_.BindSymbolToConcept("effect_" + std::to_string(proto.conceptId), proto.conceptId, 0.75);
            ++bindingBudget;
        }
    }

    void AceAquariumRuntimeController::AddEpisodeDerivedLogs(const aq::AceAqEpisode& episode, aq::AceAqAction action, bool)
    {
        AddLog(AceAquariumLogType::Action, aq::ToString(action) + " -> " + episode.lastActionResult.reason);
        AddLog(AceAquariumLogType::Body, BodyCompact(episode.bodyAfter));

        if (!episode.scheduledDelayedEffects.empty())
        {
            AddLog(AceAquariumLogType::Delay, "scheduled delayed effects=" + std::to_string(episode.scheduledDelayedEffects.size()));
        }

        if (!episode.appliedDelayedEffects.empty())
        {
            AddLog(AceAquariumLogType::Delay, "applied delayed effects=" + std::to_string(episode.appliedDelayedEffects.size()));
        }

        if (!episode.externalWorldEvents.empty())
        {
            AddLog(AceAquariumLogType::World, "world events=" + std::to_string(episode.externalWorldEvents.size()));
        }

        const auto self = selfModel_.State();
        AddLog(AceAquariumLogType::Self, "agency=" + Fixed(self.agencyConfidence, 2) + " external=" + std::to_string(self.externalEvents));

        if (!protoConcepts_.empty())
        {
            AddLog(AceAquariumLogType::Concept, "proto concepts=" + std::to_string(protoConcepts_.size()));
        }
    }

    void AceAquariumRuntimeController::AddLog(AceAquariumLogType type, std::string message)
    {
        logModel_.Add(stepIndex_, type, SanitizeAgentFacing(std::move(message)));
    }

    std::string AceAquariumRuntimeController::SanitizeAgentFacing(std::string text) const
    {
        if (!ContainsAquariumTruthLabel(text))
        {
            return text;
        }

        return "[redacted evaluator truth label]";
    }

    AceAquariumUiSnapshot AceAquariumRuntimeController::BuildSnapshot() const
    {
        AceAquariumUiSnapshot snapshot;
        snapshot.step = stepIndex_;
        snapshot.scenarioName = scenarioName_;
        snapshot.plannerName = plannerName_;
        snapshot.running = running_;
        snapshot.body = environment_.GetBodyState();
        snapshot.homeostaticError = aq::HomeostaticError(snapshot.body);
        snapshot.scenarioNames = ScenarioNames();
        snapshot.plannerNames = {"random", "safe", "counterfactual"};

        const auto agent = environment_.World().AgentPosition();
        snapshot.agentPositionText = "x=" + std::to_string(agent.x) + " y=" + std::to_string(agent.y);
        snapshot.agentDirectionText = aq::ToString(environment_.World().AgentDirection());

        snapshot.lastActionText = aq::ToString(lastEpisode_.action);
        snapshot.lastResultText = lastEpisode_.lastActionResult.reason.empty() ? "none" : SanitizeAgentFacing(lastEpisode_.lastActionResult.reason);

        const auto obs = environment_.GetObservation();
        snapshot.observationLines = {
            CellLine("front", obs.front),
            CellLine("current", obs.current),
            CellLine("left", obs.left),
            CellLine("right", obs.right)
        };

        if (!lastDecisionTrace_.evaluations.empty())
        {
            snapshot.decisionTraceLines.push_back("chosen=" + aq::ToString(lastDecisionTrace_.chosenAction));
            int count = 0;
            for (const auto& eval : lastDecisionTrace_.evaluations)
            {
                if (count++ >= 4) break;
                snapshot.decisionTraceLines.push_back(
                    aq::ToString(eval.action) +
                    " score=" + Fixed(eval.score, 3) +
                    " risk=" + Fixed(eval.risk, 3) +
                    " info=" + Fixed(eval.informationGain, 3) +
                    " conf=" + Fixed(eval.confidence, 3)
                );
            }
        }
        else
        {
            snapshot.decisionTraceLines.push_back("no decision trace yet");
        }

        if (!lastCounterfactualTrace_.candidatePlans.empty())
        {
            snapshot.counterfactualTraceLines.push_back("chosen=" + aq::ToString(lastCounterfactualTrace_.chosenAction));
            snapshot.counterfactualTraceLines.push_back("candidates=" + std::to_string(lastCounterfactualTrace_.candidatePlans.size()));
            int count = 0;
            for (const auto& plan : lastCounterfactualTrace_.candidatePlans)
            {
                if (count++ >= 3) break;
                std::string actions;
                for (const auto action : plan.actions)
                {
                    if (!actions.empty()) actions += " -> ";
                    actions += aq::ToString(action);
                }
                snapshot.counterfactualTraceLines.push_back(actions + " score=" + Fixed(plan.score, 3) + " risk=" + Fixed(plan.risk, 3));
            }
            for (const auto& note : lastCounterfactualTrace_.notes)
            {
                snapshot.counterfactualTraceLines.push_back("note=" + note);
            }
        }
        else
        {
            snapshot.counterfactualTraceLines.push_back("no counterfactual trace yet");
        }

        snapshot.protoConceptLines.push_back("count=" + std::to_string(protoConcepts_.size()));
        int conceptCount = 0;
        for (const auto& proto : protoConcepts_)
        {
            if (conceptCount++ >= 5) break;
            snapshot.protoConceptLines.push_back("concept#" + std::to_string(proto.conceptId) + " support=" + std::to_string(proto.supportCount));
        }

        const auto self = selfModel_.State();
        snapshot.selfModelLines = {
            "episodes=" + std::to_string(self.totalEpisodes),
            "owned=" + std::to_string(self.ownedEpisodes),
            "external=" + std::to_string(self.externalEvents),
            "agency=" + Fixed(self.agencyConfidence, 3),
            "symbol_bindings=" + std::to_string(symbolTable_.BindingCount())
        };

        snapshot.delayedEffectLines.push_back("pending=" + std::to_string(environment_.DelayedEffects().PendingCount()));
        snapshot.delayedEffectLines.push_back("scheduled_last=" + std::to_string(lastEpisode_.scheduledDelayedEffects.size()));
        snapshot.delayedEffectLines.push_back("applied_last=" + std::to_string(lastEpisode_.appliedDelayedEffects.size()));

        snapshot.worldEventLines.push_back("last_step_events=" + std::to_string(lastEpisode_.externalWorldEvents.size()));
        for (std::size_t i = 0; i < lastEpisode_.externalWorldEvents.size() && i < 4; ++i)
        {
            snapshot.worldEventLines.push_back("event#" + std::to_string(i + 1) + " flags=" + std::to_string(lastEpisode_.externalWorldEvents[i].eventFlags.size()));
        }

        const auto metrics = aq::ComputeRunMetrics(environment_.GetEpisodeMemory());
        snapshot.metricLines = {
            "episodes=" + std::to_string(metrics.snapshot.steps),
            "consumed=" + std::to_string(metrics.snapshot.consumedCount),
            "blocked=" + std::to_string(metrics.snapshot.blockedCount),
            "delayed_applied=" + std::to_string(metrics.snapshot.appliedDelayedEffectCount),
            "world_events=" + std::to_string(metrics.snapshot.externalWorldEventCount),
            "dynamic_damage=" + std::to_string(metrics.snapshot.dynamicDamageCount)
        };

        for (const auto& entry : logModel_.Recent(18))
        {
            snapshot.logLines.push_back("[" + ToString(entry.type) + "] " + std::to_string(entry.step) + " " + entry.message);
        }

        snapshot.debugTruthEnabled = debugTruthEnabled_;
        if (debugTruthEnabled_)
        {
            snapshot.debugTruthLines.push_back("DEBUG TRUTH - NOT AGENT INPUT");
            snapshot.debugTruthLines.push_back(environment_.GetDebugTruthJsonLike());
        }

        return snapshot;
    }

    const std::vector<AceAquariumLogEntry>& AceAquariumRuntimeController::GetRecentLogs() const
    {
        return logModel_.Entries();
    }

    std::vector<std::string> AceAquariumRuntimeController::ScenarioNames() const
    {
        return registry_.ListNames();
    }
}
