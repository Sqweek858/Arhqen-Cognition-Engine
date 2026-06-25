#include "ArhqenCognitionEngine/AquariumUI/AceAquariumLogModel.h"

#include <algorithm>
#include <utility>

namespace ace::aquarium_ui
{
    AceAquariumLogModel::AceAquariumLogModel(std::size_t maxEntries)
        : maxEntries_(std::max<std::size_t>(1, maxEntries))
    {
    }

    void AceAquariumLogModel::Clear()
    {
        entries_.clear();
    }

    void AceAquariumLogModel::Add(int step, AceAquariumLogType type, std::string message)
    {
        entries_.push_back({step, type, std::move(message)});
        while (entries_.size() > maxEntries_)
        {
            entries_.erase(entries_.begin());
        }
    }

    const std::vector<AceAquariumLogEntry>& AceAquariumLogModel::Entries() const
    {
        return entries_;
    }

    std::vector<AceAquariumLogEntry> AceAquariumLogModel::Recent(std::size_t maxCount) const
    {
        if (maxCount >= entries_.size())
        {
            return entries_;
        }

        return {entries_.end() - static_cast<std::ptrdiff_t>(maxCount), entries_.end()};
    }

    std::string ToString(AceAquariumLogType type)
    {
        switch (type)
        {
        case AceAquariumLogType::Sys: return "SYS";
        case AceAquariumLogType::Step: return "STEP";
        case AceAquariumLogType::Action: return "ACTION";
        case AceAquariumLogType::Body: return "BODY";
        case AceAquariumLogType::Plan: return "PLAN";
        case AceAquariumLogType::Counterfactual: return "CF";
        case AceAquariumLogType::Concept: return "CONCEPT";
        case AceAquariumLogType::Self: return "SELF";
        case AceAquariumLogType::Delay: return "DELAY";
        case AceAquariumLogType::World: return "WORLD";
        case AceAquariumLogType::Error: return "ERR";
        }

        return "SYS";
    }
}
