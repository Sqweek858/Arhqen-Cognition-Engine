#include "ArhqenCognitionEngine/Aquarium/AceAqBody.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace ace::aquarium
{
    namespace
    {
        constexpr AceAqIdealRange HydrationIdeal{0.65, 0.85, 1.0};
        constexpr AceAqIdealRange NutritionIdeal{0.55, 0.80, 1.0};
        constexpr AceAqIdealRange IntegrityIdeal{0.80, 1.00, 2.0};
        constexpr AceAqIdealRange TemperatureIdeal{0.45, 0.60, 1.0};

        double RangeError(double value, AceAqIdealRange range)
        {
            if (value < range.low)
            {
                return (range.low - value) * range.weight;
            }

            if (value > range.high)
            {
                return (value - range.high) * range.weight;
            }

            return 0.0;
        }
    }

    double Clamp01(double value)
    {
        return std::clamp(value, 0.0, 1.0);
    }

    void AceAqBodyState::Clamp()
    {
        hydration = Clamp01(hydration);
        nutrition = Clamp01(nutrition);
        integrity = Clamp01(integrity);
        temperature = Clamp01(temperature);
    }

    AceAqBodyState AceAqBodyState::Clamped() const
    {
        auto copy = *this;
        copy.Clamp();
        return copy;
    }

    void AceAqBodyState::ApplyDelta(const AceAqBodyDelta& delta)
    {
        hydration += delta.hydration;
        nutrition += delta.nutrition;
        integrity += delta.integrity;
        temperature += delta.temperature;
        Clamp();
    }

    AceAqComponentErrors ComponentErrors(const AceAqBodyState& body)
    {
        return {
            RangeError(body.hydration, HydrationIdeal),
            RangeError(body.nutrition, NutritionIdeal),
            RangeError(body.integrity, IntegrityIdeal),
            RangeError(body.temperature, TemperatureIdeal),
        };
    }

    double HomeostaticError(const AceAqBodyState& body)
    {
        const auto errors = ComponentErrors(body);
        return errors.hydration + errors.nutrition + errors.integrity + errors.temperature;
    }

    AceAqBodyState NaturalDecay(const AceAqBodyState& body)
    {
        auto result = body;
        result.hydration -= 0.005;
        result.nutrition -= 0.003;
        result.temperature += (0.50 - result.temperature) * 0.02;
        result.Clamp();
        return result;
    }

    bool IsDead(const AceAqBodyState& body)
    {
        return body.hydration <= 0.0 ||
               body.nutrition <= 0.0 ||
               body.integrity <= 0.0 ||
               body.temperature <= 0.0 ||
               body.temperature >= 1.0;
    }

    double HomeostaticReward(const AceAqBodyState& previousBody, const AceAqBodyState& currentBody)
    {
        return HomeostaticError(previousBody) - HomeostaticError(currentBody);
    }

    AceAqBodyDelta Difference(const AceAqBodyState& before, const AceAqBodyState& after)
    {
        return {
            after.hydration - before.hydration,
            after.nutrition - before.nutrition,
            after.integrity - before.integrity,
            after.temperature - before.temperature,
        };
    }

    std::string ToJsonLikeString(const AceAqBodyDelta& delta)
    {
        return "{" +
            std::string("\"hydration\":") + JsonNumber(delta.hydration) + "," +
            "\"nutrition\":" + JsonNumber(delta.nutrition) + "," +
            "\"integrity\":" + JsonNumber(delta.integrity) + "," +
            "\"temperature\":" + JsonNumber(delta.temperature) +
            "}";
    }

    std::string ToJsonLikeString(const AceAqBodyState& body)
    {
        return "{" +
            std::string("\"hydration\":") + JsonNumber(body.hydration) + "," +
            "\"nutrition\":" + JsonNumber(body.nutrition) + "," +
            "\"integrity\":" + JsonNumber(body.integrity) + "," +
            "\"temperature\":" + JsonNumber(body.temperature) +
            "}";
    }

    std::string ToJsonLikeString(const AceAqComponentErrors& errors)
    {
        return "{" +
            std::string("\"hydration\":") + JsonNumber(errors.hydration) + "," +
            "\"nutrition\":" + JsonNumber(errors.nutrition) + "," +
            "\"integrity\":" + JsonNumber(errors.integrity) + "," +
            "\"temperature\":" + JsonNumber(errors.temperature) +
            "}";
    }
}
