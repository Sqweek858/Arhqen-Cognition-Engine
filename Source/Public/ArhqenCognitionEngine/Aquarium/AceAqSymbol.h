#pragma once

#include <map>
#include <string>
#include <vector>

namespace ace::aquarium
{
    struct AceAqSymbolBinding
    {
        std::string symbol;
        int conceptId = 0;
        double confidence = 1.0;
    };

    struct AceAqSymbolActivation
    {
        std::string symbol;
        int conceptId = 0;
        double activation = 0.0;
    };

    struct AceAqConceptActivation
    {
        int conceptId = 0;
        std::string symbol;
        double activation = 0.0;
    };

    class AceAqSymbolTable
    {
    public:
        void BindSymbolToConcept(const std::string& symbol, int conceptId, double confidence = 1.0);
        std::vector<AceAqSymbolActivation> ActivateSymbol(const std::string& textOrSymbol) const;
        std::vector<std::string> GetSymbolsForConcept(int conceptId) const;
        std::vector<int> GetConceptIdsForSymbol(const std::string& symbol) const;
        int BindingCount() const;

    private:
        std::map<std::string, std::vector<AceAqSymbolBinding>> bySymbol_;
        std::map<int, std::vector<std::string>> byConcept_;
    };

    std::string NormalizeSymbol(const std::string& symbol);
    std::string ToJsonLikeString(const AceAqSymbolBinding& binding);
    std::string ToJsonLikeString(const AceAqSymbolActivation& activation);
}
