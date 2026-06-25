#pragma once

#include "ArhqenCognitionEngine/Aquarium/AceAqActions.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqBody.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqObservation.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqDelayedEffects.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqWorldEvents.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace ace::aquarium
{
    struct AceAqLastActionResult
    {
        AceAqAction action = AceAqAction::Wait;
        bool moved = false;
        bool blocked = false;
        bool touched = false;
        bool consumed = false;
        bool pushed = false;
        AceAqBodyDelta bodyDelta{};
        std::vector<std::string> eventFlags;
        std::string reason;
        bool externalEvent = false;
    };

    struct AceAqEpisode
    {
        std::uint64_t episodeId = 0;
        int step = 0;
        AceAqBodyState bodyBefore{};
        AceAqObservation observationBefore{};
        AceAqAction action = AceAqAction::Wait;
        AceAqBodyState bodyAfter{};
        AceAqObservation observationAfter{};
        AceAqBodyDelta actualDelta{};
        AceAqLastActionResult lastActionResult{};
        bool terminated = false;
        bool truncated = false;
        std::string debugTruthJsonLike;
        std::vector<AceAqDelayedEffect> scheduledDelayedEffects;
        std::vector<AceAqDelayedEffectApplication> appliedDelayedEffects;
        std::vector<AceAqWorldEvent> externalWorldEvents;

        bool hasPredictedBodyDelta = false;
        bool hasPredictionError = false;
        bool hasSurprise = false;
    };

    class AceAqEpisodeMemory
    {
    public:
        void Clear();
        void Add(AceAqEpisode episode);
        const std::vector<AceAqEpisode>& Episodes() const;
        std::size_t Size() const;

    private:
        std::vector<AceAqEpisode> episodes_;
    };

    class AceAqEpisodeLogger
    {
    public:
        AceAqEpisodeLogger() = default;
        explicit AceAqEpisodeLogger(std::filesystem::path path);

        void SetPath(std::filesystem::path path);
        const std::filesystem::path& Path() const;
        bool Enabled() const;
        void SetEnabled(bool enabled);
        bool Log(const AceAqEpisode& episode, std::string* error = nullptr) const;

    private:
        std::filesystem::path path_;
        bool enabled_ = false;
    };

    std::string ToJsonLikeString(const AceAqLastActionResult& result);
    std::string ToJsonLikeString(const AceAqEpisode& episode);
}
