#include "ArhqenCognitionEngine/Aquarium/AceAqSensorNoise.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <algorithm>
#include <cmath>

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

    AceAqSensorNoiseModel::AceAqSensorNoiseModel(AceAqSensorNoiseConfig config)
        : config_(config)
        , state_(config.seed == 0 ? 1u : config.seed)
    {
    }

    double AceAqSensorNoiseModel::NextUnit()
    {
        state_ = state_ * 1103515245u + 12345u;
        return static_cast<double>((state_ >> 8) & 0x00FFFFFFu) / static_cast<double>(0x00FFFFFFu);
    }

    int AceAqSensorNoiseModel::ApplyColorNoise(int value)
    {
        const int delta = static_cast<int>(std::round((NextUnit() * 2.0 - 1.0) * static_cast<double>(config_.colorNoise)));
        return ClampByte(value + delta);
    }

    double AceAqSensorNoiseModel::ApplySignalNoise(double value, double amount)
    {
        return ClampSigned(value + (NextUnit() * 2.0 - 1.0) * amount);
    }

    bool AceAqSensorNoiseModel::ShouldFlip()
    {
        return NextUnit() < config_.hintFlipProbability;
    }

    AceAqObservation AceAqSensorNoiseModel::ApplyNoise(const AceAqObservation& cleanObservation, AceAqNoisyObservationMetadata* outMetadata)
    {
        if (!config_.enabled)
        {
            if (outMetadata)
            {
                *outMetadata = {};
            }
            return cleanObservation;
        }

        AceAqObservation out = cleanObservation;
        bool flipped = false;

        auto applyCell = [&](AceAqCellObservation& cell)
        {
            cell.colorRgb.r = ApplyColorNoise(cell.colorRgb.r);
            cell.colorRgb.g = ApplyColorNoise(cell.colorRgb.g);
            cell.colorRgb.b = ApplyColorNoise(cell.colorRgb.b);
            cell.smellSignal = ApplySignalNoise(cell.smellSignal, config_.smellNoise);
            cell.temperatureSignal = ApplySignalNoise(cell.temperatureSignal, config_.temperatureNoise);
            if (ShouldFlip())
            {
                cell.solidHint = !cell.solidHint;
                flipped = true;
            }
            if (ShouldFlip())
            {
                cell.liquidLikeHint = !cell.liquidLikeHint;
                flipped = true;
            }
        };

        for (auto& cell : out.localView) applyCell(cell);
        applyCell(out.front);
        applyCell(out.left);
        applyCell(out.right);
        applyCell(out.current);

        if (outMetadata)
        {
            outMetadata->colorConfidence = std::clamp(1.0 - static_cast<double>(config_.colorNoise) / 255.0, 0.0, 1.0);
            outMetadata->smellConfidence = std::clamp(1.0 - config_.smellNoise, 0.0, 1.0);
            outMetadata->temperatureConfidence = std::clamp(1.0 - config_.temperatureNoise, 0.0, 1.0);
            outMetadata->hintFlipped = flipped;
        }

        return out;
    }

    std::string AceAqSensorNoiseToJsonLike(const AceAqSensorNoiseConfig& config)
    {
        return "{" +
            std::string("\"enabled\":") + JsonBool(config.enabled) + "," +
            "\"seed\":" + std::to_string(config.seed) + "," +
            "\"color_noise\":" + std::to_string(config.colorNoise) + "," +
            "\"smell_noise\":" + JsonNumber(config.smellNoise) + "," +
            "\"temperature_noise\":" + JsonNumber(config.temperatureNoise) + "," +
            "\"hint_flip_probability\":" + JsonNumber(config.hintFlipProbability) + "," +
            "\"include_metadata\":" + JsonBool(config.includeMetadata) +
            "}";
    }

    std::string ToJsonLikeString(const AceAqNoisyObservationMetadata& metadata)
    {
        return "{" +
            std::string("\"color_confidence\":") + JsonNumber(metadata.colorConfidence) + "," +
            "\"smell_confidence\":" + JsonNumber(metadata.smellConfidence) + "," +
            "\"temperature_confidence\":" + JsonNumber(metadata.temperatureConfidence) + "," +
            "\"hint_flipped\":" + JsonBool(metadata.hintFlipped) +
            "}";
    }
}
