#include "ArhqenCognitionEngine/AquariumRender/AceAquariumCamera.h"

#include <cmath>
#include <iostream>
#include <string>

namespace
{
    int failures = 0;

    void pass(const std::string& name)
    {
        std::cout << "PASS|" << name << "\n";
    }

    void fail(const std::string& name, const std::string& detail = {})
    {
        ++failures;
        std::cout << "FAIL|" << name;
        if (!detail.empty())
        {
            std::cout << "|" << detail;
        }
        std::cout << "\n";
    }

    bool near(float a, float b, float eps = 0.035f)
    {
        return std::fabs(a - b) <= eps;
    }

    float distance(ace::aquarium_render::AceAqVec3 a, ace::aquarium_render::AceAqVec3 b)
    {
        return ace::aquarium_render::Length(ace::aquarium_render::Sub(a, b));
    }
}

int main()
{
    using namespace ace::aquarium_render;

    AceAquariumRealCamera camera;
    camera.SetPosition({0.0f, 0.0f, 0.0f});
    camera.SetYawPitch(0.0f, 0.0f);
    const auto f0 = camera.Forward();

    camera.SetYawPitch(1.5707963f, 0.55f);
    const auto f1 = camera.Forward();
    if (!near(f0.x, f1.x) && f1.y > 0.35f)
    {
        pass("forward_vector_changes_with_yaw_pitch");
    }
    else
    {
        fail("forward_vector_changes_with_yaw_pitch");
    }

    camera.SetPosition({0.0f, 0.0f, 0.0f});
    camera.SetYawPitch(0.0f, 0.80f);
    AceAqCameraInput input{};
    input.moveForward = true;
    camera.UpdateFromInput(input, 1.0f);
    const auto afterForwardPitch = camera.Position();
    if (afterForwardPitch.y > 3.0f && afterForwardPitch.x > 2.0f)
    {
        pass("w_uses_camera_forward_not_world_axis");
    }
    else
    {
        fail("w_uses_camera_forward_not_world_axis", "W did not follow pitched camera forward");
    }

    camera.SetPosition({0.0f, 0.0f, 0.0f});
    camera.SetYawPitch(0.0f, -0.65f);
    input = {};
    input.moveForward = true;
    camera.UpdateFromInput(input, 1.0f);
    if (camera.Position().y < -2.5f)
    {
        pass("w_moves_down_when_looking_down");
    }
    else
    {
        fail("w_moves_down_when_looking_down");
    }

    camera.SetPosition({0.0f, 0.0f, 0.0f});
    camera.SetYawPitch(1.5707963f, 0.0f);
    input = {};
    input.moveRight = true;
    camera.UpdateFromInput(input, 1.0f);
    if (camera.Position().x > 5.0f && std::fabs(camera.Position().z) < 0.15f)
    {
        pass("d_uses_camera_right_from_orientation");
    }
    else
    {
        fail("d_uses_camera_right_from_orientation");
    }

    camera.SetPosition({0.0f, 0.0f, 0.0f});
    camera.SetYawPitch(1.5707963f, 0.90f);
    input = {};
    input.moveUp = true;
    camera.UpdateFromInput(input, 1.0f);
    if (near(camera.Position().x, 0.0f, 0.01f) && camera.Position().y > 5.0f && near(camera.Position().z, 0.0f, 0.01f))
    {
        pass("e_uses_world_up_strict");
    }
    else
    {
        fail("e_uses_world_up_strict");
    }

    camera.SetPosition({0.0f, 0.0f, 0.0f});
    input = {};
    input.moveDown = true;
    camera.UpdateFromInput(input, 1.0f);
    if (camera.Position().y < -5.0f && near(camera.Position().x, 0.0f, 0.01f) && near(camera.Position().z, 0.0f, 0.01f))
    {
        pass("q_uses_world_down_strict");
    }
    else
    {
        fail("q_uses_world_down_strict");
    }

    camera.SetPosition({0.0f, 0.0f, 0.0f});
    camera.SetYawPitch(0.0f, 0.0f);
    input = {};
    input.moveForward = true;
    input.moveRight = true;
    camera.UpdateFromInput(input, 1.0f);
    const float diagonalDistance = Length(camera.Position());
    if (diagonalDistance <= camera.MoveSpeed() + 0.02f)
    {
        pass("diagonal_movement_is_normalized");
    }
    else
    {
        fail("diagonal_movement_is_normalized");
    }

    camera.SetYawPitch(0.0f, 0.0f);
    camera.ApplyMouseDelta(100.0f, 0.0f);
    if (camera.Forward().z < -0.25f)
    {
        pass("mouse_right_turns_view_right");
    }
    else
    {
        fail("mouse_right_turns_view_right");
    }

    camera.SetYawPitch(0.0f, 0.0f);
    camera.ApplyMouseDelta(0.0f, -100000.0f);
    if (camera.Pitch() < 1.49f)
    {
        pass("pitch_is_clamped");
    }
    else
    {
        fail("pitch_is_clamped");
    }

    camera.SetPosition({0.0f, 0.0f, 0.0f});
    camera.SetYawPitch(0.0f, 0.0f);
    input = {};
    input.moveForward = true;
    camera.UpdateFromInput(input, 0.25f);
    const auto quarter = camera.Position();

    camera.SetPosition({0.0f, 0.0f, 0.0f});
    camera.UpdateFromInput(input, 1.0f);
    const auto full = camera.Position();

    if (distance(quarter, {0.0f, 0.0f, 0.0f}) < distance(full, {0.0f, 0.0f, 0.0f}) * 0.35f)
    {
        pass("camera_state_is_dt_based");
    }
    else
    {
        fail("camera_state_is_dt_based");
    }

    const auto view = camera.ViewMatrix();
    const auto proj = camera.ProjectionMatrix(16.0f / 9.0f);
    const auto vp = camera.ViewProjectionMatrix(16.0f / 9.0f);
    bool matricesFinite = true;
    for (float value : view.m) { matricesFinite = matricesFinite && std::isfinite(value); }
    for (float value : proj.m) { matricesFinite = matricesFinite && std::isfinite(value); }
    for (float value : vp.m) { matricesFinite = matricesFinite && std::isfinite(value); }

    if (matricesFinite && view.m[15] == 1.0f && proj.m[0] > 0.0f && proj.m[5] > 0.0f)
    {
        pass("view_projection_matrices_exist");
    }
    else
    {
        fail("view_projection_matrices_exist");
    }

    return failures == 0 ? 0 : 1;
}
