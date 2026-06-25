#include "ArhqenCognitionEngine/Aquarium/AceAqSymbol.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace ace::aquarium
{
    std::string NormalizeSymbol(const std::string& symbol)
    {
        std::string result;
        result.reserve(symbol.size());
        for (const unsigned char ch : symbol)
        {
            result.push_back(static_cast<char>(std::tolower(ch)));
        }
        return result;
    }

    void AceAqSymbolTable::BindSymbolToConcept(const std::string& symbol, int conceptId, double confidence)
    {
        const auto normalized = NormalizeSymbol(symbol);
        AceAqSymbolBinding binding{symbol, conceptId, confidence};
        bySymbol_[normalized].push_back(binding);
        byConcept_[conceptId].push_back(symbol);
    }

    std::vector<AceAqSymbolActivation> AceAqSymbolTable::ActivateSymbol(const std::string& textOrSymbol) const
    {
        const auto normalized = NormalizeSymbol(textOrSymbol);
        std::vector<AceAqSymbolActivation> activations;

        const auto found = bySymbol_.find(normalized);
        if (found == bySymbol_.end())
        {
            return activations;
        }

        for (const auto& binding : found->second)
        {
            activations.push_back({binding.symbol, binding.conceptId, binding.confidence});
        }

        return activations;
    }

    std::vector<std::string> AceAqSymbolTable::GetSymbolsForConcept(int conceptId) const
    {
        const auto found = byConcept_.find(conceptId);
        if (found == byConcept_.end())
        {
            return {};
        }

        return found->second;
    }

    std::vector<int> AceAqSymbolTable::GetConceptIdsForSymbol(const std::string& symbol) const
    {
        std::vector<int> ids;
        const auto found = bySymbol_.find(NormalizeSymbol(symbol));
        if (found == bySymbol_.end())
        {
            return ids;
        }

        for (const auto& binding : found->second)
        {
            ids.push_back(binding.conceptId);
        }

        return ids;
    }

    int AceAqSymbolTable::BindingCount() const
    {
        int count = 0;
        for (const auto& item : bySymbol_)
        {
            count += static_cast<int>(item.second.size());
        }
        return count;
    }

    std::string ToJsonLikeString(const AceAqSymbolBinding& binding)
    {
        return "{" +
            std::string("\"symbol\":") + JsonString(binding.symbol) + "," +
            "\"concept_id\":" + std::to_string(binding.conceptId) + "," +
            "\"confidence\":" + JsonNumber(binding.confidence) +
            "}";
    }

    std::string ToJsonLikeString(const AceAqSymbolActivation& activation)
    {
        return "{" +
            std::string("\"symbol\":") + JsonString(activation.symbol) + "," +
            "\"concept_id\":" + std::to_string(activation.conceptId) + "," +
            "\"activation\":" + JsonNumber(activation.activation) +
            "}";
    }
}
