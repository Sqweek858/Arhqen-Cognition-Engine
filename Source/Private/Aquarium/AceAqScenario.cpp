#include "ArhqenCognitionEngine/Aquarium/AceAqScenario.h"

#include "ArhqenCognitionEngine/Aquarium/AceAqJson.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace ace::aquarium
{
    void AceAqScenarioRegistry::RegisterScenario(AceAqScenarioDefinition definition)
    {
        scenarios_[definition.name] = std::move(definition);
    }

    bool AceAqScenarioRegistry::HasScenario(const std::string& name) const
    {
        return scenarios_.find(name) != scenarios_.end();
    }

    AceAqScenarioBuildResult AceAqScenarioRegistry::Build(const std::string& name, std::uint32_t seed) const
    {
        const auto found = scenarios_.find(name);
        if (found == scenarios_.end())
        {
            throw std::invalid_argument("unknown Aquarium scenario: " + name);
        }

        auto definition = found->second;
        definition.seed = seed;

        AceAqEnvironment environment;
        environment.Reset(AceAqGridWorld::FromAscii(definition.rows), definition.initialBody);

        if (name == "moving_hazard" || name == "dynamic_hazard_damage")
        {
            environment.SetDynamicWorldConfig({true, seed, true, false, false, 3});
        }
        else if (name == "spreading_acid")
        {
            environment.SetDynamicWorldConfig({true, seed, false, true, false, 3});
        }
        else if (name == "food_decay")
        {
            environment.SetDynamicWorldConfig({true, seed, false, false, true, 2});
        }

        return {environment, definition, definition.expectations};
    }

    std::vector<std::string> AceAqScenarioRegistry::ListNames() const
    {
        std::vector<std::string> names;
        for (const auto& item : scenarios_)
        {
            names.push_back(item.first);
        }
        return names;
    }

    int AceAqScenarioRegistry::Count() const
    {
        return static_cast<int>(scenarios_.size());
    }

    AceAqScenarioRegistry DefaultAceAqScenarioRegistry()
    {
        AceAqScenarioRegistry registry;

        auto normal = AceAqBodyState{};
        auto thirsty = AceAqBodyState{};
        thirsty.hydration = 0.20;
        auto fragile = AceAqBodyState{};
        fragile.integrity = 0.35;
        auto cold = AceAqBodyState{};
        cold.temperature = 0.30;

        registry.RegisterScenario({"basic_wall", "front wall blocks movement", {"#####", "#>#.#", "#####"}, normal, {"move_blocked"}});
        registry.RegisterScenario({"water_front", "water in front increases hydration when consumed", {"#####", "#>W.#", "#####"}, thirsty, {"consume_success", "hydration_up"}});
        registry.RegisterScenario({"acid_front", "acid in front damages integrity when touched/consumed", {"#####", "#>X.#", "#####"}, normal, {"integrity_down"}});
        registry.RegisterScenario({"food_front", "food in front increases nutrition when consumed", {"#####", "#>F.#", "#####"}, normal, {"nutrition_up"}});
        registry.RegisterScenario({"stone_push", "stone in front can be pushed into empty cell", {"######", "#>S..#", "######"}, normal, {"push_success"}});
        registry.RegisterScenario({"unknown_liquid_fragile", "fragile body faces unknown-looking risky liquid", {"#####", "#>X.#", "#####"}, fragile, {"avoid_unknown_consume"}});
        registry.RegisterScenario({"context_flip_cold_body", "same water-like observation has different value when body is cold", {"#####", "#>W.#", "#####"}, cold, {"context_sensitive_temperature"}});
        registry.RegisterScenario({"same_appearance_liquids", "evaluator scenario for same appearance with different effects", {"#####", "#>W.#", "#####"}, normal, {"same_appearance_different_effect"}});
        registry.RegisterScenario({"color_swap_train", "training appearance is color-swapped evaluator-side", {"#####", "#>W.#", "#####"}, thirsty, {"learn_from_effect_not_color"}});
        registry.RegisterScenario({"color_swap_test", "test appearance is color-swapped evaluator-side", {"#####", "#>X.#", "#####"}, normal, {"learn_from_effect_not_color"}});

        registry.RegisterScenario({"randomized_water_front", "water front with randomized appearance", {"#####", "#>W.#", "#####"}, thirsty, {"randomized_appearance", "hydration_up"}});
        registry.RegisterScenario({"randomized_acid_front", "acid front with randomized appearance", {"#####", "#>X.#", "#####"}, normal, {"randomized_appearance", "integrity_down"}});
        registry.RegisterScenario({"cross_color_water_acid", "water/acid appearances are crossed evaluator-side", {"#####", "#>W.#", "#####"}, thirsty, {"cross_color_liquids"}});
        registry.RegisterScenario({"randomized_food_front", "food front with randomized appearance", {"#####", "#>F.#", "#####"}, normal, {"randomized_appearance", "nutrition_up"}});


        registry.RegisterScenario({"poison_food_front", "poison food schedules delayed integrity damage", {"#####", "#>P.#", "#####"}, normal, {"scheduled_delayed_effect", "integrity_down_delayed"}});
        registry.RegisterScenario({"slow_medicine_front", "slow medicine schedules delayed healing", {"#####", "#>M.#", "#####"}, fragile, {"scheduled_delayed_effect", "integrity_up_delayed"}});
        registry.RegisterScenario({"cold_liquid_front", "cold liquid affects hydration and temperature", {"#####", "#>C.#", "#####"}, normal, {"temperature_down"}});
        registry.RegisterScenario({"moving_hazard", "moving hazard moves deterministically", {"#######", "#>..H.#", "#######"}, normal, {"dynamic_hazard_moved"}});
        registry.RegisterScenario({"spreading_acid", "spreading acid expands into empty space", {"#######", "#>..D.#", "#######"}, normal, {"dynamic_spread"}});
        registry.RegisterScenario({"food_decay", "food decays into poison food", {"#######", "#>..F.#", "#######"}, normal, {"dynamic_food_decay"}});
        registry.RegisterScenario({"dynamic_hazard_damage", "moving hazard can externally damage the agent", {"#####", "#>H.#", "#####"}, normal, {"external_hazard_damage"}});
        registry.RegisterScenario({"delayed_poison_damage", "poison food delayed damage applies later", {"#####", "#>P.#", "#####"}, normal, {"applied_delayed_effect"}});

        return registry;
    }

    std::vector<std::string> EvaluateScenarioExpectations(const AceAqScenarioBuildResult& build)
    {
        // Evaluator-side only. Expectations are labels for validation, not agent input.
        return build.expectations;
    }

    std::string ToJsonLikeString(const AceAqScenarioDefinition& definition)
    {
        std::ostringstream rows;
        rows << "[";
        for (std::size_t i = 0; i < definition.rows.size(); ++i)
        {
            if (i > 0) rows << ",";
            rows << JsonString(definition.rows[i]);
        }
        rows << "]";

        return "{" +
            std::string("\"name\":") + JsonString(definition.name) + "," +
            "\"description\":" + JsonString(definition.description) + "," +
            "\"rows\":" + rows.str() + "," +
            "\"initial_body\":" + ToJsonLikeString(definition.initialBody) + "," +
            "\"expectations\":" + JsonStringArray(definition.expectations) + "," +
            "\"seed\":" + std::to_string(definition.seed) +
            "}";
    }
}
