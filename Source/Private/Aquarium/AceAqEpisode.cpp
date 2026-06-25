#include "ArhqenCognitionEngine/Aquarium/AceAqEpisode.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <fstream>
#include <sstream>

namespace ace::aquarium
{
    void AceAqEpisodeMemory::Clear()
    {
        episodes_.clear();
    }

    void AceAqEpisodeMemory::Add(AceAqEpisode episode)
    {
        episodes_.push_back(std::move(episode));
    }

    const std::vector<AceAqEpisode>& AceAqEpisodeMemory::Episodes() const
    {
        return episodes_;
    }

    std::size_t AceAqEpisodeMemory::Size() const
    {
        return episodes_.size();
    }

    AceAqEpisodeLogger::AceAqEpisodeLogger(std::filesystem::path path)
        : path_(std::move(path))
        , enabled_(true)
    {
    }

    void AceAqEpisodeLogger::SetPath(std::filesystem::path path)
    {
        path_ = std::move(path);
        enabled_ = !path_.empty();
    }

    const std::filesystem::path& AceAqEpisodeLogger::Path() const
    {
        return path_;
    }

    bool AceAqEpisodeLogger::Enabled() const
    {
        return enabled_ && !path_.empty();
    }

    void AceAqEpisodeLogger::SetEnabled(bool enabled)
    {
        enabled_ = enabled;
    }

    bool AceAqEpisodeLogger::Log(const AceAqEpisode& episode, std::string* error) const
    {
        if (!Enabled())
        {
            return true;
        }

        try
        {
            const auto parent = path_.parent_path();
            if (!parent.empty())
            {
                std::filesystem::create_directories(parent);
            }

            std::ofstream output(path_, std::ios::app);
            if (!output.is_open())
            {
                if (error)
                {
                    *error = "failed to open episode log";
                }
                return false;
            }

            output << ToJsonLikeString(episode) << "\n";
            return true;
        }
        catch (const std::exception& ex)
        {
            if (error)
            {
                *error = ex.what();
            }
            return false;
        }
    }

    std::string ToJsonLikeString(const AceAqLastActionResult& result)
    {
        return "{" +
            std::string("\"action\":") + JsonString(ToString(result.action)) + "," +
            "\"moved\":" + JsonBool(result.moved) + "," +
            "\"blocked\":" + JsonBool(result.blocked) + "," +
            "\"touched\":" + JsonBool(result.touched) + "," +
            "\"consumed\":" + JsonBool(result.consumed) + "," +
            "\"pushed\":" + JsonBool(result.pushed) + "," +
            "\"body_delta\":" + ToJsonLikeString(result.bodyDelta) + "," +
            "\"event_flags\":" + JsonStringArray(result.eventFlags) + "," +
            "\"reason\":" + JsonString(result.reason) + "," +
            "\"external_event\":" + JsonBool(result.externalEvent) +
            "}";
    }

    std::string ToJsonLikeString(const AceAqEpisode& episode)
    {
        std::ostringstream out;
        out << "{";
        out << "\"episode_id\":" << episode.episodeId << ",";
        out << "\"step\":" << episode.step << ",";
        out << "\"body_before\":" << ToJsonLikeString(episode.bodyBefore) << ",";
        out << "\"observation_before\":" << ToJsonLikeString(episode.observationBefore) << ",";
        out << "\"action\":" << JsonString(ToString(episode.action)) << ",";
        out << "\"body_after\":" << ToJsonLikeString(episode.bodyAfter) << ",";
        out << "\"observation_after\":" << ToJsonLikeString(episode.observationAfter) << ",";
        out << "\"actual_delta\":" << ToJsonLikeString(episode.actualDelta) << ",";
        out << "\"last_action_result\":" << ToJsonLikeString(episode.lastActionResult) << ",";
        out << "\"terminated\":" << JsonBool(episode.terminated) << ",";
        out << "\"truncated\":" << JsonBool(episode.truncated) << ",";
        out << "\"debug_truth\":" << (episode.debugTruthJsonLike.empty() ? "null" : episode.debugTruthJsonLike) << ",";
        out << "\"scheduled_delayed_effects\":" << ToJsonLikeString(episode.scheduledDelayedEffects) << ",";
        out << "\"applied_delayed_effects\":" << ToJsonLikeString(episode.appliedDelayedEffects) << ",";
        out << "\"external_world_events\":" << ToJsonLikeString(episode.externalWorldEvents) << ",";
        out << "\"predicted_body_delta\":null,";
        out << "\"prediction_error\":null,";
        out << "\"surprise\":null";
        out << "}";
        return out.str();
    }
}
