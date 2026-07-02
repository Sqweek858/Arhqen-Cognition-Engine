#include "ArhqenCognitionEngine/AquariumRender/AceAquariumCamera.h"

#include <algorithm>
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
        if (!detail.empty()) { std::cout << "|" << detail; }
        std::cout << "\n";
    }

    bool finiteMatrix(const ace::aquarium_render::AceAqMat4& m)
    {
        for (float value : m.m)
        {
            if (!std::isfinite(value))
            {
                return false;
            }
        }
        return true;
    }

    float distance(ace::aquarium_render::AceAqVec3 a, ace::aquarium_render::AceAqVec3 b)
    {
        return ace::aquarium_render::Length(ace::aquarium_render::Sub(a, b));
    }

    void advance(
        ace::aquarium_render::AceAquariumRealCamera& camera,
        const ace::aquarium_render::AceAqCameraInput& input,
        float seconds)
    {
        constexpr float step = 1.0f / 120.0f;
        for (float elapsed = 0.0f; elapsed < seconds; elapsed += step)
        {
            camera.UpdateFromInput(input, std::min(step, seconds - elapsed));
        }
    }
}

int main()
{
    using namespace ace::aquarium_render;

    AceAquariumRealCamera camera;
    camera.SetPosition({0.0f, 0.0f, 0.0f});
    camera.SetYawPitch(0.0f, 0.65f);

    AceAqCameraInput input{};
    input.moveForward = true;
    advance(camera, input, 1.0f);

    if (camera.Position().y > 2.5f && camera.Position().x > 3.0f)
    {
        pass("single_hwnd_w_uses_camera_forward");
    }
    else
    {
        fail("single_hwnd_w_uses_camera_forward");
    }

    camera.SetPosition({0.0f, 0.0f, 0.0f});
    camera.SetYawPitch(1.5707963f, 0.0f);
    input = {};
    input.moveRight = true;
    advance(camera, input, 1.0f);

    if (camera.Position().x > 5.0f && std::fabs(camera.Position().z) < 0.20f)
    {
        pass("single_hwnd_d_uses_camera_right");
    }
    else
    {
        fail("single_hwnd_d_uses_camera_right");
    }

    camera.SetPosition({0.0f, 0.0f, 0.0f});
    input = {};
    input.moveForward = true;
    input.moveRight = true;
    advance(camera, input, 1.0f);

    if (Length(camera.Position()) <= camera.MoveSpeed() + 0.03f)
    {
        pass("single_hwnd_diagonal_normalized");
    }
    else
    {
        fail("single_hwnd_diagonal_normalized");
    }

    AceAquariumRealCamera camera30;
    AceAquariumRealCamera camera240;
    camera30.SetPosition({0.0f, 0.0f, 0.0f});
    camera240.SetPosition({0.0f, 0.0f, 0.0f});
    camera30.SetYawPitch(0.35f, -0.20f);
    camera240.SetYawPitch(0.35f, -0.20f);
    input = {};
    input.moveForward = true;
    for (int i = 0; i < 30; ++i) { camera30.UpdateFromInput(input, 1.0f / 30.0f); }
    for (int i = 0; i < 240; ++i) { camera240.UpdateFromInput(input, 1.0f / 240.0f); }
    if (distance(camera30.Position(), camera240.Position()) < 0.035f)
    {
        pass("single_hwnd_movement_frame_rate_independent");
    }
    else
    {
        fail("single_hwnd_movement_frame_rate_independent");
    }

    const auto releasePosition = camera240.Position();
    input = {};
    advance(camera240, input, 0.25f);
    if (distance(releasePosition, camera240.Position()) < 0.25f)
    {
        pass("single_hwnd_keyboard_release_brakes_promptly");
    }
    else
    {
        fail("single_hwnd_keyboard_release_brakes_promptly");
    }

    const auto beforeHitch = camera240.Position();
    input.moveForward = true;
    camera240.UpdateFromInput(input, 0.5f);
    if (distance(beforeHitch, camera240.Position()) < 0.30f)
    {
        pass("single_hwnd_hitch_does_not_teleport");
    }
    else
    {
        fail("single_hwnd_hitch_does_not_teleport");
    }

    const auto vp = camera.ViewProjectionMatrix(16.0f / 9.0f);
    if (finiteMatrix(vp))
    {
        pass("single_hwnd_view_projection_finite");
    }
    else
    {
        fail("single_hwnd_view_projection_finite");
    }

    return failures == 0 ? 0 : 1;
}
