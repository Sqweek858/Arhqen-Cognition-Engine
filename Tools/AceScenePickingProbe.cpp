#include "ArhqenCognitionEngine/Editor/Scene/AceScenePicking.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

namespace
{
    int checks = 0, failures = 0;
    void check(bool value, std::string_view name)
    { ++checks; std::cout << (value ? "PASS|" : "FAIL|") << name << '\n'; if (!value) ++failures; }
    bool near(double a, double b) { return std::abs(a - b) < 1.0e-9; }
}

int main()
{
    using namespace am::editor::scene;
    ScenePicker picker;
    const auto nearId = am::core::Guid::create(), farId = am::core::Guid::create();
    const AxisAlignedBounds unit{{-1,-1,-1},{1,1,1}};
    check(ScenePicker::validBounds(unit), "valid_bounds_are_accepted");
    AxisAlignedBounds invalid = unit; invalid.minimum.x = 2.0;
    check(!ScenePicker::validBounds(invalid) && !picker.upsert({nearId, invalid}), "inverted_bounds_are_rejected");
    check(picker.upsert({farId, {{-1,-1,9},{1,1,11}}}) &&
          picker.upsert({nearId, {{-1,-1,4},{1,1,6}}}) && picker.size() == 2, "pick_proxies_upsert_by_entity_guid");
    auto hit = picker.raycast({{0,0,0},{0,0,10}});
    check(hit && hit->entityId == nearId && near(hit->distance, 4.0) && near(hit->position.z, 4.0),
          "ray_is_normalized_and_closest_hit_wins");
    hit = picker.raycast({{0,0,5},{1,0,0}});
    check(hit && hit->entityId == nearId && near(hit->distance, 0.0), "ray_origin_inside_bounds_hits_at_zero");
    check(!picker.raycast({{5,0,0},{0,0,1}}), "parallel_ray_outside_slab_misses");
    check(!picker.raycast({{0,0,0},{0,0,1}}, 3.9), "maximum_distance_clips_far_hits");
    check(!picker.raycast({{0,0,0},{0,0,0}}) &&
          !picker.raycast({{0,0,0},{0,0,std::numeric_limits<double>::quiet_NaN()}}),
          "zero_and_nonfinite_rays_are_rejected");

    const auto hiddenId = am::core::Guid::create(), blockedId = am::core::Guid::create();
    picker.upsert({hiddenId, {{-1,-1,1},{1,1,2}}, false, true});
    picker.upsert({blockedId, {{-1,-1,2},{1,1,3}}, true, false});
    hit = picker.raycast({{0,0,0},{0,0,1}});
    check(hit && hit->entityId == nearId, "hidden_and_nonselectable_proxies_are_ignored");
    const auto priorityId = am::core::Guid::create();
    picker.upsert({priorityId, {{-1,-1,4},{1,1,6}}, true, true, 9});
    hit = picker.raycast({{0,0,0},{0,0,1}});
    check(hit && hit->entityId == priorityId, "priority_breaks_equal_distance_ties");
    check(picker.remove(priorityId) && !picker.remove(priorityId), "proxy_removal_is_idempotent");

    ScenePicker stableTie;
    const auto stableFirst = am::core::Guid::parse("00000000-0000-4000-8000-000000000001");
    const auto stableSecond = am::core::Guid::parse("00000000-0000-4000-8000-000000000002");
    stableTie.upsert({*stableSecond, {{-1,-1,4},{1,1,6}}});
    stableTie.upsert({*stableFirst, {{-1,-1,4},{1,1,6}}});
    hit = stableTie.raycast({{0,0,0},{0,0,1}});
    check(hit && hit->entityId == *stableFirst, "equal_hits_use_stable_guid_tie_break");

    am::core::scene::Transform transform;
    transform.location = {10,20,30}; transform.rotationDegrees.z = 90; transform.scale = {2,1,-3};
    const auto bounds = ScenePicker::transformBounds(unit, transform);
    check(near(bounds.minimum.x, 9) && near(bounds.maximum.x, 11) &&
          near(bounds.minimum.y, 18) && near(bounds.maximum.y, 22) &&
          near(bounds.minimum.z, 27) && near(bounds.maximum.z, 33),
          "local_bounds_transform_handles_rotation_scale_and_negative_scale");

    ScenePicker broad;
    const auto start = std::chrono::steady_clock::now();
    am::core::Guid expected;
    for (int index = 0; index < 20000; ++index)
    {
        const auto id = am::core::Guid::create();
        const double z = 10.0 + index * 2.0;
        broad.upsert({id, {{-0.5,-0.5,z},{0.5,0.5,z+1.0}}});
        if (index == 0) expected = id;
    }
    hit = broad.raycast({{0,0,0},{0,0,1}});
    const auto elapsed = std::chrono::steady_clock::now() - start;
    check(hit && hit->entityId == expected, "broad_scene_returns_nearest_proxy");
    check(elapsed < std::chrono::seconds(5), "twenty_thousand_proxy_pick_is_bounded");
    broad.clear(); check(broad.size() == 0, "picker_clear_removes_all_proxies");
    std::cout << "SUMMARY|checks=" << checks << "|failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
