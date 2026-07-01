#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace
{
    bool Has(const std::string& haystack, const std::string& needle)
    {
        return haystack.find(needle) != std::string::npos;
    }

    std::string ReadFile(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
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
    const auto shell = ReadFile(root / "Source" / "Private" / "Ui" / "AceShellUi.cpp");
    const auto console = ReadFile(root / "Source" / "Public" / "ArhqenCognitionEngine" / "Ui" / "AceEngineConsole.h");
    const auto docs = ReadFile(root / "Docs" / "ACE_PERF2R2_STAT_CONSOLE_DETAIL_RESTORE.md");

    bool ok = true;
    ok &= Check("perf2r2_stat_rhi_multiline", Has(shell, "[STAT_RHI]\\n") && Has(shell, "  viewport_mode: ") && Has(shell, "  readback_active: "));
    ok &= Check("perf2r2_stat_fps_multiline", Has(shell, "[STAT_FPS]\\n") && Has(shell, "  fps.avg: ") && Has(shell, "  fence_wait_ms: "));
    ok &= Check("perf2r2_detail_log_helper", Has(console, "AceEngineAppendLogLines") && Has(shell, "AceEngineAppendLogLines(logTag + \"_DETAIL\""));
    ok &= Check("perf2r2_console_wraps_long_lines", Has(shell, "aceWrapEngineConsoleLine") && Has(shell, "wrapped.begin()"));
    ok &= Check("perf2r2_d2d_quality_fields_visible", Has(shell, "ui_layer: D2D_RETAINED_OVERLAY") && Has(shell, "gpu_text_overlay: false") && Has(shell, "quality: preserved"));
    ok &= Check("perf2r2_no_new_stat_frame", !Has(shell, "stat_frame"));
    ok &= Check("perf2r2_docs_exist", Has(docs, "ACE-PERF2R2") && Has(docs, "No new `stat_frame`"));

    if (!ok)
    {
        return 1;
    }

    std::cout << "PASS|ace_perf2r2_stat_console_detail_probe\n";
    return 0;
}
