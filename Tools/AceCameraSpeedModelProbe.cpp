#include "ArhqenCognitionEngine/Editor/Viewport/AceCameraSpeedModel.h"
#include "ArhqenCognitionEngine/AquariumRender/AceAquariumCamera.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace
{
    int checks = 0;

    void require(bool condition, const char* message)
    {
        ++checks;
        if (!condition)
        {
            std::cerr << "FAIL|" << message << '\n';
            std::exit(1);
        }
    }

    bool close(double lhs, double rhs, double epsilon = 1.0e-10)
    {
        return std::abs(lhs - rhs) <= epsilon;
    }

    std::string read(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        std::ostringstream output;
        output << input.rdbuf();
        return output.str();
    }
}

int main()
{
    using am::editor::CameraSpeedModel;

    CameraSpeedModel defaults;
    require(close(defaults.speed(), CameraSpeedModel::DefaultSpeed), "default_speed");
    require(defaults.normalizedLogPosition() > 0.0 && defaults.normalizedLogPosition() < 1.0,
        "default_normalized");

    CameraSpeedModel clampsLow(0.0);
    CameraSpeedModel clampsHigh(1.0e12);
    require(close(clampsLow.speed(), CameraSpeedModel::MinimumSpeed), "constructor_clamps_low");
    require(close(clampsHigh.speed(), CameraSpeedModel::MaximumSpeed), "constructor_clamps_high");
    require(close(clampsLow.normalizedLogPosition(), 0.0), "normalized_low");
    require(close(clampsHigh.normalizedLogPosition(), 1.0), "normalized_high");

    CameraSpeedModel slow(1.0);
    slow.applyWheelDelta(120, 1.0);
    const double slowLogGain = std::log10(slow.speed());
    require(slowLogGain > 0.0 && slowLogGain < 0.12, "single_notch_is_precise");

    CameraSpeedModel rapid(1.0);
    rapid.applyWheelDelta(120, 1.00);
    rapid.applyWheelDelta(120, 1.03);
    rapid.applyWheelDelta(120, 1.06);
    const double rapidLogGain = std::log10(rapid.speed());
    require(rapidLogGain > slowLogGain * 3.0, "rapid_sequence_accelerates");
    require(rapid.wheelMomentum() > 1.0, "rapid_sequence_has_momentum");

    CameraSpeedModel spaced(1.0);
    spaced.applyWheelDelta(120, 1.0);
    spaced.applyWheelDelta(120, 2.0);
    require(std::log10(spaced.speed()) < rapidLogGain, "spaced_notches_stay_precise");

    const double beforeReverse = rapid.speed();
    rapid.applyWheelDelta(-120, 1.09);
    require(rapid.speed() < beforeReverse, "direction_reverses_immediately");
    require(close(rapid.wheelMomentum(), 1.0), "direction_change_resets_momentum");

    CameraSpeedModel multi(1.0);
    multi.applyWheelDelta(240, 1.0);
    require(multi.speed() > slow.speed(), "multiple_detents_are_preserved");

    CameraSpeedModel direct;
    require(close(direct.setSpeed(42.5), 42.5), "direct_value");
    require(close(direct.wheelMomentum(), 0.0), "direct_value_resets_momentum");
    require(close(direct.setSpeed(-1.0), CameraSpeedModel::MinimumSpeed), "direct_clamps_low");
    require(close(direct.setSpeed(1.0e9), CameraSpeedModel::MaximumSpeed), "direct_clamps_high");
    require(close(direct.setSpeed(std::numeric_limits<double>::quiet_NaN()), CameraSpeedModel::DefaultSpeed),
        "nan_has_safe_default");
    require(close(direct.setSpeed(std::numeric_limits<double>::infinity()), CameraSpeedModel::DefaultSpeed),
        "infinity_has_safe_default");

    CameraSpeedModel limits(CameraSpeedModel::MaximumSpeed);
    for (int i = 0; i < 100; ++i) limits.applyWheelDelta(120, 2.0 + i * 0.01);
    require(close(limits.speed(), CameraSpeedModel::MaximumSpeed), "wheel_clamps_high");
    for (int i = 0; i < 200; ++i) limits.applyWheelDelta(-120, 4.0 + i * 0.01);
    require(close(limits.speed(), CameraSpeedModel::MinimumSpeed), "wheel_clamps_low");
    require(std::isfinite(limits.speed()) && limits.speed() > 0.0, "never_nan_or_zero");

    CameraSpeedModel oddTime(1.0);
    oddTime.applyWheelDelta(120, 5.0);
    oddTime.applyWheelDelta(120, 4.0);
    require(std::isfinite(oddTime.speed()), "time_reversal_safe");
    oddTime.applyWheelDelta(120, std::numeric_limits<double>::quiet_NaN());
    require(std::isfinite(oddTime.speed()), "nonfinite_time_safe");

    ace::aquarium_render::AceAquariumRealCamera defaultCamera;
    ace::aquarium_render::AceAquariumRealCamera fastCamera;
    fastCamera.SetMoveSpeed(100.0f);
    ace::aquarium_render::AceAqCameraInput forward{};
    forward.moveForward = true;
    const auto defaultStart = defaultCamera.Position();
    const auto fastStart = fastCamera.Position();
    for (int i = 0; i < 120; ++i)
    {
        defaultCamera.UpdateFromInput(forward, 1.0f / 120.0f);
        fastCamera.UpdateFromInput(forward, 1.0f / 120.0f);
    }
    const double defaultTravel = ace::aquarium_render::Length(
        ace::aquarium_render::Sub(defaultCamera.Position(), defaultStart));
    const double fastTravel = ace::aquarium_render::Length(
        ace::aquarium_render::Sub(fastCamera.Position(), fastStart));
    require(fastTravel > defaultTravel * 10.0, "selected_high_speed_is_reachable");
    fastCamera.SetMoveSpeed(std::numeric_limits<float>::infinity());
    require(close(fastCamera.MoveSpeed(), 5.25), "camera_rejects_nonfinite_speed");

    const std::string shell = read("Source/Private/Ui/AceShellUi.cpp");
    const std::string header = read("Source/Public/ArhqenCognitionEngine/Ui/AceShellUi.h");
    require(header.find("CameraSpeedModel cameraSpeedModel_") != std::string::npos,
        "shell_owns_shared_speed_property");
    require(shell.find("else if (handleAquariumWheel(x, y, wheel))") != std::string::npos &&
        shell.find("else if (handleCameraSpeedWheel(x, y, wheel))") != std::string::npos,
        "scroll_panels_precede_viewport_speed_route");
    require(shell.find("renderCameraSpeedControl(ctx, aquariumEmbeddedViewportRect_)") != std::string::npos &&
        shell.find("renderCameraSpeedControl(ctx, viewportSurface)") != std::string::npos,
        "control_is_rendered_in_editor_and_ai_modes");
    require(shell.find("cameraSpeedPopupOpen_ ||") != std::string::npos &&
        shell.find("camera-speed-popup-open") != std::string::npos,
        "popup_forces_parent_composition");
    require(shell.find("VK_RETURN") != std::string::npos && shell.find("closeCameraSpeedPopup(true)") != std::string::npos &&
        shell.find("closeCameraSpeedPopup(false)") != std::string::npos,
        "popup_commit_and_cancel_are_wired");
    require(shell.find("std::stod(text, &consumed)") != std::string::npos &&
        shell.find("consumed != text.size()") != std::string::npos,
        "direct_entry_requires_complete_number");

    std::cout << "PASS|ace_camera_speed_model|checks=" << checks << '\n';
    return 0;
}
