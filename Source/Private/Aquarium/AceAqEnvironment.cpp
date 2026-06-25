#include "ArhqenCognitionEngine/Aquarium/AceAqEnvironment.h"

#include <utility>

namespace ace::aquarium
{
    namespace
    {
        bool AnyExternal(const std::vector<AceAqDelayedEffectApplication>& delayed, const std::vector<AceAqWorldEvent>& events)
        {
            for (const auto& item : delayed)
            {
                if (item.external) return true;
            }
            for (const auto& item : events)
            {
                if (item.external) return true;
            }
            return false;
        }
    }

    AceAqEnvironment::AceAqEnvironment()
        : world_(AceAqGridWorld::DefaultWorld())
        , body_()
        , dynamicWorld_(dynamicWorldConfig_)
    {
    }

    void AceAqEnvironment::Reset()
    {
        Reset(AceAqGridWorld::DefaultWorld(), {});
    }

    void AceAqEnvironment::Reset(AceAqGridWorld world, AceAqBodyState body)
    {
        world_ = std::move(world);
        body_ = body.Clamped();
        memory_.Clear();
        delayedEffects_.Clear();
        dynamicWorld_.Reset(dynamicWorldConfig_);
        step_ = 0;
        nextEpisodeId_ = 1;
        lastActionReason_.clear();
    }

    void AceAqEnvironment::EnableEpisodeLogging(std::filesystem::path path)
    {
        logger_.SetPath(std::move(path));
        logger_.SetEnabled(true);
    }

    void AceAqEnvironment::DisableEpisodeLogging()
    {
        logger_.SetEnabled(false);
    }

    void AceAqEnvironment::SetDynamicWorldEnabled(bool enabled)
    {
        dynamicWorldConfig_.enabled = enabled;
        dynamicWorld_.Reset(dynamicWorldConfig_);
    }

    void AceAqEnvironment::SetDynamicWorldConfig(AceAqDynamicWorldConfig config)
    {
        dynamicWorldConfig_ = config;
        dynamicWorld_.Reset(dynamicWorldConfig_);
    }

    AceAqObservation AceAqEnvironment::GetObservation() const
    {
        return BuildObservation(lastActionReason_);
    }

    AceAqBodyState AceAqEnvironment::GetBodyState() const
    {
        return body_;
    }

    std::string AceAqEnvironment::GetDebugTruthJsonLike() const
    {
        return world_.DebugTruthJsonLike();
    }

    const AceAqEpisodeMemory& AceAqEnvironment::GetEpisodeMemory() const
    {
        return memory_;
    }

    AceAqObservation AceAqEnvironment::BuildObservation(const std::string& lastReason) const
    {
        AceAqObservation obs;
        obs.bodyState = body_;
        obs.lastActionReason = lastReason;

        int index = 0;
        for (int localY = -1; localY <= 1; ++localY)
        {
            for (int localX = -1; localX <= 1; ++localX)
            {
                obs.localView[static_cast<std::size_t>(index)] = ObserveCell(world_.GetCell(world_.OffsetFromAgent(localX, localY)));
                ++index;
            }
        }

        obs.front = ObserveCell(world_.GetCell(world_.FrontPosition()));
        obs.left = ObserveCell(world_.GetCell(world_.LeftPosition()));
        obs.right = ObserveCell(world_.GetCell(world_.RightPosition()));
        obs.current = ObserveCell(world_.GetCell(world_.AgentPosition()));
        return obs;
    }

    void AceAqEnvironment::AddFlag(AceAqLastActionResult& result, std::string flag) const
    {
        result.eventFlags.push_back(std::move(flag));
    }

    AceAqLastActionResult AceAqEnvironment::ExecuteAction(AceAqAction action)
    {
        AceAqLastActionResult result;
        result.action = action;

        switch (action)
        {
        case AceAqAction::TurnLeft:
            world_.TurnAgentLeft();
            AddFlag(result, "turn_left");
            result.reason = "turned left";
            return result;

        case AceAqAction::TurnRight:
            world_.TurnAgentRight();
            AddFlag(result, "turn_right");
            result.reason = "turned right";
            return result;

        case AceAqAction::MoveForward:
            if (world_.MoveAgentForward())
            {
                result.moved = true;
                AddFlag(result, "move_success");
                result.reason = "move succeeded";
            }
            else
            {
                result.blocked = true;
                AddFlag(result, "move_blocked");
                result.reason = "front blocked";
            }
            return result;

        case AceAqAction::Wait:
            AddFlag(result, "wait");
            result.reason = "wait";
            return result;

        case AceAqAction::TouchFront:
        {
            const auto object = world_.GetCell(world_.FrontPosition());
            result.touched = true;
            result.bodyDelta = object.touchEffect;
            body_.ApplyDelta(object.touchEffect);
            AddFlag(result, "touch");
            result.reason = "front touched";
            return result;
        }

        case AceAqAction::ConsumeFront:
        {
            const auto frontPosition = world_.FrontPosition();
            const auto object = world_.GetCell(frontPosition);
            if (!object.consumable)
            {
                result.blocked = true;
                AddFlag(result, "consume_failed");
                result.reason = "front object is not consumable";
                return result;
            }

            result.consumed = true;
            result.bodyDelta = object.consumeEffect;
            body_.ApplyDelta(object.consumeEffect);
            world_.SetCell(frontPosition, AceAqObjectKind::Empty);
            AddFlag(result, "consume_success");
            result.reason = "front consumed";
            return result;
        }

        case AceAqAction::PushFront:
        {
            std::string reason;
            const auto object = world_.GetCell(world_.FrontPosition());
            const bool pushed = world_.PushFrontObject(&reason);
            result.pushed = pushed;
            result.reason = reason;

            if (pushed)
            {
                AddFlag(result, "push_success");
            }
            else if (!object.pushable)
            {
                result.blocked = true;
                AddFlag(result, "push_failed_not_pushable");
            }
            else
            {
                result.blocked = true;
                AddFlag(result, "push_failed_blocked");
            }

            return result;
        }
        }

        result.reason = "unknown action";
        return result;
    }

    std::vector<AceAqDelayedEffect> AceAqEnvironment::ScheduleDelayedEffectsForConsumedKind(AceAqObjectKind kind)
    {
        std::vector<AceAqDelayedEffect> scheduled;

        switch (kind)
        {
        case AceAqObjectKind::PoisonFood:
            scheduled.push_back(delayedEffects_.Schedule(step_, 2, "poison_food", {0.0, 0.0, -0.25, 0.0}, false));
            break;
        case AceAqObjectKind::SlowMedicine:
            scheduled.push_back(delayedEffects_.Schedule(step_, 2, "slow_medicine", {0.0, 0.0, 0.20, 0.0}, false));
            break;
        case AceAqObjectKind::ColdLiquid:
            scheduled.push_back(delayedEffects_.Schedule(step_, 1, "cold_liquid", {0.0, 0.0, 0.0, -0.09}, false));
            break;
        default:
            break;
        }

        return scheduled;
    }

    AceAqStepResult AceAqEnvironment::Step(AceAqAction action)
    {
        AceAqStepResult result;

        const auto bodyBefore = body_;
        const auto observationBefore = BuildObservation(lastActionReason_);
        const auto consumedKindCandidate = world_.GetCell(world_.FrontPosition()).kind;

        auto actionResult = ExecuteAction(action);
        const auto afterActionBody = body_;

        std::vector<AceAqDelayedEffect> scheduled;
        if (action == AceAqAction::ConsumeFront && actionResult.consumed)
        {
            scheduled = ScheduleDelayedEffectsForConsumedKind(consumedKindCandidate);
            for (const auto& item : scheduled)
            {
                actionResult.eventFlags.push_back("scheduled_delayed_effect");
                actionResult.eventFlags.push_back("scheduled_" + item.source);
            }
        }

        body_ = NaturalDecay(body_);

        const int applicationStep = step_ + 1;
        auto appliedDelayed = delayedEffects_.TickAndCollectDue(applicationStep);
        for (const auto& application : appliedDelayed)
        {
            body_.ApplyDelta(application.appliedDelta);
            actionResult.eventFlags.push_back("applied_delayed_effect");
            actionResult.eventFlags.push_back("applied_" + application.source);
            if (application.external)
            {
                actionResult.externalEvent = true;
                actionResult.eventFlags.push_back("external_event");
            }
        }

        auto worldEvents = dynamicWorld_.Tick(applicationStep, world_, body_);
        for (const auto& event : worldEvents)
        {
            actionResult.eventFlags.insert(actionResult.eventFlags.end(), event.eventFlags.begin(), event.eventFlags.end());
            if (event.external)
            {
                actionResult.externalEvent = true;
            }
        }

        if (AnyExternal(appliedDelayed, worldEvents))
        {
            actionResult.externalEvent = true;
        }

        const bool terminated = IsDead(body_);
        if (terminated)
        {
            AddFlag(actionResult, "death");
        }

        const auto observationAfter = BuildObservation(actionResult.reason);
        const auto actualDelta = Difference(bodyBefore, body_);

        actionResult.bodyDelta = Difference(bodyBefore, afterActionBody);
        lastActionReason_ = actionResult.reason;

        AceAqEpisode episode;
        episode.episodeId = nextEpisodeId_++;
        episode.step = step_;
        episode.bodyBefore = bodyBefore;
        episode.observationBefore = observationBefore;
        episode.action = action;
        episode.bodyAfter = body_;
        episode.observationAfter = observationAfter;
        episode.actualDelta = actualDelta;
        episode.lastActionResult = actionResult;
        episode.terminated = terminated;
        episode.truncated = false;
        episode.debugTruthJsonLike = world_.DebugTruthJsonLike();
        episode.scheduledDelayedEffects = scheduled;
        episode.appliedDelayedEffects = appliedDelayed;
        episode.externalWorldEvents = worldEvents;

        memory_.Add(episode);
        std::string logError;
        (void)logger_.Log(episode, &logError);

        result.observation = observationAfter;
        result.reward = HomeostaticReward(bodyBefore, body_);
        result.terminated = terminated;
        result.truncated = false;
        result.lastActionResult = actionResult;
        result.episode = episode;

        ++step_;
        return result;
    }
}
