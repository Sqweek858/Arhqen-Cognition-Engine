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
}

int main()
{
    using namespace ace::aquarium_render;

    AceAquariumRealCamera camera;
    camera.SetPosition({0.0f, 0.0f, 0.0f});
    camera.SetYawPitch(0.0f, 0.65f);

    AceAqCameraInput input{};
    input.moveForward = true;
    camera.UpdateFromInput(input, 1.0f);

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
    camera.UpdateFromInput(input, 1.0f);

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
    camera.UpdateFromInput(input, 1.0f);

    if (Length(camera.Position()) <= camera.MoveSpeed() + 0.03f)
    {
        pass("single_hwnd_diagonal_normalized");
    }
    else
    {
        fail("single_hwnd_diagonal_normalized");
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
