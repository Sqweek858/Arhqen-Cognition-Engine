#include "ArhqenCognitionEngine/Aquarium/AceAqWorldModel.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <cmath>
#include <sstream>

namespace ace::aquarium
{
    namespace
    {
        int BucketInt(int value, int size)
        {
            return value / size;
        }

        int BucketDouble(double value, double size)
        {
            return static_cast<int>(std::floor((value + 1.0) / size));
        }

        std::string CellKey(const AceAqCellObservation& cell)
        {
            std::ostringstream out;
            out << "s" << (cell.solidHint ? 1 : 0)
                << "l" << (cell.liquidLikeHint ? 1 : 0)
                << "r" << BucketInt(cell.colorRgb.r, 64)
                << "g" << BucketInt(cell.colorRgb.g, 64)
                << "b" << BucketInt(cell.colorRgb.b, 64)
                << "t" << BucketDouble(cell.temperatureSignal, 0.25)
                << "m" << BucketDouble(cell.smellSignal, 0.25);
            return out.str();
        }

        AceAqBodyDelta AddDelta(AceAqBodyDelta a, AceAqBodyDelta b)
        {
            a.hydration += b.hydration;
            a.nutrition += b.nutrition;
            a.integrity += b.integrity;
            a.temperature += b.temperature;
            return a;
        }
    }

    AceAqBodyDelta AceAqTransitionStats::MeanDelta() const
    {
        if (count <= 0)
        {
            return {};
        }

        const double n = static_cast<double>(count);
        return {
            sumDelta.hydration / n,
            sumDelta.nutrition / n,
            sumDelta.integrity / n,
            sumDelta.temperature / n,
        };
    }

    AceAqObservationSignature MakeObservationSignature(const AceAqObservation& observation)
    {
        std::ostringstream out;
        out << "front:" << CellKey(observation.front)
            << "|left:" << CellKey(observation.left)
            << "|right:" << CellKey(observation.right)
            << "|current:" << CellKey(observation.current);
        return {out.str()};
    }

    std::string MakeTransitionKey(const AceAqObservation& observation, AceAqAction action)
    {
        return MakeObservationSignature(observation).key + "|action:" + ToString(action);
    }

    AceAqPrediction AceAqTableWorldModel::Predict(const AceAqObservation& observation, AceAqAction action) const
    {
        const auto key = MakeTransitionKey(observation, action);
        const auto found = transitions_.find(key);
        if (found == transitions_.end())
        {
            return {};
        }

        const int count = found->second.count;
        const double confidence = static_cast<double>(count) / static_cast<double>(count + 3);
        return {
            found->second.MeanDelta(),
            confidence,
            1.0 - confidence,
            true,
            count,
        };
    }

    void AceAqTableWorldModel::LearnFromEpisode(const AceAqEpisode& episode)
    {
        const auto key = MakeTransitionKey(episode.observationBefore, episode.action);
        auto& stats = transitions_[key];
        stats.count += 1;
        stats.sumDelta = AddDelta(stats.sumDelta, episode.actualDelta);
        for (const auto& flag : episode.lastActionResult.eventFlags)
        {
            stats.flagCounts[flag] += 1;
        }
    }

    double AceAqTableWorldModel::PredictionError(const AceAqPrediction& prediction, const AceAqBodyDelta& actualDelta) const
    {
        return std::abs(prediction.predictedDelta.hydration - actualDelta.hydration) +
               std::abs(prediction.predictedDelta.nutrition - actualDelta.nutrition) +
               std::abs(prediction.predictedDelta.integrity - actualDelta.integrity) +
               std::abs(prediction.predictedDelta.temperature - actualDelta.temperature);
    }

    int AceAqTableWorldModel::EntryCount() const
    {
        return static_cast<int>(transitions_.size());
    }

    int AceAqTableWorldModel::SampleCountFor(const AceAqObservation& observation, AceAqAction action) const
    {
        const auto found = transitions_.find(MakeTransitionKey(observation, action));
        return found == transitions_.end() ? 0 : found->second.count;
    }

    std::string AceAqTableWorldModel::ToJsonLikeString() const
    {
        std::ostringstream out;
        out << "{\"entries\":" << transitions_.size() << ",\"keys\":[";
        bool first = true;
        for (const auto& item : transitions_)
        {
            if (!first)
            {
                out << ",";
            }
            first = false;
            out << JsonString(item.first);
        }
        out << "]}";
        return out.str();
    }

    std::string ToJsonLikeString(const AceAqPrediction& prediction)
    {
        return "{" +
            std::string("\"has_prediction\":") + JsonBool(prediction.hasPrediction) + "," +
            "\"predicted_delta\":" + ToJsonLikeString(prediction.predictedDelta) + "," +
            "\"confidence\":" + JsonNumber(prediction.confidence) + "," +
            "\"uncertainty\":" + JsonNumber(prediction.uncertainty) + "," +
            "\"sample_count\":" + std::to_string(prediction.sampleCount) +
            "}";
    }
}
