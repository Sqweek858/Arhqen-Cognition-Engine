#include "ArhqenCognitionEngine/Core/Scene/AceSceneWorld.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace
{
    int checks = 0;
    int failures = 0;
    void check(bool condition, std::string_view name)
    {
        ++checks;
        std::cout << (condition ? "PASS|" : "FAIL|") << name << '\n';
        if (!condition) ++failures;
    }
}

int main()
{
    using namespace am::core::scene;
    const auto base = std::filesystem::current_path() / "Build" / "ACE-SCENE-WORLD" / "runtime";
    const auto file = base / "World.acescene";
    std::error_code ec;
    std::filesystem::remove_all(base, ec);

    SceneWorld world;
    std::string error;
    check(world.entityCount() == 1 && world.rootId().isValid(), "new_scene_has_stable_world_root");
    check(world.validate(&error) && error.empty(), "new_scene_is_valid");
    const auto folder = world.createEntity(EntityKind::Folder, "Environment", world.rootId(), {}, &error);
    const auto camera = world.createEntity(EntityKind::Camera, "Editor Camera", world.rootId(), {}, &error);
    const auto mesh = folder ? world.createEntity(EntityKind::StaticMesh, "Rock α", *folder, am::core::Guid::create(), &error) : std::nullopt;
    check(folder && camera && mesh && world.entityCount() == 4, "entities_create_with_guid_and_utf8_labels");
    check(world.find(*mesh) && world.find(*mesh)->parentId == *folder && world.find(*mesh)->assetId.isValid(),
          "mesh_entity_keeps_parent_and_asset_reference");
    check(world.childrenOf(world.rootId()).size() == 2 && world.childrenOf(*folder).size() == 1,
          "direct_children_are_deterministic");
    check(world.isDescendantOf(*mesh, world.rootId()) && world.isDescendantOf(*mesh, *folder),
          "descendant_queries_follow_hierarchy");
    check(!world.createEntity(EntityKind::World, "Other World", world.rootId(), {}, &error) &&
          !world.createEntity(EntityKind::Empty, "Bad\nName", world.rootId(), {}, &error),
          "invalid_kind_and_control_label_are_rejected");

    const auto generationBeforeNoop = world.generation();
    check(world.renameEntity(*mesh, "Rock α", &error) && world.generation() == generationBeforeNoop,
          "no_op_rename_does_not_advance_generation");
    check(world.renameEntity(*mesh, "Hero Rock", &error) && world.find(*mesh)->label == "Hero Rock",
          "entity_rename_is_live");
    Transform transform;
    transform.location = {10.25, -2.5, 99.0};
    transform.rotationDegrees = {0.0, 45.0, 5.0};
    transform.scale = {2.0, 2.0, -1.0};
    check(world.setTransform(*mesh, transform, &error) && world.find(*mesh)->transform == transform,
          "full_double_transform_is_stored");
    Transform invalid = transform;
    invalid.location.x = std::numeric_limits<double>::quiet_NaN();
    const auto validGeneration = world.generation();
    check(!world.setTransform(*mesh, invalid, &error) && world.generation() == validGeneration &&
          world.find(*mesh)->transform == transform, "non_finite_transform_is_rejected_atomically");
    check(world.setVisible(*mesh, false, &error) && world.setLocked(*mesh, true, &error) &&
          !world.find(*mesh)->visible && world.find(*mesh)->locked, "visibility_and_lock_are_real_properties");
    check(!world.reparentEntity(*folder, *mesh, &error) && world.find(*folder)->parentId == world.rootId(),
          "cyclic_reparent_is_rejected_without_mutation");
    check(world.reparentEntity(*mesh, world.rootId(), &error) && world.find(*mesh)->parentId == world.rootId(),
          "valid_reparent_updates_hierarchy");
    check(!world.removeEntity(world.rootId(), nullptr, &error), "world_root_cannot_be_removed");

    SceneSelection selection;
    check(selection.select(world, *camera) && selection.primary() == *camera, "replace_selection_sets_primary");
    check(selection.select(world, *mesh, SceneSelectionMode::Add) && selection.selected().size() == 2 &&
          selection.primary() == *mesh, "add_selection_preserves_existing_entity");
    check(selection.select(world, *camera, SceneSelectionMode::Toggle) && selection.selected().size() == 1,
          "toggle_selection_removes_entity");
    check(selection.primaryEntity(world) && selection.primaryEntity(world)->id == *mesh,
          "selection_resolves_primary_entity");

    SceneHierarchyModel hierarchy;
    hierarchy.rebuild(world, selection);
    check(hierarchy.rows().size() == 4 && hierarchy.rows().front().kind == EntityKind::World,
          "hierarchy_root_is_expanded_by_default");
    hierarchy.setExpanded(*folder, false);
    hierarchy.rebuild(world, selection);
    check(hierarchy.rows().size() == 4, "empty_collapsed_folder_does_not_hide_siblings");
    check(world.reparentEntity(*mesh, *folder, &error), "mesh_reparented_for_expansion_test");
    hierarchy.rebuild(world, selection);
    check(hierarchy.rows().size() == 3, "collapsed_folder_hides_descendants");
    hierarchy.setExpanded(*folder, true);
    hierarchy.rebuild(world, selection);
    const auto meshRow = std::find_if(hierarchy.rows().begin(), hierarchy.rows().end(), [&](const HierarchyRow& row)
    {
        return row.id == *mesh;
    });
    check(hierarchy.rows().size() == 4 && meshRow != hierarchy.rows().end() && meshRow->depth == 2,
          "expanded_folder_projects_child_depth");
    hierarchy.setSearchText("hero");
    hierarchy.rebuild(world, selection);
    check(hierarchy.rows().size() == 3 && hierarchy.rows()[0].kind == EntityKind::World &&
          hierarchy.rows().back().id == *mesh, "search_includes_matches_and_ancestor_chain");
    hierarchy.setSearchText("missing");
    hierarchy.rebuild(world, selection);
    check(hierarchy.rows().empty(), "search_with_no_match_has_no_fake_rows");

    check(world.save(file, &error) && std::filesystem::file_size(file) > 32, "scene_saves_versioned_atomic_archive");
    const auto loaded = SceneWorld::load(file, &error);
    check(loaded && loaded->validate(&error) && loaded->rootId() == world.rootId() &&
          loaded->entityCount() == world.entityCount(), "scene_round_trip_preserves_root_and_count");
    const auto* loadedMesh = loaded ? loaded->find(*mesh) : nullptr;
    check(loadedMesh && loadedMesh->label == "Hero Rock" && loadedMesh->transform == transform &&
          loadedMesh->assetId == world.find(*mesh)->assetId && !loadedMesh->visible && loadedMesh->locked,
          "scene_round_trip_preserves_entity_payload");

    {
        std::fstream bytes(file, std::ios::binary | std::ios::in | std::ios::out);
        bytes.seekp(24);
        char value = 0;
        bytes.read(&value, 1);
        bytes.seekp(24);
        value ^= 0x55;
        bytes.write(&value, 1);
    }
    check(!SceneWorld::load(file, &error) && !error.empty(), "corrupt_scene_archive_is_rejected");

    std::vector<am::core::Guid> removed;
    check(world.removeEntity(*folder, &removed, &error) && removed.size() == 2 && !world.find(*mesh),
          "subtree_removal_returns_and_removes_every_descendant");
    selection.reconcile(world);
    check(selection.selected().empty() && !selection.primary().isValid(), "selection_repairs_after_subtree_removal");
    check(world.validate(&error), "scene_remains_valid_after_subtree_removal");

    SceneWorld deep;
    auto parent = deep.rootId();
    bool depthRejected = false;
    for (std::size_t index = 0; index < 300; ++index)
    {
        const auto next = deep.createEntity(EntityKind::Empty, "Node" + std::to_string(index), parent, {}, &error);
        if (!next) { depthRejected = true; break; }
        parent = *next;
    }
    check(depthRejected && deep.validate(&error), "excessive_hierarchy_depth_is_rejected_and_rolled_back");

    SceneWorld broad;
    const auto scaleStart = std::chrono::steady_clock::now();
    bool broadCreated = true;
    for (std::size_t index = 0; index < 20000; ++index)
    {
        if (!broad.createEntity(EntityKind::Empty, "Actor" + std::to_string(index), broad.rootId(), {}, &error))
        { broadCreated = false; break; }
    }
    SceneSelection broadSelection;
    SceneHierarchyModel broadHierarchy;
    broadHierarchy.rebuild(broad, broadSelection);
    const auto scaleElapsed = std::chrono::steady_clock::now() - scaleStart;
    check(broadCreated && broadHierarchy.rows().size() == 20001,
          "twenty_thousand_actor_hierarchy_projects_every_row");
    check(scaleElapsed < std::chrono::seconds(5), "indexed_broad_hierarchy_build_is_bounded");

    std::filesystem::remove_all(base, ec);
    std::cout << "SUMMARY|checks=" << checks << "|failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
