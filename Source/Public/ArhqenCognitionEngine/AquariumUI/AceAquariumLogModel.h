#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ace::aquarium_ui
{
    enum class AceAquariumLogType
    {
        Sys,
        Step,
        Action,
        Body,
        Plan,
        Counterfactual,
        Concept,
        Self,
        Delay,
        World,
        Error
    };

    struct AceAquariumLogEntry
    {
        int step = 0;
        AceAquariumLogType type = AceAquariumLogType::Sys;
        std::string message;
    };

    class AceAquariumLogModel
    {
    public:
        explicit AceAquariumLogModel(std::size_t maxEntries = 80);

        void Clear();
        void Add(int step, AceAquariumLogType type, std::string message);
        const std::vector<AceAquariumLogEntry>& Entries() const;
        std::vector<AceAquariumLogEntry> Recent(std::size_t maxCount) const;

    private:
        std::size_t maxEntries_ = 80;
        std::vector<AceAquariumLogEntry> entries_;
    };

    std::string ToString(AceAquariumLogType type);
}
