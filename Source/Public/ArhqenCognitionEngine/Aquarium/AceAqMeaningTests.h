#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqExperiment.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqProtoConcept.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqSelfModel.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqSymbol.h"

#include <map>
#include <string>
#include <vector>

namespace ace::aquarium
{
    struct AceAqScenarioResult
    {
        std::string name;
        bool passed = false;
        std::map<std::string, double> metrics;
        std::string agentFacingSummary;
    };

    struct AceAqMeaningTestResult
    {
        std::string name;
        bool passed = false;
        std::map<std::string, double> metrics;
        std::vector<std::string> notes;
        std::string agentFacingSummary;
    };

    class AceAqMeaningTestRunner
    {
    public:
        explicit AceAqMeaningTestRunner(AceAqScenarioRegistry registry = DefaultAceAqScenarioRegistry());
        std::vector<AceAqMeaningTestResult> RunAll() const;

        AceAqMeaningTestResult RunColorSwap() const;
        AceAqMeaningTestResult RunSameAppearanceDifferentEffect() const;
        AceAqMeaningTestResult RunContextFlip() const;
        AceAqMeaningTestResult RunUnknownLiquidSafety() const;
        AceAqMeaningTestResult RunSymbolGroundingSanity() const;
        AceAqMeaningTestResult RunSelfModelSanity() const;
        AceAqMeaningTestResult RunIntegratedContextualRun() const;

    private:
        AceAqScenarioRegistry registry_;
    };

    std::string ToJsonLikeString(const AceAqMeaningTestResult& result);
}
