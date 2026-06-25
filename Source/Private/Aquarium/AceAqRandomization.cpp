#include "ArhqenCognitionEngine/Aquarium/AceAqRandomization.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace ace::aquarium
{
    namespace
    {
        int ClampByte(int value)
        {
            return std::clamp(value, 0, 255);
        }

        double ClampSigned(double value)
        {
            return std::clamp(value, -1.0, 1.0);
        }
    }

    AceAqObjectPropertyRandomizer::AceAqObjectPropertyRandomizer(AceAqRandomizationConfig config)
        : config_(config)
        , state_(config.seed == 0 ? 1u : config.seed)
    {
    }

    double AceAqObjectPropertyRandomizer::NextUnit()
    {
        state_ = state_ * 1664525u + 1013904223u;
        return static_cast<double>((state_ >> 8) & 0x00FFFFFFu) / static_cast<double>(0x00FFFFFFu);
    }

    int AceAqObjectPropertyRandomizer::JitterChannel(int value)
    {
        const int span = std::max(0, config_.colorJitter);
        const int delta = static_cast<int>(std::round((NextUnit() * 2.0 - 1.0) * span));
        return ClampByte(value + delta);
    }

    double AceAqObjectPropertyRandomizer::JitterSignal(double value, double amount)
    {
        const double delta = (NextUnit() * 2.0 - 1.0) * amount;
        return ClampSigned(value + delta);
    }

    AceAqCellObservation AceAqObjectPropertyRandomizer::RandomizeCell(const AceAqCellObservation& cell)
    {
        if (!config_.enabled)
        {
            return cell;
        }

        AceAqCellObservation out = cell;
        out.colorRgb.r = JitterChannel(out.colorRgb.r);
        out.colorRgb.g = JitterChannel(out.colorRgb.g);
        out.colorRgb.b = JitterChannel(out.colorRgb.b);
        out.smellSignal = JitterSignal(out.smellSignal, config_.smellJitter);
        out.temperatureSignal = JitterSignal(out.temperatureSignal, config_.temperatureJitter);

        if (config_.allowCrossColorLiquids && out.liquidLikeHint)
        {
            // Appearance-only cross-color mapping. Causal effects remain in the environment.
            const int oldR = out.colorRgb.r;
            out.colorRgb.r = out.colorRgb.g;
            out.colorRgb.g = out.colorRgb.b;
            out.colorRgb.b = oldR;
        }

        return out;
    }

    AceAqObservation AceAqObjectPropertyRandomizer::RandomizeObservation(const AceAqObservation& observation)
    {
        AceAqObservation out = observation;
        for (auto& cell : out.localView)
        {
            cell = RandomizeCell(cell);
        }
        out.front = RandomizeCell(out.front);
        out.left = RandomizeCell(out.left);
        out.right = RandomizeCell(out.right);
        out.current = RandomizeCell(out.current);
        return out;
    }

    std::string AceAqRandomizationToJsonLike(const AceAqRandomizationConfig& config)
    {
        return "{" +
            std::string("\"enabled\":") + JsonBool(config.enabled) + "," +
            "\"seed\":" + std::to_string(config.seed) + "," +
            "\"color_jitter\":" + std::to_string(config.colorJitter) + "," +
            "\"smell_jitter\":" + JsonNumber(config.smellJitter) + "," +
            "\"temperature_jitter\":" + JsonNumber(config.temperatureJitter) + "," +
            "\"allow_cross_color_liquids\":" + JsonBool(config.allowCrossColorLiquids) + "," +
            "\"preserve_causal_effects\":" + JsonBool(config.preserveCausalEffects) +
            "}";
    }
}
