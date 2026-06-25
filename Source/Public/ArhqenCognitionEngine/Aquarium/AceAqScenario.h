#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqEnvironment.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ace::aquarium
{
    struct AceAqScenarioDefinition
    {
        std::string name;
        std::string description;
        std::vector<std::string> rows;
        AceAqBodyState initialBody{};
        std::vector<std::string> expectations;
        std::uint32_t seed = 0;
    };

    struct AceAqScenarioBuildResult
    {
        AceAqEnvironment environment;
        AceAqScenarioDefinition definition;
        std::vector<std::string> expectations;
    };

    class AceAqScenarioRegistry
    {
    public:
        void RegisterScenario(AceAqScenarioDefinition definition);
        bool HasScenario(const std::string& name) const;
        AceAqScenarioBuildResult Build(const std::string& name, std::uint32_t seed = 0) const;
        std::vector<std::string> ListNames() const;
        int Count() const;

    private:
        std::map<std::string, AceAqScenarioDefinition> scenarios_;
    };

    AceAqScenarioRegistry DefaultAceAqScenarioRegistry();
    std::vector<std::string> EvaluateScenarioExpectations(const AceAqScenarioBuildResult& build);

    std::string ToJsonLikeString(const AceAqScenarioDefinition& definition);
}
