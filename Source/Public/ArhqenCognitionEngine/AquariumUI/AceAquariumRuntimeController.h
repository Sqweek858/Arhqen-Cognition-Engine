#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqCounterfactual.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqEnvironment.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqExperiment.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqMetrics.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqPlanner.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqProtoConcept.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqScenario.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqSelfModel.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqSymbol.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqWorldModel.h"
#include "ArhqenCognitionEngine/AquariumUI/AceAquariumLogModel.h"
#include "ArhqenCognitionEngine/AquariumUI/AceAquariumUiSnapshot.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ace::aquarium_ui
{
    class AceAquariumRuntimeController
    {
    public:
        bool Initialize();
        bool ResetScenario(const std::string& scenarioName, std::uint32_t seed);
        bool SetPlanner(const std::string& plannerName);

        void SetRunning(bool running);
        bool IsRunning() const;

        void SetDebugTruthEnabled(bool enabled);
        bool DebugTruthEnabled() const;

        bool StepOnce();
        bool StepManual(ace::aquarium::AceAqAction action);
        void Tick(double deltaSeconds);

        AceAquariumUiSnapshot BuildSnapshot() const;
        const std::vector<AceAquariumLogEntry>& GetRecentLogs() const;
        std::vector<std::string> ScenarioNames() const;

        const std::string& CurrentScenarioName() const { return scenarioName_; }
        const std::string& CurrentPlannerName() const { return plannerName_; }
        int StepIndex() const { return stepIndex_; }
        const ace::aquarium::AceAqEnvironment& Environment() const { return environment_; }

    private:
        ace::aquarium::AceAqAction ChoosePlannerAction();
        bool ApplyStep(ace::aquarium::AceAqAction action, bool manual);
        void RebuildConcepts();
        void AddEpisodeDerivedLogs(const ace::aquarium::AceAqEpisode& episode, ace::aquarium::AceAqAction action, bool manual);
        void AddLog(AceAquariumLogType type, std::string message);
        std::string SanitizeAgentFacing(std::string text) const;

        ace::aquarium::AceAqScenarioRegistry registry_;
        ace::aquarium::AceAqEnvironment environment_;
        ace::aquarium::AceAqTableWorldModel worldModel_;
        ace::aquarium::AceAqSafeCuriosityPlanner safePlanner_;
        ace::aquarium::AceAqCounterfactualPlanner counterfactualPlanner_;
        ace::aquarium::AceAqProtoConceptMiner protoMiner_;
        ace::aquarium::AceAqSymbolTable symbolTable_;
        ace::aquarium::AceAqSelfModel selfModel_;
        std::vector<ace::aquarium::AceAqProtoConcept> protoConcepts_;

        ace::aquarium::AceAqDecisionTrace lastDecisionTrace_{};
        ace::aquarium::AceAqCounterfactualTrace lastCounterfactualTrace_{};
        ace::aquarium::AceAqEpisode lastEpisode_{};

        AceAquariumLogModel logModel_{120};

        std::string scenarioName_ = "moving_hazard";
        std::string plannerName_ = "counterfactual";
        std::uint32_t seed_ = 123;
        std::uint32_t randomState_ = 123;
        bool initialized_ = false;
        bool running_ = false;
        bool debugTruthEnabled_ = false;
        int stepIndex_ = 0;
        double tickAccumulatorSeconds_ = 0.0;
        double maxStepsPerSecond_ = 4.0;
    };
}
