#include "ArhqenCognitionEngine/Aquarium/AceAqProtoConcept.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <algorithm>
#include <map>
#include <sstream>

namespace ace::aquarium
{
    namespace
    {
        bool HasFlag(const AceAqLastActionResult& result, const std::string& flag)
        {
            return std::find(result.eventFlags.begin(), result.eventFlags.end(), flag) != result.eventFlags.end();
        }

        std::string JoinProfile(const std::vector<std::string>& profile)
        {
            std::ostringstream out;
            for (std::size_t i = 0; i < profile.size(); ++i)
            {
                if (i > 0)
                {
                    out << "+";
                }
                out << profile[i];
            }
            return out.str();
        }

        std::string MakeSummaryLabel(AceAqAction action, const std::vector<std::string>& profile)
        {
            return "concept_" + ToString(action) + "_" + JoinProfile(profile);
        }
    }

    std::vector<std::string> AceAqEffectProfileFromDelta(
        const AceAqBodyDelta& delta,
        const AceAqLastActionResult& result)
    {
        std::vector<std::string> profile;

        if (delta.hydration > 0.01) profile.push_back("hydration_up");
        if (delta.nutrition > 0.01) profile.push_back("nutrition_up");
        if (delta.integrity < -0.01) profile.push_back("integrity_down");
        if (delta.temperature < -0.01) profile.push_back("temperature_down");
        if (result.blocked || HasFlag(result, "move_blocked")) profile.push_back("blocked");
        if (result.moved || HasFlag(result, "move_success")) profile.push_back("moved");
        if (result.pushed || HasFlag(result, "push_success")) profile.push_back("pushable_effect");
        if (HasFlag(result, "consume_failed") || HasFlag(result, "push_failed_not_pushable")) profile.push_back("not_consumable_or_blocked");

        if (profile.empty())
        {
            profile.push_back("neutral");
        }

        std::sort(profile.begin(), profile.end());
        profile.erase(std::unique(profile.begin(), profile.end()), profile.end());
        return profile;
    }

    std::vector<AceAqProtoConcept> AceAqProtoConceptMiner::Mine(const AceAqEpisodeMemory& memory) const
    {
        return Mine(memory.Episodes());
    }

    std::vector<AceAqProtoConcept> AceAqProtoConceptMiner::Mine(const std::vector<AceAqEpisode>& episodes) const
    {
        std::map<std::string, AceAqProtoConcept> byKey;

        for (const auto& episode : episodes)
        {
            auto profile = AceAqEffectProfileFromDelta(episode.actualDelta, episode.lastActionResult);
            const auto signature = MakeObservationSignature(episode.observationBefore);
            const auto key = signature.key + "|action:" + ToString(episode.action) + "|effect:" + JoinProfile(profile);

            auto& proto = byKey[key];
            if (proto.supportCount == 0)
            {
                proto.signatureKey = key;
                proto.effectProfile = profile;
                proto.summaryLabel = MakeSummaryLabel(episode.action, profile);
            }

            proto.supportCount += 1;
            proto.supportingEpisodeIds.push_back(episode.episodeId);
        }

        std::vector<AceAqProtoConcept> concepts;
        int nextId = 1;
        for (auto& item : byKey)
        {
            item.second.conceptId = nextId++;
            concepts.push_back(item.second);
        }

        return concepts;
    }

    std::string ToJsonLikeString(const AceAqProtoConcept& proto)
    {
        std::ostringstream ids;
        ids << "[";
        for (std::size_t i = 0; i < proto.supportingEpisodeIds.size(); ++i)
        {
            if (i > 0)
            {
                ids << ",";
            }
            ids << proto.supportingEpisodeIds[i];
        }
        ids << "]";

        return "{" +
            std::string("\"concept_id\":") + std::to_string(proto.conceptId) + "," +
            "\"support_count\":" + std::to_string(proto.supportCount) + "," +
            "\"contradiction_count\":" + std::to_string(proto.contradictionCount) + "," +
            "\"effect_profile\":" + JsonStringArray(proto.effectProfile) + "," +
            "\"supporting_episode_ids\":" + ids.str() + "," +
            "\"summary_label\":" + JsonString(proto.summaryLabel) +
            "}";
    }
}
