#include "ArhqenCognitionEngine/Editor/Scene/AceAquariumScenePickAdapter.h"

#include <iostream>
#include <string_view>
#include <vector>

namespace
{
    int checks = 0, failures = 0;
    void check(bool value, std::string_view name)
    { ++checks; std::cout << (value ? "PASS|" : "FAIL|") << name << '\n'; if (!value) ++failures; }
}

int main()
{
    using ace::aquarium_render::AceAqRenderPrimitive;
    using ace::aquarium_render::AceAqRenderPrimitiveKind;
    using namespace am::editor::scene;
    am::core::scene::SceneWorld world;
    const auto folder = world.createEntity(am::core::scene::EntityKind::Folder, "Preview", world.rootId());
    const auto geometry = world.createEntity(am::core::scene::EntityKind::StaticMesh, "Geometry", *folder);
    const auto grid = world.createEntity(am::core::scene::EntityKind::Empty, "Grid", *folder);
    ScenePicker picker;

    AceAqRenderPrimitive gridLine;
    gridLine.Kind = AceAqRenderPrimitiveKind::GridLine;
    gridLine.X = 0.0f; gridLine.Y = 0.0f; gridLine.SizeX = 10.0f; gridLine.SizeY = 0.0f;
    AceAqRenderPrimitive block;
    block.Kind = AceAqRenderPrimitiveKind::Block;
    block.X = 1.0f; block.Y = 1.0f; block.SizeX = 1.0f; block.SizeY = 1.0f; block.SizeZ = 0.85f;
    AceAqRenderPrimitive debug = block;
    debug.Kind = AceAqRenderPrimitiveKind::DebugLabel;
    const std::vector primitives{gridLine, block, debug};

    auto sync = AquariumScenePickAdapter::synchronize(primitives, world, *geometry, *grid, picker);
    check(sync.renderedGeometryPrimitives == 1 && sync.renderedGridPrimitives == 1 &&
          sync.publishedProxies == 2 && picker.size() == 2, "rendered_primitive_classes_publish_two_scene_proxies");
    auto hit = picker.raycast({{5.5, 0.5, -5.0}, {0.0, 0.0, 1.0}});
    check(hit && hit->entityId == *geometry, "rendered_block_bounds_map_to_geometry_entity");
    hit = picker.raycast({{4.0, 0.015, -5.0}, {0.0, 0.0, 1.0}});
    check(hit && hit->entityId == *grid, "rendered_grid_bounds_map_to_grid_entity");

    world.setVisible(*geometry, false);
    sync = AquariumScenePickAdapter::synchronize(primitives, world, *geometry, *grid, picker);
    hit = picker.raycast({{5.5, 0.5, -5.0}, {0.0, 0.0, 1.0}});
    check(!hit, "hidden_scene_entity_keeps_render_proxy_unpickable");
    world.setVisible(*geometry, true); world.setLocked(*geometry, true);
    sync = AquariumScenePickAdapter::synchronize(primitives, world, *geometry, *grid, picker);
    hit = picker.raycast({{5.5, 0.5, -5.0}, {0.0, 0.0, 1.0}});
    check(!hit, "locked_scene_entity_keeps_render_proxy_unselectable");
    world.setLocked(*geometry, false);

    sync = AquariumScenePickAdapter::synchronize({}, world, *geometry, *grid, picker);
    check(sync.publishedProxies == 0 && picker.size() == 0, "empty_render_frame_clears_stale_pick_proxies");
    sync = AquariumScenePickAdapter::synchronize(primitives, world, {}, *grid, picker);
    check(sync.publishedProxies == 0 && picker.size() == 0, "invalid_binding_ids_publish_nothing");

    std::cout << "SUMMARY|checks=" << checks << "|failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
