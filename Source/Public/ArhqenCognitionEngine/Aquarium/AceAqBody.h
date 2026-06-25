#pragma once

#include <array>
#include <string>

namespace ace::aquarium
{
    struct AceAqBodyDelta
    {
        double hydration = 0.0;
        double nutrition = 0.0;
        double integrity = 0.0;
        double temperature = 0.0;
    };

    struct AceAqBodyState
    {
        double hydration = 0.75;
        double nutrition = 0.70;
        double integrity = 1.00;
        double temperature = 0.50;

        void Clamp();
        AceAqBodyState Clamped() const;
        void ApplyDelta(const AceAqBodyDelta& delta);
    };

    struct AceAqIdealRange
    {
        double low = 0.0;
        double high = 1.0;
        double weight = 1.0;
    };

    struct AceAqComponentErrors
    {
        double hydration = 0.0;
        double nutrition = 0.0;
        double integrity = 0.0;
        double temperature = 0.0;
    };

    double Clamp01(double value);
    AceAqComponentErrors ComponentErrors(const AceAqBodyState& body);
    double HomeostaticError(const AceAqBodyState& body);
    AceAqBodyState NaturalDecay(const AceAqBodyState& body);
    bool IsDead(const AceAqBodyState& body);
    double HomeostaticReward(const AceAqBodyState& previousBody, const AceAqBodyState& currentBody);
    AceAqBodyDelta Difference(const AceAqBodyState& before, const AceAqBodyState& after);

    std::string ToJsonLikeString(const AceAqBodyDelta& delta);
    std::string ToJsonLikeString(const AceAqBodyState& body);
    std::string ToJsonLikeString(const AceAqComponentErrors& errors);
}
