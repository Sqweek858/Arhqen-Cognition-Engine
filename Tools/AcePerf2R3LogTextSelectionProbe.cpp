#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace
{
    std::string ReadFile(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    bool Has(const std::string& haystack, const std::string& needle)
    {
        return haystack.find(needle) != std::string::npos;
    }

    bool Check(const char* name, bool ok)
    {
        std::cout << (ok ? "PASS|" : "FAIL|") << name << '\n';
        return ok;
    }
}

int main()
{
    const auto root = std::filesystem::current_path();
    const auto shellHeader = ReadFile(root / "Source" / "Public" / "ArhqenCognitionEngine" / "Ui" / "AceShellUi.h");
    const auto shellCpp = ReadFile(root / "Source" / "Private" / "Ui" / "AceShellUi.cpp");
    const auto docs = ReadFile(root / "Docs" / "ACE_PERF2R3_LOG_TEXT_SELECTION.md");

    bool ok = true;
    ok &= Check("perf2r3_selection_state_fields", Has(shellHeader, "engineLogSelectionAnchor_") && Has(shellHeader, "engineLogSelectionActive_") && Has(shellHeader, "engineLogTextSelecting_"));
    ok &= Check("perf2r3_text_hit_test", Has(shellHeader, "hitTestEngineLogText") && Has(shellCpp, "AceShellUi::hitTestEngineLogText"));
    ok &= Check("perf2r3_multiline_selected_text", Has(shellCpp, "selectedEngineLogText") && Has(shellCpp, "out << L\"\\r\\n\""));
    ok &= Check("perf2r3_clipboard_copy", Has(shellCpp, "copyEngineLogTextSelectionToClipboard") && Has(shellCpp, "D2DClipboard::writeText"));
    ok &= Check("perf2r3_ctrl_c_prefers_log_selection", Has(shellCpp, "keyboard.ctrl && wParam == 'C'") && Has(shellCpp, "engineLogHasTextSelection() && copyEngineLogTextSelectionToClipboard()"));
    ok &= Check("perf2r3_ctrl_a_select_all_log", Has(shellCpp, "keyboard.ctrl && wParam == 'A'") && Has(shellCpp, "selectAllEngineLogText()"));
    ok &= Check("perf2r3_drag_selection", Has(shellCpp, "engineLogTextSelecting_ = true") && Has(shellCpp, "engineLogSelectionActive_ = hitTestEngineLogText(x, y)"));
    ok &= Check("perf2r3_drag_autoscroll_lmb_lifetime", Has(shellCpp, "bool leftButtonDown") && Has(shellCpp, "Autoscroll must never outlive drag"));
    ok &= Check("perf2r3_scrollbar_drag_released", Has(shellCpp, "aquariumDraggingScroll_ == &engineLogOverlayScroll_") && Has(shellCpp, "endAquariumScrollbarDrag()"));
    ok &= Check("perf2r3_double_triple_click", Has(shellCpp, "WM_LBUTTONDBLCLK") && Has(shellCpp, "Double click selects one word") && Has(shellCpp, "Triple click selects the complete visual log row"));
    ok &= Check("perf2r3_lazy_line_layout_cache", Has(shellHeader, "engineLogOverlayLineLayouts_") && Has(shellCpp, "engineLogTextLayoutForLine"));
    ok &= Check("perf2r3_d2d_selection_highlight", Has(shellCpp, "renderEngineLogTextSelection") && Has(shellCpp, "FillRectangle(highlight.d2d()"));
    ok &= Check("perf2r3_single_selection_tint", Has(shellCpp, "PERF2R3.2") && Has(shellCpp, "SetOpacity(0.36f)") && !Has(shellCpp, "rowBand") && !Has(shellCpp, "SetOpacity(0.16f)") && !Has(shellCpp, "SetOpacity(0.48f)"));
    ok &= Check("perf2r3_ibeam_cursor", Has(shellCpp, "engineLogOverlayLogViewportRect_.contains(x, y)") && Has(shellCpp, "IDC_IBEAM"));
    ok &= Check("perf2r3_no_char_leak_to_main_input", Has(shellCpp, "return engineLogTextFocused_;") && Has(shellCpp, "printable chars must not"));
    ok &= Check("perf2r3_no_renderer_path_change", !Has(shellCpp, "DX12_GPU_COMPOSITED") && !Has(shellCpp, "submitAndPresentBgra8ToComposition"));
    ok &= Check("perf2r3_docs", Has(docs, "ACE-PERF2R3") && Has(docs, "No renderer/RHI changes") && Has(docs, "PERF2R3.2"));

    if (!ok)
    {
        return 1;
    }

    std::cout << "PASS|ace_perf2r3_log_text_selection_probe\n";
    return 0;
}
