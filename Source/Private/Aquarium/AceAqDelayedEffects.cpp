#include "ArhqenCognitionEngine/Aquarium/AceAqDelayedEffects.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace ace::aquarium
{
    void AceAqDelayedEffectQueue::Clear()
    {
        pending_.clear();
        nextId_ = 1;
    }

    AceAqDelayedEffect AceAqDelayedEffectQueue::Schedule(int currentStep, int delaySteps, std::string source, AceAqBodyDelta delta, bool external)
    {
        AceAqDelayedEffect effect;
        effect.id = nextId_++;
        effect.createdStep = currentStep;
        effect.dueStep = currentStep + std::max(1, delaySteps);
        effect.source = std::move(source);
        effect.delta = delta;
        effect.external = external;
        pending_.push_back(effect);
        return effect;
    }

    std::vector<AceAqDelayedEffectApplication> AceAqDelayedEffectQueue::TickAndCollectDue(int currentStep)
    {
        std::vector<AceAqDelayedEffectApplication> applications;
        std::vector<AceAqDelayedEffect> stillPending;

        for (const auto& effect : pending_)
        {
            if (effect.dueStep <= currentStep)
            {
                applications.push_back({
                    effect.id,
                    currentStep,
                    effect.source,
                    effect.delta,
                    effect.external,
                });
            }
            else
            {
                stillPending.push_back(effect);
            }
        }

        pending_ = std::move(stillPending);
        return applications;
    }

    int AceAqDelayedEffectQueue::PendingCount() const
    {
        return static_cast<int>(pending_.size());
    }

    const std::vector<AceAqDelayedEffect>& AceAqDelayedEffectQueue::Pending() const
    {
        return pending_;
    }

    std::string AceAqDelayedEffectQueue::ToJsonLikeString() const
    {
        return ace::aquarium::ToJsonLikeString(pending_);
    }

    std::string ToJsonLikeString(const AceAqDelayedEffect& effect)
    {
        return "{" +
            std::string("\"id\":") + std::to_string(effect.id) + "," +
            "\"created_step\":" + std::to_string(effect.createdStep) + "," +
            "\"due_step\":" + std::to_string(effect.dueStep) + "," +
            "\"source\":" + JsonString(effect.source) + "," +
            "\"delta\":" + ToJsonLikeString(effect.delta) + "," +
            "\"external\":" + JsonBool(effect.external) +
            "}";
    }

    std::string ToJsonLikeString(const AceAqDelayedEffectApplication& application)
    {
        return "{" +
            std::string("\"effect_id\":") + std::to_string(application.effectId) + "," +
            "\"step_applied\":" + std::to_string(application.stepApplied) + "," +
            "\"source\":" + JsonString(application.source) + "," +
            "\"applied_delta\":" + ToJsonLikeString(application.appliedDelta) + "," +
            "\"external\":" + JsonBool(application.external) +
            "}";
    }

    std::string ToJsonLikeString(const std::vector<AceAqDelayedEffect>& effects)
    {
        std::ostringstream out;
        out << "[";
        for (std::size_t i = 0; i < effects.size(); ++i)
        {
            if (i > 0) out << ",";
            out << ToJsonLikeString(effects[i]);
        }
        out << "]";
        return out.str();
    }

    std::string ToJsonLikeString(const std::vector<AceAqDelayedEffectApplication>& applications)
    {
        std::ostringstream out;
        out << "[";
        for (std::size_t i = 0; i < applications.size(); ++i)
        {
            if (i > 0) out << ",";
            out << ToJsonLikeString(applications[i]);
        }
        out << "]";
        return out.str();
    }
}
