#include "ArhqenCognitionEngine/Aquarium/AceAqCounterfactual.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqEnvironment.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqPlanner.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqProtoConcept.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqSelfModel.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqSymbol.h"
#include "ArhqenCognitionEngine/Aquarium/AceAqWorldModel.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace aq = ace::aquarium;

namespace
{
    int g_failures = 0;

    void Pass(const std::string& name)
    {
        std::cout << "PASS|" << name << "\n";
    }

    void Fail(const std::string& name, const std::string& reason)
    {
        ++g_failures;
        std::cout << "FAIL|" << name << "|" << reason << "\n";
    }

    void Check(const std::string& name, bool ok, const std::string& reason)
    {
        if (ok) Pass(name);
        else Fail(name, reason);
    }

    bool HasAction(const aq::AceAqDecisionTrace& trace, aq::AceAqAction action)
    {
        return std::any_of(trace.evaluations.begin(), trace.evaluations.end(), [action](const auto& evaluation)
        {
            return evaluation.action == action;
        });
    }

    bool HasEffect(const aq::AceAqProtoConcept& proto, const std::string& effect)
    {
        return std::find(proto.effectProfile.begin(), proto.effectProfile.end(), effect) != proto.effectProfile.end();
    }

    template <typename T>
    bool Contains(const std::vector<T>& values, const T& value)
    {
        return std::find(values.begin(), values.end(), value) != values.end();
    }

    aq::AceAqTableWorldModel TrainWaterModel(aq::AceAqEnvironment& env)
    {
        aq::AceAqTableWorldModel model;
        env.Step(aq::AceAqAction::ConsumeFront);
        model.LearnFromEpisode(env.GetEpisodeMemory().Episodes().back());
        return model;
    }
}

int main()
{
    // M3: Table world model.
    {
        auto world = aq::AceAqGridWorld::FromAscii({"#####", "#>W.#", "#####"});
        aq::AceAqBodyState body;
        body.hydration = 0.30;

        aq::AceAqEnvironment env;
        env.Reset(world, body);
        const auto observationBefore = env.GetObservation();
        auto model = TrainWaterModel(env);

        const auto prediction = model.Predict(observationBefore, aq::AceAqAction::ConsumeFront);
        Check("world_model_learns_water_consume", prediction.hasPrediction && prediction.predictedDelta.hydration > 0.20, "model did not learn hydration increase");
        Check("world_model_prediction_confidence_increases", prediction.confidence > 0.0 && prediction.uncertainty < 1.0, "confidence did not increase");

        aq::AceAqBodyDelta wrong;
        wrong.integrity = -0.55;
        Check("prediction_error_nonzero_for_wrong_prediction", model.PredictionError(prediction, wrong) > 0.50, "wrong prediction error should be nonzero");
    }

    // M4: planner.
    {
        auto riskyWorld = aq::AceAqGridWorld::FromAscii({"#####", "#>X.#", "#####"});
        aq::AceAqEnvironment riskyEnv;
        riskyEnv.Reset(riskyWorld);
        aq::AceAqTableWorldModel emptyModel;
        aq::AceAqSafeCuriosityPlanner planner;
        const auto trace = planner.ChooseAction(riskyEnv.GetBodyState(), riskyEnv.GetObservation(), emptyModel);

        Check("safe_planner_avoids_unknown_risky_consume", trace.chosenAction != aq::AceAqAction::ConsumeFront, "planner consumed unknown smelly liquid");
        Check("decision_trace_contains_all_actions", trace.evaluations.size() == 7 &&
              HasAction(trace, aq::AceAqAction::TurnLeft) &&
              HasAction(trace, aq::AceAqAction::TurnRight) &&
              HasAction(trace, aq::AceAqAction::MoveForward) &&
              HasAction(trace, aq::AceAqAction::Wait) &&
              HasAction(trace, aq::AceAqAction::TouchFront) &&
              HasAction(trace, aq::AceAqAction::ConsumeFront) &&
              HasAction(trace, aq::AceAqAction::PushFront), "decision trace missing action evaluations");

        auto waterWorld = aq::AceAqGridWorld::FromAscii({"#####", "#>W.#", "#####"});
        aq::AceAqBodyState thirsty;
        thirsty.hydration = 0.20;
        aq::AceAqEnvironment waterEnv;
        waterEnv.Reset(waterWorld, thirsty);
        const auto waterObs = waterEnv.GetObservation();
        auto waterModel = TrainWaterModel(waterEnv);

        const auto thirstyTrace = planner.ChooseAction(thirsty, waterObs, waterModel);
        Check("safe_planner_prefers_hydration_when_thirsty_after_learning", thirstyTrace.chosenAction == aq::AceAqAction::ConsumeFront, "planner should prefer learned water consume when thirsty");
    }

    // M5: proto-concepts.
    aq::AceAqEpisodeMemory conceptMemory;
    {
        aq::AceAqEnvironment env;
        aq::AceAqBodyState thirsty;
        thirsty.hydration = 0.20;
        env.Reset(aq::AceAqGridWorld::FromAscii({"#####", "#>W.#", "#####"}), thirsty);
        env.Step(aq::AceAqAction::ConsumeFront);
        conceptMemory.Add(env.GetEpisodeMemory().Episodes().back());

        aq::AceAqEnvironment acidEnv;
        acidEnv.Reset(aq::AceAqGridWorld::FromAscii({"#####", "#>X.#", "#####"}));
        acidEnv.Step(aq::AceAqAction::TouchFront);
        conceptMemory.Add(acidEnv.GetEpisodeMemory().Episodes().back());

        aq::AceAqProtoConceptMiner miner;
        const auto concepts = miner.Mine(conceptMemory);

        bool hydrationUp = false;
        bool integrityDown = false;
        bool supportingIds = false;
        for (const auto& proto : concepts)
        {
            hydrationUp = hydrationUp || HasEffect(proto, "hydration_up");
            integrityDown = integrityDown || HasEffect(proto, "integrity_down");
            supportingIds = supportingIds || !proto.supportingEpisodeIds.empty();
        }

        Check("proto_concept_hydration_up", hydrationUp, "missing hydration_up concept");
        Check("proto_concept_integrity_down", integrityDown, "missing integrity_down concept");
        Check("proto_concept_supporting_episode_ids", supportingIds, "missing supporting episode ids");
    }

    // M6: counterfactual.
    {
        auto waterWorld = aq::AceAqGridWorld::FromAscii({"#####", "#>W.#", "#####"});
        aq::AceAqBodyState thirsty;
        thirsty.hydration = 0.20;
        aq::AceAqEnvironment env;
        env.Reset(waterWorld, thirsty);
        const auto obs = env.GetObservation();
        auto model = TrainWaterModel(env);

        aq::AceAqCounterfactualPlanner planner;
        const auto trace = planner.ChoosePlan(thirsty, obs, model);

        Check("counterfactual_returns_candidate_plans", !trace.candidatePlans.empty(), "no candidate plans");
        Check("counterfactual_depth2_notes", Contains(trace.notes, std::string("depth_2_rollout_approximation")) &&
              Contains(trace.notes, std::string("reused_current_observation_for_depth_2")), "missing depth 2 approximation notes");
        Check("counterfactual_chosen_action_present", !trace.chosenPlan.empty(), "missing chosen plan");
    }

    // M7: symbol binding.
    {
        aq::AceAqSymbolTable table;
        table.BindSymbolToConcept("drinkable", 1);
        table.BindSymbolToConcept("Drinkable", 2);

        const auto activations = table.ActivateSymbol("DRINKABLE");
        Check("symbol_binding_case_insensitive", activations.size() == 2, "case-insensitive activation failed");
        Check("symbol_binding_ambiguity_supported", activations.size() == 2 && activations[0].conceptId != activations[1].conceptId, "ambiguity not supported");

        const auto symbols = table.GetSymbolsForConcept(1);
        Check("symbol_binding_reverse_lookup", !symbols.empty(), "reverse symbol lookup failed");
    }

    // M8: self-model.
    {
        aq::AceAqEnvironment env;
        env.Reset(aq::AceAqGridWorld::FromAscii({"#####", "#>F.#", "#####"}));
        env.Step(aq::AceAqAction::ConsumeFront);

        aq::AceAqSelfModel self;
        const auto assessment = self.ObserveEpisode(env.GetEpisodeMemory().Episodes().back());

        Check("self_model_records_owned_episode", self.Records().size() == 1 && self.State().ownedEpisodes == 1, "owned episode not recorded");
        Check("self_model_agency_confidence_increases", assessment.causedBySelf && self.State().agencyConfidence > 0.0, "agency confidence did not increase");

        aq::AceAqEpisode external = env.GetEpisodeMemory().Episodes().back();
        external.episodeId = 999;
        external.lastActionResult.externalEvent = true;
        external.lastActionResult.eventFlags.push_back("external_event");
        const auto externalAssessment = self.ObserveEpisode(external);

        Check("self_model_external_event_not_self_caused", !externalAssessment.causedBySelf && self.State().externalEvents == 1, "external event treated as self-caused");
    }

    // Privacy still holds.
    {
        aq::AceAqEnvironment env;
        env.Reset(aq::AceAqGridWorld::FromAscii({"#####", "#>W.#", "#####"}));
        const auto obsJson = aq::ToJsonLikeString(env.GetObservation());
        const auto truthJson = env.GetDebugTruthJsonLike();

        Check("observation_privacy_still_no_objectkind_labels", !aq::ObservationJsonContainsWorldTruthLabels(obsJson), "observation leaked world truth label");
        Check("debug_truth_still_separate", truthJson.find("\"WATER\"") != std::string::npos, "debug truth should contain evaluator-side label");
    }

    return g_failures == 0 ? 0 : 1;
}
