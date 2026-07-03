#include "ArhqenCognitionEngine/Editor/Viewport/AceViewportProjection.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

namespace
{
    int checks = 0, failures = 0;
    void check(bool value, std::string_view name)
    { ++checks; std::cout << (value ? "PASS|" : "FAIL|") << name << '\n'; if (!value) ++failures; }
    bool near(double left, double right, double tolerance = 1.0e-9)
    { return std::abs(left - right) <= tolerance; }
}

int main()
{
    using namespace am::editor::viewport;
    ViewportCamera camera;
    camera.position = {10.0, 20.0, 30.0};
    ViewportArea area{100.0, 50.0, 1600.0, 900.0};
    check(ViewportProjection::valid(camera, area), "canonical_camera_and_area_are_valid");

    auto ray = ViewportProjection::screenRay(camera, area, 900.0, 500.0);
    check(ray && near(ray->origin.x, 10.0) && near(ray->origin.y, 20.0) && near(ray->origin.z, 30.05) &&
          near(ray->direction.x, 0.0) &&
          near(ray->direction.y, 0.0) && near(ray->direction.z, 1.0), "viewport_center_ray_matches_forward");
    ray = ViewportProjection::screenRay(camera, area, 1700.0, 500.0);
    check(ray && ray->direction.x > 0.0 && ray->direction.z > 0.0, "right_edge_ray_points_camera_right");
    ray = ViewportProjection::screenRay(camera, area, 900.0, 50.0);
    check(ray && ray->direction.y > 0.0, "top_edge_ray_points_camera_up");
    check(!ViewportProjection::screenRay(camera, area, 99.0, 500.0), "screen_points_outside_viewport_are_rejected");

    auto point = ViewportProjection::project(camera, area, {10.0, 20.0, 40.0});
    check(point && near(point->x, 900.0) && near(point->y, 500.0) &&
          near(point->viewDepth, 10.0) && point->insideViewport, "forward_world_point_projects_to_center");
    point = ViewportProjection::project(camera, area, {20.0, 20.0, 40.0});
    check(point && point->x > 900.0, "camera_right_world_point_projects_right");
    check(!ViewportProjection::project(camera, area, {10.0, 20.0, 29.0}), "point_behind_camera_is_rejected");
    check(!ViewportProjection::project(camera, area, {10.0, 20.0, 300.1}), "point_beyond_far_plane_is_rejected");

    ray = ViewportProjection::screenRay(camera, area, 1300.0, 275.0);
    const auto roundTripPoint = am::core::scene::Vec3d{
        camera.position.x + ray->direction.x * 25.0,
        camera.position.y + ray->direction.y * 25.0,
        camera.position.z + ray->direction.z * 25.0};
    point = ViewportProjection::project(camera, area, roundTripPoint);
    check(point && near(point->x, 1300.0, 1.0e-8) && near(point->y, 275.0, 1.0e-8),
          "screen_ray_and_projection_round_trip");

    const auto units = ViewportProjection::worldUnitsPerPixel(camera, area, 10.0);
    const auto twiceUnits = ViewportProjection::worldUnitsPerPixel(camera, area, 20.0);
    check(units && twiceUnits && near(*twiceUnits, *units * 2.0), "gizmo_pixel_scale_grows_linearly_with_depth");
    check(!ViewportProjection::worldUnitsPerPixel(camera, area, 0.01), "pixel_scale_respects_near_plane");

    ViewportCamera rotated = camera;
    rotated.forward = {1.0, 0.0, 0.0}; rotated.right = {0.0, 0.0, -1.0}; rotated.up = {0.0, 1.0, 0.0};
    ray = ViewportProjection::screenRay(rotated, area, 900.0, 500.0);
    check(ray && near(ray->direction.x, 1.0) && near(ray->direction.z, 0.0), "arbitrary_camera_basis_is_supported");

    ViewportCamera scaledBasis = camera;
    scaledBasis.forward = {0.0, 0.0, 7.0}; scaledBasis.right = {4.0, 0.0, 0.2}; scaledBasis.up = {0.0, 3.0, 0.0};
    ray = ViewportProjection::screenRay(scaledBasis, area, 900.0, 500.0);
    check(ray && near(ray->direction.z, 1.0), "basis_is_normalized_and_orthogonalized");

    ViewportCamera invalid = camera; invalid.forward = {};
    check(!ViewportProjection::valid(invalid, area), "zero_forward_is_rejected");
    invalid = camera; invalid.right = invalid.forward;
    check(!ViewportProjection::valid(invalid, area), "parallel_right_is_rejected");
    invalid = camera; invalid.up = {0.0, -1.0, 0.0};
    check(!ViewportProjection::valid(invalid, area), "opposite_handed_basis_is_rejected");
    invalid = camera; invalid.verticalFieldOfViewRadians = 3.14159265358979323846;
    check(!ViewportProjection::valid(invalid, area), "degenerate_field_of_view_is_rejected");
    invalid = camera; invalid.nearPlane = 10.0; invalid.farPlane = 5.0;
    check(!ViewportProjection::valid(invalid, area), "inverted_clip_planes_are_rejected");
    ViewportArea invalidArea = area; invalidArea.width = 0.0;
    check(!ViewportProjection::valid(camera, invalidArea), "zero_size_viewport_is_rejected");
    check(!ViewportProjection::screenRay(camera, area, std::numeric_limits<double>::quiet_NaN(), 0.0),
          "nonfinite_screen_coordinates_are_rejected");

    std::cout << "SUMMARY|checks=" << checks << "|failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
