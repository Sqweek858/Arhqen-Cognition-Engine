#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace
{
    int failures = 0;

    std::string read(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        std::ostringstream output;
        output << input.rdbuf();
        return output.str();
    }

    void check(bool value, std::string_view name)
    {
        std::cout << (value ? "PASS|" : "FAIL|") << name << '\n';
        if (!value) ++failures;
    }
}

int main()
{
    const std::string shell = read("Source/Private/Ui/AceShellUi.cpp");
    const std::string header = read("Source/Public/ArhqenCognitionEngine/Ui/AceShellUi.h");
    const auto renderStart = shell.find("void AceShellUi::renderEngineEditorMode");
    const auto renderEnd = shell.find("void AceShellUi::renderAquariumFullScreen3DMode", renderStart);
    const std::string editorRender = renderStart != std::string::npos && renderEnd != std::string::npos
        ? shell.substr(renderStart, renderEnd - renderStart) : std::string{};

    check(header.find("EditorWorkspaceController engineWorkspaceController_") != std::string::npos,
        "engine_mode_owns_workspace_controller");
    check(shell.find("if (engineEditorModeActive_)\n        {\n            renderEngineEditorMode(ctx);") != std::string::npos,
        "engine_mode_has_dedicated_render_route");
    check(editorRender.find("renderAquariumDx12ViewportSurface") != std::string::npos,
        "engine_mode_reuses_real_dx12_viewport");
    check(editorRender.find("renderAquariumViewportHudLayer") == std::string::npos &&
        editorRender.find("aquariumTelemetryOverlayRect_ = makeUiRect(0, 0, 0, 0)") != std::string::npos,
        "engine_mode_suppresses_aquarium_telemetry");
    check(editorRender.find("snapshot.scenarioName") == std::string::npos &&
        editorRender.find("snapshot.plannerName") == std::string::npos &&
        editorRender.find("World:") == std::string::npos,
        "engine_panels_do_not_expose_ai_scenario_identity");
    check(editorRender.find("Content Browser") == std::string::npos,
        "unfinished_content_browser_is_not_exposed");
    check(editorRender.find("Scene") != std::string::npos && editorRender.find("Editor Camera") != std::string::npos &&
        editorRender.find("Move speed") != std::string::npos,
        "engine_panels_show_real_generic_scene_and_camera_data");
    check(shell.find("L\"Engine\"") != std::string::npos && shell.find("L\"AI Details\"") != std::string::npos,
        "engine_and_ai_mode_switches_are_visible");
    check(shell.find("EditorWorkspaceLayout::load") != std::string::npos &&
        shell.find("engineWorkspaceController_.layout().save") != std::string::npos,
        "engine_layout_load_and_save_are_wired");
    check(shell.find("engineWorkspaceController_.pointerDown") != std::string::npos &&
        shell.find("engineWorkspaceController_.pointerMove") != std::string::npos &&
        shell.find("engineWorkspaceController_.pointerUp") != std::string::npos,
        "engine_splitter_pointer_lifecycle_is_wired");
    check(shell.find("engineWorkspaceController_.cancelPointerInteraction") != std::string::npos &&
        shell.find("case WM_CAPTURECHANGED") != std::string::npos,
        "engine_splitter_capture_loss_is_recoverable");
    check(shell.find("if (!engineEditorModeActive_)\n        {\n            renderAquariumViewportHudLayer") != std::string::npos,
        "fast_viewport_path_also_suppresses_editor_telemetry");
    check(header.find("CommandRegistry engineCommandRegistry_") != std::string::npos &&
        shell.find("initializeEngineCommands();") != std::string::npos,
        "editor_owns_initialized_command_registry");
    check(shell.find("window.toggle_outliner") != std::string::npos &&
        shell.find("window.toggle_details") != std::string::npos &&
        shell.find("window.reset_layout") != std::string::npos,
        "window_menu_commands_are_real");
    check(shell.find("view.reset_camera") != std::string::npos &&
        shell.find("view.camera_speed") != std::string::npos &&
        shell.find("view.toggle_console") != std::string::npos,
        "view_menu_commands_are_real");
    check(shell.find("help.shortcuts") != std::string::npos && shell.find("help.about") != std::string::npos,
        "help_menu_commands_are_real");
    check(shell.find("engineCommandRegistry_.resolve(chord, {\"Editor\"})") != std::string::npos &&
        shell.find("repeated && !command->repeatable") != std::string::npos,
        "editor_shortcuts_use_context_and_repeat_policy");
    check(editorRender.find("renderEngineCommandSurface(ctx, topBarHeight)") != std::string::npos,
        "command_surface_paints_as_final_editor_overlay");
    check(shell.find("commandIds = {\"window.toggle_outliner\", \"window.toggle_details\", \"window.reset_layout\"}") != std::string::npos &&
        shell.find("commandIds = {\"view.reset_camera\", \"view.camera_speed\", \"view.toggle_console\"}") != std::string::npos,
        "menus_expose_only_registered_backend_actions");
    check(shell.find("!engineOpenMenu_.empty() ||") != std::string::npos &&
        shell.find("editor-menu-open") != std::string::npos,
        "editor_menus_join_dx12_overlay_policy");

    std::cout << (failures == 0 ? "PASS|" : "FAIL|") << "ace_engine_mode_shell_probe\n";
    return failures == 0 ? 0 : 1;
}
