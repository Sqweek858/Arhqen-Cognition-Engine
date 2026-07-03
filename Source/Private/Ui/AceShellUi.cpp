#include "ArhqenCognitionEngine/Ui/AceShellUi.h"

#include "ArhqenCognitionEngine/Ui/Core/UiFramework.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DCyberEffects.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DCyberBackgroundField.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DCyberText.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DGlassEffects.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DBlurRuntime.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DCachedEffects.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DDisplayMetrics.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DTextLayoutFoundation.h"
#include "ArhqenCognitionEngine/Ui/Core/AceUiStyleSet.h"
#include "ArhqenCognitionEngine/AquariumRender/AceAquariumViewport.h"
#include "ArhqenCognitionEngine/AquariumRender/AceAquariumEmbeddedDx12Viewport.h"

#include <Windowsx.h>
#include <d2d1.h>
#include <d2d1_1.h>
#include <d3d11.h>
#include <d3d12.h>
#include <d3d11on12.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dxgi1_6.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <stdexcept>
#include <sstream>
#include <cwctype>
#include <tuple>
#include <utility>
#include <vector>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace
{
    std::string aceUtf8FromWide(std::wstring_view value)
    {
        if (value.empty()) return {};
        const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (length <= 0) return {};
        std::string result(static_cast<std::size_t>(length), '\0');
        if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()),
            result.data(), length, nullptr, nullptr) != length) return {};
        return result;
    }

    D2D1_COLOR_F aceAquariumCompositeColor(float r, float g, float b, float a)
    {
        return D2D1::ColorF(
            std::clamp(r, 0.0f, 1.0f),
            std::clamp(g, 0.0f, 1.0f),
            std::clamp(b, 0.0f, 1.0f),
            std::clamp(a, 0.0f, 1.0f));
    }

    const char* aceDxgiFormatName(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_B8G8R8A8_UNORM: return "B8G8R8A8_UNORM";
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return "B8G8R8A8_UNORM_SRGB";
        case DXGI_FORMAT_R8G8B8A8_UNORM: return "R8G8B8A8_UNORM";
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return "R8G8B8A8_UNORM_SRGB";
        case DXGI_FORMAT_R16G16B16A16_FLOAT: return "R16G16B16A16_FLOAT";
        case DXGI_FORMAT_UNKNOWN: return "UNKNOWN";
        default: return "OTHER";
        }
    }

    const char* aceD3D12DimensionName(D3D12_RESOURCE_DIMENSION dimension)
    {
        switch (dimension)
        {
        case D3D12_RESOURCE_DIMENSION_BUFFER: return "BUFFER";
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D: return "TEXTURE1D";
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D: return "TEXTURE2D";
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D: return "TEXTURE3D";
        default: return "UNKNOWN";
        }
    }

    std::string aceHex32(std::uint64_t value)
    {
        std::ostringstream out;
        out << "0x" << std::hex << value;
        return out.str();
    }

    std::string aceD3D12ResourceDescText(const D3D12_RESOURCE_DESC& desc)
    {
        std::ostringstream out;
        out << "d3d12_desc{dim=" << aceD3D12DimensionName(desc.Dimension)
            << ",format=" << aceDxgiFormatName(desc.Format) << "(" << static_cast<unsigned>(desc.Format) << ")"
            << ",size=" << desc.Width << "x" << desc.Height
            << ",array=" << desc.DepthOrArraySize
            << ",mips=" << desc.MipLevels
            << ",samples=" << desc.SampleDesc.Count << ":" << desc.SampleDesc.Quality
            << ",layout=" << static_cast<unsigned>(desc.Layout)
            << ",flags=" << aceHex32(desc.Flags) << "}";
        return out.str();
    }

    std::string aceDxgiSurfaceDescText(const DXGI_SURFACE_DESC& desc)
    {
        std::ostringstream out;
        out << "dxgi_surface{format=" << aceDxgiFormatName(desc.Format) << "(" << static_cast<unsigned>(desc.Format) << ")"
            << ",size=" << desc.Width << "x" << desc.Height
            << ",samples=" << desc.SampleDesc.Count << ":" << desc.SampleDesc.Quality << "}";
        return out.str();
    }

    std::string aceTexture2DDescText(const D3D11_TEXTURE2D_DESC& desc)
    {
        std::ostringstream out;
        out << "d3d11_tex2d{format=" << aceDxgiFormatName(desc.Format) << "(" << static_cast<unsigned>(desc.Format) << ")"
            << ",size=" << desc.Width << "x" << desc.Height
            << ",bind=" << aceHex32(desc.BindFlags)
            << ",misc=" << aceHex32(desc.MiscFlags)
            << ",usage=" << static_cast<unsigned>(desc.Usage) << "}";
        return out.str();
    }

    am::ui::UiRect aceRectAround(float x, float y, float width, float height)
    {
        return am::ui::makeUiRect(x - width * 0.5f, y - height * 0.5f, x + width * 0.5f, y + height * 0.5f);
    }

    am::ui::UiRect aceRectBetween(float ax, float ay, float bx, float by, float minThickness)
    {
        const float left = std::min(ax, bx);
        const float top = std::min(ay, by);
        const float right = std::max(ax, bx);
        const float bottom = std::max(ay, by);
        return am::ui::makeUiRect(left, top, std::max(left + minThickness, right), std::max(top + minThickness, bottom));
    }


    std::wstring aceTrimCommand(std::wstring value)
    {
        const auto first = value.find_first_not_of(L" \t\r\n");
        const auto last = value.find_last_not_of(L" \t\r\n");
        if (first == std::wstring::npos || last == std::wstring::npos)
        {
            return L"";
        }
        value = value.substr(first, last - first + 1);
        if (!value.empty() && value.front() == L'/')
        {
            value.erase(value.begin());
        }
        for (wchar_t& ch : value)
        {
            ch = static_cast<wchar_t>(std::towlower(ch));
        }
        return value;
    }

    std::wstring aceFormatDouble(double value, int decimals = 2)
    {
        if (value < 0.0)
        {
            return L"n/a";
        }
        std::wstringstream ss;
        ss << std::fixed << std::setprecision(decimals) << value;
        return ss.str();
    }

    std::string aceFormatDoubleUtf8(double value, int decimals = 2)
    {
        if (value < 0.0)
        {
            return "n/a";
        }
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(decimals) << value;
        return ss.str();
    }

    std::wstring aceVec3Wide(const ace::aquarium_render::AceAqVec3& v)
    {
        std::wstringstream ss;
        ss << std::fixed << std::setprecision(2) << L"(" << v.x << L", " << v.y << L", " << v.z << L")";
        return ss.str();
    }

    std::string aceVec3Utf8(const ace::aquarium_render::AceAqVec3& v)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << "(" << v.x << "," << v.y << "," << v.z << ")";
        return ss.str();
    }

    std::wstring aceWideLossy(const std::string& text)
    {
        std::wstring out;
        out.reserve(text.size());
        for (unsigned char ch : text)
        {
            if (ch == '\t')
            {
                out.push_back(L' ');
                out.push_back(L' ');
                continue;
            }
            out.push_back(ch >= 32 ? static_cast<wchar_t>(ch) : L' ');
        }
        return out;
    }

    std::string aceNarrowLossy(const std::wstring& text)
    {
        std::string out;
        out.reserve(text.size());
        for (wchar_t ch : text)
        {
            if (ch == L'\t' || ch == L'\r' || ch == L'\n')
            {
                out.push_back(' ');
                continue;
            }
            out.push_back(ch >= 32 && ch <= 126 ? static_cast<char>(ch) : '?');
        }
        return out;
    }

    std::string aceNarrowLossyLine(const std::wstring& text)
    {
        std::string out;
        out.reserve(text.size());
        for (wchar_t ch : text)
        {
            if (ch == L'\t')
            {
                out.push_back(' ');
                out.push_back(' ');
                continue;
            }
            if (ch == L'\r' || ch == L'\n')
            {
                continue;
            }
            out.push_back(ch >= 32 && ch <= 126 ? static_cast<char>(ch) : '?');
        }
        return out;
    }

    std::vector<std::string> aceDetailLogLinesFromBody(const std::wstring& body)
    {
        std::vector<std::string> lines;
        if (body.find(L'\n') == std::wstring::npos)
        {
            return lines;
        }

        std::wstringstream input(body);
        std::wstring line;
        while (std::getline(input, line))
        {
            if (!line.empty() && line.back() == L'\r')
            {
                line.pop_back();
            }
            const std::string narrow = aceNarrowLossyLine(line);
            if (!narrow.empty())
            {
                lines.push_back(narrow);
            }
        }
        return lines;
    }

    std::vector<std::wstring> aceWrapEngineConsoleLine(const std::wstring& line, std::size_t maxChars = 118)
    {
        std::vector<std::wstring> out;
        if (line.size() <= maxChars)
        {
            out.push_back(line);
            return out;
        }

        std::size_t start = 0;
        bool continuation = false;
        while (start < line.size())
        {
            const std::wstring prefix = continuation ? L"  " : L"";
            const std::size_t available = maxChars > prefix.size() ? maxChars - prefix.size() : maxChars;
            std::size_t end = std::min(line.size(), start + available);
            if (end < line.size())
            {
                const std::size_t softMin = start + std::min<std::size_t>(available, 48);
                const std::size_t space = line.rfind(L' ', end);
                if (space != std::wstring::npos && space >= softMin && space > start)
                {
                    end = space;
                }
            }

            std::wstring chunk = line.substr(start, end - start);
            while (!chunk.empty() && chunk.front() == L' ')
            {
                chunk.erase(chunk.begin());
            }
            while (!chunk.empty() && chunk.back() == L' ')
            {
                chunk.pop_back();
            }
            out.push_back(prefix + (chunk.empty() ? L" " : chunk));

            start = end;
            while (start < line.size() && line[start] == L' ')
            {
                ++start;
            }
            continuation = true;
        }
        return out;
    }

    std::wstring aceJoinLogLines(const std::vector<std::wstring>& lines, std::size_t maxLines)
    {
        if (lines.empty())
        {
            return L"";
        }
        const std::size_t count = std::min(maxLines, lines.size());
        const std::size_t start = lines.size() - count;
        std::wstringstream ss;
        for (std::size_t i = start; i < lines.size(); ++i)
        {
            ss << lines[i];
            if (i + 1 < lines.size())
            {
                ss << L'\n';
            }
        }
        return ss.str();
    }

    template <typename Clock = std::chrono::steady_clock>
    double aceElapsedMs(typename Clock::time_point start, typename Clock::time_point end)
    {
        return std::chrono::duration<double, std::milli>(end - start).count();
    }
}


    struct AceSingleHwndProjectedPoint
    {
        float x = 0.0f;
        float y = 0.0f;
        float depth = 0.0f;
        bool visible = false;
    };

    AceSingleHwndProjectedPoint aceProjectSingleHwnd3D(
        const ace::aquarium_render::AceAqMat4& viewProjection,
        float worldX,
        float worldY,
        float worldZ,
        am::ui::UiRect rect)
    {
        const float clipX =
            worldX * viewProjection.m[0] +
            worldY * viewProjection.m[4] +
            worldZ * viewProjection.m[8] +
            viewProjection.m[12];

        const float clipY =
            worldX * viewProjection.m[1] +
            worldY * viewProjection.m[5] +
            worldZ * viewProjection.m[9] +
            viewProjection.m[13];

        const float clipZ =
            worldX * viewProjection.m[2] +
            worldY * viewProjection.m[6] +
            worldZ * viewProjection.m[10] +
            viewProjection.m[14];

        const float clipW =
            worldX * viewProjection.m[3] +
            worldY * viewProjection.m[7] +
            worldZ * viewProjection.m[11] +
            viewProjection.m[15];

        if (clipW <= 0.001f)
        {
            return {};
        }

        const float ndcX = clipX / clipW;
        const float ndcY = clipY / clipW;

        AceSingleHwndProjectedPoint out{};
        out.x = rect.left + (ndcX * 0.5f + 0.5f) * rect.width();
        out.y = rect.top + (0.5f - ndcY * 0.5f) * rect.height();
        out.depth = clipZ / clipW;
        out.visible = out.x >= rect.left - 160.0f && out.x <= rect.right + 160.0f &&
            out.y >= rect.top - 160.0f && out.y <= rect.bottom + 160.0f;
        return out;
    }


namespace am::ui
{
    AceShellUi::~AceShellUi()
    {
        destroyAquariumResizeShieldWindow();
        discardDeviceResources();
    }

    void AceShellUi::setSubmitHandler(SubmitHandler handler)
    {
        submitHandler_ = std::move(handler);
    }

    void AceShellUi::setSnapshotProvider(SnapshotProvider provider)
    {
        snapshotProvider_ = std::move(provider);
    }

    void AceShellUi::setSuggestionProvider(SuggestionProvider provider)
    {
        suggestionProvider_ = std::move(provider);
    }

    void AceShellUi::setInspectorProvider(InspectorProvider provider)
    {
        inspectorProvider_ = std::move(provider);
    }

    void AceShellUi::setLayoutProfilePath(std::filesystem::path path)
    {
        layoutProfilePath_ = std::move(path);
    }

    double aceMonotonicSeconds()
    {
        return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    void AceShellUi::setVsyncEnabled(bool enabled)
    {
        uiVsyncEnabled_ = enabled;
    }

    void AceShellUi::setContentBrowserController(
        am::editor::content_browser::ContentBrowserController* controller) noexcept
    {
        contentBrowserController_ = controller;
    }

    void AceShellUi::setRuntimeFrameDeltaSeconds(double deltaSeconds)
    {
        runtimeFrameDeltaSeconds_ = std::clamp(deltaSeconds, 0.0, 1.0);
    }

    void AceShellUi::flushPendingPaint()
    {
        if (parent_)
        {
            // Keyboard camera movement is updated from tick(), while rendering is
            // hosted by WM_PAINT. Flush now so translation and mouse-look both
            // reach the screen in the tick that produced them.
            UpdateWindow(parent_);
        }
    }

    bool AceShellUi::create(HWND parent, int width, int height, std::string* error)
    {
        parent_ = parent;
        width_ = std::max(width, 760);
        height_ = std::max(height, 520);
        refreshDisplayMetrics("create");
        uiStyleSet_ = AceUiStyleSet::MakeDefaultArhqen();

        if (!createDeviceIndependentResources(error))
        {
            return false;
        }

        loadLayoutProfile();
        seedInitialState();
        refreshSnapshot();
        refreshSuggestions();
        layout(width_, height_);

        if (!createDeviceResources(error))
        {
            return false;
        }

        created_ = true;
        invalidate();
        return true;
    }

    void AceShellUi::layout(int width, int height)
    {
        const int nextWidth = std::max(width, 920);
        const int nextHeight = std::max(height, 620);
        const bool backbufferSizeChanged = width_ != nextWidth || height_ != nextHeight;
        width_ = nextWidth;
        height_ = nextHeight;

        const float appBarHeight = 58.0f;
        const float sidebarWidth = 272.0f;
        const float outerPad = 22.0f;
        const float contentPad = 26.0f;
        (void)contentPad;

        appTopBarRect_ = makeUiRect(0.0f, 0.0f, static_cast<float>(width_), appBarHeight);
        headerRect_ = appTopBarRect_;
        brandLogoRect_ = makeUiRect(22.0f, 13.0f, 54.0f, 45.0f);
        settingsButtonRect_ = makeUiRect(static_cast<float>(width_) - 122.0f, 12.0f, static_cast<float>(width_) - 22.0f, 46.0f);
        environmentButtonRect_ = makeUiRect(settingsButtonRect_.left - 150.0f, 12.0f, settingsButtonRect_.left - 12.0f, 46.0f);

        sidebarRect_ = makeUiRect(0.0f, appBarHeight, sidebarWidth, static_cast<float>(height_));
        newConversationRect_ = makeUiRect(22.0f, appBarHeight + 24.0f, sidebarWidth - 22.0f, appBarHeight + 66.0f);
        conversationListRect_ = makeUiRect(18.0f, newConversationRect_.bottom + 28.0f, sidebarWidth - 18.0f, static_cast<float>(height_) - 28.0f);

        const float contentLeft = sidebarWidth;
        const float contentTop = appBarHeight;
        const float contentWidth = static_cast<float>(width_) - contentLeft;
        const float contentHeight = static_cast<float>(height_) - contentTop;
        (void)contentHeight;

        const float chatMaxWidth = 860.0f;
        const float chatWidth = std::min(chatMaxWidth, std::max(520.0f, contentWidth - outerPad * 2.0f));
        const float chatLeft = contentLeft + (contentWidth - chatWidth) * 0.5f;
        const float chatRight = chatLeft + chatWidth;

        const float statusHeight = 0.0f;
        (void)statusHeight;
        const float sendSize = 46.0f;
        const float inputAvailableWidth = chatWidth - sendSize - 12.0f;
        const float inputHeight = input_.preferredHeightForWidth(inputAvailableWidth);
        const float inputBottom = static_cast<float>(height_) - 24.0f;
        const float inputTop = inputBottom - inputHeight;

        mainRect_ = makeUiRect(
            chatLeft,
            contentTop + 26.0f,
            chatRight,
            static_cast<float>(height_) - 18.0f
        );

        toolbarRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);

        conversationRect_ = makeUiRect(
            chatLeft,
            contentTop + 32.0f,
            chatRight,
            inputTop - 18.0f
        );

        inputRect_ = makeUiRect(
            chatLeft,
            inputTop,
            chatRight - sendSize - 12.0f,
            inputBottom
        );

        sendCircleRect_ = makeUiRect(
            chatRight - sendSize,
            inputBottom - sendSize,
            chatRight,
            inputBottom
        );

        sendButtonRect_ = sendCircleRect_;

        // ACE-CLEAN0: no visible status bar. It looked cheap and clipped text for no useful reason.
        statusRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);

        // ACE-CLEAN0 minimal shell: advanced workspace regions are intentionally not visible.
        tabRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);
        workspaceRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);
        inspectorRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);
        rightSplitterRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);
        inspectorSplitterRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);
        layoutOverlayRect_ = makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);

        autocompleteRect_ = makeUiRect(
            inputRect_.left,
            inputRect_.top - 190.0f,
            std::min(inputRect_.right, inputRect_.left + 460.0f),
            inputRect_.top - 10.0f
        );

        const float modalWidth = std::clamp(static_cast<float>(width_) * 0.50f, 480.0f, 760.0f);
        const float modalHeight = std::clamp(static_cast<float>(height_) * 0.50f, 320.0f, 520.0f);
        settingsModalRect_ = makeUiRect(
            (static_cast<float>(width_) - modalWidth) * 0.5f,
            (static_cast<float>(height_) - modalHeight) * 0.5f,
            (static_cast<float>(width_) + modalWidth) * 0.5f,
            (static_cast<float>(height_) + modalHeight) * 0.5f
        );

        settingsModalCloseRect_ = makeUiRect(
            settingsModalRect_.right - 46.0f,
            settingsModalRect_.top + 12.0f,
            settingsModalRect_.right - 14.0f,
            settingsModalRect_.top + 44.0f
        );

        // ACE-AQ3D2: Environment is now a full workspace, not a centered settings modal.
        environmentModalRect_ = makeUiRect(
            0.0f,
            appTopBarRect_.bottom,
            static_cast<float>(width_),
            static_cast<float>(height_)
        );
        environmentModalCloseRect_ = makeUiRect(
            environmentModalRect_.left + 18.0f,
            environmentModalRect_.top + 18.0f,
            environmentModalRect_.left + 104.0f,
            environmentModalRect_.top + 54.0f
        );

        const float paletteWidth = std::min(680.0f, static_cast<float>(width_) - 140.0f);
        const float paletteLeft = (static_cast<float>(width_) - paletteWidth) * 0.5f;
        commandPalette_.setRect(makeUiRect(paletteLeft, 88.0f, paletteLeft + paletteWidth, 520.0f));

        const float diagnosticsWidth = std::min(700.0f, static_cast<float>(width_) - 140.0f);
        const float diagnosticsLeft = (static_cast<float>(width_) - diagnosticsWidth) * 0.5f;
        diagnosticsRect_ = makeUiRect(diagnosticsLeft, 92.0f, diagnosticsLeft + diagnosticsWidth, 620.0f);

        toastRect_ = makeUiRect(
            static_cast<float>(width_) - 400.0f,
            76.0f,
            static_cast<float>(width_) - 22.0f,
            static_cast<float>(height_) - 22.0f
        );

        const float helpWidth = std::min(820.0f, static_cast<float>(width_) - 140.0f);
        const float helpLeft = (static_cast<float>(width_) - helpWidth) * 0.5f;
        shortcutHelpRect_ = makeUiRect(helpLeft, 92.0f, helpLeft + helpWidth, 620.0f);

        conversationRowRects_.clear();
        conversationRenameRects_.clear();

        float rowY = conversationListRect_.top + 28.0f;
        const float rowHeight = 38.0f;
        const float rowGap = 8.0f;

        for (std::size_t i = 0; i < conversations_.size() && rowY + rowHeight <= conversationListRect_.bottom; ++i)
        {
            UiRect row = makeUiRect(conversationListRect_.left + 8.0f, rowY, conversationListRect_.right - 8.0f, rowY + rowHeight);
            conversationRowRects_.push_back(row);
            conversationRenameRects_.push_back(makeUiRect(row.right - 62.0f, row.top + 7.0f, row.right - 8.0f, row.bottom - 7.0f));
            rowY += rowHeight + rowGap;
        }

        messageList_.setRect(conversationRect_);
        input_.setRect(inputRect_);
        sendButton_.setRect(sendCircleRect_);
        sendButton_.setLabel(L"");
        sendButton_.setEnabled(true);
        statusBar_.setRect(statusRect_);
        autocomplete_.setRect(autocompleteRect_);
        diagnostics_.setRect(diagnosticsRect_);
        toastCenter_.setRect(toastRect_);
        shortcutHelp_.setRect(shortcutHelpRect_);
        layoutOverlay_.setRect(layoutOverlayRect_);

        sidebar_.setRect(sidebarRect_);
        workspaceTabs_.setRect(tabRect_);
        workspacePanel_.setRect(workspaceRect_);
        inspectorPanel_.setRect(inspectorRect_);
        workspaceSplitter_.setEnabled(false);
        inspectorSplitter_.setEnabled(false);

        uiRetainedLayout_.Clear();
        const auto rootNode = uiRetainedLayout_.AddNode(L"root", makeUiRect(0.0f, 0.0f, static_cast<float>(width_), static_cast<float>(height_)), AceUiLayoutAxis::Vertical);
        auto* rootLayout = uiRetainedLayout_.Node(rootNode);
        if (rootLayout)
        {
            rootLayout->padding = 0.0f;
            rootLayout->gap = 0.0f;
        }
        auto topNode = uiRetainedLayout_.AddNode(L"app.topbar", appTopBarRect_, AceUiLayoutAxis::None, rootNode);
        auto sideNode = uiRetainedLayout_.AddNode(L"sidebar", sidebarRect_, AceUiLayoutAxis::Vertical, rootNode);
        auto mainNode = uiRetainedLayout_.AddNode(L"main", mainRect_, AceUiLayoutAxis::Vertical, rootNode);
        if (auto* n = uiRetainedLayout_.Node(topNode)) { n->minHeight = appTopBarRect_.height(); }
        if (auto* n = uiRetainedLayout_.Node(sideNode)) { n->minWidth = sidebarRect_.width(); n->weight = 1.0f; }
        if (auto* n = uiRetainedLayout_.Node(mainNode)) { n->weight = 1.0f; }
        uiRetainedLayout_.Arrange(rootNode, makeUiRect(0.0f, 0.0f, static_cast<float>(width_), static_cast<float>(height_)));
        ++aceUi6RetainedLayoutFrameCount_;

        if (renderTarget_)
        {
            if (backbufferSizeChanged)
            {
                std::string resizeError;
                if (!resizeD2DDeviceContextBackbufferTarget(width_, height_, &resizeError))
                {
                    // A failed ResizeBuffers invalidates every device-dependent
                    // object. Never continue into gradient creation with the now
                    // released renderTarget_; the next paint recreates the stack.
                    discardDeviceResources();
                    invalidate();
                    return;
                }
            }
            if (windowLiveResizeActive_)
            {
                // ACE-AQ3D8: gradient_recreation_deferred_or_documented. During
                // live resize, keep existing D2D gradient resources and rebuild
                // once on WM_EXITSIZEMOVE to avoid paint/resource churn.
                gradientsDirty_ = true;
            }
            else
            {
                createGradients(nullptr);
                gradientsDirty_ = false;
            }
        }

        invalidate();
    }

    void AceShellUi::refreshDisplayMetrics(const char* reason)
    {
        (void)reason;
        displayMetrics_ = D2DDisplayMetrics::capture(parent_);
        uiDpiScale_ = displayMetrics_.valid ? displayMetrics_.uiScale : D2DDisplayMetrics::dpiScaleForWindow(parent_);
        ++displayMetricsRefreshCount_;
    }

    float AceShellUi::dpiScale() const
    {
        return std::clamp(uiDpiScale_, 0.75f, 2.50f);
    }

    LRESULT AceShellUi::handleWindowMessage(UINT message, WPARAM wParam, LPARAM lParam, bool* handled)
    {
        if (handled)
        {
            *handled = false;
        }

        switch (message)
        {
        case WM_PAINT:
            if (handled) { *handled = true; }
            ++parentPaintCount_;
            render();
            ValidateRect(parent_, nullptr);
            return 0;

        case WM_ERASEBKGND:
            if (handled) { *handled = true; }
            return 1;

        case WM_DPICHANGED:
        {
            ++dpiChangedMessageCount_;
            refreshDisplayMetrics("WM_DPICHANGED");
            if (lParam && parent_)
            {
                const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
                SetWindowPos(
                    parent_,
                    nullptr,
                    suggested->left,
                    suggested->top,
                    suggested->right - suggested->left,
                    suggested->bottom - suggested->top,
                    SWP_NOZORDER | SWP_NOACTIVATE
                );
            }
            RECT client{};
            if (parent_ && GetClientRect(parent_, &client))
            {
                handleResize(static_cast<int>(client.right - client.left), static_cast<int>(client.bottom - client.top));
            }
            invalidate();
            if (handled) { *handled = true; }
            return 0;
        }

        case WM_DISPLAYCHANGE:
            ++displayChangeMessageCount_;
            refreshDisplayMetrics("WM_DISPLAYCHANGE");
            layout(width_, height_);
            if (handled) { *handled = true; }
            return 0;

        case WM_NCLBUTTONDOWN:
            // ACE-AQ3D12: arm the resize shield before the modal sizing loop
            // starts. WM_ENTERSIZEMOVE/WM_SIZING can arrive after the first
            // compositor update, which is too late for the DX12 child area.
            if (wParam == HTLEFT || wParam == HTRIGHT || wParam == HTTOP || wParam == HTBOTTOM ||
                wParam == HTTOPLEFT || wParam == HTTOPRIGHT || wParam == HTBOTTOMLEFT || wParam == HTBOTTOMRIGHT)
            {
                beginWindowLiveResize();
            }
            break;

        case WM_SIZING:
            beginWindowLiveResize();
            if (!aquariumUseSingleHwndCompositeViewport_ && handleFrozenNativeResizeSizing(lParam))
            {
                if (handled) { *handled = true; }
                return TRUE;
            }
            // ACE-AQ3D14: the single-HWND composite viewport can live-resize with
            // the parent D2D target. Do not freeze the native rect anymore; that
            // workaround only existed for the legacy child-HWND swapchain path.
            break;

        case WM_ENTERSIZEMOVE:
            beginWindowLiveResize();
            if (handled) { *handled = true; }
            return 0;

        case WM_EXITSIZEMOVE:
            endWindowLiveResize();
            if (handled) { *handled = true; }
            return 0;

        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED)
            {
                handleResize(LOWORD(lParam), HIWORD(lParam));
            }
            if (handled) { *handled = true; }
            return 0;

        case WM_MOUSEWHEEL:
        {
            if (!commandPalette_.active())
            {
                const int wheel = GET_WHEEL_DELTA_WPARAM(wParam);
                POINT p;
                GetCursorPos(&p);
                ScreenToClient(parent_, &p);

                auto ctx = makeContext();
                if (engineEditorModeActive_ && handleContentBrowserMouseWheel(
                    ctx, static_cast<float>(p.x), static_cast<float>(p.y), wheel))
                {
                    invalidateRect(contentBrowserRect_);
                    if (handled) { *handled = true; }
                    return 0;
                }
                if (engineLogOverlayVisible_ && handleEngineLogOverlayWheel(ctx, static_cast<float>(p.x), static_cast<float>(p.y), wheel))
                {
                    invalidateRect(engineLogOverlayRect_.empty() ? aquariumEmbeddedViewportRect_ : engineLogOverlayRect_);
                    if (handled) { *handled = true; }
                    return 0;
                }

                if (environmentOpen_ && environmentModalRect_.contains(static_cast<float>(p.x), static_cast<float>(p.y)))
                {
                    const float x = static_cast<float>(p.x);
                    const float y = static_cast<float>(p.y);
                    if (cameraSpeedPopupOpen_ && cameraSpeedPopupRect_.contains(x, y))
                    {
                        // The direct-entry popup is a modal wheel barrier. A
                        // wheel over it must never leak into the viewport.
                    }
                    else if (handleAquariumWheel(x, y, wheel))
                    {
                        invalidateAquariumChrome();
                    }
                    else if (handleCameraSpeedWheel(x, y, wheel))
                    {
                        invalidateRect(inflateRect(cameraSpeedPopupOpen_ ? cameraSpeedPopupRect_ : cameraSpeedButtonRect_, 12.0f));
                    }
                    if (handled) { *handled = true; }
                    return 0;
                }

                if (input_.hitTest(static_cast<float>(p.x), static_cast<float>(p.y)))
                {
                    input_.onMouseWheel(ctx, static_cast<float>(p.x), static_cast<float>(p.y), wheel);
                }
                else
                {
                    messageList_.scrollBy(static_cast<float>(-wheel) * 0.42f);
                }

                invalidate();
            }
            if (handled) { *handled = true; }
            return 0;
        }

        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        {
            const float x = static_cast<float>(GET_X_LPARAM(lParam));
            const float y = static_cast<float>(GET_Y_LPARAM(lParam));

            mouseX_ = x;
            mouseY_ = y;
            cyberBackground_.setMouse(x, y);

            if (beginAquariumSingleHwndMouseLook(x, y, message == WM_RBUTTONDBLCLK ? "wm_rbutton_dblclk" : "wm_rbutton_down"))
            {
                if (handled) { *handled = true; }
                return 0;
            }

            if (environmentOpen_ && environmentModalRect_.contains(x, y))
            {
                // The Environment owns RMB while open so the OS context menu cannot
                // steal focus from the single-HWND viewport path. A non-viewport RMB
                // is still recorded by beginAquariumSingleHwndMouseLook as a miss or
                // overlay block above.
                if (handled) { *handled = true; }
                return 0;
            }
            break;
        }

        case WM_RBUTTONUP:
        {
            const float x = static_cast<float>(GET_X_LPARAM(lParam));
            const float y = static_cast<float>(GET_Y_LPARAM(lParam));

            mouseX_ = x;
            mouseY_ = y;
            cyberBackground_.setMouse(x, y);

            if (endAquariumSingleHwndMouseLook("wm_rbutton_up"))
            {
                if (handled) { *handled = true; }
                return 0;
            }

            if (environmentOpen_ && environmentModalRect_.contains(x, y))
            {
                if (handled) { *handled = true; }
                return 0;
            }
            break;
        }

        case WM_INPUT:
        {
            if (!aquariumSingleHwndMouseLookActive_ || !aquariumRawMouseRegistered_)
            {
                break;
            }

            RAWINPUT raw{};
            UINT rawSize = sizeof(raw);
            if (GetRawInputData(
                    reinterpret_cast<HRAWINPUT>(lParam),
                    RID_INPUT,
                    &raw,
                    &rawSize,
                    sizeof(RAWINPUTHEADER)) == rawSize &&
                raw.header.dwType == RIM_TYPEMOUSE &&
                (raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0)
            {
                aquariumPendingRawMouseX_ += raw.data.mouse.lLastX;
                aquariumPendingRawMouseY_ += raw.data.mouse.lLastY;
                ++aquariumRawMousePacketCount_;
            }
            if (handled) { *handled = true; }
            return 0;
        }

        case WM_CAPTURECHANGED:
            if (engineWorkspaceController_.draggingSplitter())
            {
                std::string ignored;
                engineWorkspaceController_.cancelPointerInteraction(&ignored);
                mouseCaptured_ = false;
                invalidate();
            }
            if (aquariumSingleHwndMouseCaptured_)
            {
                cancelAquariumSingleHwndMouseLook("capture_lost");
                if (handled) { *handled = true; }
                return 0;
            }
            if (engineLogTextSelecting_ || aquariumDraggingScroll_ == &engineLogOverlayScroll_)
            {
                engineLogTextSelecting_ = false;
                if (aquariumDraggingScroll_ == &engineLogOverlayScroll_)
                {
                    endAquariumScrollbarDrag();
                }
                mouseCaptured_ = false;
                invalidateRect(inflateRect(engineLogOverlayRect_, 8.0f));
            }
            break;

        case WM_CANCELMODE:
        case WM_KILLFOCUS:
            if (cameraSpeedPopupOpen_) closeCameraSpeedPopup(false);
            if (!engineOpenMenu_.empty()) closeEngineMenu();
            if (engineWorkspaceController_.draggingSplitter())
            {
                std::string ignored;
                engineWorkspaceController_.cancelPointerInteraction(&ignored);
                mouseCaptured_ = false;
                invalidate();
            }
            if (aquariumSingleHwndMouseLookActive_ || aquariumSingleHwndMouseCaptured_)
            {
                cancelAquariumSingleHwndMouseLook(message == WM_KILLFOCUS ? "kill_focus" : "cancel_mode");
                if (handled) { *handled = true; }
                return 0;
            }
            break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        {
            const float x = static_cast<float>(GET_X_LPARAM(lParam));
            const float y = static_cast<float>(GET_Y_LPARAM(lParam));

            if (shortcutHelp_.visible())
            {
                shortcutHelp_.onMouseDown(x, y);
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (environmentOpen_)
            {
                auto ctx = makeContext();
                if (handleCameraSpeedMouseDown(ctx, x, y))
                {
                    invalidateRect(inflateRect(cameraSpeedPopupOpen_ ? cameraSpeedPopupRect_ : cameraSpeedButtonRect_, 12.0f));
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (engineLogOverlayVisible_)
            {
                const std::uint32_t clickTime = static_cast<std::uint32_t>(GetMessageTime());
                const float maxClickDx = static_cast<float>(std::max(2, GetSystemMetrics(SM_CXDOUBLECLK) / 2));
                const float maxClickDy = static_cast<float>(std::max(2, GetSystemMetrics(SM_CYDOUBLECLK) / 2));
                const bool continuesClickSequence =
                    engineLogClickCount_ > 0 &&
                    clickTime - engineLogLastClickTime_ <= GetDoubleClickTime() &&
                    std::fabs(x - engineLogLastClickX_) <= maxClickDx &&
                    std::fabs(y - engineLogLastClickY_) <= maxClickDy;
                engineLogClickCount_ = continuesClickSequence ?
                    (engineLogClickCount_ >= 3 ? 1u : engineLogClickCount_ + 1u) : 1u;
                engineLogLastClickTime_ = clickTime;
                engineLogLastClickX_ = x;
                engineLogLastClickY_ = y;

                auto ctx = makeContext();
                if (handleEngineLogOverlayMouseDown(ctx, x, y, engineLogClickCount_))
                {
                    mouseCaptured_ = true;
                    SetCapture(parent_);
                    invalidateRect(engineLogOverlayRect_.empty() ? aquariumEmbeddedViewportRect_ : engineLogOverlayRect_);
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (settingsOpen_)
            {
                if (settingsModalCloseRect_.contains(x, y))
                {
                    settingsOpen_ = false;
                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }

                if (settingsModalRect_.contains(x, y))
                {
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (environmentOpen_)
            {
                if (engineEditorModeActive_)
                {
                    handleEngineEditorClick(x, y, message == WM_LBUTTONDBLCLK ? 2u : 1u);
                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }
                if (environmentModalCloseRect_.contains(x, y))
                {
                    if (aquarium3DModeActive_)
                    {
                        aquarium3DModeActive_ = false;
                        aquariumEmbeddedViewportVisible_ = false;
                        aquariumEmbeddedDx12Viewport_.Hide();
                        aquariumEmbeddedViewportStatus_ = L"DX12 Environment ready";
                    }
                    else
                    {
                        environmentOpen_ = false;
                        aquariumEmbeddedViewportVisible_ = false;
                        aquariumEmbeddedDx12Viewport_.Hide();
                    }
                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }

                if (environmentModalRect_.contains(x, y))
                {
                    if (beginAquariumPanelResize(x, y))
                    {
                        mouseCaptured_ = true;
                        SetCapture(parent_);
                        invalidateAquariumChrome();
                        if (handled) { *handled = true; }
                        return 0;
                    }

                    if (beginAquariumScrollbarDrag(x, y))
                    {
                        mouseCaptured_ = true;
                        SetCapture(parent_);
                        invalidateAquariumChrome();
                        if (handled) { *handled = true; }
                        return 0;
                    }

                    if (handleAquariumPanelClick(x, y))
                    {
                        // ACE-AQ3D7: button press updates only the Environment chrome;
                        // it must not full-invalidate the parent or touch the child DX12 HWND.
                        invalidateAquariumChrome();
                    }

                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (environmentButtonRect_.contains(x, y))
            {
                environmentOpen_ = true;
                aquarium3DModeActive_ = false;
                aquariumEmbeddedViewportVisible_ = false;
                aquariumContentScroll_.offset = 0.0f;
                aquariumEmbeddedViewportStatus_ = L"DX12 Environment ready";
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (settingsButtonRect_.contains(x, y))
            {
                settingsOpen_ = true;
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (newConversationRect_.contains(x, y))
            {
                startNewConversation();
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            for (std::size_t i = 0; i < conversationRowRects_.size(); ++i)
            {
                if (i < conversationRenameRects_.size() && conversationRenameRects_[i].contains(x, y))
                {
                    prefillRenameConversation(i);
                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }

                if (conversationRowRects_[i].contains(x, y))
                {
                    selectConversation(i);
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (toastCenter_.onMouseDown(x, y))
            {
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (diagnostics_.visible())
            {
                diagnostics_.onMouseDown(x, y);
                toolbar_.setToggled(L"diagnostics", diagnostics_.visible());
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (commandPalette_.active())
            {
                commandPalette_.onMouseDown(x, y);
                focus_.set(D2DFocusTarget::CommandPalette);
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (autocomplete_.active() && autocomplete_.onMouseDown(x, y))
            {
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            auto ctx = makeContext();

            if (conversationRect_.contains(x, y))
            {
                if (auto messageSelection = messageList_.hitTest(ctx, x, y))
                {
                    inspectLocalMessage(messageSelection->id);
                    focus_.set(D2DFocusTarget::MessageList);
                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (input_.onMouseDown(ctx, x, y))
            {
                focus_.set(D2DFocusTarget::TextInput);
                mouseCaptured_ = true;
                SetCapture(parent_);
            }
            else if (sendButton_.rect().contains(x, y))
            {
                focus_.set(D2DFocusTarget::SendButton);
            }
            else if (conversationRect_.contains(x, y))
            {
                focus_.set(D2DFocusTarget::MessageList);
            }

            if (sendButton_.onMouseDown(x, y))
            {
                mouseCaptured_ = true;
                SetCapture(parent_);
            }

            invalidate();
            if (handled) { *handled = true; }
            return 0;
        }

        case WM_LBUTTONUP:
        {
            const float x = static_cast<float>(GET_X_LPARAM(lParam));
            const float y = static_cast<float>(GET_Y_LPARAM(lParam));

            if (cameraSpeedPopupOpen_)
            {
                auto ctx = makeContext();
                if (handleCameraSpeedMouseUp(ctx, x, y))
                {
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (engineEditorModeActive_ && engineWorkspaceController_.draggingSplitter())
            {
                std::string error;
                engineWorkspaceController_.pointerUp(x, y, &error);
                if (mouseCaptured_)
                {
                    mouseCaptured_ = false;
                    ReleaseCapture();
                }
                commitEngineWorkspaceLayout();
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (engineLogOverlayVisible_)
            {
                auto ctx = makeContext();
                if (handleEngineLogOverlayMouseUp(ctx, x, y))
                {
                    if (mouseCaptured_)
                    {
                        mouseCaptured_ = false;
                        ReleaseCapture();
                    }
                    invalidateRect(engineLogOverlayRect_.empty() ? aquariumEmbeddedViewportRect_ : engineLogOverlayRect_);
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (aquariumPanelResizeTarget_ != AquariumPanelResizeTarget::None)
            {
                endAquariumPanelResize();
                if (mouseCaptured_)
                {
                    mouseCaptured_ = false;
                    ReleaseCapture();
                }
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (aquariumDraggingScroll_)
            {
                endAquariumScrollbarDrag();
                if (mouseCaptured_)
                {
                    mouseCaptured_ = false;
                    ReleaseCapture();
                }
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (commandPalette_.active())
            {
                commandPalette_.onMouseUp(x, y);
                if (auto command = commandPalette_.takePendingCommand())
                {
                    executeCommand(*command);
                }
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (autocomplete_.active() && autocomplete_.onMouseUp(x, y))
            {
                acceptAutocomplete();
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            {
                auto ctx = makeContext();
                if (input_.onMouseUp(ctx, x, y))
                {
                    if (mouseCaptured_)
                    {
                        mouseCaptured_ = false;
                        ReleaseCapture();
                    }

                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (sendButton_.onMouseUp(x, y))
            {
                sendCurrentInput();
            }

            if (mouseCaptured_)
            {
                mouseCaptured_ = false;
                ReleaseCapture();
            }

            invalidate();
            if (handled) { *handled = true; }
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            const float x = static_cast<float>(GET_X_LPARAM(lParam));
            const float y = static_cast<float>(GET_Y_LPARAM(lParam));

            mouseX_ = x;
            mouseY_ = y;
            cyberBackground_.setMouse(x, y);

            if (cameraSpeedPopupOpen_)
            {
                auto ctx = makeContext();
                if (handleCameraSpeedMouseMove(ctx, x, y))
                {
                    invalidateRect(inflateRect(cameraSpeedPopupRect_, 8.0f));
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (aquariumSingleHwndMouseLookActive_ || aquariumSingleHwndMouseCaptured_)
            {
                // Raw input is relative, does not stop at a screen edge, and is
                // consumed once from tick().  WM_MOUSEMOVE remains a fallback
                // for systems where raw registration failed (for example RDP).
                if (!aquariumRawMouseRegistered_)
                {
                    updateAquariumSingleHwndMouseLook(x, y, "wm_mousemove_fallback");
                }
                if (handled) { *handled = true; }
                return 0;
            }

            if (engineEditorModeActive_ && engineWorkspaceController_.draggingSplitter())
            {
                std::string error;
                engineWorkspaceController_.pointerMove(x, y, &error);
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (aquariumPanelResizeTarget_ != AquariumPanelResizeTarget::None)
            {
                updateAquariumPanelResize(x, y);
                invalidateAquariumChrome();
                if (handled) { *handled = true; }
                return 0;
            }

            if (aquariumDraggingScroll_)
            {
                updateAquariumScrollbarDrag(x, y);
                if (aquariumDraggingScroll_ == &engineLogOverlayScroll_)
                {
                    invalidateRect(inflateRect(engineLogOverlayRect_, 8.0f));
                }
                else
                {
                    invalidateAquariumChrome();
                }
                if (handled) { *handled = true; }
                return 0;
            }

            if (engineLogOverlayVisible_)
            {
                auto ctx = makeContext();
                if (handleEngineLogOverlayMouseMove(ctx, x, y, (wParam & MK_LBUTTON) != 0))
                {
                    invalidateRect(engineLogOverlayRect_.empty() ? aquariumEmbeddedViewportRect_ : engineLogOverlayRect_);
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            bool changed = false;

            if (environmentOpen_ && environmentModalRect_.contains(x, y))
            {
                if (engineEditorModeActive_ && contentBrowserVisible() && contentBrowserRect_.contains(x, y))
                {
                    invalidateRect(contentBrowserRect_);
                    if (handled) { *handled = true; }
                    return 0;
                }
                // ACE-AQ3D6: hover/click stability. While the Environment UI is
                // active, do not run hover logic for the chat shell underneath it.
                // A hover-only move changes exactly one hot-id visual state and
                // does not structurally rebuild the 3D UI layout.
                const int oldHot = aquariumHoverHotId_;
                aquariumHoverHotId_ = aquariumHotIdAt(x, y);
                if (oldHot != aquariumHoverHotId_)
                {
                    ++aquariumHoverStateChangeCount_;
                    ++hoverInvalidationCount_;
                    invalidateRect(inflateRect(aquariumHotRectById(oldHot), 10.0f));
                    invalidateRect(inflateRect(aquariumHotRectById(aquariumHoverHotId_), 10.0f));
                }
                if (handled) { *handled = true; }
                return 0;
            }

            const bool oldSettingsHovered = settingsHovered_;
            const bool oldEnvironmentHovered = environmentHovered_;
            const bool oldNewHovered = newConversationHovered_;
            const std::size_t oldHoveredConversation = hoveredConversationIndex_;

            settingsHovered_ = settingsButtonRect_.contains(x, y);
            environmentHovered_ = environmentButtonRect_.contains(x, y);
            newConversationHovered_ = newConversationRect_.contains(x, y);
            hoveredConversationIndex_ = kNoHoveredConversation;

            for (std::size_t i = 0; i < conversationRowRects_.size(); ++i)
            {
                if (conversationRowRects_[i].contains(x, y))
                {
                    hoveredConversationIndex_ = i;
                    break;
                }
            }

            changed = changed || oldSettingsHovered != settingsHovered_ || oldEnvironmentHovered != environmentHovered_ || oldNewHovered != newConversationHovered_ || oldHoveredConversation != hoveredConversationIndex_;

            if (commandPalette_.active())
            {
                changed = commandPalette_.onMouseMove(x, y);
            }
            else if (autocomplete_.active())
            {
                changed = autocomplete_.onMouseMove(x, y);
            }
            else
            {
                auto ctx = makeContext();
                if (input_.onMouseMove(ctx, x, y))
                {
                    changed = true;
                }
                else
                {
                    changed = sendButton_.onMouseMove(x, y);
                }
            }

            if (changed)
            {
                invalidate();
            }

            if (handled) { *handled = true; }
            return 0;
        }

        case WM_SETCURSOR:
            updateCursor();
            if (handled) { *handled = true; }
            return TRUE;

        case WM_CHAR:
            if (cameraSpeedPopupOpen_ && cameraSpeedInputFocused_ && handleCameraSpeedChar(wParam))
            {
                invalidateRect(inflateRect(cameraSpeedPopupRect_, 8.0f));
                if (handled) { *handled = true; }
                return 0;
            }
            if (engineEditorModeActive_ && handleContentBrowserChar(wParam))
            {
                invalidateRect(contentBrowserRect_);
                if (handled) { *handled = true; }
                return 0;
            }
            if (environmentOpen_ && aquarium3DModeActive_ &&
                !(engineLogOverlayVisible_ && engineLogOverlayInputFocused_))
            {
                const wchar_t cameraChar = static_cast<wchar_t>(std::towlower(static_cast<wchar_t>(wParam)));
                if (cameraChar == L'w' || cameraChar == L'a' || cameraChar == L's' ||
                    cameraChar == L'd' || cameraChar == L'q' || cameraChar == L'e')
                {
                    // Camera flight owns these keys. Do not also edit the hidden
                    // chat input, rebuild layout and resize the D2D swapchain.
                    if (handled) { *handled = true; }
                    return 0;
                }
            }
            if ((wParam == L'`' || wParam == L'~') && D2DKeyboardState::current().noModifiers())
            {
                if (engineLogOverlaySuppressNextBacktickChar_)
                {
                    engineLogOverlaySuppressNextBacktickChar_ = false;
                }
                else
                {
                    toggleEngineLogOverlay();
                }
                if (handled) { *handled = true; }
                return 0;
            }

            if (engineLogOverlayVisible_ && handleEngineLogOverlayChar(wParam))
            {
                invalidateRect(engineLogOverlayRect_.empty() ? aquariumEmbeddedViewportRect_ : engineLogOverlayRect_);
                if (handled) { *handled = true; }
                return 0;
            }

            if (engineEditorModeActive_ && !commandPalette_.active())
            {
                if (handled) { *handled = true; }
                return 0;
            }

            if (commandPalette_.active())
            {
                if (commandPalette_.onChar(wParam))
                {
                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }
            }
            else if (input_.onChar(wParam))
            {
                refreshSuggestions();
                layout(width_, height_);
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }
            break;

        case WM_KEYDOWN:
        {
            const auto keyboard = D2DKeyboardState::current();

            if (cameraSpeedPopupOpen_ && handleCameraSpeedKeyDown(wParam, keyboard))
            {
                invalidateRect(inflateRect(cameraSpeedPopupOpen_ ? cameraSpeedPopupRect_ : cameraSpeedButtonRect_, 10.0f));
                if (handled) { *handled = true; }
                return 0;
            }

            if (engineEditorModeActive_ && wParam == VK_ESCAPE && !engineOpenMenu_.empty())
            {
                closeEngineMenu();
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (shortcutHelp_.visible() && shortcutHelp_.onKeyDown(wParam))
            {
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (engineEditorModeActive_ && handleContentBrowserKeyDown(wParam, keyboard))
            {
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            const bool repeatedKey = (static_cast<std::uintptr_t>(lParam) & (std::uintptr_t{1} << 30u)) != 0;
            if (handleEngineCommandShortcut(wParam, keyboard, repeatedKey))
            {
                if (handled) { *handled = true; }
                return 0;
            }

            if (engineEditorModeActive_ && wParam == VK_ESCAPE && engineWorkspaceController_.draggingSplitter())
            {
                std::string error;
                engineWorkspaceController_.cancelPointerInteraction(&error);
                if (mouseCaptured_)
                {
                    mouseCaptured_ = false;
                    ReleaseCapture();
                }
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (engineEditorModeActive_ && wParam == VK_ESCAPE && keyboard.noModifiers())
            {
                leaveEngineEditorMode();
                if (handled) { *handled = true; }
                return 0;
            }

            if (environmentOpen_ && aquarium3DModeActive_ && keyboard.noModifiers() &&
                !(engineLogOverlayVisible_ && engineLogOverlayInputFocused_) &&
                (wParam == 'W' || wParam == 'A' || wParam == 'S' ||
                 wParam == 'D' || wParam == 'Q' || wParam == 'E'))
            {
                if (handled) { *handled = true; }
                return 0;
            }

            if (keyboard.noModifiers() && wParam == VK_OEM_3)
            {
                engineLogOverlaySuppressNextBacktickChar_ = true;
                toggleEngineLogOverlay();
                if (handled) { *handled = true; }
                return 0;
            }

            if (engineLogOverlayVisible_ && handleEngineLogOverlayKeyDown(wParam, keyboard))
            {
                invalidateRect(engineLogOverlayRect_.empty() ? aquariumEmbeddedViewportRect_ : engineLogOverlayRect_);
                if (handled) { *handled = true; }
                return 0;
            }

            if (keyboard.ctrl && wParam == VK_UP)
            {
                if (auto previous = commandHistory_.previous())
                {
                    input_.replaceAllText(*previous);
                    refreshSuggestions();
                    statusBar_.setText(L"Command history: previous.");
                    layout(width_, height_);
                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (keyboard.ctrl && wParam == VK_DOWN)
            {
                if (auto next = commandHistory_.next())
                {
                    input_.replaceAllText(*next);
                    refreshSuggestions();
                    statusBar_.setText(L"Command history: next.");
                    layout(width_, height_);
                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (wParam == VK_F1)
            {
                shortcutHelp_.toggle();
                showToast(L"Help", shortcutHelp_.visible() ? L"Shortcut help opened." : L"Shortcut help closed.", D2DToastKind::Info);
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (wParam == VK_F9)
            {
                uiDebugOverlay_.Toggle();
                ++aceUi9DebugOverlayToggleCount_;
                refreshUiDebugOverlay();
                showToast(L"UI Debug", uiDebugOverlay_.Visible() ? L"UI debug overlay opened." : L"UI debug overlay closed.", D2DToastKind::Info);
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (wParam == VK_F12)
            {
                diagnostics_.toggle();
                toolbar_.setToggled(L"diagnostics", diagnostics_.visible());
                refreshDiagnostics();
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (diagnostics_.visible() && diagnostics_.onKeyDown(wParam))
            {
                toolbar_.setToggled(L"diagnostics", diagnostics_.visible());
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (D2DKeyboard::isCtrlChord(wParam, L'K') || D2DKeyboard::isCtrlChord(wParam, L'P'))
            {
                openCommandPalette();
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (commandPalette_.active())
            {
                if (commandPalette_.onKeyDown(wParam))
                {
                    if (auto command = commandPalette_.takePendingCommand())
                    {
                        executeCommand(*command);
                    }

                    invalidate();
                    if (handled) { *handled = true; }
                    return 0;
                }
            }

            if (autocomplete_.active() && autocomplete_.onKeyDown(wParam))
            {
                acceptAutocomplete();
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (D2DKeyboard::isCtrlChord(wParam, L'C'))
            {
                std::string error;
                if (input_.copySelectionToClipboard(parent_, &error))
                {
                    statusBar_.setText(L"Copied selection to clipboard.");
                }
                else
                {
                    statusBar_.setText(L"Nothing selected to copy.");
                }
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (D2DKeyboard::isCtrlChord(wParam, L'X'))
            {
                std::string error;
                if (input_.cutSelectionToClipboard(parent_, &error))
                {
                    statusBar_.setText(L"Cut selection to clipboard.");
                }
                else
                {
                    statusBar_.setText(L"Nothing selected to cut.");
                }
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (D2DKeyboard::isCtrlChord(wParam, L'V'))
            {
                std::string error;
                if (input_.pasteFromClipboard(parent_, &error))
                {
                    refreshSuggestions();
                    statusBar_.setText(L"Pasted clipboard text into input.");
                }
                else
                {
                    statusBar_.setText(L"Paste failed or clipboard had no text.");
                }
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (wParam == VK_TAB)
            {
                if (keyboard.shift)
                {
                    focus_.focusPrevious();
                }
                else
                {
                    focus_.focusNext();
                }

                input_.setFocused(focus_.is(D2DFocusTarget::TextInput));
                statusBar_.setText(std::wstring(L"Focus: ") + focus_.name());
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (wParam == VK_RETURN && !keyboard.shift)
            {
                if (focus_.is(D2DFocusTarget::SendButton))
                {
                    sendCurrentInput();
                }
                else if (input_.focused())
                {
                    sendCurrentInput();
                }

                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }

            if (input_.onKeyDown(wParam, keyboard.ctrl, keyboard.shift))
            {
                refreshSuggestions();
                layout(width_, height_);
                invalidate();
                if (handled) { *handled = true; }
                return 0;
            }
            break;
        }

        default:
            break;
        }

        return 0;
    }

    bool AceShellUi::created() const
    {
        return created_;
    }

    bool AceShellUi::wantsUnthrottledTick() const
    {
        if (!environmentOpen_ || !aquarium3DModeActive_)
        {
            return false;
        }
        if (aquariumControllerReady_ && aquariumController_.IsRunning())
        {
            return true;
        }
        if (aquariumSingleHwndMouseLookActive_ || aquariumSingleHwndMouseCaptured_ || windowLiveResizeActive_)
        {
            return true;
        }
        if (GetForegroundWindow() != parent_)
        {
            return false;
        }
        return (GetAsyncKeyState('W') & 0x8000) != 0 ||
            (GetAsyncKeyState('A') & 0x8000) != 0 ||
            (GetAsyncKeyState('S') & 0x8000) != 0 ||
            (GetAsyncKeyState('D') & 0x8000) != 0 ||
            (GetAsyncKeyState('Q') & 0x8000) != 0 ||
            (GetAsyncKeyState('E') & 0x8000) != 0;
    }

    void AceShellUi::tick(float dtSeconds)
    {
        messageList_.update(dtSeconds);
        toastCenter_.update(dtSeconds);
        cyberBackground_.update(dtSeconds);
        cyberTextTime_ += std::clamp(dtSeconds, 0.0f, 0.10f);

        if (aquariumDirectCompositionActive_ && (!aquariumUseSingleHwndCompositeViewport_ || !environmentOpen_ || !aquarium3DModeActive_))
        {
            resetAquariumDirectCompositionIfActive();
        }

        const bool viewportOverlayActive = isViewportLocalOverlayActive();
        if (viewportOverlayActive != aquariumViewportLocalOverlayActiveLastFrame_)
        {
            aquariumViewportLocalOverlayActiveLastFrame_ = viewportOverlayActive;
            ++aquariumViewportLayerModeSwitchCount_;
            requestParentCompositedViewportHold(viewportOverlayActive ? 24u : 18u, viewportOverlayActive ? L"overlay-open" : L"overlay-close");
        }
        else if (!viewportOverlayActive && aquariumParentCompositedHoldFrames_ > 0)
        {
            --aquariumParentCompositedHoldFrames_;
        }

        if (aquariumControllerReady_)
        {
            aquariumController_.Tick(dtSeconds);

            // Resize quarantine belongs to the viewport lifecycle, not to the
            // legacy child-HWND renderer branch. Panel/window resize is fully
            // supported; after the drag ends, keep the conservative path for a
            // short settle interval and then restore normal composition in every
            // renderer mode, including the current single-HWND path.
            if (aquariumResizeQuarantineActive_ &&
                !windowLiveResizeActive_ &&
                aquariumPanelResizeTarget_ == AquariumPanelResizeTarget::None)
            {
                aquariumResizeQuarantineDelaySeconds_ -= std::clamp(dtSeconds, 0.0f, 0.10f);
                if (aquariumResizeQuarantineDelaySeconds_ <= 0.0f)
                {
                    aquariumResizeQuarantineActive_ = false;
                    aquariumResizeQuarantineDelaySeconds_ = 0.0f;
                    ++aquariumResizeQuarantineExitCount_;
                    aquariumEmbeddedDx12Viewport_.SetResizeApplySuspended(false);
                    aquariumEmbeddedViewportSyncNeeded_ = true;
                }
            }

            if (aquariumUseSingleHwndCompositeViewport_ && environmentOpen_ && aquarium3DModeActive_)
            {
                // ACE-AQ3D12: single-HWND 3D composition is the main path.
                // No child HWND, no separate flip-model swapchain, no DWM resize circus.
                if (aquariumEmbeddedDx12Viewport_.IsVisible())
                {
                    aquariumEmbeddedDx12Viewport_.Hide();
                    ++aquariumLegacyChildSuppressedCount_;
                }
                aquariumEmbeddedViewportSyncNeeded_ = false;
                aquariumEmbeddedDx12Viewport_.SetResizeApplySuspended(true);
                aquariumEmbeddedViewportStatus_ = L"Single-HWND 3D active | RMB look | WASD move | Q/E vertical";

                ace::aquarium_render::AceAqCameraInput input{};
                const bool engineConsoleCapturesKeyboard = engineLogOverlayVisible_ && engineLogOverlayInputFocused_;
                const bool cameraKeyboardAvailable = GetForegroundWindow() == parent_ && !engineConsoleCapturesKeyboard;
                input.moveForward = cameraKeyboardAvailable && (GetAsyncKeyState('W') & 0x8000) != 0;
                input.moveBackward = cameraKeyboardAvailable && (GetAsyncKeyState('S') & 0x8000) != 0;
                input.moveLeft = cameraKeyboardAvailable && (GetAsyncKeyState('A') & 0x8000) != 0;
                input.moveRight = cameraKeyboardAvailable && (GetAsyncKeyState('D') & 0x8000) != 0;
                input.moveDown = cameraKeyboardAvailable && (GetAsyncKeyState('Q') & 0x8000) != 0;
                input.moveUp = cameraKeyboardAvailable && (GetAsyncKeyState('E') & 0x8000) != 0;

                const bool hasKeyboardCameraInput =
                    input.moveForward || input.moveBackward ||
                    input.moveLeft || input.moveRight ||
                    input.moveDown || input.moveUp;

                aquariumSingleHwndCamera_.UpdateFromInput(input, dtSeconds);

                bool mouseLookChanged = consumeAquariumRawMouseDelta();
                if ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0 && isAquariumSingleHwndViewportInputAvailable())
                {
                    // WM_RBUTTONDOWN normally starts capture. Keep only a missed-
                    // message recovery path here; do not poll absolute cursor
                    // movement in parallel with WM_INPUT.
                    if (!aquariumSingleHwndMouseLookActive_)
                    {
                        POINT p{};
                        GetCursorPos(&p);
                        ScreenToClient(parent_, &p);
                        const float mx = static_cast<float>(p.x);
                        const float my = static_cast<float>(p.y);
                        ++aquariumViewportInputPollBeginCount_;
                        mouseLookChanged = beginAquariumSingleHwndMouseLook(mx, my, "tick_recover_rbutton") || mouseLookChanged;
                    }
                }
                else if (aquariumSingleHwndMouseLookActive_ || aquariumSingleHwndMouseCaptured_)
                {
                    mouseLookChanged = endAquariumSingleHwndMouseLook("tick_rbutton_released");
                }

                const bool needsViewportFrame =
                    aquariumController_.IsRunning() || windowLiveResizeActive_ ||
                    hasKeyboardCameraInput || aquariumSingleHwndMouseLookActive_ || mouseLookChanged ||
                    (aquariumDirectCompositionActive_ &&
                        (!aquariumD2DCompositionHudAttached_ || !aquariumD2DCompositionHudCacheValid_));
                if (needsViewportFrame)
                {
                    if (aquariumDirectCompositionActive_ && !windowLiveResizeActive_ && !isViewportLocalOverlayActive())
                    {
                        std::string directCompositionError;
                        if (!renderAquariumDirectCompositionFrame(&directCompositionError))
                        {
                            aquariumEmbeddedViewportStatus_ = L"DirectComposition tick render failed; returning to parent composition.";
                            resetAquariumDirectCompositionIfActive();
                            requestParentCompositedViewportHold(12u, L"dcomp-tick-failure");
                            invalidateRect(aquariumEmbeddedViewportRect_.empty() ? mainRect_ : aquariumEmbeddedViewportRect_);
                        }
                    }
                    else
                    {
                        invalidateRect(aquariumEmbeddedViewportRect_.empty() ? mainRect_ : aquariumEmbeddedViewportRect_);
                    }
                }
            }
            else if (windowLiveResizeActive_ || aquariumPanelResizeTarget_ != AquariumPanelResizeTarget::None || aquariumResizeQuarantineActive_)
            {
                // ACE-AQ3D11R5: no 2D/3D ping-pong during resize. Keep the DX12
                // child viewport visible at its last stable rect, suspend child
                // move/resize/swapchain resize, and apply the final rect once
                // after a short stable delay.
                aquariumEmbeddedDx12Viewport_.SetResizeApplySuspended(true);
                aquariumEmbeddedViewportStatus_ = L"Real DX12 3D resize freeze active";

                if (environmentOpen_ && aquarium3DModeActive_ && aquariumEmbeddedViewportVisible_ && aquariumEmbeddedDx12Viewport_.IsVisible())
                {
                    std::string viewportError;
                    if (!aquariumEmbeddedDx12Viewport_.RenderFrame(aquariumController_, aquariumSceneAdapter_, aquariumController_.DebugTruthEnabled(), dtSeconds, &viewportError))
                    {
                        aquariumEmbeddedViewportStatus_ = L"Single-HWND 3D renderer unavailable during resize.";
                    }
                    else
                    {
                        aquariumActiveRenderPath_ = AceEngineRenderPath::ChildDx12;
                    }
                }
            }
            else
            {
                syncAquariumEmbeddedViewportWindow();

                if (environmentOpen_ && aquarium3DModeActive_ && aquariumEmbeddedViewportVisible_ && aquariumEmbeddedDx12Viewport_.IsVisible())
                {
                    std::string viewportError;
                    if (!aquariumEmbeddedDx12Viewport_.RenderFrame(aquariumController_, aquariumSceneAdapter_, aquariumController_.DebugTruthEnabled(), dtSeconds, &viewportError))
                    {
                        aquariumEmbeddedViewportStatus_ = L"Single-HWND 3D renderer unavailable; check logs.";
                    }
                    else if (aquariumEmbeddedDx12Viewport_.IsVisible())
                    {
                        aquariumActiveRenderPath_ = AceEngineRenderPath::ChildDx12;
                        aquariumEmbeddedViewportStatus_ = L"Single-HWND 3D active | RMB look | WASD move | Q/E vertical";
                    }
                }
            }
        }

        if (messageList_.animating())
        {
            invalidate();
        }

        if (diagnostics_.visible())
        {
            refreshDiagnostics();
        }

        if (toastCenter_.hasActiveToasts())
        {
            invalidate();
        }

        // ACE-AQ3D5: no_flicker_idle_marker. In 3D environment mode the DX12
        // child surface renders independently, so the D2D shell is not invalidated
        // every idle tick. This keeps panel rects and the viewport rect stable and
        // prevents UI/DX12 clear order flicker while paused.
        if (environmentOpen_ && aquarium3DModeActive_)
        {
            if (!windowLiveResizeActive_ && aquariumControllerReady_ && aquariumController_.IsRunning() &&
                !aquariumDirectCompositionActive_)
            {
                // The parent-composited scene is the only continuously changing
                // layer here. Invalidating the whole HWND defeated the retained
                // flip-sequential path and rebuilt every panel/text run per tick.
                invalidateRect(currentAquariumViewportDynamicLayerRect());
            }
        }
        else
        {
            // ACE-CLEAN0: background, text pulse and hover glow are intentionally alive.
            invalidate();
        }
    }

    bool AceShellUi::createDeviceIndependentResources(std::string* error)
    {
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&d2dFactory_));
        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("D2D1CreateFactory", hr); }
            return false;
        }

        hr = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(dwriteFactory_.GetAddressOf())
        );

        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("DWriteCreateFactory", hr); }
            return false;
        }

        if (!fontEngine_.initialize(dwriteFactory_.Get(), error))
        {
            return false;
        }

        return createTextFormats(error);
    }

    bool AceShellUi::createD2DDeviceContextBackbufferTarget(std::string* error)
    {
        if (!renderTarget_ || !uiSwapChain_)
        {
            if (error) { *error = "D2D DeviceContext target requires a live DXGI swapchain."; }
            return false;
        }

        renderTarget_->SetTarget(nullptr);
        uiD2DTargetBitmap_.Reset();

        Microsoft::WRL::ComPtr<IDXGISurface> backBuffer;
        HRESULT hr = uiSwapChain_->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
        if (FAILED(hr) || !backBuffer)
        {
            if (error) { *error = hresultToString("IDXGISwapChain1::GetBuffer D2D DeviceContext target", hr); }
            return false;
        }

        const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
            96.0f,
            96.0f);

        hr = renderTarget_->CreateBitmapFromDxgiSurface(backBuffer.Get(), &props, uiD2DTargetBitmap_.GetAddressOf());
        if (FAILED(hr) || !uiD2DTargetBitmap_)
        {
            if (error) { *error = hresultToString("ID2D1DeviceContext::CreateBitmapFromDxgiSurface UI backbuffer", hr); }
            uiD2DTargetBitmap_.Reset();
            return false;
        }

        renderTarget_->SetTarget(uiD2DTargetBitmap_.Get());
        applyPixelAlignedD2DTargetDpi();
        return true;
    }

    bool AceShellUi::resizeD2DDeviceContextBackbufferTarget(int width, int height, std::string* error)
    {
        if (!uiSwapChain_ || !renderTarget_)
        {
            return true;
        }

        renderTarget_->SetTarget(nullptr);
        renderTarget_->Flush();
        uiD2DTargetBitmap_.Reset();
        d2dFrameCompositor_.NotifyResize(static_cast<std::uint32_t>(std::max(1, width)), static_cast<std::uint32_t>(std::max(1, height)));

        const UINT w = static_cast<UINT>(std::max(1, width));
        const UINT h = static_cast<UINT>(std::max(1, height));
        const HRESULT hr = uiSwapChain_->ResizeBuffers(0, w, h, DXGI_FORMAT_UNKNOWN, 0);
        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("IDXGISwapChain1::ResizeBuffers D2D DeviceContext target", hr); }
            return false;
        }

        return createD2DDeviceContextBackbufferTarget(error);
    }

    bool AceShellUi::createDeviceResources(std::string* error)
    {
        if (renderTarget_ && uiD2DTargetBitmap_)
        {
            return true;
        }

        RECT rc{};
        GetClientRect(parent_, &rc);
        const UINT width = static_cast<UINT>(std::max<LONG>(1, rc.right - rc.left));
        const UINT height = static_cast<UINT>(std::max<LONG>(1, rc.bottom - rc.top));

        if (!uiD3D11Device_)
        {
            UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
            flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
            // D3D11.0 is enough for the D2D DeviceContext swapchain target and
            // avoids the old E_INVALIDARG trap when a runtime does not accept
            // D3D_FEATURE_LEVEL_11_1 in the first creation attempt. Humanity
            // survived COM; it can survive one less footgun.
            const D3D_FEATURE_LEVEL levels[] = {
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1,
                D3D_FEATURE_LEVEL_10_0
            };
            D3D_FEATURE_LEVEL actualLevel{};
            Microsoft::WRL::ComPtr<IDXGIAdapter1> preferredAdapter;
            Microsoft::WRL::ComPtr<IDXGIFactory6> adapterFactory;
            if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(adapterFactory.GetAddressOf()))) && adapterFactory)
            {
                for (UINT adapterIndex = 0; ; ++adapterIndex)
                {
                    Microsoft::WRL::ComPtr<IDXGIAdapter1> candidate;
                    const HRESULT enumHr = adapterFactory->EnumAdapterByGpuPreference(
                        adapterIndex,
                        DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                        IID_PPV_ARGS(candidate.GetAddressOf()));
                    if (enumHr == DXGI_ERROR_NOT_FOUND) { break; }
                    if (FAILED(enumHr)) { break; }
                    DXGI_ADAPTER_DESC1 candidateDesc{};
                    candidate->GetDesc1(&candidateDesc);
                    if ((candidateDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
                    {
                        preferredAdapter = candidate;
                        break;
                    }
                }
            }
            const D3D_DRIVER_TYPE driverType = preferredAdapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;
            HRESULT hr = D3D11CreateDevice(
                preferredAdapter.Get(),
                driverType,
                nullptr,
                flags,
                levels,
                static_cast<UINT>(sizeof(levels) / sizeof(levels[0])),
                D3D11_SDK_VERSION,
                uiD3D11Device_.GetAddressOf(),
                &actualLevel,
                uiD3D11Context_.GetAddressOf());
#if defined(_DEBUG)
            if (FAILED(hr))
            {
                flags &= ~D3D11_CREATE_DEVICE_DEBUG;
                hr = D3D11CreateDevice(
                    preferredAdapter.Get(),
                    driverType,
                    nullptr,
                    flags,
                    levels,
                    static_cast<UINT>(sizeof(levels) / sizeof(levels[0])),
                    D3D11_SDK_VERSION,
                    uiD3D11Device_.GetAddressOf(),
                    &actualLevel,
                    uiD3D11Context_.GetAddressOf());
            }
#endif
            if (FAILED(hr) || !uiD3D11Device_)
            {
                if (error) { *error = hresultToString("D3D11CreateDevice D2D DeviceContext UI target", hr); }
                return false;
            }

            Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
            hr = uiD3D11Device_.As(&dxgiDevice);
            if (FAILED(hr) || !dxgiDevice)
            {
                if (error) { *error = hresultToString("Query IDXGIDevice D2D DeviceContext UI target", hr); }
                return false;
            }

            Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
            hr = dxgiDevice->GetAdapter(adapter.GetAddressOf());
            if (FAILED(hr) || !adapter)
            {
                if (error) { *error = hresultToString("IDXGIDevice::GetAdapter D2D DeviceContext UI target", hr); }
                return false;
            }

            hr = adapter->GetParent(IID_PPV_ARGS(uiDxgiFactory_.GetAddressOf()));
            if (FAILED(hr) || !uiDxgiFactory_)
            {
                if (error) { *error = hresultToString("IDXGIAdapter::GetParent IDXGIFactory2", hr); }
                return false;
            }

            DXGI_SWAP_CHAIN_DESC1 desc{};
            desc.Width = width;
            desc.Height = height;
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desc.Stereo = FALSE;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            desc.BufferCount = 2;
            desc.Scaling = DXGI_SCALING_STRETCH;
            desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

            hr = uiDxgiFactory_->CreateSwapChainForHwnd(
                uiD3D11Device_.Get(),
                parent_,
                &desc,
                nullptr,
                nullptr,
                uiSwapChain_.GetAddressOf());
            if (FAILED(hr) || !uiSwapChain_)
            {
                if (error) { *error = hresultToString("IDXGIFactory2::CreateSwapChainForHwnd D2D DeviceContext UI target", hr); }
                return false;
            }

            uiDxgiFactory_->MakeWindowAssociation(parent_, DXGI_MWA_NO_ALT_ENTER);

            hr = d2dFactory_->CreateDevice(dxgiDevice.Get(), uiD2DDevice_.GetAddressOf());
            if (FAILED(hr) || !uiD2DDevice_)
            {
                if (error) { *error = hresultToString("ID2D1Factory1::CreateDevice D2D DeviceContext UI target", hr); }
                return false;
            }

            hr = uiD2DDevice_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, renderTarget_.GetAddressOf());
            if (FAILED(hr) || !renderTarget_)
            {
                if (error) { *error = hresultToString("ID2D1Device::CreateDeviceContext UI target", hr); }
                return false;
            }
        }

        if (!createD2DDeviceContextBackbufferTarget(error))
        {
            return false;
        }

        if (!createBrushes(error))
        {
            return false;
        }

        if (!createGradients(error))
        {
            return false;
        }

        return true;
    }

    bool AceShellUi::createBrushes(std::string* error)
    {
        if (!renderTarget_)
        {
            if (error) { *error = "Cannot create D2D brushes without a render target."; }
            return false;
        }

        auto createBrush = [&](const D2D1_COLOR_F& color, Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>& brush) -> bool
        {
            const HRESULT hr = renderTarget_->CreateSolidColorBrush(color, &brush);
            if (FAILED(hr))
            {
                if (error) { *error = hresultToString("ID2D1RenderTarget::CreateSolidColorBrush", hr); }
                return false;
            }

            return true;
        };

        if (!createBrush(theme_.text, textBrush_)) { return false; }
        if (!createBrush(theme_.textDim, textDimBrush_)) { return false; }
        if (!createBrush(theme_.muted, mutedBrush_)) { return false; }
        if (!createBrush(theme_.panel, panelBrush_)) { return false; }
        if (!createBrush(theme_.panelDeep, panelDeepBrush_)) { return false; }
        if (!createBrush(theme_.panelSoft, panelSoftBrush_)) { return false; }
        if (!createBrush(theme_.panelElevated, panelElevatedBrush_)) { return false; }
        if (!createBrush(theme_.border, borderBrush_)) { return false; }
        if (!createBrush(theme_.borderDim, borderDimBrush_)) { return false; }
        if (!createBrush(theme_.accent, accentBrush_)) { return false; }
        if (!createBrush(theme_.accentBlue, accentBlueBrush_)) { return false; }
        if (!createBrush(theme_.accentWarm, accentWarmBrush_)) { return false; }
        if (!createBrush(theme_.danger, dangerBrush_)) { return false; }
        if (!createBrush(theme_.userBubble, userBubbleBrush_)) { return false; }
        if (!createBrush(theme_.assistantBubble, assistantBubbleBrush_)) { return false; }
        if (!createBrush(theme_.systemBubble, systemBubbleBrush_)) { return false; }
        if (!createBrush(theme_.input, inputBrush_)) { return false; }
        if (!createBrush(theme_.inputFocused, inputFocusedBrush_)) { return false; }

        return true;
    }

    bool AceShellUi::createGradients(std::string* error)
    {
        if (!renderTarget_)
        {
            if (error) { *error = "Cannot create D2D gradients without a render target."; }
            return false;
        }

        backgroundGradientBrush_.Reset();
        accentGradientBrush_.Reset();
        buttonGradientBrush_.Reset();
        buttonHoverGradientBrush_.Reset();

        Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> backgroundStops;
        const D2D1_GRADIENT_STOP bgRaw[] =
        {
            {0.0f, theme_.backgroundTop},
            {0.48f, theme_.backgroundMid},
            {1.0f, theme_.backgroundBottom}
        };

        HRESULT hr = renderTarget_->CreateGradientStopCollection(bgRaw, 3, &backgroundStops);
        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("CreateGradientStopCollection background", hr); }
            return false;
        }

        hr = renderTarget_->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(0.0f, 0.0f),
                D2D1::Point2F(0.0f, static_cast<float>(height_))
            ),
            backgroundStops.Get(),
            &backgroundGradientBrush_
        );

        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("CreateLinearGradientBrush background", hr); }
            return false;
        }

        Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> accentStops;
        const D2D1_GRADIENT_STOP accentRaw[] =
        {
            {0.0f, theme_.accent},
            {1.0f, theme_.accentBlue}
        };

        hr = renderTarget_->CreateGradientStopCollection(accentRaw, 2, &accentStops);
        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("CreateGradientStopCollection accent", hr); }
            return false;
        }

        hr = renderTarget_->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(headerRect_.left, headerRect_.top),
                D2D1::Point2F(headerRect_.right, headerRect_.top)
            ),
            accentStops.Get(),
            &accentGradientBrush_
        );

        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("CreateLinearGradientBrush accent", hr); }
            return false;
        }

        Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> buttonStops;
        const D2D1_GRADIENT_STOP buttonRaw[] =
        {
            {0.0f, theme_.buttonTop},
            {1.0f, theme_.buttonBottom}
        };

        hr = renderTarget_->CreateGradientStopCollection(buttonRaw, 2, &buttonStops);
        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("CreateGradientStopCollection button", hr); }
            return false;
        }

        hr = renderTarget_->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(sendButtonRect_.left, sendButtonRect_.top),
                D2D1::Point2F(sendButtonRect_.left, sendButtonRect_.bottom)
            ),
            buttonStops.Get(),
            &buttonGradientBrush_
        );

        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("CreateLinearGradientBrush button", hr); }
            return false;
        }

        Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> hoverStops;
        const D2D1_GRADIENT_STOP hoverRaw[] =
        {
            {0.0f, theme_.buttonHoverTop},
            {1.0f, theme_.buttonHoverBottom}
        };

        hr = renderTarget_->CreateGradientStopCollection(hoverRaw, 2, &hoverStops);
        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("CreateGradientStopCollection hover", hr); }
            return false;
        }

        hr = renderTarget_->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(
                D2D1::Point2F(sendButtonRect_.left, sendButtonRect_.top),
                D2D1::Point2F(sendButtonRect_.left, sendButtonRect_.bottom)
            ),
            hoverStops.Get(),
            &buttonHoverGradientBrush_
        );

        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("CreateLinearGradientBrush hover", hr); }
            return false;
        }

        return true;
    }

    bool AceShellUi::createTextFormats(std::string* error)
    {
        if (!createTextFormat(L"Segoe UI", 32.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, titleFormat_, error)) { return false; }
        if (!createTextFormat(L"Segoe UI", 16.0f, DWRITE_FONT_WEIGHT_NORMAL, subtitleFormat_, error)) { return false; }
        if (!createTextFormat(L"Segoe UI", 19.0f, DWRITE_FONT_WEIGHT_NORMAL, bodyFormat_, error)) { return false; }
        if (!createTextFormat(L"Segoe UI", 19.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, bodyStrongFormat_, error)) { return false; }
        if (!createTextFormat(L"Segoe UI", 15.0f, DWRITE_FONT_WEIGHT_NORMAL, smallFormat_, error)) { return false; }
        if (!createTextFormat(L"Cascadia Mono", 16.0f, DWRITE_FONT_WEIGHT_NORMAL, monoFormat_, error)) { return false; }
        if (!createTextFormat(L"Segoe UI", 19.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, buttonFormat_, error)) { return false; }

        if (buttonFormat_)
        {
            buttonFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            buttonFormat_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        return true;
    }

    bool AceShellUi::createTextFormat(const wchar_t* family, float size, DWRITE_FONT_WEIGHT weight, Microsoft::WRL::ComPtr<IDWriteTextFormat>& format, std::string* error)
    {
        const HRESULT hr = dwriteFactory_->CreateTextFormat(
            family,
            nullptr,
            weight,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            size,
            L"ro-RO",
            &format
        );

        if (FAILED(hr))
        {
            if (error) { *error = hresultToString("IDWriteFactory::CreateTextFormat", hr); }
            return false;
        }

        format->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
        return true;
    }

    void AceShellUi::discardDeviceResources()
    {
        backgroundGradientBrush_.Reset();
        accentGradientBrush_.Reset();
        buttonGradientBrush_.Reset();
        buttonHoverGradientBrush_.Reset();

        textBrush_.Reset();
        textDimBrush_.Reset();
        mutedBrush_.Reset();
        panelBrush_.Reset();
        panelDeepBrush_.Reset();
        panelSoftBrush_.Reset();
        panelElevatedBrush_.Reset();
        borderBrush_.Reset();
        borderDimBrush_.Reset();
        accentBrush_.Reset();
        accentBlueBrush_.Reset();
        accentWarmBrush_.Reset();
        dangerBrush_.Reset();
        userBubbleBrush_.Reset();
        assistantBubbleBrush_.Reset();
        systemBubbleBrush_.Reset();
        inputBrush_.Reset();
        inputFocusedBrush_.Reset();

        aquariumSlateViewportBitmap_.Reset();
        aquariumSlateViewportBitmapExtent_ = {};
        if (aquariumGpuViewportRenderer_ && aquariumDirectCompositionActive_)
        {
            aquariumGpuViewportRenderer_->resetCompositionHost();
            aquariumDirectCompositionActive_ = false;
        }
        resetAquariumD2DCompositionHud();
        resetAquariumD2DTextureBridge();
        if (renderTarget_)
        {
            renderTarget_->SetTarget(nullptr);
        }
        uiD2DTargetBitmap_.Reset();
        renderTarget_.Reset();
        uiD2DDevice_.Reset();
        d2dFrameCompositor_.Reset();
        d2dViewportBridgePolicy_.Reset();
        d2dViewportBridgeRuntime_.Reset();
        d2dViewportCopyScheduler_.Reset();
        d2dViewportTextureCache_.Reset();
        d2dPresentScheduler_.Reset();
        d2dFrameDiagnostics_.Reset();
        d2dCompositorAudit_.Reset();
        d2dFrameTransaction_.Reset();
        slateFrameElements_.Reset();
        slateRendererPipeline_.Reset();
        slateLayerTree_.Reset();
        slatePaintJournal_.Reset();
        uiSwapChain_.Reset();
        uiDxgiFactory_.Reset();
        uiD3D11Context_.Reset();
        uiD3D11Device_.Reset();
        D2DCachedEffects::reset();
    }

    void AceShellUi::applyPixelAlignedD2DTargetDpi()
    {
        if (!renderTarget_)
        {
            return;
        }

        // ACE-UI3F/VTBRIDGE4: keep the modern D2D DeviceContext in explicit
        // 96-DPI DIPs so the existing Arhqen UI layout remains pixel-space.
        // UI3 still captures monitor DPI for placement/scaling diagnostics,
        // but D2D must not implicitly scale every coordinate on high-DPI
        // monitors. That was the bug that pushed panels/text far outside
        // their intended rects.
        renderTarget_->SetDpi(96.0f, 96.0f);
        ++d2dPixelDpiFixApplyCount_;
    }

    D2DRenderContext AceShellUi::makeContext()
    {
        D2DRenderContext ctx;
        ctx.target = renderTarget_.Get();
        ctx.theme = &theme_;
        ctx.fontEngine = &fontEngine_;
        ctx.textCache = &textCache_;
        ctx.width = static_cast<float>(width_);
        ctx.height = static_cast<float>(height_);
        ctx.dpiScale = dpiScale();
        ctx.dpiX = displayMetrics_.nearestMonitor.dpiX;
        ctx.dpiY = displayMetrics_.nearestMonitor.dpiY;

        ctx.brushes.text = textBrush_.Get();
        ctx.brushes.textDim = textDimBrush_.Get();
        ctx.brushes.muted = mutedBrush_.Get();
        ctx.brushes.panel = panelBrush_.Get();
        ctx.brushes.panelDeep = panelDeepBrush_.Get();
        ctx.brushes.panelSoft = panelSoftBrush_.Get();
        ctx.brushes.panelElevated = panelElevatedBrush_.Get();
        ctx.brushes.border = borderBrush_.Get();
        ctx.brushes.borderDim = borderDimBrush_.Get();
        ctx.brushes.accent = accentBrush_.Get();
        ctx.brushes.accentBlue = accentBlueBrush_.Get();
        ctx.brushes.accentWarm = accentWarmBrush_.Get();
        ctx.brushes.danger = dangerBrush_.Get();
        ctx.brushes.userBubble = userBubbleBrush_.Get();
        ctx.brushes.assistantBubble = assistantBubbleBrush_.Get();
        ctx.brushes.systemBubble = systemBubbleBrush_.Get();
        ctx.brushes.input = inputBrush_.Get();
        ctx.brushes.inputFocused = inputFocusedBrush_.Get();
        ctx.brushes.backgroundGradient = backgroundGradientBrush_.Get();
        ctx.brushes.accentGradient = accentGradientBrush_.Get();
        ctx.brushes.buttonGradient = buttonGradientBrush_.Get();
        ctx.brushes.buttonHoverGradient = buttonHoverGradientBrush_.Get();

        ctx.fonts.title = titleFormat_.Get();
        ctx.fonts.subtitle = subtitleFormat_.Get();
        ctx.fonts.body = bodyFormat_.Get();
        ctx.fonts.bodyStrong = bodyStrongFormat_.Get();
        ctx.fonts.small = smallFormat_.Get();
        ctx.fonts.mono = monoFormat_.Get();
        ctx.fonts.button = buttonFormat_.Get();

        return ctx;
    }

    void AceShellUi::seedInitialState()
    {
        if (conversations_.empty())
        {
            conversations_.push_back(L"Conversation 1");
            conversationMessages_.push_back({});
            activeConversationIndex_ = 0;
            nextConversationNumber_ = 2;
        }

        std::vector<ChatMessage> messages;
        messageList_.setMessages(messages);
        if (!conversationMessages_.empty())
        {
            conversationMessages_[activeConversationIndex_] = messages;
        }

        input_.setPlaceholder(L"Command or note for Arhqen Cognition Engine...");
        input_.setText(L"");
        input_.setFocused(true);
        engineLogOverlayInput_.setPlaceholder(L"engine command: stat_rhi, stat_coords, stat_fps, clear_log...");
        engineLogOverlayInput_.setText(L"");
        engineLogOverlayInput_.setFocused(false);

        sendButton_.setLabel(L"");
        sendButton_.setEnabled(true);

        toolbar_.setItems({});

        commandPalette_.setItems({
            {L"backend_summary", L"Summary", L"Append backend summary to the chat.", L"/summary"},
            {L"backend_help", L"Help", L"Show supported commands.", L"/help"},
            {L"show_concepts", L"Concepts", L"Append concept list from backend.", L"/concepts"},
            {L"sample_concept", L"Insert concept command", L"Insert a parser command into the input field.", L""},
            {L"cache_stats", L"Cache stats", L"Append font/layout cache information.", L""},
            {L"stat_coords", L"Engine coords", L"Append camera, viewport, scenario, and agent coordinates.", L"stat_coords"},
            {L"stat_rhi", L"RHI stats", L"Append DX12/RHI viewport counters and active render path.", L"stat_rhi"},
            {L"stat_fps", L"FPS stats", L"Append rolling CPU frame timing stats.", L"stat_fps"},
            {L"ui_debug", L"UI debug overlay", L"Toggle retained-layout/draw/dirty rect debug overlay.", L"F9"},
            {L"ui_stats", L"UI subsystem stats", L"Append UI5-UI11 subsystem counters.", L""},
            {L"seed_demo", L"Seed demo messages", L"Add messages for scroll testing.", L""},
            {L"focus_input", L"Focus input", L"Move keyboard focus back to input.", L"Tab"}
        });

        shortcutHelp_.setItems({
            {L"Ctrl+K / Ctrl+P", L"Command palette", L"Global"},
            {L"F1", L"Shortcut help", L"Global"},
            {L"F12", L"Diagnostics", L"Global"},
            {L"F9", L"UI debug overlay", L"Development"},
            {L"`", L"Docked engine log console", L"Development"},
            {L"Ctrl+Shift+O", L"Toggle World Outliner", L"ACE Editor"},
            {L"Ctrl+Shift+D", L"Toggle Details", L"ACE Editor"},
            {L"Ctrl+0", L"Reset editor camera", L"ACE Editor"},
            {L"Mouse wheel", L"Adaptive camera speed over viewport", L"ACE Editor"},
            {L"Esc", L"Close overlay", L"Global"},
            {L"Enter", L"Send", L"Input"},
            {L"/rename name", L"Rename active conversation", L"Workspaces"},
            {L"Mouse wheel", L"Scroll messages", L"Messages"}
        });

        initializeEngineCommands();

        focus_.set(D2DFocusTarget::TextInput);
        statusBar_.setText(L"");
    }

    void AceShellUi::render()
    {
        const auto acePerfFrameStart = std::chrono::steady_clock::now();
        std::string error;
        if (!createDeviceResources(&error))
        {
            return;
        }

        PAINTSTRUCT ps{};
        BeginPaint(parent_, &ps);

        renderTarget_->BeginDraw();
        renderTarget_->SetTransform(D2D1::Matrix3x2F::Identity());

        auto ctx = makeContext();
        const UiRect dirtyRect = makeUiRect(
            static_cast<float>(ps.rcPaint.left),
            static_cast<float>(ps.rcPaint.top),
            static_cast<float>(ps.rcPaint.right),
            static_cast<float>(ps.rcPaint.bottom));
        const UiRect requestedDirtyRect = dirtyRect.empty() ? makeUiRect(0.0f, 0.0f, ctx.width, ctx.height) : dirtyRect;
        const bool retainedPartialRedrawSafe = canUseFastAquariumViewportFrame(requestedDirtyRect);

        AceD2DFrameInput d2dFrameInput{};
        d2dFrameInput.width = static_cast<std::uint32_t>(std::max(1, width_));
        d2dFrameInput.height = static_cast<std::uint32_t>(std::max(1, height_));
        // The fast path fully redraws the viewport/HUD/console layer. Advertise
        // that complete dynamic layer to Present1, even if Win32 coalesced a
        // smaller input/selection invalidation inside it.
        d2dFrameInput.dirtyRect = retainedPartialRedrawSafe
            ? currentAquariumViewportDynamicLayerRect()
            : requestedDirtyRect;
        d2dFrameInput.environmentOpen = environmentOpen_;
        d2dFrameInput.viewport3DActive = aquarium3DModeActive_ && aquariumEmbeddedViewportVisible_;
        d2dFrameInput.hasDxgiSwapChain = uiSwapChain_ != nullptr;
        d2dFrameInput.liveResize = windowLiveResizeActive_ || aquariumResizeQuarantineActive_;
        d2dFrameInput.diagnosticsVisible = diagnostics_.visible();
        d2dFrameInput.overlayVisible = engineLogOverlayVisible_ || shortcutHelp_.visible() || commandPalette_.active();
        d2dFrameInput.retainedPartialRedrawSafe = retainedPartialRedrawSafe;
        d2dFrameInput.invalidationSerial = d2dFrameInvalidationSerial_;
        d2dFrameInput.viewportResourceEpoch = aquariumD2DBridgeSharedBitmapRecreateCount_;
        const AceD2DFramePlan d2dFramePlan = d2dFrameCompositor_.BeginFrame(d2dFrameInput);
        const UiRect effectiveDirtyRect = d2dFramePlan.fullFrameRedraw ? d2dFramePlan.paintRect : d2dFrameInput.dirtyRect;

        const auto slateFramePolicy = d2dFramePlan.fullFrameRedraw
            ? slate::AceSlateFramePolicy::FlipModelFullFrame(static_cast<std::uint32_t>(std::max(1, width_)), static_cast<std::uint32_t>(std::max(1, height_)))
            : slate::AceSlateFramePolicy::FlipModelDirtyRect(static_cast<std::uint32_t>(std::max(1, width_)), static_cast<std::uint32_t>(std::max(1, height_)));
        slateFrameElements_.BeginFrame(
            d2dFrameCompositor_.Stats().frameNumber,
            makeUiRect(0.0f, 0.0f, ctx.width, ctx.height),
            effectiveDirtyRect,
            slateFramePolicy);
        slateFrameElements_.MakeBox(0, makeUiRect(0.0f, 0.0f, ctx.width, ctx.height), slate::AceSlateColor::Black(1.0f), slate::AceSlateDrawEffect::PixelSnap | slate::AceSlateDrawEffect::ForceOpaque, "full_frame_clear_contract");
        if (environmentOpen_ && aquarium3DModeActive_ && !aquariumEmbeddedViewportRect_.empty())
        {
            slate::AceSlateViewportDescriptor viewportElement{};
            viewportElement.valid = aquariumActiveRenderPath_ == AceEngineRenderPath::Dx12D2DTextureBridge;
            viewportElement.width = static_cast<std::uint32_t>(std::max(0.0f, aquariumEmbeddedViewportRect_.width()));
            viewportElement.height = static_cast<std::uint32_t>(std::max(0.0f, aquariumEmbeddedViewportRect_.height()));
            viewportElement.allowScaling = true;
            viewportElement.ignoreAlpha = true;
            viewportElement.resourceEpoch = aquariumD2DBridgeSharedBitmapRecreateCount_;
            slateFrameElements_.MakeViewport(20, aquariumEmbeddedViewportRect_, viewportElement, slate::AceSlateDrawEffect::IgnoreTextureAlpha | slate::AceSlateDrawEffect::PixelSnap, slate::AceSlateColor::White(1.0f), "aquarium_gpu_viewport_element_contract");
        }
        slate::AceSlatePipelineInput slatePipelineInput{};
        slatePipelineInput.invalidation.windowRect = makeUiRect(0.0f, 0.0f, ctx.width, ctx.height);
        slatePipelineInput.invalidation.osDirtyRect = d2dFrameInput.dirtyRect;
        slatePipelineInput.invalidation.viewportRect = aquariumEmbeddedViewportRect_;
        slatePipelineInput.invalidation.flipSwapChain = uiSwapChain_ != nullptr;
        slatePipelineInput.invalidation.deviceContextTarget = renderTarget_ != nullptr;
        slatePipelineInput.invalidation.viewportActive = aquarium3DModeActive_ && aquariumEmbeddedViewportVisible_;
        slatePipelineInput.invalidation.liveResize = windowLiveResizeActive_ || aquariumResizeQuarantineActive_;
        slatePipelineInput.invalidation.overlayVisible = engineLogOverlayVisible_ || shortcutHelp_.visible() || commandPalette_.active();
        slatePipelineInput.invalidation.animationActive = true;
        slatePipelineInput.invalidation.textSelectionActive = engineLogHasTextSelection();
        slatePipelineInput.invalidation.diagnosticsVisible = diagnostics_.visible();
        slatePipelineInput.invalidation.retainedPartialPaintSafe = !d2dFramePlan.fullFrameRedraw;
        slatePipelineInput.invalidation.invalidationSerial = d2dFrameInvalidationSerial_;
        slatePipelineInput.invalidation.resizeEpoch = aquariumD2DBridgeSharedBitmapRecreateCount_;
        slatePipelineInput.invalidation.viewportResourceEpoch = aquariumD2DBridgeSharedBitmapRecreateCount_;
        slatePipelineInput.invalidation.frameNumber = d2dFrameCompositor_.Stats().frameNumber;
        slatePipelineInput.framePolicy = slateFramePolicy;
        slatePipelineInput.resourceEpoch = aquariumD2DBridgeSharedBitmapRecreateCount_;
        slatePipelineInput.viewportAsElement = true;
        slatePipelineInput.debugValidateLayerOrder = true;
        const auto slatePipelineOutput = slateRendererPipeline_.Build(slateFrameElements_, slatePipelineInput);
        slateLayerTree_.Build(slateFrameElements_);
        slatePaintJournal_.RecordFrame(slateFrameElements_, d2dFrameCompositor_.Stats().frameNumber, slatePipelineInput.invalidation.windowRect, slatePipelineOutput.fullFrame);
        d2dFrameCompositor_.RecordSlateElements(slateFrameElements_);
        d2dFrameDiagnostics_.BeginFrame(d2dFrameCompositor_.Stats().frameNumber, aquariumD2DBridgeSharedBitmapRecreateCount_, slatePipelineOutput.invalidationPlan.paintRect, slatePipelineOutput.fullFrame);
        d2dFrameDiagnostics_.RecordPhase(AceD2DFramePhase::BuildElementList, 0.0, slatePipelineOutput.valid ? "slate_pipeline_ok" : "slate_pipeline_failed");
        if (slatePipelineOutput.fullFrame)
        {
            d2dFrameDiagnostics_.RecordFullFrameRedraw();
        }
        else
        {
            d2dFrameDiagnostics_.RecordPartialPaintRejected();
        }
        AceD2DCompositorAuditInput auditInput{};
        auditInput.framePlan = d2dFramePlan;
        auditInput.bridgeStats = d2dViewportBridgeRuntime_.Stats();
        auditInput.slateStats = slateRendererPipeline_.Stats();
        auditInput.hasSwapChain = uiSwapChain_ != nullptr;
        auditInput.hasDeviceContext = renderTarget_ != nullptr;
        auditInput.readbackActive = false;
        auditInput.legacyFallback = false;
        auditInput.fastPartialPaintAllowed = d2dFrameCompositor_.AllowsFastPartialViewportPaint();
        auditInput.viewportActive = aquarium3DModeActive_ && aquariumEmbeddedViewportVisible_;
        auditInput.fullFrameRedraw = d2dFrameCompositor_.RequiresFullFrameRedraw();
        auditInput.frameNumber = d2dFrameCompositor_.Stats().frameNumber;
        d2dCompositorAudit_.Run(auditInput);

        uiDrawCommands_.BeginFrame();
        ++aceUi7DrawCommandFrameCount_;
        uiDrawCommands_.RoundedRect(mainRect_, 18.0f, 0);

        const bool fastAquariumViewportPaint = d2dFrameCompositor_.AllowsFastPartialViewportPaint() && renderFastAquariumViewportFrame(ctx, effectiveDirtyRect);
        if (!fastAquariumViewportPaint)
        {
            ++aquariumFullViewportPaintCount_;
            const float textPulse = 0.5f + 0.5f * std::sin(cyberTextTime_ * 2.10f);
            D2DCyberTextState cyberText;
            cyberText.enabled = true;
            cyberText.primaryAlpha = 0.92f + textPulse * 0.08f;
            cyberText.mutedAlpha = 0.72f + textPulse * 0.12f;
            cyberText.driftX = std::sin(cyberTextTime_ * 0.73f) * 0.22f;
            cyberText.driftY = std::cos(cyberTextTime_ * 0.61f) * 0.16f;
            cyberText.glowAlpha = 0.08f + textPulse * 0.10f;
            D2DCyberText::setState(cyberText);

            renderBackground(ctx);
            if (!environmentOpen_)
            {
                renderAppTopBar(ctx);
                renderSideNav(ctx);
            }

            // ACE-UI1R2: while Environment is open, the chat/home layer stays behind
            // the frosted workspace overlay instead of bleeding through and colliding
            // with Logs / Episodes.
            if (!environmentOpen_)
            {
                renderMainPanel(ctx);
                messageList_.render(ctx);

                if (messageList_.size() == 0)
                {
                    renderEmptyState(ctx);
                }

                input_.render(ctx);
                renderSendCircle(ctx);
                autocomplete_.render(ctx);
            }

            toastCenter_.render(ctx);
            diagnostics_.render(ctx);
            shortcutHelp_.render(ctx);
            commandPalette_.render(ctx);
            renderSettingsModal(ctx);
            renderEnvironmentPlaceholder(ctx);
            renderUiDebugOverlay(ctx);
            renderEngineLogOverlay(ctx);

            D2DCyberText::reset();
        }

        const HRESULT hr = renderTarget_->EndDraw();
        d2dFrameCompositor_.RecordEndDraw(hr);
        d2dFrameDiagnostics_.RecordEndDraw(hr);

        if (hr == D2DERR_RECREATE_TARGET)
        {
            discardDeviceResources();
        }
        else if (FAILED(hr))
        {
            discardDeviceResources();
        }
        else if (uiSwapChain_)
        {
            AceD2DPresentInput presentInput{};
            presentInput.hasSwapChain = uiSwapChain_ != nullptr;
            presentInput.fullFrameRedraw = d2dFramePlan.fullFrameRedraw;
            presentInput.liveResize = windowLiveResizeActive_ || aquariumResizeQuarantineActive_;
            presentInput.viewportActive = aquarium3DModeActive_ && aquariumEmbeddedViewportVisible_;
            presentInput.deviceLost = false;
            presentInput.frameNumber = d2dFrameCompositor_.Stats().frameNumber;
            presentInput.resizeEpoch = aquariumD2DBridgeSharedBitmapRecreateCount_;
            presentInput.frameRect = makeUiRect(0.0f, 0.0f, ctx.width, ctx.height);
            presentInput.requestedDirtyRect = effectiveDirtyRect;
            const AceD2DPresentPlan presentPlan = d2dPresentScheduler_.BuildPlan(presentInput);

            const auto acePerfPresentStart = std::chrono::steady_clock::now();
            HRESULT presentHr = S_OK;
            if (presentPlan.usePresent1DirtyRects)
            {
                RECT dirty{};
                dirty.left = static_cast<LONG>(std::max(0.0f, std::floor(presentPlan.presentRect.left)));
                dirty.top = static_cast<LONG>(std::max(0.0f, std::floor(presentPlan.presentRect.top)));
                dirty.right = static_cast<LONG>(std::min(ctx.width, std::ceil(presentPlan.presentRect.right)));
                dirty.bottom = static_cast<LONG>(std::min(ctx.height, std::ceil(presentPlan.presentRect.bottom)));
                DXGI_PRESENT_PARAMETERS parameters{};
                parameters.DirtyRectsCount = 1;
                parameters.pDirtyRects = &dirty;
                presentHr = uiSwapChain_->Present1(uiVsyncEnabled_ ? 1u : 0u, 0, &parameters);
            }
            else
            {
                presentHr = uiSwapChain_->Present(uiVsyncEnabled_ ? 1u : 0u, 0);
            }
            const double presentMs = aceElapsedMs(acePerfPresentStart, std::chrono::steady_clock::now());
            d2dFrameCompositor_.RecordPresent(presentHr, presentMs);
            d2dFrameDiagnostics_.RecordPresent(presentHr, presentMs);
            d2dPresentScheduler_.RecordPresent(presentPlan, presentHr, presentMs);
            slateRendererPipeline_.CommitPresent(presentHr);
            if (presentHr == DXGI_ERROR_DEVICE_REMOVED || presentHr == DXGI_ERROR_DEVICE_RESET)
            {
                discardDeviceResources();
            }
        }
        d2dFrameCompositor_.EndFrame();
        d2dFrameDiagnostics_.CompleteFrame(aceElapsedMs(acePerfFrameStart, std::chrono::steady_clock::now()));

        // ACE-UI12: defer DirectComposition teardown until after the parent D2D
        // frame has been submitted. This mirrors Slate's idea that a viewport is
        // just one paint element in the window draw list: when a UI layer opens,
        // keep the previous scene visual alive until the first parent-composited
        // frame is already on the HWND. No black blink, no one-frame emotional
        // support rectangle pretending to be a viewport.
        if (aquariumResetDirectCompositionAfterPaint_)
        {
            aquariumResetDirectCompositionAfterPaint_ = false;
            resetAquariumDirectCompositionIfActive();
            if (!aquariumEmbeddedViewportRect_.empty())
            {
                invalidateRect(aquariumEmbeddedViewportRect_);
            }
        }

        EndPaint(parent_, &ps);

        const auto acePerfFrameEnd = std::chrono::steady_clock::now();
        AceEnginePerfSample sample{};
        sample.uiMs = aceElapsedMs(acePerfFrameStart, acePerfFrameEnd);
        sample.frameMs = runtimeFrameDeltaSeconds_ > 0.0 ? runtimeFrameDeltaSeconds_ * 1000.0 : sample.uiMs;
        sample.layoutMs = -1.0;
        sample.aquariumBuildMs = enginePerfStats_.TakeLastAquariumBuildMs();
        sample.rhiRenderMs = enginePerfStats_.TakeLastRhiRenderMs();
        sample.presentOrCompositeMs = enginePerfStats_.TakeLastPresentOrCompositeMs();
        sample.fallbackPath = aquariumActiveRenderPath_;
        enginePerfStats_.Push(sample);
    }

    bool AceShellUi::canUseFastAquariumViewportFrame(UiRect dirtyRect) const
    {
        if (!environmentOpen_ || !aquarium3DModeActive_ || !aquariumUseSingleHwndCompositeViewport_)
        {
            return false;
        }
        if (dirtyRect.empty() || aquariumEmbeddedViewportRect_.empty())
        {
            return false;
        }
        if (windowLiveResizeActive_ || aquariumPanelResizeTarget_ != AquariumPanelResizeTarget::None || aquariumResizeQuarantineActive_)
        {
            return false;
        }
        if (settingsOpen_ || diagnostics_.visible() || shortcutHelp_.visible() || commandPalette_.active() || uiDebugOverlay_.Visible())
        {
            return false;
        }
        if (!engineOpenMenu_.empty())
        {
            return false;
        }

        const UiRect dynamicRect = currentAquariumViewportDynamicLayerRect();
        const float pad = 4.0f;
        return dirtyRect.left >= dynamicRect.left - pad &&
            dirtyRect.top >= dynamicRect.top - pad &&
            dirtyRect.right <= dynamicRect.right + pad &&
            dirtyRect.bottom <= dynamicRect.bottom + pad;
    }

    UiRect AceShellUi::currentAquariumViewportDynamicLayerRect() const
    {
        UiRect dynamicRect = aquariumEmbeddedViewportRect_;
        auto unite = [](UiRect a, UiRect b) -> UiRect
        {
            if (a.empty()) { return b; }
            if (b.empty()) { return a; }
            return makeUiRect(
                std::min(a.left, b.left),
                std::min(a.top, b.top),
                std::max(a.right, b.right),
                std::max(a.bottom, b.bottom));
        };

        dynamicRect = unite(dynamicRect, aquariumTelemetryOverlayRect_);
        dynamicRect = unite(dynamicRect, cameraSpeedButtonRect_);
        if (cameraSpeedPopupOpen_)
        {
            dynamicRect = unite(dynamicRect, cameraSpeedPopupRect_);
        }
        if (engineLogOverlayVisible_)
        {
            dynamicRect = unite(dynamicRect, engineLogOverlayRect_);
        }
        return dynamicRect;
    }

    bool AceShellUi::renderFastAquariumViewportFrame(D2DRenderContext& ctx, UiRect dirtyRect)
    {
        if (!canUseFastAquariumViewportFrame(dirtyRect) || !ctx.target)
        {
            return false;
        }

        const auto snapshot = aquariumController_.BuildSnapshot();
        const UiRect viewportSurface = aquariumEmbeddedViewportRect_;
        if (viewportSurface.empty())
        {
            return false;
        }

        ++aquariumFastViewportPaintCount_;

        // ACE-PERF1: UE/Slate does not rebuild the whole chrome tree just because
        // the scene viewport needs another image. Treat the viewport as a retained
        // paint layer: repaint only the dynamic viewport/HUD/console stack and let
        // flip-sequential Present1 dirty rectangles preserve the side panels. Humanity
        // may recover from overdraw eventually, but we do not need to help it fail.
        const UiRect clipRect = currentAquariumViewportDynamicLayerRect();
        ctx.target->PushAxisAlignedClip(clipRect.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        renderAquariumDx12ViewportSurface(ctx, viewportSurface, snapshot.debugTruthEnabled);
        if (!engineEditorModeActive_)
        {
            renderAquariumViewportHudLayer(ctx, viewportSurface, snapshot);
        }
        renderCameraSpeedControl(ctx, viewportSurface);
        renderEngineLogOverlay(ctx);
        ctx.target->PopAxisAlignedClip();
        return true;
    }

    void AceShellUi::renderBackground(D2DRenderContext& ctx)
    {
        D2DWidgetUtils::fillRect(ctx, makeUiRect(0.0f, 0.0f, ctx.width, ctx.height), ctx.brushes.backgroundGradient);

        const UiRect fieldRect = makeUiRect(0.0f, 0.0f, ctx.width, ctx.height);
        cyberBackground_.render(ctx, fieldRect);

        // ACE-CLEAN0: final lighter cyan wash. Subtle enough to keep contrast, visible enough to avoid the too-dark blanket.
        if (ctx.brushes.accentBlue)
        {
            ctx.brushes.accentBlue->SetOpacity(0.045f);
            ctx.target->FillRectangle(fieldRect.d2d(), ctx.brushes.accentBlue);
            ctx.brushes.accentBlue->SetOpacity(1.0f);
        }

        const UiCyberpunkThemeTokens tokens = makeDefaultCyberpunkThemeTokens();
        blurStatus_ = D2DBlurRuntime::evaluate(ctx, tokens.effects.blurRadiusFuture);
        D2DCyberEffects::drawBackgroundGrid(ctx, fieldRect, 42.0f, tokens.effects.backgroundGridAlpha * 1.85f);
        D2DCyberEffects::drawBackgroundGrid(ctx, fieldRect.inset(18.0f), 84.0f, tokens.effects.backgroundGridAlpha * 1.05f);
        D2DCyberEffects::drawScanlines(ctx, fieldRect, 7.0f, tokens.effects.scanlineAlpha * 0.88f);
    }

    void AceShellUi::renderHeader(D2DRenderContext& ctx)
    {
        renderAppTopBar(ctx);
    }

    void AceShellUi::renderMainPanel(D2DRenderContext& ctx)
    {
        // ACE-CLEAN0: no giant decorative chat shell. The message list and input are the shell.
        if (ctx.brushes.accent)
        {
            const float uplightPulse = 0.50f + 0.50f * std::sin(cyberTextTime_ * 1.20f);
            ctx.brushes.accent->SetOpacity(input_.focused() ? (0.075f + uplightPulse * 0.055f) : 0.035f);
            ctx.target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(inputRect_.left + inputRect_.width() * 0.52f, inputRect_.top - 12.0f), inputRect_.width() * 0.48f, 74.0f), ctx.brushes.accent);
            ctx.brushes.accent->SetOpacity(1.0f);
        }

        D2DGlassMaterial inputGlass;
        inputGlass.radius = 16.0f;
        inputGlass.fillAlpha = input_.focused() ? 0.32f : 0.22f;
        inputGlass.borderAlpha = input_.focused() ? 0.72f : 0.32f;
        inputGlass.glowAlpha = input_.focused() ? 0.32f : 0.12f;
        inputGlass.shadowAlpha = 0.18f;
        inputGlass.blurFallbackAlpha = input_.focused() ? 0.14f : 0.08f;
        inputGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, inputRect_, inputGlass);

        if (input_.focused())
        {
            const float inputPulse = 0.50f + 0.50f * std::sin(cyberTextTime_ * 1.55f);
            D2DGlassEffects::drawCyanEdgeSweep(ctx, inputRect_, 16.0f, cyberTextTime_ * 1.15f, 0.16f + inputPulse * 0.18f);
        }

        D2DGlassEffects::drawDepthSeparator(ctx, makeUiRect(sidebarRect_.right, appTopBarRect_.bottom, sidebarRect_.right + 1.0f, ctx.height), 0.42f);
    }

    void AceShellUi::renderAppTopBar(D2DRenderContext& ctx)
    {
        D2DGlassMaterial topBarGlass;
        topBarGlass.radius = 0.0f;
        topBarGlass.fillAlpha = 0.24f;
        topBarGlass.borderAlpha = 0.28f;
        topBarGlass.glowAlpha = 0.09f;
        topBarGlass.shadowAlpha = 0.10f;
        topBarGlass.blurFallbackAlpha = 0.10f;
        topBarGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, appTopBarRect_, topBarGlass);
        D2DWidgetUtils::drawSoftSeparator(ctx, makeUiRect(0.0f, appTopBarRect_.bottom - 1.0f, ctx.width, appTopBarRect_.bottom));

        const float logoPulse = 0.55f + 0.45f * std::sin(cyberTextTime_ * 1.8f);
        D2DCyberEffects::drawBorderGlow(ctx, brandLogoRect_, 9.0f, 0.28f + logoPulse * 0.18f);
        D2DWidgetUtils::fillRounded(ctx, brandLogoRect_, 9.0f, ctx.brushes.accentGradient, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"A", FontRole::BodyStrong, brandLogoRect_, ctx.brushes.text, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Arhqen Cognition Engine",
            FontRole::BodyStrong,
            makeUiRect(brandLogoRect_.right + 12.0f, appTopBarRect_.top + 14.0f, brandLogoRect_.right + 240.0f, appTopBarRect_.bottom - 12.0f),
            ctx.brushes.text
        );

        D2DCyberEffects::drawBorderGlow(ctx, environmentButtonRect_, 17.0f, environmentHovered_ ? 0.46f : 0.18f);
        D2DCyberEffects::drawCornerTicks(ctx, environmentButtonRect_, 8.0f, environmentHovered_ ? 0.56f : 0.26f);
        D2DWidgetUtils::fillRounded(ctx, environmentButtonRect_, 17.0f, ctx.brushes.panelElevated, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"Environment", FontRole::Small, environmentButtonRect_, ctx.brushes.text, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        D2DCyberEffects::drawBorderGlow(ctx, settingsButtonRect_, 17.0f, settingsHovered_ ? 0.42f : 0.16f);
        D2DCyberEffects::drawCornerTicks(ctx, settingsButtonRect_, 8.0f, settingsHovered_ ? 0.52f : 0.24f);
        D2DWidgetUtils::fillRounded(ctx, settingsButtonRect_, 17.0f, ctx.brushes.panel, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"Settings", FontRole::Small, settingsButtonRect_, ctx.brushes.muted, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    void AceShellUi::renderSideNav(D2DRenderContext& ctx)
    {
        D2DGlassMaterial sidebarGlass;
        sidebarGlass.radius = 0.0f;
        sidebarGlass.fillAlpha = 0.26f;
        sidebarGlass.borderAlpha = 0.34f;
        sidebarGlass.glowAlpha = 0.12f;
        sidebarGlass.shadowAlpha = 0.12f;
        sidebarGlass.blurFallbackAlpha = 0.10f;
        sidebarGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, sidebarRect_, sidebarGlass);
        D2DWidgetUtils::drawSoftSeparator(ctx, makeUiRect(sidebarRect_.right - 1.0f, sidebarRect_.top, sidebarRect_.right, sidebarRect_.bottom));

        D2DGlassMaterial newChatGlass;
        newChatGlass.radius = 13.0f;
        newChatGlass.fillAlpha = newConversationHovered_ ? 0.46f : 0.30f;
        newChatGlass.borderAlpha = newConversationHovered_ ? 0.82f : 0.42f;
        newChatGlass.glowAlpha = newConversationHovered_ ? 0.40f : 0.16f;
        newChatGlass.shadowAlpha = 0.14f;
        newChatGlass.blurFallbackAlpha = 0.12f;
        newChatGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, newConversationRect_, newChatGlass);
        D2DWidgetUtils::drawTextEx(ctx, L"+  New workspace", FontRole::Small, newConversationRect_.inset({14.0f, 10.0f, 14.0f, 8.0f}), ctx.brushes.text);

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Workspaces",
            FontRole::Small,
            makeUiRect(conversationListRect_.left + 10.0f, conversationListRect_.top, conversationListRect_.right, conversationListRect_.top + 22.0f),
            ctx.brushes.muted
        );

        for (std::size_t i = 0; i < conversationRowRects_.size() && i < conversations_.size(); ++i)
        {
            const bool active = i == activeConversationIndex_;
            const UiRect row = conversationRowRects_[i];

            const bool hovered = i == hoveredConversationIndex_;
            if (active || hovered)
            {
                D2DCyberEffects::drawBorderGlow(ctx, row, 11.0f, active ? 0.34f : 0.22f);
            }

            D2DGlassMaterial rowGlass;
            rowGlass.radius = 11.0f;
            rowGlass.fillAlpha = active ? 0.42f : (hovered ? 0.34f : 0.18f);
            rowGlass.borderAlpha = active ? 0.80f : (hovered ? 0.60f : 0.20f);
            rowGlass.glowAlpha = active ? 0.30f : (hovered ? 0.20f : 0.06f);
            rowGlass.shadowAlpha = active ? 0.12f : 0.04f;
            rowGlass.blurFallbackAlpha = active ? 0.10f : 0.04f;
            rowGlass.useCornerTicks = false;
            D2DGlassEffects::drawGlassPanel(ctx, row, rowGlass);

            D2DWidgetUtils::drawTextEx(
                ctx,
                conversations_[i],
                FontRole::Small,
                row.inset({12.0f, 9.0f, 70.0f, 7.0f}),
                active ? ctx.brushes.text : ctx.brushes.muted
            );

            if (i < conversationRenameRects_.size())
            {
                const UiRect editRect = conversationRenameRects_[i];
                D2DWidgetUtils::fillRounded(ctx, editRect, 9.0f, ctx.brushes.panel, ctx.brushes.borderDim, 0.8f);
                D2DWidgetUtils::drawTextEx(ctx, L"rename", FontRole::Small, editRect, ctx.brushes.muted, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            }
        }
    }

    void AceShellUi::renderSendCircle(D2DRenderContext& ctx)
    {
        const float cx = (sendCircleRect_.left + sendCircleRect_.right) * 0.5f;
        const float cy = (sendCircleRect_.top + sendCircleRect_.bottom) * 0.5f;
        const float radius = std::min(sendCircleRect_.width(), sendCircleRect_.height()) * 0.5f;

        ID2D1Brush* fill = ctx.brushes.buttonGradient;

        if (sendButton_.pressed())
        {
            fill = ctx.brushes.accentBlue;
        }
        else if (sendButton_.hovered())
        {
            fill = ctx.brushes.buttonHoverGradient;
        }

        const float sendPulse = 0.55f + 0.45f * std::sin(cyberTextTime_ * 2.6f);
        const float sendGlow = sendButton_.hovered() ? 0.72f : (0.30f + sendPulse * 0.18f);
        D2DGlassEffects::drawBlurFallback(ctx, sendCircleRect_, radius, sendButton_.hovered() ? 0.28f : 0.16f);
        D2DCyberEffects::drawBorderGlow(ctx, sendCircleRect_, radius, sendGlow);
        ctx.target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), radius, radius), fill);
        ctx.target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), radius - 0.5f, radius - 0.5f), ctx.brushes.border, 1.0f);

        // Simple drawn send glyph: no text label, no font dependency, no Unicode lottery.
        const D2D1_POINT_2F tip = D2D1::Point2F(cx + 9.0f, cy);
        const D2D1_POINT_2F leftTop = D2D1::Point2F(cx - 8.0f, cy - 7.0f);
        const D2D1_POINT_2F leftBottom = D2D1::Point2F(cx - 8.0f, cy + 7.0f);
        const D2D1_POINT_2F mid = D2D1::Point2F(cx - 2.0f, cy);

        ctx.target->DrawLine(leftTop, tip, ctx.brushes.text, 2.0f);
        ctx.target->DrawLine(leftBottom, tip, ctx.brushes.text, 2.0f);
        ctx.target->DrawLine(leftTop, mid, ctx.brushes.text, 2.0f);
        ctx.target->DrawLine(leftBottom, mid, ctx.brushes.text, 2.0f);
    }


    void AceShellUi::renderEmptyState(D2DRenderContext& ctx)
    {
        const UiRect center = makeUiRect(
            conversationRect_.left + conversationRect_.width() * 0.5f - 222.0f,
            conversationRect_.top + conversationRect_.height() * 0.5f - 92.0f,
            conversationRect_.left + conversationRect_.width() * 0.5f + 222.0f,
            conversationRect_.top + conversationRect_.height() * 0.5f + 92.0f
        );

        const float pulse = 0.50f + 0.50f * std::sin(cyberTextTime_ * 1.45f);

        if (ctx.brushes.accentBlue)
        {
            ctx.brushes.accentBlue->SetOpacity(0.055f + pulse * 0.035f);
            ctx.target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(center.left + center.width() * 0.5f, center.top + 46.0f), 138.0f, 58.0f), ctx.brushes.accentBlue);
            ctx.brushes.accentBlue->SetOpacity(1.0f);
        }

        UiRect orb = makeUiRect(center.left + center.width() * 0.5f - 30.0f, center.top + 8.0f, center.left + center.width() * 0.5f + 30.0f, center.top + 68.0f);
        D2DGlassEffects::drawBlurFallback(ctx, orb, 30.0f, 0.24f);
        D2DCyberEffects::drawBorderGlow(ctx, orb, 30.0f, 0.34f + pulse * 0.18f);

        if (ctx.brushes.accent)
        {
            ctx.brushes.accent->SetOpacity(0.68f + pulse * 0.22f);
            ctx.target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(orb.left + 30.0f, orb.top + 30.0f), 24.0f, 18.0f), ctx.brushes.accent, 1.2f);
            ctx.brushes.accent->SetOpacity(1.0f);
        }

        D2DWidgetUtils::drawTextEx(ctx, L"Arhqen Cognition Engine", FontRole::Title, makeUiRect(center.left, center.top + 74.0f, center.right, center.top + 116.0f), ctx.brushes.text, DWRITE_TEXT_ALIGNMENT_CENTER);
        D2DWidgetUtils::drawTextEx(ctx, L"Ready for the next grounded cognition experiment.", FontRole::Body, makeUiRect(center.left, center.top + 118.0f, center.right, center.top + 146.0f), ctx.brushes.muted, DWRITE_TEXT_ALIGNMENT_CENTER);
        D2DWidgetUtils::drawTextEx(ctx, L"Use Environment for the future 3D sandbox placeholder.", FontRole::Small, makeUiRect(center.left, center.top + 150.0f, center.right, center.top + 178.0f), ctx.brushes.textDim, DWRITE_TEXT_ALIGNMENT_CENTER);
    }

    void AceShellUi::renderSettingsModal(D2DRenderContext& ctx)
    {
        if (!settingsOpen_)
        {
            return;
        }

        // ACE-UI1R9: stronger-but-still-subtle backdrop blur when a tab is open.
        D2DGlassEffects::drawGlassOverlay(ctx, makeUiRect(0.0f, 0.0f, ctx.width, ctx.height), 0.58f);
        D2DGlassEffects::drawBlurFallback(ctx, settingsModalRect_.inset(-18.0f), 28.0f, 0.18f);

        // ACE-UI1R2: no fake vignette bars. The overlay is a clean frosted
        // focus layer; the panel itself carries depth and glow.
        D2DGlassMaterial modalGlass;
        modalGlass.radius = 20.0f;
        modalGlass.fillAlpha = 0.46f;
        modalGlass.borderAlpha = 0.86f;
        modalGlass.highlightAlpha = 0.42f;
        modalGlass.glowAlpha = 0.42f;
        modalGlass.shadowAlpha = 0.48f;
        modalGlass.blurFallbackAlpha = 0.56f;
        modalGlass.useCornerTicks = true;
        D2DGlassEffects::drawGlassPanel(ctx, settingsModalRect_, modalGlass);

        const UiRect topBar = makeUiRect(settingsModalRect_.left, settingsModalRect_.top, settingsModalRect_.right, settingsModalRect_.top + 56.0f);
        D2DGlassMaterial modalTopGlass;
        modalTopGlass.radius = 20.0f;
        modalTopGlass.fillAlpha = 0.30f;
        modalTopGlass.borderAlpha = 0.36f;
        modalTopGlass.glowAlpha = 0.10f;
        modalTopGlass.shadowAlpha = 0.04f;
        modalTopGlass.blurFallbackAlpha = 0.08f;
        modalTopGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, topBar, modalTopGlass);
        D2DWidgetUtils::fillRounded(ctx, makeUiRect(settingsModalRect_.left + 18.0f, topBar.bottom - 3.0f, settingsModalRect_.left + 120.0f, topBar.bottom), 2.0f, ctx.brushes.accentGradient);

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Settings",
            FontRole::BodyStrong,
            makeUiRect(settingsModalRect_.left + 20.0f, settingsModalRect_.top + 14.0f, settingsModalRect_.right - 70.0f, settingsModalRect_.top + 44.0f),
            ctx.brushes.text
        );

        D2DWidgetUtils::fillRounded(ctx, settingsModalCloseRect_, 10.0f, ctx.brushes.panelDeep, ctx.brushes.borderDim, 1.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"X", FontRole::Small, settingsModalCloseRect_, ctx.brushes.text, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Settings",
            FontRole::Body,
            settingsModalRect_.inset({20.0f, 78.0f, 20.0f, 78.0f}),
            ctx.brushes.text
        );

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Reserved for real options later.",
            FontRole::Small,
            settingsModalRect_.inset({20.0f, 114.0f, 20.0f, 24.0f}),
            ctx.brushes.muted
        );
    }



    std::wstring AceShellUi::widen(const std::string& text) const
    {
        if (text.empty()) return {};
        const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
            static_cast<int>(text.size()), nullptr, 0);
        if (length <= 0) return std::wstring(text.begin(), text.end());
        std::wstring result(static_cast<std::size_t>(length), L'\0');
        if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
            result.data(), length) != length) return std::wstring(text.begin(), text.end());
        return result;
    }

    bool AceShellUi::isAquariumButtonHovered(UiRect rect) const
    {
        return environmentOpen_ && rect.contains(mouseX_, mouseY_);
    }

    int AceShellUi::aquariumHotIdAt(float x, float y) const
    {
        // ACE-AQ3D6: hover_state_change_count is based on stable hit ids, not on
        // rebuilding every control tree during simple mouse motion. The order is
        // deterministic and covers only live Aquarium UI rects, so underlying chat
        // shell controls cannot flicker the 3D mode on hover.
        const UiRect rects[] = {
            aquariumDetailsToggleRect_,
            aquariumLogsToggleRect_,
            aquariumEngineModeRect_,
            aquariumDebugRect_,
            environmentModalCloseRect_,
            aquariumScenarioPrevRect_,
            aquariumScenarioNextRect_,
            aquariumPlannerPrevRect_,
            aquariumPlannerNextRect_,
            aquariumResetRect_,
            aquariumStepRect_,
            aquariumRunPauseRect_,
            aquariumCameraResetRect_,
            aquariumDetailsDebugRect_,
            aquariumManualForwardRect_,
            aquariumManualLeftRect_,
            aquariumManualRightRect_,
            aquariumManualWaitRect_,
            aquariumManualTouchRect_,
            aquariumManualConsumeRect_,
            aquariumManualPushRect_,
            cameraSpeedButtonRect_,
            engineMenuWindowRect_,
            engineMenuViewRect_,
            engineMenuHelpRect_,
            engineBackToAiRect_,
            engineCameraResetRect_,
            engineConsoleToggleRect_,
            engineOutlinerToggleRect_,
            engineDetailsToggleRect_,
            engineContentBrowserToggleRect_,
            engineShortcutHelpRect_,
            aquariumLeftResizeHandleRect_,
            aquariumRightResizeHandleRect_
        };

        for (int i = 0; i < static_cast<int>(sizeof(rects) / sizeof(rects[0])); ++i)
        {
            if (rects[i].contains(x, y))
            {
                return i;
            }
        }

        if (aquarium3DLogsScroll_.thumb.contains(x, y) || aquarium3DLogsScroll_.track.contains(x, y))
        {
            return 100;
        }

        if (aquariumLogScroll_.thumb.contains(x, y) || aquariumLogScroll_.track.contains(x, y))
        {
            return 101;
        }

        for (std::size_t i = 0; i < engineMenuRows_.size(); ++i)
        {
            if (engineMenuRows_[i].second.contains(x, y)) return 200 + static_cast<int>(i);
        }

        return -1;
    }

    UiRect AceShellUi::aquariumHotRectById(int hotId) const
    {
        const UiRect rects[] = {
            aquariumDetailsToggleRect_,
            aquariumLogsToggleRect_,
            aquariumEngineModeRect_,
            aquariumDebugRect_,
            environmentModalCloseRect_,
            aquariumScenarioPrevRect_,
            aquariumScenarioNextRect_,
            aquariumPlannerPrevRect_,
            aquariumPlannerNextRect_,
            aquariumResetRect_,
            aquariumStepRect_,
            aquariumRunPauseRect_,
            aquariumCameraResetRect_,
            aquariumDetailsDebugRect_,
            aquariumManualForwardRect_,
            aquariumManualLeftRect_,
            aquariumManualRightRect_,
            aquariumManualWaitRect_,
            aquariumManualTouchRect_,
            aquariumManualConsumeRect_,
            aquariumManualPushRect_,
            cameraSpeedButtonRect_,
            engineMenuWindowRect_,
            engineMenuViewRect_,
            engineMenuHelpRect_,
            engineBackToAiRect_,
            engineCameraResetRect_,
            engineConsoleToggleRect_,
            engineOutlinerToggleRect_,
            engineDetailsToggleRect_,
            engineContentBrowserToggleRect_,
            engineShortcutHelpRect_,
            aquariumLeftResizeHandleRect_,
            aquariumRightResizeHandleRect_
        };

        if (hotId >= 0 && hotId < static_cast<int>(sizeof(rects) / sizeof(rects[0])))
        {
            return rects[hotId];
        }

        if (hotId == 100)
        {
            return aquarium3DLogsScroll_.track.empty() ? aquariumRightLogsPanelRect_ : aquarium3DLogsScroll_.track;
        }

        if (hotId == 101)
        {
            return aquariumLogScroll_.track;
        }

        if (hotId >= 200)
        {
            const std::size_t index = static_cast<std::size_t>(hotId - 200);
            if (index < engineMenuRows_.size()) return engineMenuRows_[index].second;
        }

        return am::ui::makeUiRect(0, 0, 0, 0);
    }

    bool AceShellUi::isAquariumSingleHwndViewportInputAvailable() const
    {
        return parent_ != nullptr &&
               aquariumUseSingleHwndCompositeViewport_ &&
               environmentOpen_ &&
               aquarium3DModeActive_ &&
               aquariumEmbeddedViewportVisible_ &&
               !aquariumEmbeddedViewportRect_.empty();
    }

    bool AceShellUi::isAquariumSingleHwndViewportPoint(float x, float y) const
    {
        return isAquariumSingleHwndViewportInputAvailable() && aquariumEmbeddedViewportRect_.contains(x, y);
    }

    bool AceShellUi::shouldAquariumViewportInputDeferToOverlay(float x, float y) const
    {
        if (!isAquariumSingleHwndViewportInputAvailable())
        {
            return true;
        }

        if (settingsOpen_ || commandPalette_.active() || shortcutHelp_.visible())
        {
            return true;
        }

        if (engineLogOverlayVisible_ && engineLogOverlayRect_.contains(x, y))
        {
            return true;
        }

        if (cameraSpeedPopupOpen_ && cameraSpeedPopupRect_.contains(x, y))
        {
            return true;
        }

        if (!engineOpenMenu_.empty() && engineMenuPopupRect_.contains(x, y))
        {
            return true;
        }

        if (diagnostics_.visible())
        {
            return true;
        }

        return false;
    }

    bool AceShellUi::beginAquariumSingleHwndMouseLook(float x, float y, const char* reason)
    {
        ++aquariumViewportInputRmbDownCount_;
        aquariumViewportInputLastX_ = x;
        aquariumViewportInputLastY_ = y;
        aquariumViewportInputLastDeltaX_ = 0.0f;
        aquariumViewportInputLastDeltaY_ = 0.0f;

        if (shouldAquariumViewportInputDeferToOverlay(x, y))
        {
            ++aquariumViewportInputOverlayBlockedCount_;
            aquariumViewportInputLastRoute_ = reason ? std::string("blocked_overlay:") + reason : "blocked_overlay";
            return false;
        }

        if (!isAquariumSingleHwndViewportPoint(x, y))
        {
            ++aquariumViewportInputMissCount_;
            aquariumViewportInputLastRoute_ = reason ? std::string("miss:") + reason : "miss";
            return false;
        }

        ++aquariumViewportInputHitCount_;
        if (!aquariumSingleHwndMouseLookActive_)
        {
            ++aquariumViewportInputBeginCount_;
        }

        aquariumSingleHwndMouseLookActive_ = true;
        aquariumSingleHwndMouseCaptured_ = true;
        aquariumSingleHwndLastMouseX_ = x;
        aquariumSingleHwndLastMouseY_ = y;
        aquariumPendingRawMouseX_ = 0;
        aquariumPendingRawMouseY_ = 0;
        registerAquariumRawMouseInput();
        aquariumViewportInputLastRoute_ = reason ? std::string("begin:") + reason : "begin";

        if (parent_ && GetCapture() != parent_)
        {
            SetCapture(parent_);
        }
        SetFocus(parent_);
        if (!aquariumDirectCompositionActive_)
        {
            invalidateRect(aquariumEmbeddedViewportRect_);
            ++aquariumViewportInputInvalidationCount_;
        }
        return true;
    }

    bool AceShellUi::updateAquariumSingleHwndMouseLook(float x, float y, const char* reason)
    {
        aquariumViewportInputLastX_ = x;
        aquariumViewportInputLastY_ = y;

        if (!aquariumSingleHwndMouseLookActive_)
        {
            aquariumViewportInputLastRoute_ = reason ? std::string("move_ignored:") + reason : "move_ignored";
            return false;
        }

        const float dx = x - aquariumSingleHwndLastMouseX_;
        const float dy = y - aquariumSingleHwndLastMouseY_;
        aquariumSingleHwndLastMouseX_ = x;
        aquariumSingleHwndLastMouseY_ = y;
        aquariumViewportInputLastDeltaX_ = dx;
        aquariumViewportInputLastDeltaY_ = dy;

        if (std::fabs(dx) <= 0.0001f && std::fabs(dy) <= 0.0001f)
        {
            ++aquariumViewportInputZeroDeltaMoveCount_;
            aquariumViewportInputLastRoute_ = reason ? std::string("move_zero:") + reason : "move_zero";
            return true;
        }

        aquariumSingleHwndCamera_.ApplyMouseDelta(dx, dy);
        ++aquariumViewportInputSceneMoveCount_;
        aquariumViewportInputLastRoute_ = reason ? std::string("scene_move:") + reason : "scene_move";
        if (!aquariumDirectCompositionActive_)
        {
            invalidateRect(aquariumEmbeddedViewportRect_.empty() ? mainRect_ : aquariumEmbeddedViewportRect_);
            ++aquariumViewportInputInvalidationCount_;
        }
        return true;
    }

    bool AceShellUi::endAquariumSingleHwndMouseLook(const char* reason)
    {
        ++aquariumViewportInputRmbUpCount_;
        if (!aquariumSingleHwndMouseLookActive_ && !aquariumSingleHwndMouseCaptured_)
        {
            aquariumViewportInputLastRoute_ = reason ? std::string("up_idle:") + reason : "up_idle";
            return false;
        }

        aquariumSingleHwndMouseLookActive_ = false;
        const bool releaseMouseCapture = aquariumSingleHwndMouseCaptured_ && parent_ && GetCapture() == parent_;
        aquariumSingleHwndMouseCaptured_ = false;
        unregisterAquariumRawMouseInput();
        if (releaseMouseCapture)
        {
            ReleaseCapture();
        }
        ++aquariumViewportInputEndCount_;
        aquariumViewportInputLastRoute_ = reason ? std::string("end:") + reason : "end";
        if (!aquariumDirectCompositionActive_)
        {
            invalidateRect(aquariumEmbeddedViewportRect_.empty() ? mainRect_ : aquariumEmbeddedViewportRect_);
            ++aquariumViewportInputInvalidationCount_;
        }
        return true;
    }

    void AceShellUi::cancelAquariumSingleHwndMouseLook(const char* reason)
    {
        if (!aquariumSingleHwndMouseLookActive_ && !aquariumSingleHwndMouseCaptured_)
        {
            return;
        }

        if (reason && std::string(reason) == "capture_lost")
        {
            ++aquariumViewportInputCaptureLostCount_;
        }
        else
        {
            ++aquariumViewportInputCancelCount_;
        }

        aquariumSingleHwndMouseLookActive_ = false;
        aquariumSingleHwndMouseCaptured_ = false;
        unregisterAquariumRawMouseInput();
        aquariumViewportInputLastDeltaX_ = 0.0f;
        aquariumViewportInputLastDeltaY_ = 0.0f;
        aquariumViewportInputLastRoute_ = reason ? std::string("cancel:") + reason : "cancel";
        if (!aquariumDirectCompositionActive_)
        {
            invalidateRect(aquariumEmbeddedViewportRect_.empty() ? mainRect_ : aquariumEmbeddedViewportRect_);
            ++aquariumViewportInputInvalidationCount_;
        }
    }

    std::string AceShellUi::aquariumViewportInputDiagnostics() const
    {
        std::ostringstream os;
        os << "vtbridge5_input;active=" << (aquariumSingleHwndMouseLookActive_ ? "true" : "false")
           << ";captured=" << (aquariumSingleHwndMouseCaptured_ ? "true" : "false")
           << ";available=" << (isAquariumSingleHwndViewportInputAvailable() ? "true" : "false")
           << ";viewport=" << static_cast<int>(std::round(aquariumEmbeddedViewportRect_.left))
           << "," << static_cast<int>(std::round(aquariumEmbeddedViewportRect_.top))
           << "," << static_cast<int>(std::round(aquariumEmbeddedViewportRect_.right))
           << "," << static_cast<int>(std::round(aquariumEmbeddedViewportRect_.bottom))
           << ";rmb_down=" << aquariumViewportInputRmbDownCount_
           << ";rmb_up=" << aquariumViewportInputRmbUpCount_
           << ";begin=" << aquariumViewportInputBeginCount_
           << ";end=" << aquariumViewportInputEndCount_
           << ";cancel=" << aquariumViewportInputCancelCount_
           << ";capture_lost=" << aquariumViewportInputCaptureLostCount_
           << ";hit=" << aquariumViewportInputHitCount_
           << ";miss=" << aquariumViewportInputMissCount_
           << ";overlay_blocked=" << aquariumViewportInputOverlayBlockedCount_
           << ";scene_moves=" << aquariumViewportInputSceneMoveCount_
           << ";zero_moves=" << aquariumViewportInputZeroDeltaMoveCount_
           << ";poll_begin=" << aquariumViewportInputPollBeginCount_
           << ";raw_registered=" << (aquariumRawMouseRegistered_ ? "true" : "false")
           << ";raw_packets=" << aquariumRawMousePacketCount_
           << ";raw_consumes=" << aquariumRawMouseConsumeCount_
           << ";invalidations=" << aquariumViewportInputInvalidationCount_
           << ";last_xy=" << aceFormatDoubleUtf8(aquariumViewportInputLastX_) << "," << aceFormatDoubleUtf8(aquariumViewportInputLastY_)
           << ";last_delta=" << aceFormatDoubleUtf8(aquariumViewportInputLastDeltaX_) << "," << aceFormatDoubleUtf8(aquariumViewportInputLastDeltaY_)
           << ";route=" << aquariumViewportInputLastRoute_;
        return os.str();
    }

    std::wstring AceShellUi::aquariumViewportInputDiagnosticsWide() const
    {
        return widen(aquariumViewportInputDiagnostics());
    }

    void AceShellUi::renderAquariumDx12ViewportSurface(D2DRenderContext& ctx, UiRect rect, bool debugTruthEnabled)
    {
        (void)debugTruthEnabled;

        if (aquariumUseSingleHwndCompositeViewport_)
        {
            // ACE-AQ3D12: main path is single-HWND composition. It deliberately
            // avoids the child HWND/DX12 swapchain that flickers during resize.
            aquariumEmbeddedViewportRect_ = rect;
            aquariumPendingViewportRect_ = rect;
            aquariumPendingViewportValid_ = true;
            aquariumEmbeddedViewportVisible_ = true;
            aquariumEmbeddedViewportSyncNeeded_ = false;
            aquariumEmbeddedViewportStatus_ = L"Single-HWND 3D active | RMB look | WASD move | Q/E vertical";
            renderAquariumSlateCompositeViewport(ctx, rect, debugTruthEnabled);
            return;
        }

        // ACE-AQ3D7: child_hwnd_sync_not_called_from_render_path. Render only
        // publishes the desired stable child HWND rect. Child HWND placement and
        // visibility sync happens later, outside WM_PAINT.
        aquariumEmbeddedViewportRect_ = rect;
        aquariumNativeViewportRect_ = computeAquariumNativeViewportRect(rect);

        const bool viewportRectChanged =
            !aquariumPendingViewportValid_ ||
            std::fabs(aquariumPendingViewportRect_.left - aquariumNativeViewportRect_.left) > 0.5f ||
            std::fabs(aquariumPendingViewportRect_.top - aquariumNativeViewportRect_.top) > 0.5f ||
            std::fabs(aquariumPendingViewportRect_.right - aquariumNativeViewportRect_.right) > 0.5f ||
            std::fabs(aquariumPendingViewportRect_.bottom - aquariumNativeViewportRect_.bottom) > 0.5f;
        aquariumPendingViewportRect_ = aquariumNativeViewportRect_;
        aquariumPendingViewportValid_ = true;
        aquariumEmbeddedViewportSyncNeeded_ = aquariumEmbeddedViewportSyncNeeded_ || viewportRectChanged || !aquariumEmbeddedDx12Viewport_.IsVisible();

        const float left = aquariumPendingViewportRect_.left;
        const float top = aquariumPendingViewportRect_.top;
        const float right = aquariumPendingViewportRect_.right;
        const float bottom = aquariumPendingViewportRect_.bottom;
        if (!aquariumEmbeddedViewportVisible_ || !environmentOpen_ || !aquarium3DModeActive_ || right <= left + 64.0f || bottom <= top + 64.0f)
        {
            aquariumEmbeddedViewportVisible_ = false;
            aquariumEmbeddedViewportStatus_ = engineLogOverlayVisible_
                ? L"DX12 child viewport hidden; parent-composited UI owns overlay"
                : L"DX12 Environment pending hidden";
            return;
        }

        // ACE-AQ3D11R5: do not swap to the old D2D resize proxy while resizing.
        // The child DX12 viewport remains visible at the last stable rect until
        // syncAquariumEmbeddedViewportWindow applies the final rect after resize.
        if (windowLiveResizeActive_ ||
            aquariumPanelResizeTarget_ != AquariumPanelResizeTarget::None ||
            aquariumResizeQuarantineActive_)
        {
            aquariumEmbeddedViewportStatus_ = L"Real DX12 3D resize freeze active";
            return;
        }

        aquariumEmbeddedViewportStatus_ = L"Single-HWND 3D active | RMB look | WASD move | Q/E vertical";
    }

    void AceShellUi::renderAquariumResizeProxyViewport(D2DRenderContext& ctx, UiRect rect)
    {
        if (rect.empty())
        {
            return;
        }

        ++liveResizeProxyPaintCount_;

        // ACE-AQ3D11R2: during native top-level resize, hide the flip-model child
        // swapchain and paint the old stable single-HWND D2D composite in the same
        // rectangle. This is a resize-only proxy, not the main 3D path.
        renderAquariumSlateCompositeViewport(ctx, rect, aquariumController_.DebugTruthEnabled());

        const UiRect label = makeUiRect(rect.left + 18.0f, rect.top + 16.0f, rect.right - 18.0f, rect.top + 44.0f);
        D2DWidgetUtils::drawTextEx(ctx, L"Legacy resize helper inactive", FontRole::Small, label, ctx.brushes.muted);
    }

    am::renderer::scene::AceAquariumGpuViewportOverlay AceShellUi::buildAquariumGpuViewportOverlay(UiRect viewportSurface, const ace::aquarium_ui::AceAquariumUiSnapshot& snapshot) const
    {
        am::renderer::scene::AceAquariumGpuViewportOverlay overlay{};
        if (viewportSurface.empty())
        {
            return overlay;
        }

        auto local = [&](UiRect r) -> UiRect
        {
            return makeUiRect(
                r.left - viewportSurface.left,
                r.top - viewportSurface.top,
                r.right - viewportSurface.left,
                r.bottom - viewportSurface.top);
        };

        auto addRect = [&](UiRect r, float cr, float cg, float cb, float ca)
        {
            if (r.empty())
            {
                return;
            }
            overlay.rects.push_back({r.left, r.top, r.width(), r.height(), cr, cg, cb, ca});
            overlay.enabled = true;
        };

        auto addText = [&](float x, float y, const std::string& text, float scale, float cr, float cg, float cb)
        {
            if (text.empty())
            {
                return;
            }
            overlay.texts.push_back({x, y, scale, cr, cg, cb, 1.0f, text});
            overlay.enabled = true;
        };

        const UiRect logRect = engineLogOverlayVisible_ ? computeEngineLogOverlayRect(viewportSurface) : makeUiRect(0, 0, 0, 0);
        const UiRect telemetryRect = computeAquariumTelemetryOverlayRect(viewportSurface, logRect);
        const UiRect tl = local(telemetryRect);
        if (!tl.empty())
        {
            addRect(tl, 0.020f, 0.085f, 0.125f, 0.94f);
            addRect(makeUiRect(tl.left + 2.0f, tl.top + 2.0f, tl.right - 2.0f, tl.top + 4.0f), 0.02f, 0.72f, 0.90f, 1.0f);
            addText(tl.left + 12.0f, tl.top + 12.0f, "AQUARIUM TELEMETRY", 2.0f, 0.62f, 0.96f, 1.0f);

            auto bar = [&](float y, const char* name, double value, float r, float g, float b)
            {
                const float left = tl.left + 14.0f;
                const float right = tl.right - 14.0f;
                const float labelY = y;
                addText(left, labelY, name, 1.65f, 0.72f, 0.96f, 1.0f);
                const float barTop = labelY + 16.0f;
                const float barW = right - left;
                addRect(makeUiRect(left, barTop, right, barTop + 8.0f), 0.025f, 0.050f, 0.075f, 1.0f);
                addRect(makeUiRect(left + 2.0f, barTop + 2.0f, left + 2.0f + std::max(0.0f, std::min(1.0f, static_cast<float>(value))) * (barW - 4.0f), barTop + 6.0f), r, g, b, 1.0f);
            };
            bar(tl.top + 34.0f, "HYDRATION", snapshot.body.hydration, 0.00f, 0.88f, 0.92f);
            bar(tl.top + 58.0f, "NUTRITION", snapshot.body.nutrition, 0.20f, 0.56f, 1.00f);
            bar(tl.top + 82.0f, "INTEGRITY", snapshot.body.integrity, 1.00f, 0.60f, 0.18f);
        }

        if (engineLogOverlayVisible_)
        {
            const UiRect lg = local(logRect);
            if (!lg.empty())
            {
                addRect(lg, 0.012f, 0.042f, 0.070f, 0.96f);
                addRect(makeUiRect(lg.left, lg.top, lg.right, lg.top + 3.0f), 0.02f, 0.44f, 0.95f, 1.0f);
                addText(lg.left + 12.0f, lg.top + 14.0f, "ACE ENGINE LOG CONSOLE", 2.1f, 0.72f, 0.96f, 1.0f);
                addText(lg.right - 360.0f, lg.top + 16.0f, "ENTER=RUN  PGUP/PGDN=SCROLL  ESC=CLOSE", 1.55f, 0.30f, 0.64f, 0.84f);

                const float listTop = lg.top + 48.0f;
                const float inputH = 30.0f;
                const float listBottom = lg.bottom - inputH - 18.0f;
                addRect(makeUiRect(lg.left + 12.0f, listTop, lg.right - 12.0f, listBottom), 0.020f, 0.040f, 0.065f, 1.0f);
                addRect(makeUiRect(lg.left + 12.0f, lg.bottom - inputH - 8.0f, lg.right - 12.0f, lg.bottom - 8.0f), 0.015f, 0.035f, 0.055f, 1.0f);

                const std::size_t maxLines = static_cast<std::size_t>(std::max(1.0f, (listBottom - listTop - 12.0f) / 16.0f));
                const std::size_t count = std::min(maxLines, engineLogOverlayLines_.size());
                const std::size_t start = engineLogOverlayLines_.size() - count;
                float y = listTop + 10.0f;
                for (std::size_t i = start; i < engineLogOverlayLines_.size(); ++i)
                {
                    std::string line = aceNarrowLossy(engineLogOverlayLines_[i]);
                    if (line.size() > 120)
                    {
                        line.resize(120);
                    }
                    addText(lg.left + 22.0f, y, line, 1.55f, 0.18f, 0.82f, 1.0f);
                    y += 16.0f;
                }

                std::string inputText = "> " + aceNarrowLossy(engineLogOverlayInput_.text());
                if (inputText.size() > 118)
                {
                    inputText = inputText.substr(inputText.size() - 118);
                }
                addText(lg.left + 22.0f, lg.bottom - inputH, inputText, 1.70f, 0.80f, 1.0f, 1.0f);
            }
        }

        return overlay;
    }

    void AceShellUi::renderAquariumSlateCompositeViewport(D2DRenderContext& ctx, UiRect rect, bool debugTruthEnabled)
    {
        if (rect.empty() || !ctx.target)
        {
            return;
        }

        ++aquariumCompositeFrameCount_;
        if (windowLiveResizeActive_)
        {
            ++aquariumCompositeCachedResizeFrameCount_;
        }

        aquariumCompositeViewportLastRect_ = rect;
        aquariumCompositeViewportFrameValid_ = true;

        if (aquariumEmbeddedDx12Viewport_.IsVisible())
        {
            aquariumEmbeddedDx12Viewport_.Hide();
            ++aquariumLegacyChildSuppressedCount_;
        }

        D2DWidgetUtils::fillRect(ctx, rect, ctx.brushes.panelDeep);
        ctx.target->PushAxisAlignedClip(rect.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        auto makeBrush = [&](float r, float g, float b, float a) -> Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>
        {
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
            ctx.target->CreateSolidColorBrush(aceAquariumCompositeColor(r, g, b, a), brush.GetAddressOf());
            return brush;
        };

        auto fill = [&](UiRect r, float cr, float cg, float cb, float ca)
        {
            if (r.empty())
            {
                return;
            }
            auto brush = makeBrush(cr, cg, cb, ca);
            if (brush)
            {
                ctx.target->FillRectangle(r.d2d(), brush.Get());
            }
        };


        fill(rect, 0.006f, 0.014f, 0.032f, 1.0f);
        fill(makeUiRect(rect.left, rect.top, rect.right, std::min(rect.bottom, rect.top + 30.0f)),
             debugTruthEnabled ? 0.52f : 0.020f,
             debugTruthEnabled ? 0.20f : 0.060f,
             debugTruthEnabled ? 0.06f : 0.110f,
             0.92f);

        if (!aquariumControllerReady_)
        {
            ctx.target->PopAxisAlignedClip();
            D2DWidgetUtils::drawTextEx(ctx, L"Aquarium runtime not ready", FontRole::Small, rect.inset(18.0f), ctx.brushes.muted);
            return;
        }

        const float surfaceW = std::max(1.0f, rect.width());
        const float surfaceH = std::max(1.0f, rect.height());
        const float aspect = surfaceW / surfaceH;
        const auto viewProjection = aquariumSingleHwndCamera_.ViewProjectionMatrix(aspect);
        const auto requestedW = static_cast<am::renderer::rhi::U32>(std::max(64.0f, std::round(surfaceW)));
        const auto requestedH = static_cast<am::renderer::rhi::U32>(std::max(64.0f, std::round(surfaceH)));
        const auto viewportCacheKey = makeAquariumViewportCacheKey(requestedW, requestedH, debugTruthEnabled);

        // ACE-VTBRIDGE5: CPU readback/cache fallback is intentionally disabled for the
        // D2D DeviceContext path. The viewport must either draw as a GPU-sampled
        // frame element or report a fatal bridge error. This keeps the frame graph
        // honest and avoids retained-content/readback ghosts.
        const auto acePerfBuildStart = std::chrono::steady_clock::now();
        const auto primitives = aquariumSceneAdapter_.BuildPrimitives(aquariumController_, debugTruthEnabled);
        enginePerfStats_.SetLastAquariumBuildMs(aceElapsedMs(acePerfBuildStart, std::chrono::steady_clock::now()));

        // ACE-RHI7/RHI8: first try the real 3D GPU path. DX12 renders
        // depth-tested Aquarium mesh geometry into offscreen SceneColor/SceneDepth.
        // RHI8 then presents SceneColor through DirectComposition zero-copy when
        // possible; readback/D2D bitmap remains a safe fallback instead of a black
        // rectangle with an attitude problem.
        {
            if (!aquariumGpuViewportRenderer_)
            {
                aquariumGpuViewportRenderer_ = std::make_unique<am::renderer::scene::AceAquariumGpuViewportRenderer>();
            }

            std::array<float, 16> wvp{};
            for (std::size_t i = 0; i < wvp.size(); ++i)
            {
                wvp[i] = viewProjection.m[i];
            }
            aquariumGpuViewportRenderer_->setWorldToClipMatrix(wvp);

            am::renderer::scene::AceAquariumGpuViewportSnapshot gpuSnapshot{};
            std::string gpuError;
            const bool useDirectComposition = shouldUseDirectCompositionForAquariumViewport();
            const bool transitioningFromDirectComposition = !useDirectComposition && aquariumDirectCompositionActive_;
            if (!useDirectComposition && (isViewportLocalOverlayActive() || transitioningFromDirectComposition))
            {
                // ACE-UI12: mirror Slate/SViewport layering without the flicker tax.
                // When a viewport-local UI layer needs to be above the scene, render
                // the scene as a parent-composited viewport element. If a DComp visual
                // is already alive, keep it until the new parent frame is submitted,
                // then tear it down after EndDraw. Human eyes dislike one-frame black
                // holes, a shocking discovery in UI engineering.
                requestParentCompositedViewportHold(12u, L"parent-layered-ui");
            }

            if (useDirectComposition && aquariumDirectCompositionActive_)
            {
                // UE's viewport renderer has one frame owner. A shell repaint must
                // not submit/present the same scene a second time while the
                // independent viewport tick is already driving DirectComposition.
                aquariumActiveRenderPath_ = AceEngineRenderPath::Dx12GpuComposited;
                enginePerfStats_.SetLastFallbackPath(aquariumActiveRenderPath_);
                ctx.target->PopAxisAlignedClip();
                return;
            }

            const auto acePerfRhiStart = std::chrono::steady_clock::now();
            auto renderGpuViewport = [&](bool preferTextureBridge) -> bool
            {
                return aquariumGpuViewportRenderer_->render(
                    primitives,
                    requestedW,
                    requestedH,
                    debugTruthEnabled,
                    &gpuSnapshot,
                    &gpuError,
                    nullptr,
                    useDirectComposition ? parent_ : nullptr,
                    rect.left,
                    rect.top,
                    preferTextureBridge && !useDirectComposition);
            };
            auto failD2DDeviceContextBridge = [&](const std::string& rawError)
            {
                const std::string errorText = rawError.empty() ?
                    "D2D DeviceContext viewport bridge failed before producing a diagnostic." :
                    rawError;
                setAquariumD2DBridgeFatalError(errorText);
                aquariumD2DBridgeDisabled_ = true;
                resetAquariumDirectCompositionIfActive();
                aquariumActiveRenderPath_ = AceEngineRenderPath::FailedD2DDeviceContext;
                enginePerfStats_.SetLastFallbackPath(aquariumActiveRenderPath_);
                enginePerfStats_.SetLastRhiRenderMs(aceElapsedMs(acePerfRhiStart, std::chrono::steady_clock::now()));
                enginePerfStats_.SetLastPresentOrCompositeMs(0.0);
                ctx.target->PopAxisAlignedClip();

                const UiRect panel = rect.inset(18.0f);
                D2DWidgetUtils::fillRounded(ctx, panel, 14.0f, ctx.brushes.panelDeep, ctx.brushes.danger, 1.2f);
                D2DWidgetUtils::drawTextEx(ctx,
                    L"DX12/D2D DeviceContext bridge failed.",
                    FontRole::BodyStrong,
                    makeUiRect(panel.left + 18.0f, panel.top + 16.0f, panel.right - 18.0f, panel.top + 44.0f),
                    ctx.brushes.danger,
                    DWRITE_TEXT_ALIGNMENT_LEADING);
                D2DWidgetUtils::drawTextEx(ctx,
                    std::wstring(L"Step: ") + widen(aquariumD2DBridgeFatalStep_),
                    FontRole::Small,
                    makeUiRect(panel.left + 18.0f, panel.top + 52.0f, panel.right - 18.0f, panel.top + 76.0f),
                    ctx.brushes.text,
                    DWRITE_TEXT_ALIGNMENT_LEADING);
                D2DWidgetUtils::drawTextEx(ctx,
                    std::wstring(L"HRESULT: ") + widen(aquariumD2DBridgeFatalHresult_.empty() ? aquariumD2DBridgeLastError_ : aquariumD2DBridgeFatalHresult_),
                    FontRole::Small,
                    makeUiRect(panel.left + 18.0f, panel.top + 80.0f, panel.right - 18.0f, panel.top + 104.0f),
                    ctx.brushes.muted,
                    DWRITE_TEXT_ALIGNMENT_LEADING);
                aquariumEmbeddedViewportStatus_ = L"DX12/D2D DeviceContext bridge failed; no legacy/readback fallback.";
            };

            bool aceGpuRenderOk = renderGpuViewport(true);
            const double aceRhiRenderMs = aceElapsedMs(acePerfRhiStart, std::chrono::steady_clock::now());
            if (!aceGpuRenderOk)
            {
                failD2DDeviceContextBridge(gpuError.empty() ? "DX12 render for D2D DeviceContext bridge failed." : gpuError);
                return;
            }
            enginePerfStats_.SetLastRhiRenderMs(aceRhiRenderMs);

            if (useDirectComposition)
            {
                if (!gpuSnapshot.valid || !gpuSnapshot.zeroCopyPresented)
                {
                    failD2DDeviceContextBridge(gpuSnapshot.status.empty() ?
                        "DirectComposition viewport did not present the current GPU frame." : gpuSnapshot.status);
                    return;
                }

                const bool compositionJustStarted = !aquariumDirectCompositionActive_;
                aquariumDirectCompositionActive_ = true;
                if (compositionJustStarted)
                {
                    aquariumD2DCompositionHudCacheValid_ = false;
                }
                aquariumActiveRenderPath_ = AceEngineRenderPath::Dx12GpuComposited;
                enginePerfStats_.SetLastFallbackPath(aquariumActiveRenderPath_);
                enginePerfStats_.SetLastPresentOrCompositeMs(aceRhiRenderMs);
                aquariumEmbeddedViewportStatus_ = L"DX12 scene + independent D2D composition HUD active";
                ctx.target->PopAxisAlignedClip();
                return;
            }

            const bool attemptedD2DTextureBridge = gpuSnapshot.valid && gpuSnapshot.d2dTextureBridgeReady;
            if (!attemptedD2DTextureBridge)
            {
                const std::string status = gpuSnapshot.status.empty() ?
                    "DX12 renderer did not expose a GPU texture snapshot for the D2D DeviceContext bridge." :
                    gpuSnapshot.status;
                failD2DDeviceContextBridge(status);
                return;
            }

            const auto acePerfCompositeStart = std::chrono::steady_clock::now();
            const bool d2dTextureBridgeDrawOk = drawAquariumGpuTextureWithD2DDeviceContext(ctx, rect, gpuSnapshot, viewportCacheKey, &gpuError);
            if (!d2dTextureBridgeDrawOk)
            {
                failD2DDeviceContextBridge(gpuError.empty() ? aquariumD2DBridgeLastError_ : gpuError);
                return;
            }

            if (transitioningFromDirectComposition)
            {
                aquariumResetDirectCompositionAfterPaint_ = true;
            }
            else
            {
                aquariumDirectCompositionActive_ = false;
            }
            aquariumActiveRenderPath_ = AceEngineRenderPath::Dx12D2DTextureBridge;
            enginePerfStats_.SetLastFallbackPath(aquariumActiveRenderPath_);
            enginePerfStats_.SetLastPresentOrCompositeMs(aceElapsedMs(acePerfCompositeStart, std::chrono::steady_clock::now()));
            ctx.target->PopAxisAlignedClip();

            const auto stats = aquariumGpuViewportRenderer_->stats();
            std::wstringstream labelText;
            labelText << L"GPU 3D Environment | D2D DeviceContext GPU texture bridge "
                      << gpuSnapshot.extent.width << L"x" << gpuSnapshot.extent.height
                      << L" | verts " << stats.lastVertexCount
                      << L" | frame " << stats.framesRendered;
            const UiRect label = makeUiRect(rect.left + 16.0f, rect.top + 8.0f, rect.right - 16.0f, rect.top + 30.0f);
            D2DWidgetUtils::drawTextEx(ctx,
                debugTruthEnabled ? L"DEBUG TRUTH - NOT AGENT INPUT | D2D DeviceContext GPU texture bridge" : labelText.str(),
                FontRole::Small,
                label,
                debugTruthEnabled ? ctx.brushes.accentWarm : ctx.brushes.muted,
                DWRITE_TEXT_ALIGNMENT_LEADING);
            aquariumEmbeddedViewportStatus_ = L"GPU 3D Environment active | D2D DeviceContext/D3D11On12 DXGI surface bridge; no CPU readback this frame";
            return;
        }

        // ACE-VTBRIDGE5: no software single-HWND fallback after the GPU/D2D
        // DeviceContext bridge succeeds or fails. Leaving the old immediate-mode
        // projection code below the guaranteed bridge return produced MSVC C4702
        // storms and, worse, implied that stale retained pixels were still a
        // supported frame path. They are not.
    }

    bool AceShellUi::renderAquariumDirectCompositionFrame(std::string* error)
    {
        if (!aquariumDirectCompositionActive_ || !aquariumControllerReady_ ||
            !aquariumGpuViewportRenderer_ || !parent_ || aquariumEmbeddedViewportRect_.empty())
        {
            return false;
        }

        const UiRect rect = aquariumEmbeddedViewportRect_;
        const auto requestedWidth = static_cast<am::renderer::rhi::U32>(std::max(64.0f, std::round(rect.width())));
        const auto requestedHeight = static_cast<am::renderer::rhi::U32>(std::max(64.0f, std::round(rect.height())));
        const float aspect = static_cast<float>(requestedWidth) / static_cast<float>(requestedHeight);
        const auto viewProjection = aquariumSingleHwndCamera_.ViewProjectionMatrix(aspect);

        std::array<float, 16> wvp{};
        for (std::size_t i = 0; i < wvp.size(); ++i)
        {
            wvp[i] = viewProjection.m[i];
        }
        aquariumGpuViewportRenderer_->setWorldToClipMatrix(wvp);

        const bool debugTruthEnabled = aquariumController_.DebugTruthEnabled();
        const auto uiSnapshot = aquariumController_.BuildSnapshot();
        const auto buildStart = std::chrono::steady_clock::now();
        const auto primitives = aquariumSceneAdapter_.BuildPrimitives(aquariumController_, debugTruthEnabled);
        enginePerfStats_.SetLastAquariumBuildMs(aceElapsedMs(buildStart, std::chrono::steady_clock::now()));

        am::renderer::scene::AceAquariumGpuViewportSnapshot snapshot{};
        const auto renderStart = std::chrono::steady_clock::now();
        const bool rendered = aquariumGpuViewportRenderer_->render(
            primitives,
            requestedWidth,
            requestedHeight,
            debugTruthEnabled,
            &snapshot,
            error,
            nullptr,
            parent_,
            rect.left,
            rect.top,
            false);
        const double renderMs = aceElapsedMs(renderStart, std::chrono::steady_clock::now());
        enginePerfStats_.SetLastRhiRenderMs(renderMs);
        enginePerfStats_.SetLastPresentOrCompositeMs(renderMs);

        if (!rendered || !snapshot.valid || !snapshot.zeroCopyPresented)
        {
            if (error && error->empty())
            {
                *error = snapshot.status.empty() ?
                    "DirectComposition tick render did not present a current frame." : snapshot.status;
            }
            return false;
        }

        if (!renderAquariumD2DCompositionHud(uiSnapshot, error))
        {
            return false;
        }

        aquariumActiveRenderPath_ = AceEngineRenderPath::Dx12GpuComposited;
        enginePerfStats_.SetLastFallbackPath(aquariumActiveRenderPath_);
        return true;
    }

    bool AceShellUi::registerAquariumRawMouseInput()
    {
        if (aquariumRawMouseRegistered_ || !parent_)
        {
            return aquariumRawMouseRegistered_;
        }

        RAWINPUTDEVICE device{};
        device.usUsagePage = 0x01;
        device.usUsage = 0x02;
        device.dwFlags = 0;
        device.hwndTarget = parent_;
        aquariumRawMouseRegistered_ = RegisterRawInputDevices(&device, 1, sizeof(device)) == TRUE;
        return aquariumRawMouseRegistered_;
    }

    void AceShellUi::unregisterAquariumRawMouseInput()
    {
        if (!aquariumRawMouseRegistered_)
        {
            return;
        }

        RAWINPUTDEVICE device{};
        device.usUsagePage = 0x01;
        device.usUsage = 0x02;
        device.dwFlags = RIDEV_REMOVE;
        device.hwndTarget = nullptr;
        RegisterRawInputDevices(&device, 1, sizeof(device));
        aquariumRawMouseRegistered_ = false;
        aquariumPendingRawMouseX_ = 0;
        aquariumPendingRawMouseY_ = 0;
    }

    bool AceShellUi::consumeAquariumRawMouseDelta()
    {
        if (!aquariumSingleHwndMouseLookActive_ || !aquariumRawMouseRegistered_)
        {
            return false;
        }

        const LONG dx = aquariumPendingRawMouseX_;
        const LONG dy = aquariumPendingRawMouseY_;
        aquariumPendingRawMouseX_ = 0;
        aquariumPendingRawMouseY_ = 0;
        if (dx == 0 && dy == 0)
        {
            return false;
        }

        aquariumViewportInputLastDeltaX_ = static_cast<float>(dx);
        aquariumViewportInputLastDeltaY_ = static_cast<float>(dy);
        aquariumSingleHwndCamera_.ApplyMouseDelta(static_cast<float>(dx), static_cast<float>(dy));
        ++aquariumViewportInputSceneMoveCount_;
        ++aquariumRawMouseConsumeCount_;
        aquariumViewportInputLastRoute_ = "scene_move:raw_input_tick";
        return true;
    }

    void AceShellUi::syncAquariumEmbeddedViewportWindow()
    {
        if (aquariumUseSingleHwndCompositeViewport_)
        {
            // ACE-AQ3D14: main 3D path is Slate-style single-HWND composition.
            // The legacy child HWND is suppressed so resize cannot flicker through
            // a separate DWM surface.
            if (aquariumEmbeddedDx12Viewport_.IsVisible())
            {
                aquariumEmbeddedDx12Viewport_.Hide();
                ++aquariumLegacyChildSuppressedCount_;
            }
            aquariumEmbeddedViewportVisible_ = false;
            aquariumEmbeddedViewportSyncNeeded_ = false;
            return;
        }

        // ACE-AQ3D7: stable child HWND sync. This is the only place that may
        // update embedded DX12 child visibility/placement; hover/click paints
        // update dirty D2D rects only and do not resize/recreate the child.
        if (windowLiveResizeActive_ || aquariumPanelResizeTarget_ != AquariumPanelResizeTarget::None || aquariumResizeQuarantineActive_)
        {
            // ACE-AQ3D11R3: sync is guarded during native resize, side-panel
            // resize and the post-resize quarantine delay. Do not Show/Move/resize
            // the child HWND until the parent layout has been stable briefly.
            aquariumEmbeddedViewportSyncNeeded_ = true;
            return;
        }

        const bool wantVisible = environmentOpen_ && aquarium3DModeActive_ && aquariumEmbeddedViewportVisible_ && aquariumPendingViewportValid_;
        if (!wantVisible)
        {
            aquariumEmbeddedDx12Viewport_.Hide();
            aquariumViewportHiddenForLiveResize_ = false;
            aquariumViewportWasVisibleBeforeLiveResize_ = false;
            return;
        }

        const UiRect rect = aquariumPendingViewportRect_;
        const float left = rect.left;
        const float top = rect.top;
        const float right = rect.right;
        const float bottom = rect.bottom;
        if (right <= left + 64.0f || bottom <= top + 64.0f)
        {
            aquariumEmbeddedDx12Viewport_.Hide();
            return;
        }

        const int x = static_cast<int>(std::round(left));
        const int y = static_cast<int>(std::round(top));
        const int w = static_cast<int>(std::round(right - left));
        const int h = static_cast<int>(std::round(bottom - top));

        if (!aquariumEmbeddedViewportSyncNeeded_ &&
            aquariumEmbeddedDx12Viewport_.IsVisible() &&
            aquariumEmbeddedDx12Viewport_.X() == x &&
            aquariumEmbeddedDx12Viewport_.Y() == y &&
            aquariumEmbeddedDx12Viewport_.Width() == w &&
            aquariumEmbeddedDx12Viewport_.Height() == h)
        {
            return;
        }

        std::string viewportError;
        ++childSyncCount_;
        if (!aquariumEmbeddedDx12Viewport_.Show(parent_, x, y, w, h, &viewportError))
        {
            aquariumEmbeddedViewportStatus_ = L"Single-HWND 3D renderer unavailable; check logs.";
            aquariumEmbeddedDx12Viewport_.Hide();
            return;
        }

        if (aquariumViewportHiddenForLiveResize_)
        {
            ++viewportShowAfterLiveResizeCount_;
        }
        aquariumViewportHiddenForLiveResize_ = false;
        aquariumViewportWasVisibleBeforeLiveResize_ = false;
        aquariumEmbeddedViewportSyncNeeded_ = false;
        aquariumEmbeddedViewportStatus_ = L"Single-HWND 3D active | RMB look | WASD move | Q/E vertical";
    }

    void AceShellUi::renderAquariumButton(D2DRenderContext& ctx, UiRect rect, const std::wstring& label, bool active)
    {
        const bool hovered = isAquariumButtonHovered(rect);
        const float glow = active ? 0.62f : (hovered ? 0.42f : 0.28f);
        D2DCyberEffects::drawBorderGlow(ctx, rect, 10.0f, glow);

        D2DGlassMaterial buttonGlass;
        buttonGlass.radius = 10.0f;
        buttonGlass.fillAlpha = active ? 0.42f : (hovered ? 0.34f : 0.26f);
        buttonGlass.borderAlpha = active ? 0.82f : (hovered ? 0.60f : 0.42f);
        buttonGlass.highlightAlpha = active ? 0.36f : (hovered ? 0.28f : 0.20f);
        buttonGlass.glowAlpha = active ? 0.46f : (hovered ? 0.30f : 0.20f);
        buttonGlass.shadowAlpha = hovered ? 0.12f : 0.08f;
        buttonGlass.blurFallbackAlpha = active ? 0.10f : (hovered ? 0.09f : 0.05f);
        buttonGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, rect, buttonGlass);

        D2DWidgetUtils::drawTextEx(ctx, label, FontRole::Small, rect, (active || hovered) ? ctx.brushes.text : ctx.brushes.muted, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    void AceShellUi::renderAquariumMiniButton(D2DRenderContext& ctx, UiRect rect, const std::wstring& label, bool active)
    {
        const bool hovered = isAquariumButtonHovered(rect);
        D2DGlassMaterial buttonGlass;
        buttonGlass.radius = 6.0f;
        buttonGlass.fillAlpha = active ? 0.36f : (hovered ? 0.28f : 0.18f);
        buttonGlass.borderAlpha = active ? 0.72f : (hovered ? 0.52f : 0.32f);
        buttonGlass.highlightAlpha = active ? 0.22f : 0.14f;
        buttonGlass.glowAlpha = active ? 0.26f : (hovered ? 0.18f : 0.08f);
        buttonGlass.shadowAlpha = 0.04f;
        buttonGlass.blurFallbackAlpha = 0.08f;
        buttonGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, rect, buttonGlass);
        if (hovered || active)
        {
            D2DCyberEffects::drawBorderGlow(ctx, rect, 6.0f, active ? 0.34f : 0.20f);
        }

        D2DWidgetUtils::drawTextEx(ctx, label, FontRole::Small, rect, (active || hovered) ? ctx.brushes.text : ctx.brushes.muted, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    void AceShellUi::clampAquariumScroll(AquariumScrollPanel& scroll)
    {
        scroll.maxScroll = std::max(0.0f, scroll.contentHeight - scroll.viewportHeight);
        scroll.offset = std::clamp(scroll.offset, 0.0f, scroll.maxScroll);
        const float lineHeight = 19.0f;
        scroll.visibleLogStart = static_cast<std::size_t>(std::max(0.0f, std::floor(scroll.offset / lineHeight)));
        const float visibleBottom = scroll.offset + scroll.viewportHeight;
        scroll.visibleLogEnd = std::min(scroll.lineCount, static_cast<std::size_t>(std::ceil(visibleBottom / lineHeight)) + 1U);
    }

    void AceShellUi::renderAquariumScrollbar(D2DRenderContext& ctx, AquariumScrollPanel& scroll)
    {
        if (scroll.contentHeight <= scroll.viewportHeight + 1.0f || scroll.track.height() <= 8.0f)
        {
            scroll.thumb = {};
            return;
        }

        clampAquariumScroll(scroll);

        const float trackH = std::max(1.0f, scroll.track.height());
        const float ratio = std::clamp(scroll.viewportHeight / std::max(scroll.contentHeight, 1.0f), 0.10f, 1.0f);
        const float thumbH = std::clamp(trackH * ratio, 24.0f, trackH);
        const float maxOffset = std::max(1.0f, scroll.contentHeight - scroll.viewportHeight);
        const float maxThumbTravel = std::max(1.0f, trackH - thumbH);
        const float thumbTop = scroll.track.top + (scroll.offset / maxOffset) * maxThumbTravel;

        scroll.thumb = makeUiRect(scroll.track.left, thumbTop, scroll.track.right, thumbTop + thumbH);

        if (ctx.brushes.panelDeep)
        {
            const float oldTrack = ctx.brushes.panelDeep->GetOpacity();
            ctx.brushes.panelDeep->SetOpacity(0.42f);
            D2DWidgetUtils::fillRounded(ctx, scroll.track, 4.0f, ctx.brushes.panelDeep);
            ctx.brushes.panelDeep->SetOpacity(oldTrack);
        }

        D2DCyberEffects::drawBorderGlow(ctx, scroll.thumb, 4.0f, aquariumDraggingScroll_ == &scroll ? 0.48f : 0.24f);

        if (ctx.brushes.accent)
        {
            const float oldAccent = ctx.brushes.accent->GetOpacity();
            ctx.brushes.accent->SetOpacity(aquariumDraggingScroll_ == &scroll ? 0.84f : 0.58f);
            D2DWidgetUtils::fillRounded(ctx, scroll.thumb, 4.0f, ctx.brushes.accent);
            ctx.brushes.accent->SetOpacity(oldAccent);
        }
    }

    void AceShellUi::renderAquariumLines(D2DRenderContext& ctx, const std::wstring& title, const std::vector<std::string>& lines, UiRect rect, std::size_t maxLines)
    {
        if (rect.empty() || rect.height() < 18.0f || rect.width() < 24.0f)
        {
            return;
        }

        // ACE-AQ3D6: section draw is clipped to its computed section_rect. Static
        // cards stay static; scrollbars are only rendered by renderAquariumScrollableLines
        // for explicitly long sections. This keeps panel_content_clipped_to_content_rect
        // true even when the panel is resized small.
        const bool pushedClip = ctx.target != nullptr;
        if (pushedClip)
        {
            ctx.target->PushAxisAlignedClip(rect.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        }

        D2DGlassMaterial sectionGlass;
        sectionGlass.radius = 13.0f;
        sectionGlass.fillAlpha = 0.38f;
        sectionGlass.borderAlpha = 0.58f;
        sectionGlass.highlightAlpha = 0.26f;
        sectionGlass.glowAlpha = 0.22f;
        sectionGlass.shadowAlpha = 0.10f;
        sectionGlass.blurFallbackAlpha = 0.12f;
        sectionGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, rect, sectionGlass);
        D2DCyberEffects::drawBorderGlow(ctx, rect, 13.0f, 0.16f);

        if (ctx.brushes.panelDeep)
        {
            const float oldDeep = ctx.brushes.panelDeep->GetOpacity();
            ctx.brushes.panelDeep->SetOpacity(0.18f);
            D2DWidgetUtils::fillRounded(ctx, rect.inset(4.0f), 10.0f, ctx.brushes.panelDeep);
            ctx.brushes.panelDeep->SetOpacity(oldDeep);
        }

        D2DWidgetUtils::drawTextEx(
            ctx,
            title,
            FontRole::Small,
            makeUiRect(rect.left + 10.0f, rect.top + 8.0f, rect.right - 10.0f, rect.top + 28.0f),
            ctx.brushes.text
        );

        float y = rect.top + 32.0f;
        const float lineHeight = 19.0f;
        std::size_t count = 0;
        for (const auto& line : lines)
        {
            if (count >= maxLines || y + lineHeight > rect.bottom - 6.0f)
            {
                break;
            }

            std::string clipped = line;
            if (clipped.size() > 112)
            {
                clipped = clipped.substr(0, 109) + "...";
            }

            D2DWidgetUtils::drawTextEx(
                ctx,
                widen(clipped),
                FontRole::Small,
                makeUiRect(rect.left + 10.0f, y, rect.right - 10.0f, y + lineHeight),
                ctx.brushes.muted
            );

            y += lineHeight;
            ++count;
        }

        if (pushedClip)
        {
            ctx.target->PopAxisAlignedClip();
        }
    }

    void AceShellUi::renderAquariumScrollableLines(D2DRenderContext& ctx, const std::wstring& title, const std::vector<std::string>& lines, UiRect rect, AquariumScrollPanel& scroll, std::size_t maxVisibleLines)
    {
        if (rect.empty() || rect.height() < 36.0f || rect.width() < 40.0f)
        {
            scroll.viewport = {};
            scroll.track = {};
            scroll.thumb = {};
            scroll.viewportHeight = 0.0f;
            scroll.contentHeight = 0.0f;
            scroll.maxScroll = 0.0f;
            scroll.visibleLogStart = 0;
            scroll.visibleLogEnd = 0;
            return;
        }

        D2DGlassMaterial sectionGlass;
        sectionGlass.radius = 13.0f;
        sectionGlass.fillAlpha = 0.42f;
        sectionGlass.borderAlpha = 0.66f;
        sectionGlass.highlightAlpha = 0.22f;
        sectionGlass.glowAlpha = 0.28f;
        sectionGlass.shadowAlpha = 0.08f;
        sectionGlass.blurFallbackAlpha = 0.16f;
        sectionGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, rect, sectionGlass);
        D2DCyberEffects::drawBorderGlow(ctx, rect, 13.0f, 0.20f);

        D2DWidgetUtils::drawTextEx(
            ctx,
            title,
            FontRole::Small,
            makeUiRect(rect.left + 10.0f, rect.top + 8.0f, rect.right - 10.0f, rect.top + 28.0f),
            ctx.brushes.text
        );

        const float lineHeight = 19.0f;
        const float viewportTop = rect.top + 36.0f;
        const float viewportBottom = rect.bottom - 8.0f;
        const float scrollBarWidth = 8.0f;

        scroll.viewport = makeUiRect(rect.left + 10.0f, viewportTop, rect.right - 14.0f - scrollBarWidth, viewportBottom);
        scroll.track = makeUiRect(rect.right - 12.0f, viewportTop + 1.0f, rect.right - 6.0f, viewportBottom - 1.0f);
        const float previousMaxScroll = scroll.maxScroll;
        const bool wasAtBottom = scroll.offset >= previousMaxScroll - 2.0f;
        scroll.viewportHeight = std::max(0.0f, scroll.viewport.height());
        scroll.lineCount = lines.size();
        scroll.contentHeight = static_cast<float>(lines.size()) * lineHeight;
        scroll.maxScroll = std::max(0.0f, scroll.contentHeight - scroll.viewportHeight);
        if (scroll.autoScrollWhenAtBottom && (!scroll.userScrolled || wasAtBottom))
        {
            scroll.offset = scroll.maxScroll;
            scroll.userScrolled = false;
        }
        clampAquariumScroll(scroll);

        if (ctx.target)
        {
            ctx.target->PushAxisAlignedClip(scroll.viewport.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        }

        (void)maxVisibleLines;
        float y = scroll.viewport.top - scroll.offset;
        for (const auto& line : lines)
        {
            if (y + lineHeight >= scroll.viewport.top && y <= scroll.viewport.bottom)
            {
                std::string clipped = line;
                if (clipped.size() > 126)
                {
                    clipped = clipped.substr(0, 123) + "...";
                }

                D2DWidgetUtils::drawTextEx(
                    ctx,
                    widen(clipped),
                    FontRole::Small,
                    makeUiRect(scroll.viewport.left, y, scroll.viewport.right, y + lineHeight),
                    ctx.brushes.muted
                );
            }

            y += lineHeight;
        }

        if (ctx.target)
        {
            ctx.target->PopAxisAlignedClip();
        }

        renderAquariumScrollbar(ctx, scroll);
    }

    AceShellUi::AquariumScrollPanel* AceShellUi::activeAquariumScrollPanelAt(float x, float y)
    {
        auto hitScrollable = [x, y](AquariumScrollPanel& panel) -> AquariumScrollPanel*
        {
            if (panel.contentHeight <= panel.viewportHeight + 1.0f)
            {
                return nullptr;
            }
            return panel.thumb.contains(x, y) || panel.track.contains(x, y) || panel.viewport.contains(x, y)
                ? &panel : nullptr;
        };

        // Never route through stale rectangles left by another mode. A hidden
        // panel is not a wheel target merely because it existed last frame.
        if (engineLogOverlayVisible_)
        {
            if (auto* panel = hitScrollable(engineLogOverlayScroll_)) return panel;
        }

        if (aquarium3DModeActive_ && !engineEditorModeActive_)
        {
            if (aquariumLogsPanelVisible_)
                if (auto* panel = hitScrollable(aquarium3DLogsScroll_)) return panel;
            if (aquariumDetailsPanelVisible_)
                if (auto* panel = hitScrollable(aquarium3DDetailsScroll_)) return panel;
        }
        else if (!aquarium3DModeActive_)
        {
            if (auto* panel = hitScrollable(aquariumLogScroll_)) return panel;
            if (auto* panel = hitScrollable(aquariumContentScroll_)) return panel;
        }

        return nullptr;
    }

    bool AceShellUi::handleAquariumWheel(float x, float y, int wheelDelta)
    {
        auto* panel = activeAquariumScrollPanelAt(x, y);
        if (!panel)
        {
            return false;
        }

        panel->offset += static_cast<float>(-wheelDelta) * 0.34f;
        panel->userScrolled = true;
        clampAquariumScroll(*panel);
        panel->autoScrollWhenAtBottom = panel->offset >= panel->maxScroll - 2.0f;
        if (panel->autoScrollWhenAtBottom)
        {
            panel->userScrolled = false;
        }
        return true;
    }

    bool AceShellUi::beginAquariumScrollbarDrag(float x, float y)
    {
        auto* panel = activeAquariumScrollPanelAt(x, y);
        if (!panel || !panel->thumb.contains(x, y))
        {
            return false;
        }

        aquariumDraggingScroll_ = panel;
        aquariumDragStartY_ = y;
        aquariumDragStartOffset_ = panel->offset;
        panel->userScrolled = true;
        panel->autoScrollWhenAtBottom = false;
        return true;
    }

    bool AceShellUi::updateAquariumScrollbarDrag(float, float y)
    {
        if (!aquariumDraggingScroll_)
        {
            return false;
        }

        auto& scroll = *aquariumDraggingScroll_;
        const float trackTravel = std::max(1.0f, scroll.track.height() - scroll.thumb.height());
        const float maxOffset = std::max(0.0f, scroll.contentHeight - scroll.viewportHeight);
        const float dy = y - aquariumDragStartY_;
        scroll.offset = aquariumDragStartOffset_ + (dy / trackTravel) * maxOffset;
        scroll.userScrolled = true;
        clampAquariumScroll(scroll);
        scroll.autoScrollWhenAtBottom = scroll.offset >= scroll.maxScroll - 2.0f;
        if (scroll.autoScrollWhenAtBottom)
        {
            scroll.userScrolled = false;
        }
        return true;
    }

    void AceShellUi::endAquariumScrollbarDrag()
    {
        aquariumDraggingScroll_ = nullptr;
        aquariumDragStartY_ = 0.0f;
        aquariumDragStartOffset_ = 0.0f;
    }

    bool AceShellUi::beginAquariumPanelResize(float x, float y)
    {
        if (!environmentOpen_ || !aquarium3DModeActive_)
        {
            return false;
        }

        aquariumPanelResizeEdges_ = PanelResizePolicy::hitTest(
            aquariumLeftPanelRect_, x, y, PanelResizeEdge::Right | PanelResizeEdge::Bottom);
        if (aquariumPanelResizeEdges_ != PanelResizeEdge::None)
        {
            aquariumPanelResizeTarget_ = AquariumPanelResizeTarget::LeftDetails;
        }
        else
        {
            aquariumPanelResizeEdges_ = PanelResizePolicy::hitTest(
                aquariumRightLogsPanelRect_, x, y, PanelResizeEdge::Left | PanelResizeEdge::Bottom);
            if (aquariumPanelResizeEdges_ != PanelResizeEdge::None)
            {
                aquariumPanelResizeTarget_ = AquariumPanelResizeTarget::RightLogs;
            }
            else
            {
                return false;
            }
        }

        aquariumPanelResizeStartX_ = x;
        aquariumPanelResizeStartY_ = y;
        aquariumPanelResizeStartState_ = aquarium3DPanelState_;

        // ACE-AQ3D11R5: side-panel resize freezes the child DX12 viewport at
        // its last stable rect. No D2D proxy swap, no hide/show loop.
        aquariumResizeQuarantineActive_ = true;
        aquariumResizeQuarantineDelaySeconds_ = 0.18f;
        ++aquariumResizeQuarantineEnterCount_;
        aquariumEmbeddedDx12Viewport_.SetResizeApplySuspended(true);
        aquariumEmbeddedViewportStatus_ = L"Real DX12 3D resize freeze active";
        aquariumEmbeddedViewportSyncNeeded_ = true;
        return true;
    }

    bool AceShellUi::updateAquariumPanelResize(float x, float y)
    {
        if (aquariumPanelResizeTarget_ == AquariumPanelResizeTarget::None)
        {
            return false;
        }

        if (aquariumPanelResizeTarget_ == AquariumPanelResizeTarget::LeftDetails)
        {
            const float requestedWidth = hasEdge(aquariumPanelResizeEdges_, PanelResizeEdge::Right)
                ? aquariumPanelResizeStartState_.detailsWidth + (x - aquariumPanelResizeStartX_)
                : aquariumPanelResizeStartState_.detailsWidth;
            const float requestedHeight = hasEdge(aquariumPanelResizeEdges_, PanelResizeEdge::Bottom)
                ? aquariumPanelResizeStartState_.detailsHeight + (y - aquariumPanelResizeStartY_)
                : aquariumPanelResizeStartState_.detailsHeight;
            environment3DMode_.ResizeLeftPanel(aquarium3DPanelState_, requestedWidth, requestedHeight, static_cast<float>(width_), static_cast<float>(height_));
            aquariumResizeQuarantineActive_ = true;
            aquariumResizeQuarantineDelaySeconds_ = 0.18f;
            aquariumEmbeddedViewportSyncNeeded_ = true;
            invalidateRect(aquariumEmbeddedViewportRect_.empty() ? mainRect_ : aquariumEmbeddedViewportRect_);
            return true;
        }

        if (aquariumPanelResizeTarget_ == AquariumPanelResizeTarget::RightLogs)
        {
            const float requestedWidth = hasEdge(aquariumPanelResizeEdges_, PanelResizeEdge::Left)
                ? aquariumPanelResizeStartState_.logsWidth - (x - aquariumPanelResizeStartX_)
                : aquariumPanelResizeStartState_.logsWidth;
            const float requestedHeight = hasEdge(aquariumPanelResizeEdges_, PanelResizeEdge::Bottom)
                ? aquariumPanelResizeStartState_.logsHeight + (y - aquariumPanelResizeStartY_)
                : aquariumPanelResizeStartState_.logsHeight;
            environment3DMode_.ResizeRightPanel(aquarium3DPanelState_, requestedWidth, requestedHeight, static_cast<float>(width_), static_cast<float>(height_));
            aquariumResizeQuarantineActive_ = true;
            aquariumResizeQuarantineDelaySeconds_ = 0.18f;
            aquariumEmbeddedViewportSyncNeeded_ = true;
            invalidateRect(aquariumEmbeddedViewportRect_.empty() ? mainRect_ : aquariumEmbeddedViewportRect_);
            return true;
        }

        return false;
    }

    void AceShellUi::endAquariumPanelResize()
    {
        aquariumPanelResizeTarget_ = AquariumPanelResizeTarget::None;
        aquariumPanelResizeEdges_ = PanelResizeEdge::None;
        aquariumPanelResizeStartX_ = 0.0f;
        aquariumPanelResizeStartY_ = 0.0f;
        layoutProfile_.aquariumDetailsWidth = aquarium3DPanelState_.detailsWidth;
        layoutProfile_.aquariumDetailsHeight = aquarium3DPanelState_.detailsHeight;
        layoutProfile_.aquariumLogsWidth = aquarium3DPanelState_.logsWidth;
        layoutProfile_.aquariumLogsHeight = aquarium3DPanelState_.logsHeight;
        if (!layoutProfilePath_.empty())
        {
            std::string ignoredError;
            D2DLayoutPersistence::save(layoutProfilePath_, layoutProfile_, &ignoredError);
        }
        if (environmentOpen_ && aquarium3DModeActive_ && aquariumEmbeddedViewportVisible_)
        {
            aquariumResizeQuarantineActive_ = true;
            aquariumResizeQuarantineDelaySeconds_ = 0.18f;
            aquariumEmbeddedViewportSyncNeeded_ = true;
            invalidateRect(aquariumEmbeddedViewportRect_.empty() ? mainRect_ : aquariumEmbeddedViewportRect_);
        }
    }

    bool AceShellUi::handleAquariumPanelClick(float x, float y)
    {
        if (!aquariumControllerReady_)
        {
            return false;
        }

        if (aquariumEngineModeRect_.contains(x, y))
        {
            enterEngineEditorMode();
            return true;
        }

        if (aquariumDetailsToggleRect_.contains(x, y))
        {
            aquariumDetailsPanelVisible_ = !aquariumDetailsPanelVisible_;
            aquarium3DPanelState_.detailsVisible = aquariumDetailsPanelVisible_;
            return true;
        }

        if (aquariumLogsToggleRect_.contains(x, y))
        {
            aquariumLogsPanelVisible_ = !aquariumLogsPanelVisible_;
            aquarium3DPanelState_.logsVisible = aquariumLogsPanelVisible_;
            return true;
        }

        if (aquariumScenarioPrevRect_.contains(x, y))
        {
            cycleAquariumScenario(-1);
            return true;
        }

        if (aquariumScenarioNextRect_.contains(x, y))
        {
            cycleAquariumScenario(1);
            return true;
        }

        if (aquariumPlannerPrevRect_.contains(x, y))
        {
            cycleAquariumPlanner(-1);
            return true;
        }

        if (aquariumPlannerNextRect_.contains(x, y))
        {
            cycleAquariumPlanner(1);
            return true;
        }

        if (aquariumResetRect_.contains(x, y))
        {
            aquariumController_.ResetScenario(aquariumController_.CurrentScenarioName(), 123);
            return true;
        }

        if (aquariumStepRect_.contains(x, y))
        {
            aquariumController_.StepOnce();
            return true;
        }

        if (aquariumRunPauseRect_.contains(x, y))
        {
            aquariumController_.SetRunning(!aquariumController_.IsRunning());
            return true;
        }

        if (aquariumDebugRect_.contains(x, y) || aquariumDetailsDebugRect_.contains(x, y))
        {
            aquariumController_.SetDebugTruthEnabled(!aquariumController_.DebugTruthEnabled());
            return true;
        }

        if (aquariumOpen3DRect_.contains(x, y))
        {
            aquarium3DModeActive_ = true;
            aquariumUseSingleHwndCompositeViewport_ = true;
            aquariumEmbeddedViewportVisible_ = true;
            aquariumEmbeddedDx12Viewport_.Hide();
            aquariumContentScroll_.offset = 0.0f;
            aquariumEmbeddedViewportStatus_ = L"Single-HWND 3D active | RMB look | WASD move | Q/E vertical";
            showToast(L"3D Environment", L"Single-HWND 3D viewport active.", D2DToastKind::Success);
            return true;
        }

        if (aquariumCameraResetRect_.contains(x, y))
        {
            aquariumSingleHwndCamera_.Reset();
            aquariumEmbeddedViewportVisible_ = true;
            aquariumContentScroll_.offset = 0.0f;
            aquariumEmbeddedViewportStatus_ = L"Single-HWND 3D active | RMB look | WASD move | Q/E vertical";
            showToast(L"Camera", L"Environment camera reset.", D2DToastKind::Info);
            invalidateRect(aquariumEmbeddedViewportRect_.empty() ? mainRect_ : aquariumEmbeddedViewportRect_);
            return true;
        }

        if (aquariumManualForwardRect_.contains(x, y))
        {
            aquariumController_.StepManual(ace::aquarium::AceAqAction::MoveForward);
            return true;
        }

        if (aquariumManualLeftRect_.contains(x, y))
        {
            aquariumController_.StepManual(ace::aquarium::AceAqAction::TurnLeft);
            return true;
        }

        if (aquariumManualRightRect_.contains(x, y))
        {
            aquariumController_.StepManual(ace::aquarium::AceAqAction::TurnRight);
            return true;
        }

        if (aquariumManualWaitRect_.contains(x, y))
        {
            aquariumController_.StepManual(ace::aquarium::AceAqAction::Wait);
            return true;
        }

        if (aquariumManualTouchRect_.contains(x, y))
        {
            aquariumController_.StepManual(ace::aquarium::AceAqAction::TouchFront);
            return true;
        }

        if (aquariumManualConsumeRect_.contains(x, y))
        {
            aquariumController_.StepManual(ace::aquarium::AceAqAction::ConsumeFront);
            return true;
        }

        if (aquariumManualPushRect_.contains(x, y))
        {
            aquariumController_.StepManual(ace::aquarium::AceAqAction::PushFront);
            return true;
        }

        return false;
    }

    void AceShellUi::cycleAquariumScenario(int direction)
    {
        const auto names = aquariumController_.ScenarioNames();
        if (names.empty())
        {
            return;
        }

        auto it = std::find(names.begin(), names.end(), aquariumController_.CurrentScenarioName());
        std::ptrdiff_t index = it == names.end() ? 0 : std::distance(names.begin(), it);
        index += direction;

        if (index < 0)
        {
            index = static_cast<std::ptrdiff_t>(names.size()) - 1;
        }

        if (index >= static_cast<std::ptrdiff_t>(names.size()))
        {
            index = 0;
        }

        aquariumController_.ResetScenario(names[static_cast<std::size_t>(index)], 123);
    }

    void AceShellUi::cycleAquariumPlanner(int direction)
    {
        const std::vector<std::string> planners = {"random", "safe", "counterfactual"};
        auto it = std::find(planners.begin(), planners.end(), aquariumController_.CurrentPlannerName());
        std::ptrdiff_t index = it == planners.end() ? 0 : std::distance(planners.begin(), it);
        index += direction;

        if (index < 0)
        {
            index = static_cast<std::ptrdiff_t>(planners.size()) - 1;
        }

        if (index >= static_cast<std::ptrdiff_t>(planners.size()))
        {
            index = 0;
        }

        aquariumController_.SetPlanner(planners[static_cast<std::size_t>(index)]);
    }

    void AceShellUi::renderEnvironmentPlaceholder(D2DRenderContext& ctx)
    {
        renderAquariumControlPanel(ctx);
    }

    void AceShellUi::renderAquariumControlPanel(D2DRenderContext& ctx)
    {
        if (!environmentOpen_)
        {
            aquarium3DModeActive_ = false;
            aquariumEmbeddedViewportVisible_ = false;
            aquariumPendingViewportValid_ = false;
            aquariumEmbeddedViewportSyncNeeded_ = true;
            return;
        }

        if (!aquariumControllerReady_)
        {
            aquariumControllerReady_ = aquariumController_.Initialize();
        }

        if (engineEditorModeActive_)
        {
            renderEngineEditorMode(ctx);
        }
        else if (aquarium3DModeActive_)
        {
            renderAquariumFullScreen3DMode(ctx);
        }
        else
        {
            renderAquariumControlPanelHome(ctx);
        }
    }

    void AceShellUi::renderAquariumControlPanelHome(D2DRenderContext& ctx)
    {
        aquariumEmbeddedViewportVisible_ = false;
        aquariumPendingViewportValid_ = false;
        aquariumEmbeddedViewportSyncNeeded_ = true;

        const float modalW = std::min(1080.0f, ctx.width - 72.0f);
        const float modalH = std::min(720.0f, ctx.height - 96.0f);
        const float left = (ctx.width - modalW) * 0.5f;
        const float top = (ctx.height - modalH) * 0.5f;
        const UiRect panel = makeUiRect(left, top, left + modalW, top + modalH);
        environmentModalRect_ = panel;
        environmentModalCloseRect_ = makeUiRect(panel.right - 48.0f, panel.top + 16.0f, panel.right - 18.0f, panel.top + 46.0f);

        aquariumDetailsToggleRect_ = makeUiRect(0, 0, 0, 0);
        aquariumLogsToggleRect_ = makeUiRect(0, 0, 0, 0);
        aquariumEngineModeRect_ = makeUiRect(0, 0, 0, 0);
        aquariumLeftPanelRect_ = makeUiRect(0, 0, 0, 0);
        aquariumRightLogsPanelRect_ = makeUiRect(0, 0, 0, 0);
        aquariumLeftResizeHandleRect_ = makeUiRect(0, 0, 0, 0);
        aquariumRightResizeHandleRect_ = makeUiRect(0, 0, 0, 0);
        aquariumDebugRect_ = makeUiRect(0, 0, 0, 0);
        aquariumCameraResetRect_ = makeUiRect(0, 0, 0, 0);
        aquariumDetailsDebugRect_ = makeUiRect(0, 0, 0, 0);

        D2DGlassEffects::drawGlassOverlay(ctx, makeUiRect(0.0f, 0.0f, ctx.width, ctx.height), 0.42f);

        D2DGlassMaterial glass;
        glass.radius = 22.0f;
        glass.fillAlpha = 0.64f;
        glass.borderAlpha = 0.58f;
        glass.highlightAlpha = 0.20f;
        glass.glowAlpha = 0.20f;
        glass.shadowAlpha = 0.10f;
        glass.blurFallbackAlpha = 0.12f;
        glass.useCornerTicks = true;
        D2DGlassEffects::drawGlassPanel(ctx, panel, glass);
        D2DCyberEffects::drawBorderGlow(ctx, panel, 22.0f, 0.20f);

        renderAquariumMiniButton(ctx, environmentModalCloseRect_, L"X", false);

        D2DWidgetUtils::drawTextEx(
            ctx,
            L"Environment Control Panel",
            FontRole::Title,
            makeUiRect(panel.left + 28.0f, panel.top + 18.0f, panel.right - 64.0f, panel.top + 56.0f),
            ctx.brushes.text
        );

        const auto snapshot = aquariumController_.BuildSnapshot();
        const float margin = 28.0f;
        const float gap = 16.0f;
        const float contentLeft = panel.left + margin;
        const float contentRight = panel.right - margin;
        const float contentTop = panel.top + 72.0f;
        const float columnW = (contentRight - contentLeft - gap) * 0.5f;

        const UiRect runtimeCard = makeUiRect(contentLeft, contentTop, contentLeft + columnW, contentTop + 118.0f);
        renderAquariumLines(ctx, L"Runtime", {
            "scenario: " + snapshot.scenarioName,
            "planner: " + snapshot.plannerName,
            "step: " + std::to_string(snapshot.step),
            "status: " + std::string(snapshot.running ? "running" : "paused")
        }, runtimeCard, 5);

        const UiRect scenarioCard = makeUiRect(runtimeCard.right + gap, contentTop, contentRight, contentTop + 118.0f);
        renderAquariumLines(ctx, L"Scenario / Planner", {
            "Scenario: " + snapshot.scenarioName,
            "Planner:  " + snapshot.plannerName
        }, scenarioCard, 3);
        aquariumScenarioPrevRect_ = makeUiRect(scenarioCard.left + 18.0f, scenarioCard.bottom - 44.0f, scenarioCard.left + 56.0f, scenarioCard.bottom - 12.0f);
        aquariumScenarioNextRect_ = makeUiRect(aquariumScenarioPrevRect_.right + 8.0f, aquariumScenarioPrevRect_.top, aquariumScenarioPrevRect_.right + 46.0f, aquariumScenarioPrevRect_.bottom);
        aquariumPlannerPrevRect_ = makeUiRect(scenarioCard.left + 160.0f, aquariumScenarioPrevRect_.top, scenarioCard.left + 198.0f, aquariumScenarioPrevRect_.bottom);
        aquariumPlannerNextRect_ = makeUiRect(aquariumPlannerPrevRect_.right + 8.0f, aquariumScenarioPrevRect_.top, aquariumPlannerPrevRect_.right + 46.0f, aquariumScenarioPrevRect_.bottom);
        renderAquariumButton(ctx, aquariumScenarioPrevRect_, L"<", false);
        renderAquariumButton(ctx, aquariumScenarioNextRect_, L">", false);
        renderAquariumButton(ctx, aquariumPlannerPrevRect_, L"<", false);
        renderAquariumButton(ctx, aquariumPlannerNextRect_, L">", false);

        const UiRect mainActions = makeUiRect(contentLeft, runtimeCard.bottom + gap, contentRight, runtimeCard.bottom + gap + 86.0f);
        renderAquariumLines(ctx, L"Main Actions", {}, mainActions, 0);
        aquariumOpen3DRect_ = makeUiRect(mainActions.left + 18.0f, mainActions.top + 38.0f, mainActions.left + 284.0f, mainActions.bottom - 14.0f);
        aquariumResetRect_ = makeUiRect(aquariumOpen3DRect_.right + 16.0f, aquariumOpen3DRect_.top, aquariumOpen3DRect_.right + 106.0f, aquariumOpen3DRect_.bottom);
        aquariumStepRect_ = makeUiRect(aquariumResetRect_.right + 10.0f, aquariumOpen3DRect_.top, aquariumResetRect_.right + 90.0f, aquariumOpen3DRect_.bottom);
        aquariumRunPauseRect_ = makeUiRect(aquariumStepRect_.right + 10.0f, aquariumOpen3DRect_.top, aquariumStepRect_.right + 112.0f, aquariumOpen3DRect_.bottom);
        renderAquariumButton(ctx, aquariumOpen3DRect_, L"Enter 3D Environment >", true);
        renderAquariumButton(ctx, aquariumResetRect_, L"Reset", false);
        renderAquariumButton(ctx, aquariumStepRect_, L"Step", false);
        renderAquariumButton(ctx, aquariumRunPauseRect_, snapshot.running ? L"Pause" : L"Run", snapshot.running);

        const UiRect manual = makeUiRect(contentLeft, mainActions.bottom + gap, contentRight, mainActions.bottom + gap + 86.0f);
        renderAquariumLines(ctx, L"Manual Actions", {}, manual, 0);
        float bx = manual.left + 18.0f;
        const float by = manual.top + 38.0f;
        const float bh = manual.bottom - 14.0f;
        aquariumManualForwardRect_ = makeUiRect(bx, by, bx + 94.0f, bh); bx += 104.0f;
        aquariumManualLeftRect_ = makeUiRect(bx, by, bx + 78.0f, bh); bx += 88.0f;
        aquariumManualRightRect_ = makeUiRect(bx, by, bx + 82.0f, bh); bx += 92.0f;
        aquariumManualWaitRect_ = makeUiRect(bx, by, bx + 68.0f, bh); bx += 78.0f;
        aquariumManualTouchRect_ = makeUiRect(bx, by, bx + 78.0f, bh); bx += 88.0f;
        aquariumManualConsumeRect_ = makeUiRect(bx, by, bx + 96.0f, bh); bx += 106.0f;
        aquariumManualPushRect_ = makeUiRect(bx, by, bx + 70.0f, bh);
        renderAquariumButton(ctx, aquariumManualForwardRect_, L"Forward", false);
        renderAquariumButton(ctx, aquariumManualLeftRect_, L"Turn L", false);
        renderAquariumButton(ctx, aquariumManualRightRect_, L"Turn R", false);
        renderAquariumButton(ctx, aquariumManualWaitRect_, L"Wait", false);
        renderAquariumButton(ctx, aquariumManualTouchRect_, L"Touch", false);
        renderAquariumButton(ctx, aquariumManualConsumeRect_, L"Consume", false);
        renderAquariumButton(ctx, aquariumManualPushRect_, L"Push", false);

        const UiRect logs = makeUiRect(contentLeft, manual.bottom + gap, contentRight, panel.bottom - margin);
        renderAquariumScrollableLines(ctx, L"Logs / Episodes", snapshot.logLines, logs, aquariumLogScroll_, 64);
    }

    std::filesystem::path AceShellUi::engineWorkspaceLayoutPath() const
    {
        if (layoutProfilePath_.empty()) return {};
        return layoutProfilePath_.parent_path() / L"ace-engine-workspace.acebin";
    }

    void AceShellUi::enterEngineEditorMode()
    {
        if (!engineWorkspaceLoaded_)
        {
            const auto path = engineWorkspaceLayoutPath();
            if (!path.empty() && std::filesystem::exists(path))
            {
                std::string error;
                if (auto loaded = am::editor::EditorWorkspaceLayout::load(path, &error))
                {
                    const bool canonical = loaded->findNode("stack.viewport") && loaded->findNode("stack.outliner") &&
                        loaded->findNode("stack.details") && loaded->findNode("stack.content") &&
                        loaded->findTab("tab.viewport") && loaded->findTab("tab.outliner") &&
                        loaded->findTab("tab.details") && loaded->findTab("tab.content");
                    if (canonical) engineWorkspaceController_ = am::editor::EditorWorkspaceController(std::move(*loaded));
                    else showToast(L"Editor layout", L"Saved layout topology was incomplete; defaults restored.", D2DToastKind::Warning);
                }
                else
                    showToast(L"Editor layout", L"Saved layout was rejected; defaults restored.", D2DToastKind::Warning);
            }
            engineWorkspaceLoaded_ = true;
        }
        engineEditorModeActive_ = true;
        aquarium3DModeActive_ = true;
        aquariumUseSingleHwndCompositeViewport_ = true;
        aquariumEmbeddedViewportVisible_ = true;
        aquariumEmbeddedDx12Viewport_.Hide();
        engineLogOverlayVisible_ = false;
        invalidate();
    }

    void AceShellUi::leaveEngineEditorMode()
    {
        std::string ignored;
        if (engineWorkspaceController_.draggingSplitter())
            engineWorkspaceController_.cancelPointerInteraction(&ignored);
        closeEngineMenu();
        cancelContentBrowserEdit();
        contentBrowserAddPopupOpen_ = false;
        contentBrowserFocused_ = false;
        if (cameraSpeedPopupOpen_) closeCameraSpeedPopup(false);
        engineEditorModeActive_ = false;
        aquarium3DModeActive_ = true;
        aquariumEmbeddedViewportVisible_ = true;
        invalidate();
    }

    void AceShellUi::commitEngineWorkspaceLayout()
    {
        const auto path = engineWorkspaceLayoutPath();
        if (path.empty()) return;
        if (!engineWorkspaceController_.takeCommitRequested()) return;
        std::string error;
        if (engineWorkspaceController_.layout().save(path, &error))
            engineWorkspaceController_.markSaved();
        else
            showToast(L"Editor layout save failed", widen(error), D2DToastKind::Error);
    }

    void AceShellUi::initializeEngineCommands()
    {
        if (engineCommandsInitialized_)
        {
            return;
        }

        using namespace am::editor::commands;
        const Modifier ctrlShift = Modifier::Control | Modifier::Shift;
        bool registered = true;
        auto add = [&](CommandDescriptor descriptor)
        {
            std::string error;
            if (!engineCommandRegistry_.registerCommand(std::move(descriptor), &error))
            {
                registered = false;
            }
        };

        add({"editor.return_ai", L"AI Details", L"Return to the AI details workspace.", L"Editor", "Editor",
            CommandType::Action, std::nullopt, [this]() { leaveEngineEditorMode(); }});
        add({"window.toggle_outliner", L"World Outliner", L"Show or hide the real scene hierarchy panel.", L"Window", "Editor",
            CommandType::Toggle, KeyChord{'O', ctrlShift}, [this]() {
                std::string error;
                const auto* tab = engineWorkspaceController_.layout().findTab("tab.outliner");
                if (tab) engineWorkspaceController_.setTabVisible(tab->id, !tab->visible, &error);
                commitEngineWorkspaceLayout();
            }, {}, [this]() {
                const auto* tab = engineWorkspaceController_.layout().findTab("tab.outliner");
                return tab && tab->visible;
            }});
        add({"window.toggle_details", L"Details", L"Show or hide the selection details panel.", L"Window", "Editor",
            CommandType::Toggle, KeyChord{'D', ctrlShift}, [this]() {
                std::string error;
                const auto* tab = engineWorkspaceController_.layout().findTab("tab.details");
                if (tab) engineWorkspaceController_.setTabVisible(tab->id, !tab->visible, &error);
                commitEngineWorkspaceLayout();
            }, {}, [this]() {
                const auto* tab = engineWorkspaceController_.layout().findTab("tab.details");
                return tab && tab->visible;
            }});
        add({"window.toggle_content_browser", L"Content Browser", L"Open or close the project asset drawer.", L"Window", "Editor",
            CommandType::Toggle, KeyChord{VK_SPACE, Modifier::Control}, [this]() { toggleContentBrowser(); },
            [this]() { return contentBrowserController_ && contentBrowserController_->initialized(); },
            [this]() { return contentBrowserVisible(); }});
        add({"window.reset_layout", L"Reset Layout", L"Restore the canonical editor panel layout.", L"Window", "Editor",
            CommandType::Action, std::nullopt, [this]() {
                std::string error;
                engineWorkspaceController_.resetLayout(&error);
                commitEngineWorkspaceLayout();
            }});
        add({"view.reset_camera", L"Reset Camera", L"Restore the editor camera position and orientation.", L"View", "Editor",
            CommandType::Action, KeyChord{'0', Modifier::Control}, [this]() {
                aquariumSingleHwndCamera_.Reset();
                invalidateRect(aquariumEmbeddedViewportRect_);
            }});
        add({"view.camera_speed", L"Camera Speed...", L"Edit the shared logarithmic camera flight speed.", L"View", "Editor",
            CommandType::Action, std::nullopt, [this]() { openCameraSpeedPopup(); }});
        add({"view.toggle_console", L"Output Log / Console", L"Open or close the docked engine log console.", L"View", "Editor",
            CommandType::Toggle, std::nullopt, [this]() { toggleEngineLogOverlay(); }, {},
            [this]() { return engineLogOverlayVisible_; }});
        add({"help.shortcuts", L"Editor Shortcuts", L"Show the active editor shortcut reference.", L"Help", "Editor",
            CommandType::Action, KeyChord{VK_F1, Modifier::None}, [this]() {
                shortcutHelp_.toggle();
                requestParentCompositedViewportHold(24u, L"editor-shortcuts");
            }});
        add({"help.about", L"About ACE Editor", L"Show editor and renderer identity.", L"Help", "Editor",
            CommandType::Action, std::nullopt, [this]() {
                showToast(L"ACE Engine Editor", L"Native DX12 renderer + D2D editor shell.", D2DToastKind::Info);
            }});

        engineCommandsInitialized_ = registered;
    }

    bool AceShellUi::executeEngineCommand(std::string_view commandId)
    {
        initializeEngineCommands();
        const bool executed = engineCommandRegistry_.execute(commandId);
        if (executed)
        {
            closeEngineMenu();
            invalidate();
        }
        return executed;
    }

    bool AceShellUi::handleEngineCommandShortcut(WPARAM key, const D2DKeyboardState& keyboard, bool repeated)
    {
        if (!engineEditorModeActive_ || cameraSpeedInputFocused_ ||
            (engineLogOverlayVisible_ && engineLogOverlayInputFocused_))
        {
            return false;
        }

        using namespace am::editor::commands;
        Modifier modifiers = Modifier::None;
        if (keyboard.ctrl) modifiers = modifiers | Modifier::Control;
        if (keyboard.shift) modifiers = modifiers | Modifier::Shift;
        if (keyboard.alt) modifiers = modifiers | Modifier::Alt;
        const KeyChord chord{static_cast<std::uint16_t>(key), modifiers};
        const auto command = engineCommandRegistry_.resolve(chord, {"Editor"});
        if (!command)
        {
            return false;
        }
        if (repeated && !command->repeatable)
        {
            return true;
        }
        return executeEngineCommand(command->id);
    }

    void AceShellUi::closeEngineMenu()
    {
        const bool wasOpen = !engineOpenMenu_.empty();
        engineOpenMenu_.clear();
        engineMenuRows_.clear();
        engineMenuPopupRect_ = makeUiRect(0, 0, 0, 0);
        if (wasOpen) requestParentCompositedViewportHold(12u, L"editor-menu-close");
    }

    bool AceShellUi::handleEngineCommandSurfaceClick(float x, float y)
    {
        auto toggleMenu = [this](std::string name)
        {
            if (engineOpenMenu_ == name) closeEngineMenu();
            else
            {
                engineOpenMenu_ = std::move(name);
                requestParentCompositedViewportHold(24u, L"editor-menu-open");
            }
        };

        if (engineMenuWindowRect_.contains(x, y)) { toggleMenu("Window"); return true; }
        if (engineMenuViewRect_.contains(x, y)) { toggleMenu("View"); return true; }
        if (engineMenuHelpRect_.contains(x, y)) { toggleMenu("Help"); return true; }

        if (!engineOpenMenu_.empty())
        {
            for (const auto& [commandId, rect] : engineMenuRows_)
            {
                if (rect.contains(x, y))
                {
                    executeEngineCommand(commandId);
                    return true;
                }
            }
            if (engineMenuPopupRect_.contains(x, y)) return true;
            closeEngineMenu();
        }

        if (engineBackToAiRect_.contains(x, y)) return executeEngineCommand("editor.return_ai");
        if (engineCameraResetRect_.contains(x, y)) return executeEngineCommand("view.reset_camera");
        if (engineConsoleToggleRect_.contains(x, y)) return executeEngineCommand("view.toggle_console");
        if (engineOutlinerToggleRect_.contains(x, y)) return executeEngineCommand("window.toggle_outliner");
        if (engineDetailsToggleRect_.contains(x, y)) return executeEngineCommand("window.toggle_details");
        if (engineContentBrowserToggleRect_.contains(x, y)) return executeEngineCommand("window.toggle_content_browser");
        if (engineShortcutHelpRect_.contains(x, y)) return executeEngineCommand("help.shortcuts");
        return false;
    }

    void AceShellUi::renderEngineCommandSurface(D2DRenderContext& ctx, float topBarHeight)
    {
        initializeEngineCommands();
        const UiRect menuBar = makeUiRect(0.0f, 0.0f, ctx.width, 34.0f);
        const UiRect toolBar = makeUiRect(0.0f, 34.0f, ctx.width, topBarHeight);
        D2DWidgetUtils::fillRect(ctx, menuBar, ctx.brushes.panelDeep);
        D2DWidgetUtils::fillRect(ctx, toolBar, ctx.brushes.panelElevated);
        D2DWidgetUtils::drawSoftSeparator(ctx, makeUiRect(0.0f, 33.0f, ctx.width, 34.0f));

        D2DTextLayoutFoundation::Draw(ctx, L"ACE", FontRole::BodyStrong, makeUiRect(12.0f, 0.0f, 72.0f, 34.0f),
            ctx.brushes.accent, D2DTextOverflowMode::Ellipsis, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        engineMenuWindowRect_ = makeUiRect(74.0f, 0.0f, 150.0f, 34.0f);
        engineMenuViewRect_ = makeUiRect(150.0f, 0.0f, 208.0f, 34.0f);
        engineMenuHelpRect_ = makeUiRect(208.0f, 0.0f, 264.0f, 34.0f);
        auto drawMenuTitle = [&](UiRect rect, const wchar_t* label, const char* id)
        {
            if (engineOpenMenu_ == id || rect.contains(mouseX_, mouseY_))
                D2DWidgetUtils::fillRounded(ctx, rect.inset({2.0f, 2.0f, 2.0f, 2.0f}), 5.0f, ctx.brushes.panelSoft);
            D2DTextLayoutFoundation::Draw(ctx, label, FontRole::Small, rect, ctx.brushes.text,
                D2DTextOverflowMode::Ellipsis, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        };
        drawMenuTitle(engineMenuWindowRect_, L"Window", "Window");
        drawMenuTitle(engineMenuViewRect_, L"View", "View");
        drawMenuTitle(engineMenuHelpRect_, L"Help", "Help");
        D2DTextLayoutFoundation::Draw(ctx, L"ACE ENGINE EDITOR", FontRole::Small,
            makeUiRect(280.0f, 0.0f, ctx.width - 14.0f, 34.0f), ctx.brushes.muted,
            D2DTextOverflowMode::Ellipsis, DWRITE_TEXT_ALIGNMENT_TRAILING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        const float buttonTop = 42.0f;
        const float buttonBottom = topBarHeight - 8.0f;
        engineBackToAiRect_ = makeUiRect(12.0f, buttonTop, 116.0f, buttonBottom);
        engineCameraResetRect_ = makeUiRect(124.0f, buttonTop, 238.0f, buttonBottom);
        engineConsoleToggleRect_ = makeUiRect(246.0f, buttonTop, 348.0f, buttonBottom);
        engineOutlinerToggleRect_ = makeUiRect(356.0f, buttonTop, 458.0f, buttonBottom);
        engineDetailsToggleRect_ = makeUiRect(466.0f, buttonTop, 554.0f, buttonBottom);
        engineContentBrowserToggleRect_ = makeUiRect(562.0f, buttonTop, 678.0f, buttonBottom);
        engineShortcutHelpRect_ = makeUiRect(686.0f, buttonTop, 770.0f, buttonBottom);
        engineResetLayoutRect_ = makeUiRect(0, 0, 0, 0);

        const auto outliner = engineCommandRegistry_.find("window.toggle_outliner");
        const auto details = engineCommandRegistry_.find("window.toggle_details");
        const auto console = engineCommandRegistry_.find("view.toggle_console");
        const auto content = engineCommandRegistry_.find("window.toggle_content_browser");
        renderAquariumMiniButton(ctx, engineBackToAiRect_, L"AI Details", false);
        renderAquariumMiniButton(ctx, engineCameraResetRect_, L"Reset Camera", false);
        renderAquariumMiniButton(ctx, engineConsoleToggleRect_, L"Console", console && console->checked);
        renderAquariumMiniButton(ctx, engineOutlinerToggleRect_, L"Outliner", outliner && outliner->checked);
        renderAquariumMiniButton(ctx, engineDetailsToggleRect_, L"Details", details && details->checked);
        renderAquariumMiniButton(ctx, engineContentBrowserToggleRect_, L"Content", content && content->checked);
        renderAquariumMiniButton(ctx, engineShortcutHelpRect_, L"Shortcuts", shortcutHelp_.visible());

        engineMenuRows_.clear();
        engineMenuPopupRect_ = makeUiRect(0, 0, 0, 0);
        if (engineOpenMenu_.empty()) return;

        std::vector<std::string> commandIds;
        UiRect anchor{};
        if (engineOpenMenu_ == "Window")
        {
            anchor = engineMenuWindowRect_;
            commandIds = {"window.toggle_outliner", "window.toggle_details", "window.toggle_content_browser", "window.reset_layout"};
        }
        else if (engineOpenMenu_ == "View")
        {
            anchor = engineMenuViewRect_;
            commandIds = {"view.reset_camera", "view.camera_speed", "view.toggle_console"};
        }
        else
        {
            anchor = engineMenuHelpRect_;
            commandIds = {"help.shortcuts", "help.about"};
        }

        constexpr float rowHeight = 34.0f;
        constexpr float popupWidth = 266.0f;
        engineMenuPopupRect_ = makeUiRect(anchor.left, 34.0f, anchor.left + popupWidth,
            42.0f + rowHeight * static_cast<float>(commandIds.size()));
        D2DWidgetUtils::fillRounded(ctx, engineMenuPopupRect_, 7.0f, ctx.brushes.panelElevated, ctx.brushes.border, 1.0f);
        float top = 38.0f;
        for (const auto& commandId : commandIds)
        {
            const auto command = engineCommandRegistry_.find(commandId);
            if (!command) continue;
            const UiRect row = makeUiRect(anchor.left + 4.0f, top, anchor.left + popupWidth - 4.0f, top + rowHeight);
            engineMenuRows_.push_back({commandId, row});
            if (row.contains(mouseX_, mouseY_)) D2DWidgetUtils::fillRounded(ctx, row, 5.0f, ctx.brushes.panelSoft);
            const std::wstring marker = command->type == am::editor::commands::CommandType::Toggle ?
                (command->checked ? L"\x2713" : L" ") : L"";
            D2DTextLayoutFoundation::Draw(ctx, marker, FontRole::Small, makeUiRect(row.left + 8.0f, row.top, row.left + 30.0f, row.bottom),
                ctx.brushes.accent, D2DTextOverflowMode::Ellipsis, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            D2DTextLayoutFoundation::Draw(ctx, command->label, FontRole::Small, makeUiRect(row.left + 34.0f, row.top, row.right - 76.0f, row.bottom),
                command->enabled ? ctx.brushes.text : ctx.brushes.muted, D2DTextOverflowMode::Ellipsis,
                DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            const std::wstring shortcut = command->chord ? chordText(*command->chord) : L"";
            D2DTextLayoutFoundation::Draw(ctx, shortcut, FontRole::Small, makeUiRect(row.right - 74.0f, row.top, row.right - 8.0f, row.bottom),
                ctx.brushes.muted, D2DTextOverflowMode::Ellipsis, DWRITE_TEXT_ALIGNMENT_TRAILING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            top += rowHeight;
        }
    }

    bool AceShellUi::contentBrowserVisible() const
    {
        const auto* tab = engineWorkspaceController_.layout().findTab("tab.content");
        return contentBrowserController_ && contentBrowserController_->initialized() && tab && tab->visible;
    }

    void AceShellUi::toggleContentBrowser()
    {
        if (!contentBrowserController_ || !contentBrowserController_->initialized()) return;
        std::string error;
        const bool visible = contentBrowserVisible();
        if (!engineWorkspaceController_.setTabVisible("tab.content", !visible, &error))
        {
            showToast(L"Content Browser", widen(error), D2DToastKind::Error);
            return;
        }
        if (visible)
        {
            cancelContentBrowserEdit();
            contentBrowserAddPopupOpen_ = false;
            contentBrowserFocused_ = false;
        }
        else
        {
            contentBrowserController_->synchronize();
            contentBrowserFocused_ = true;
            requestParentCompositedViewportHold(24u, L"content-browser-open");
        }
        commitEngineWorkspaceLayout();
    }

    void AceShellUi::beginContentBrowserCreateFolder()
    {
        if (!contentBrowserVisible()) return;
        contentBrowserAddPopupOpen_ = false;
        contentBrowserEditMode_ = ContentBrowserEditMode::CreateFolder;
        contentBrowserEditInput_.setText(L"NewFolder");
        contentBrowserEditInput_.setFocused(true);
        contentBrowserEditInput_.selectAll();
    }

    bool AceShellUi::beginContentBrowserRename()
    {
        if (!contentBrowserVisible()) return false;
        auto* model = contentBrowserController_->model();
        if (!model || !model->beginRenameSelected()) return false;
        const auto* item = model->renameItem();
        if (!item) return false;
        contentBrowserEditMode_ = ContentBrowserEditMode::Rename;
        contentBrowserEditInput_.setText(widen(item->displayName));
        contentBrowserEditInput_.setFocused(true);
        contentBrowserEditInput_.selectAll();
        return true;
    }

    void AceShellUi::cancelContentBrowserEdit()
    {
        if (contentBrowserController_ && contentBrowserController_->model())
            contentBrowserController_->model()->cancelRename();
        contentBrowserEditMode_ = ContentBrowserEditMode::None;
        contentBrowserEditInput_.setFocused(false);
    }

    bool AceShellUi::commitContentBrowserEdit()
    {
        if (!contentBrowserController_) return false;
        if (contentBrowserEditMode_ != ContentBrowserEditMode::CreateFolder &&
            contentBrowserEditMode_ != ContentBrowserEditMode::Rename) return false;
        const std::string name = aceUtf8FromWide(contentBrowserEditInput_.text());
        if (name.empty())
        {
            showToast(L"Content Browser", L"Name cannot be empty or invalid Unicode.", D2DToastKind::Warning);
            return true;
        }
        const auto result = contentBrowserEditMode_ == ContentBrowserEditMode::CreateFolder ?
            contentBrowserController_->createFolder(name) : contentBrowserController_->renameSelection(name);
        if (!result)
        {
            showToast(L"Content Browser", widen(result.error), D2DToastKind::Warning);
            return false;
        }
        cancelContentBrowserEdit();
        contentBrowserItemsScroll_ = 0.0f;
        return true;
    }

    void AceShellUi::renderContentBrowser(D2DRenderContext& ctx, UiRect contentRect)
    {
        using namespace am::editor::content_browser;
        contentBrowserRect_ = contentRect;
        contentBrowserTreeHits_.clear();
        contentBrowserItemHits_.clear();
        if (!contentBrowserVisible() || contentRect.empty()) return;

        auto* model = contentBrowserController_->model();
        if (!model) return;
        D2DWidgetUtils::fillRect(ctx, contentRect, ctx.brushes.panelDeep);

        const bool compactToolbar = contentRect.width() < 760.0f;
        const float desiredToolbarHeight = compactToolbar ? 76.0f : 44.0f;
        const float toolbarHeight = std::min(desiredToolbarHeight,
            std::max(32.0f, contentRect.height() * (compactToolbar ? 0.42f : 0.22f)));
        const UiRect toolbar = makeUiRect(contentRect.left, contentRect.top, contentRect.right, contentRect.top + toolbarHeight);
        D2DWidgetUtils::fillRect(ctx, toolbar, ctx.brushes.panelElevated);
        D2DWidgetUtils::drawSoftSeparator(ctx, makeUiRect(toolbar.left, toolbar.bottom - 1.0f, toolbar.right, toolbar.bottom));

        float x = toolbar.left + 8.0f;
        const float buttonTop = toolbar.top + 6.0f;
        const float buttonBottom = std::min(toolbar.bottom - 4.0f, toolbar.top + 36.0f);
        contentBrowserBackRect_ = makeUiRect(x, buttonTop, x + 30.0f, buttonBottom); x += 34.0f;
        contentBrowserForwardRect_ = makeUiRect(x, buttonTop, x + 30.0f, buttonBottom); x += 34.0f;
        contentBrowserUpRect_ = makeUiRect(x, buttonTop, x + 30.0f, buttonBottom); x += 38.0f;
        contentBrowserAddRect_ = makeUiRect(x, buttonTop, x + 92.0f, buttonBottom); x += 100.0f;
        renderAquariumMiniButton(ctx, contentBrowserBackRect_, L"<", model->canBack());
        renderAquariumMiniButton(ctx, contentBrowserForwardRect_, L">", model->canForward());
        renderAquariumMiniButton(ctx, contentBrowserUpRect_, L"^", model->canGoUp());
        renderAquariumMiniButton(ctx, contentBrowserAddRect_, L"+ Add", contentBrowserAddPopupOpen_);

        const float viewRight = toolbar.right - 8.0f;
        contentBrowserListRect_ = makeUiRect(viewRight - 32.0f, buttonTop, viewRight, buttonBottom);
        contentBrowserTilesRect_ = makeUiRect(viewRight - 68.0f, buttonTop, viewRight - 36.0f, buttonBottom);
        renderAquariumMiniButton(ctx, contentBrowserTilesRect_, L"[]", model->viewMode() == ViewMode::Tiles);
        renderAquariumMiniButton(ctx, contentBrowserListRect_, L"=", model->viewMode() == ViewMode::List);
        const float searchWidth = std::clamp(contentRect.width() * 0.22f, 150.0f, 290.0f);
        const bool showSearch = compactToolbar ? toolbarHeight >= 64.0f : contentBrowserTilesRect_.left > x + 170.0f;
        contentBrowserSearchRect_ = !showSearch ? makeUiRect(0, 0, 0, 0) : (compactToolbar ?
            makeUiRect(toolbar.left + 8.0f, toolbar.top + 42.0f, toolbar.right - 8.0f, toolbar.bottom - 5.0f) :
            makeUiRect(std::max(x + 120.0f, contentBrowserTilesRect_.left - searchWidth - 8.0f),
                buttonTop, contentBrowserTilesRect_.left - 8.0f, buttonBottom));
        if (showSearch)
        {
            contentBrowserSearchInput_.setRect(contentBrowserSearchRect_);
            contentBrowserSearchInput_.setPlaceholder(L"Search assets...");
            contentBrowserSearchInput_.render(ctx);
        }

        UiRect breadcrumbRect = compactToolbar ? makeUiRect(0, 0, 0, 0) :
            makeUiRect(x, buttonTop, std::max(x, contentBrowserSearchRect_.left - 8.0f), buttonBottom);
        std::wstring breadcrumbText;
        for (const auto& crumb : model->breadcrumbs())
        {
            if (!breadcrumbText.empty()) breadcrumbText += L"  >  ";
            breadcrumbText += widen(crumb.label);
        }
        D2DTextLayoutFoundation::Draw(ctx, breadcrumbText, FontRole::Small, breadcrumbRect, ctx.brushes.textDim,
            D2DTextOverflowMode::Ellipsis, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        const float bodyTop = toolbar.bottom;
        if (bodyTop + 24.0f >= contentRect.bottom)
        {
            contentBrowserTreeRect_ = makeUiRect(0, 0, 0, 0);
            contentBrowserItemsRect_ = makeUiRect(0, 0, 0, 0);
            return;
        }
        const float treeWidth = std::min(contentRect.width() * 0.46f,
            std::clamp(contentRect.width() * 0.22f, 150.0f, 280.0f));
        contentBrowserTreeRect_ = makeUiRect(contentRect.left, bodyTop, contentRect.left + treeWidth, contentRect.bottom);
        contentBrowserItemsRect_ = makeUiRect(contentBrowserTreeRect_.right + 1.0f, bodyTop, contentRect.right, contentRect.bottom);
        D2DWidgetUtils::fillRect(ctx, contentBrowserTreeRect_, ctx.brushes.panel);
        D2DWidgetUtils::drawSoftSeparator(ctx, makeUiRect(contentBrowserTreeRect_.right, bodyTop,
            contentBrowserTreeRect_.right + 1.0f, contentRect.bottom));

        constexpr float treeRowHeight = 24.0f;
        const float treeContentHeight = treeRowHeight * static_cast<float>(model->folderTree().size()) + 12.0f;
        const float treeMaxScroll = std::max(0.0f, treeContentHeight - contentBrowserTreeRect_.height());
        contentBrowserTreeScroll_ = std::clamp(contentBrowserTreeScroll_, 0.0f, treeMaxScroll);
        if (ctx.target) ctx.target->PushAxisAlignedClip(contentBrowserTreeRect_.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        float treeTop = contentBrowserTreeRect_.top + 6.0f - contentBrowserTreeScroll_;
        for (const auto& folder : model->folderTree())
        {
            const UiRect row = makeUiRect(contentBrowserTreeRect_.left + 4.0f, treeTop,
                contentBrowserTreeRect_.right - 4.0f, treeTop + treeRowHeight);
            if (row.bottom >= contentBrowserTreeRect_.top && row.top <= contentBrowserTreeRect_.bottom)
            {
                contentBrowserTreeHits_.push_back({folder.path, row});
                const bool active = folder.path.comparisonKey() == model->currentFolder().comparisonKey();
                if (active || row.contains(mouseX_, mouseY_))
                    D2DWidgetUtils::fillRounded(ctx, row, 4.0f, active ? ctx.brushes.panelSoft : ctx.brushes.panelElevated);
                const float indent = 8.0f + static_cast<float>(folder.depth) * 14.0f;
                const std::wstring label = folder.path.isRoot() ? L"Content" : widen(std::string(folder.path.leafName()));
                D2DTextLayoutFoundation::Draw(ctx, L"\x25B8", FontRole::Small,
                    makeUiRect(row.left + indent, row.top, row.left + indent + 14.0f, row.bottom), ctx.brushes.accent,
                    D2DTextOverflowMode::Ellipsis, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                D2DTextLayoutFoundation::Draw(ctx, label, FontRole::Small,
                    makeUiRect(row.left + indent + 18.0f, row.top, row.right - 8.0f, row.bottom),
                    active ? ctx.brushes.text : ctx.brushes.textDim, D2DTextOverflowMode::Ellipsis,
                    DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            }
            treeTop += treeRowHeight;
        }
        if (ctx.target) ctx.target->PopAxisAlignedClip();

        const auto& items = model->visibleItems();
        const UiRect itemClip = contentBrowserItemsRect_.inset({8.0f, 6.0f, 8.0f, 22.0f});
        const bool tiles = model->viewMode() == ViewMode::Tiles;
        const float itemWidth = tiles ? 132.0f : itemClip.width();
        const float itemHeight = tiles ? 82.0f : 30.0f;
        const std::size_t columns = tiles ? std::max<std::size_t>(1, static_cast<std::size_t>(itemClip.width() / itemWidth)) : 1;
        const std::size_t rows = items.empty() ? 0 : (items.size() + columns - 1) / columns;
        const float itemsContentHeight = static_cast<float>(rows) * itemHeight + 10.0f;
        const float itemsMaxScroll = std::max(0.0f, itemsContentHeight - itemClip.height());
        contentBrowserItemsScroll_ = std::clamp(contentBrowserItemsScroll_, 0.0f, itemsMaxScroll);
        if (ctx.target) ctx.target->PushAxisAlignedClip(itemClip.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        for (std::size_t index = 0; index < items.size(); ++index)
        {
            const std::size_t rowIndex = index / columns;
            const std::size_t columnIndex = index % columns;
            const float left = itemClip.left + static_cast<float>(columnIndex) * itemWidth;
            const float top = itemClip.top + static_cast<float>(rowIndex) * itemHeight - contentBrowserItemsScroll_;
            const UiRect itemRect = makeUiRect(left + 2.0f, top + 2.0f,
                std::min(itemClip.right, left + itemWidth - 4.0f), top + itemHeight - 4.0f);
            if (itemRect.bottom < itemClip.top || itemRect.top > itemClip.bottom) continue;
            contentBrowserItemHits_.push_back({index, itemRect});
            const bool selected = model->isSelected(items[index]);
            if (selected || itemRect.contains(mouseX_, mouseY_))
                D2DWidgetUtils::fillRounded(ctx, itemRect, 5.0f, selected ? ctx.brushes.panelSoft : ctx.brushes.panelElevated,
                    selected ? ctx.brushes.accent : ctx.brushes.borderDim, 1.0f);

            const wchar_t* glyph = items[index].kind == ItemKind::Folder ? L"\x25A0" : L"\x25C6";
            const UiRect iconRect = tiles ? makeUiRect(itemRect.left + 8.0f, itemRect.top + 5.0f,
                itemRect.right - 8.0f, itemRect.top + 42.0f) : makeUiRect(itemRect.left + 6.0f, itemRect.top,
                itemRect.left + 30.0f, itemRect.bottom);
            D2DTextLayoutFoundation::Draw(ctx, glyph, tiles ? FontRole::BodyStrong : FontRole::Small, iconRect,
                items[index].kind == ItemKind::Folder ? ctx.brushes.accentWarm : ctx.brushes.accent,
                D2DTextOverflowMode::Ellipsis, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            const UiRect labelRect = tiles ? makeUiRect(itemRect.left + 6.0f, itemRect.top + 43.0f,
                itemRect.right - 6.0f, itemRect.bottom - 3.0f) : makeUiRect(itemRect.left + 34.0f, itemRect.top,
                itemRect.right - 82.0f, itemRect.bottom);
            D2DTextLayoutFoundation::Draw(ctx, widen(items[index].displayName), FontRole::Small, labelRect,
                selected ? ctx.brushes.text : ctx.brushes.textDim, D2DTextOverflowMode::Ellipsis,
                tiles ? DWRITE_TEXT_ALIGNMENT_CENTER : DWRITE_TEXT_ALIGNMENT_LEADING,
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            if (!tiles && items[index].kind == ItemKind::Asset)
                D2DTextLayoutFoundation::Draw(ctx, widen(std::string(am::core::assets::AssetRegistry::typeName(items[index].assetType))),
                    FontRole::Small, makeUiRect(itemRect.right - 80.0f, itemRect.top, itemRect.right - 8.0f, itemRect.bottom),
                    ctx.brushes.muted, D2DTextOverflowMode::Ellipsis, DWRITE_TEXT_ALIGNMENT_TRAILING,
                    DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        if (contentBrowserEditMode_ == ContentBrowserEditMode::CreateFolder)
        {
            const UiRect editRect = makeUiRect(itemClip.left + 4.0f, itemClip.top + 4.0f,
                std::min(itemClip.right, itemClip.left + 260.0f), itemClip.top + 36.0f);
            contentBrowserEditInput_.setRect(editRect);
            contentBrowserEditInput_.render(ctx);
        }
        else if (contentBrowserEditMode_ == ContentBrowserEditMode::Rename && model->renameItem())
        {
            for (const auto& hit : contentBrowserItemHits_)
            {
                if (hit.index < items.size() && model->isSelected(items[hit.index]))
                {
                    const UiRect editRect = tiles ? makeUiRect(hit.rect.left + 4.0f, hit.rect.bottom - 32.0f,
                        hit.rect.right - 4.0f, hit.rect.bottom - 2.0f) : hit.rect.inset({32.0f, 1.0f, 70.0f, 1.0f});
                    contentBrowserEditInput_.setRect(editRect);
                    contentBrowserEditInput_.render(ctx);
                    break;
                }
            }
        }
        if (ctx.target) ctx.target->PopAxisAlignedClip();

        const std::wstring count = std::to_wstring(items.size()) + L" items" +
            (model->unfilteredItemCount() == items.size() ? L"" : L" (filtered)");
        D2DTextLayoutFoundation::Draw(ctx, count, FontRole::Small,
            makeUiRect(contentBrowserItemsRect_.left + 10.0f, contentBrowserItemsRect_.bottom - 20.0f,
                contentBrowserItemsRect_.right - 10.0f, contentBrowserItemsRect_.bottom), ctx.brushes.muted,
            D2DTextOverflowMode::Ellipsis, DWRITE_TEXT_ALIGNMENT_TRAILING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        contentBrowserAddPopupRect_ = makeUiRect(0, 0, 0, 0);
        contentBrowserNewFolderRect_ = makeUiRect(0, 0, 0, 0);
        if (contentBrowserAddPopupOpen_)
        {
            contentBrowserAddPopupRect_ = makeUiRect(contentBrowserAddRect_.left, contentBrowserAddRect_.bottom + 2.0f,
                contentBrowserAddRect_.left + 190.0f, contentBrowserAddRect_.bottom + 44.0f);
            contentBrowserNewFolderRect_ = contentBrowserAddPopupRect_.inset({4.0f, 4.0f, 4.0f, 4.0f});
            D2DWidgetUtils::fillRounded(ctx, contentBrowserAddPopupRect_, 6.0f, ctx.brushes.panelElevated,
                ctx.brushes.border, 1.0f);
            if (contentBrowserNewFolderRect_.contains(mouseX_, mouseY_))
                D2DWidgetUtils::fillRounded(ctx, contentBrowserNewFolderRect_, 4.0f, ctx.brushes.panelSoft);
            D2DTextLayoutFoundation::Draw(ctx, L"New Folder", FontRole::Small,
                contentBrowserNewFolderRect_.inset({10.0f, 0.0f, 8.0f, 0.0f}), ctx.brushes.text,
                D2DTextOverflowMode::Ellipsis, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }

    bool AceShellUi::handleContentBrowserMouseDown(D2DRenderContext& ctx, float x, float y, unsigned clickCount)
    {
        using namespace am::editor::content_browser;
        if (!contentBrowserVisible() || !contentBrowserRect_.contains(x, y)) return false;
        contentBrowserFocused_ = true;
        auto* model = contentBrowserController_->model();
        if (!model) return false;

        if (contentBrowserAddPopupOpen_)
        {
            if (contentBrowserNewFolderRect_.contains(x, y)) { beginContentBrowserCreateFolder(); return true; }
            if (contentBrowserAddPopupRect_.contains(x, y)) return true;
            contentBrowserAddPopupOpen_ = false;
        }
        if (contentBrowserBackRect_.contains(x, y)) { if (model->back()) contentBrowserItemsScroll_ = 0.0f; return true; }
        if (contentBrowserForwardRect_.contains(x, y)) { if (model->forward()) contentBrowserItemsScroll_ = 0.0f; return true; }
        if (contentBrowserUpRect_.contains(x, y)) { if (model->up()) contentBrowserItemsScroll_ = 0.0f; return true; }
        if (contentBrowserAddRect_.contains(x, y)) { contentBrowserAddPopupOpen_ = !contentBrowserAddPopupOpen_; return true; }
        if (contentBrowserTilesRect_.contains(x, y)) { model->setViewMode(ViewMode::Tiles); return true; }
        if (contentBrowserListRect_.contains(x, y)) { model->setViewMode(ViewMode::List); return true; }
        if (contentBrowserSearchRect_.contains(x, y))
        {
            if ((contentBrowserEditMode_ == ContentBrowserEditMode::CreateFolder ||
                 contentBrowserEditMode_ == ContentBrowserEditMode::Rename) && !commitContentBrowserEdit()) return true;
            contentBrowserEditMode_ = ContentBrowserEditMode::Search;
            contentBrowserSearchInput_.setFocused(true);
            contentBrowserSearchInput_.onMouseDown(ctx, x, y);
            return true;
        }
        contentBrowserSearchInput_.setFocused(false);
        if ((contentBrowserEditMode_ == ContentBrowserEditMode::CreateFolder ||
             contentBrowserEditMode_ == ContentBrowserEditMode::Rename) && contentBrowserEditInput_.hitTest(x, y))
        {
            contentBrowserEditInput_.onMouseDown(ctx, x, y);
            return true;
        }
        if ((contentBrowserEditMode_ == ContentBrowserEditMode::CreateFolder ||
             contentBrowserEditMode_ == ContentBrowserEditMode::Rename) && !commitContentBrowserEdit()) return true;
        if (contentBrowserEditMode_ == ContentBrowserEditMode::Search) contentBrowserEditMode_ = ContentBrowserEditMode::None;

        for (const auto& hit : contentBrowserTreeHits_)
        {
            if (hit.rect.contains(x, y))
            {
                if (model->navigateTo(hit.path)) contentBrowserItemsScroll_ = 0.0f;
                return true;
            }
        }
        for (const auto& hit : contentBrowserItemHits_)
        {
            if (!hit.rect.contains(x, y) || hit.index >= model->visibleItems().size()) continue;
            const auto keyboard = D2DKeyboardState::current();
            const SelectionMode mode = keyboard.shift ? SelectionMode::Range :
                (keyboard.ctrl ? SelectionMode::Toggle : SelectionMode::Replace);
            (void)model->selectVisible(hit.index, mode);
            const auto item = model->visibleItems()[hit.index];
            if (clickCount >= 2 && item.kind == ItemKind::Folder)
            {
                if (model->navigateTo(item.path)) contentBrowserItemsScroll_ = 0.0f;
            }
            return true;
        }
        if (contentBrowserItemsRect_.contains(x, y)) model->clearSelection();
        return true;
    }

    bool AceShellUi::handleContentBrowserMouseWheel(D2DRenderContext& ctx, float x, float y, int wheelDelta)
    {
        (void)ctx;
        if (!contentBrowserVisible() || !contentBrowserRect_.contains(x, y)) return false;
        const float delta = -static_cast<float>(wheelDelta) * 0.28f;
        if (contentBrowserTreeRect_.contains(x, y)) contentBrowserTreeScroll_ = std::max(0.0f, contentBrowserTreeScroll_ + delta);
        else if (contentBrowserItemsRect_.contains(x, y)) contentBrowserItemsScroll_ = std::max(0.0f, contentBrowserItemsScroll_ + delta);
        return true;
    }

    bool AceShellUi::handleContentBrowserChar(WPARAM wParam)
    {
        if (!contentBrowserVisible() || !contentBrowserFocused_) return false;
        if (contentBrowserEditMode_ == ContentBrowserEditMode::Search && contentBrowserSearchInput_.onChar(wParam))
        {
            contentBrowserController_->model()->setSearchText(aceUtf8FromWide(contentBrowserSearchInput_.text()));
            contentBrowserItemsScroll_ = 0.0f;
            return true;
        }
        if ((contentBrowserEditMode_ == ContentBrowserEditMode::CreateFolder ||
             contentBrowserEditMode_ == ContentBrowserEditMode::Rename) && contentBrowserEditInput_.onChar(wParam)) return true;
        return false;
    }

    bool AceShellUi::handleContentBrowserKeyDown(WPARAM key, const D2DKeyboardState& keyboard)
    {
        using namespace am::editor::content_browser;
        if (!contentBrowserVisible() || !contentBrowserFocused_) return false;
        auto* model = contentBrowserController_->model();
        if (!model) return false;
        if (keyboard.ctrl && key == VK_SPACE) return false;

        if (contentBrowserEditMode_ == ContentBrowserEditMode::Search)
        {
            if (key == VK_ESCAPE)
            {
                contentBrowserSearchInput_.setFocused(false);
                contentBrowserEditMode_ = ContentBrowserEditMode::None;
                return true;
            }
            if (key == VK_RETURN)
            {
                contentBrowserSearchInput_.setFocused(false);
                contentBrowserEditMode_ = ContentBrowserEditMode::None;
                return true;
            }
            if (contentBrowserSearchInput_.onKeyDown(key, keyboard.ctrl, keyboard.shift))
            {
                model->setSearchText(aceUtf8FromWide(contentBrowserSearchInput_.text()));
                contentBrowserItemsScroll_ = 0.0f;
                return true;
            }
        }
        if (contentBrowserEditMode_ == ContentBrowserEditMode::CreateFolder ||
            contentBrowserEditMode_ == ContentBrowserEditMode::Rename)
        {
            if (key == VK_ESCAPE) { cancelContentBrowserEdit(); return true; }
            if (key == VK_RETURN) { (void)commitContentBrowserEdit(); return true; }
            if (contentBrowserEditInput_.onKeyDown(key, keyboard.ctrl, keyboard.shift)) return true;
            return false;
        }
        if (key == VK_F2 && keyboard.noModifiers()) return beginContentBrowserRename();
        if (keyboard.ctrl && key == 'A')
        {
            model->clearSelection();
            for (std::size_t index = 0; index < model->visibleItems().size(); ++index)
                (void)model->selectVisible(index, index == 0 ? SelectionMode::Replace : SelectionMode::Add);
            return true;
        }
        if (keyboard.ctrl && key == 'Z')
        {
            const auto result = contentBrowserController_->undo();
            if (!result) showToast(L"Undo", widen(result.error), D2DToastKind::Warning);
            return true;
        }
        if (keyboard.ctrl && key == 'Y')
        {
            const auto result = contentBrowserController_->redo();
            if (!result) showToast(L"Redo", widen(result.error), D2DToastKind::Warning);
            return true;
        }
        const auto& items = model->visibleItems();
        if ((key == VK_UP || key == VK_DOWN || key == VK_HOME || key == VK_END) && !items.empty())
        {
            std::size_t selectedIndex = 0;
            bool found = false;
            for (std::size_t index = 0; index < items.size(); ++index)
            {
                if (model->isSelected(items[index])) { selectedIndex = index; found = true; break; }
            }
            if (key == VK_HOME) selectedIndex = 0;
            else if (key == VK_END) selectedIndex = items.size() - 1;
            else if (key == VK_UP && found && selectedIndex > 0) --selectedIndex;
            else if (key == VK_DOWN && found && selectedIndex + 1 < items.size()) ++selectedIndex;
            else if (key == VK_DOWN && !found) selectedIndex = 0;
            (void)model->selectVisible(selectedIndex, keyboard.shift ? SelectionMode::Range : SelectionMode::Replace);
            return true;
        }
        if (key == VK_RETURN && model->selectionCount() == 1)
        {
            const auto selected = model->selectedItems();
            if (!selected.empty() && selected.front().kind == ItemKind::Folder)
            {
                (void)model->navigateTo(selected.front().path);
                contentBrowserItemsScroll_ = 0.0f;
            }
            return true;
        }
        if (key == VK_BACK && keyboard.noModifiers()) { (void)model->up(); return true; }
        if (key == VK_ESCAPE && keyboard.noModifiers()) { model->clearSelection(); return true; }
        return false;
    }

    bool AceShellUi::handleEngineEditorClick(float x, float y, unsigned clickCount)
    {
        if (handleEngineCommandSurfaceClick(x, y)) { contentBrowserFocused_ = false; return true; }
        auto ctx = makeContext();
        if (handleContentBrowserMouseDown(ctx, x, y, clickCount)) return true;
        contentBrowserFocused_ = false;
        std::string error;
        if (engineWorkspaceController_.pointerDown(x, y, &error))
        {
            if (engineWorkspaceController_.draggingSplitter())
            {
                mouseCaptured_ = true;
                SetCapture(parent_);
            }
            commitEngineWorkspaceLayout();
            return true;
        }
        return false;
    }

    void AceShellUi::renderEngineEditorMode(D2DRenderContext& ctx)
    {
        const auto snapshot = aquariumController_.BuildSnapshot();
        const float topBarHeight = 92.0f;
        const am::editor::WorkspaceRect workspaceBounds{0.0, topBarHeight, ctx.width, ctx.height};
        std::string layoutError;
        if (!engineWorkspaceController_.arrange(workspaceBounds, {}, &layoutError))
        {
            engineWorkspaceController_.resetLayout(&layoutError);
            engineWorkspaceController_.arrange(workspaceBounds, {}, &layoutError);
        }
        const auto* geometry = engineWorkspaceController_.geometry();
        if (!geometry) return;

        auto toUiRect = [](const am::editor::WorkspaceRect& rect)
        {
            return makeUiRect(static_cast<float>(rect.left), static_cast<float>(rect.top),
                static_cast<float>(rect.right), static_cast<float>(rect.bottom));
        };

        environmentModalRect_ = makeUiRect(0.0f, 0.0f, ctx.width, ctx.height);
        environmentModalCloseRect_ = makeUiRect(0, 0, 0, 0);
        aquariumEngineModeRect_ = makeUiRect(0, 0, 0, 0);
        aquariumDetailsToggleRect_ = makeUiRect(0, 0, 0, 0);
        aquariumLogsToggleRect_ = makeUiRect(0, 0, 0, 0);
        aquariumLeftPanelRect_ = makeUiRect(0, 0, 0, 0);
        aquariumRightLogsPanelRect_ = makeUiRect(0, 0, 0, 0);

        D2DWidgetUtils::fillRect(ctx, environmentModalRect_, ctx.brushes.panelDeep);

        for (const auto& nodeGeometry : geometry->nodes())
        {
            if (nodeGeometry.kind != am::editor::DockNodeKind::Stack) continue;
            const UiRect bounds = toUiRect(nodeGeometry.bounds);
            const UiRect tabBar = toUiRect(nodeGeometry.tabBar);
            D2DWidgetUtils::fillRect(ctx, bounds, nodeGeometry.nodeId == "stack.viewport" ? ctx.brushes.panelDeep : ctx.brushes.panel);
            D2DWidgetUtils::fillRect(ctx, tabBar, ctx.brushes.panelElevated);
            if (ctx.target && ctx.brushes.borderDim) ctx.target->DrawRectangle(bounds.d2d(), ctx.brushes.borderDim, 1.0f);
        }

        for (const auto& tabGeometry : geometry->tabs())
        {
            const UiRect tabRect = toUiRect(tabGeometry.bounds);
            if (tabGeometry.active) D2DWidgetUtils::fillRect(ctx, tabRect, ctx.brushes.panelSoft);
            const auto* tab = engineWorkspaceController_.layout().findTab(tabGeometry.tabId);
            if (tab)
                D2DTextLayoutFoundation::Draw(ctx, widen(tab->label), FontRole::Small, tabRect.inset({10.0f, 0.0f, 8.0f, 0.0f}),
                    tabGeometry.active ? ctx.brushes.text : ctx.brushes.muted, D2DTextOverflowMode::Ellipsis,
                    DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        for (const auto& splitter : geometry->splitters())
            D2DWidgetUtils::fillRect(ctx, toUiRect(splitter.visualBounds), ctx.brushes.border);

        const auto* viewportNode = geometry->findNode("stack.viewport");
        aquariumEmbeddedViewportRect_ = viewportNode ? toUiRect(viewportNode->content) : makeUiRect(0, 0, 0, 0);
        aquariumLastViewportRect_ = aquariumEmbeddedViewportRect_;
        aquariumEmbeddedViewportVisible_ = !aquariumEmbeddedViewportRect_.empty();
        aquariumUseSingleHwndCompositeViewport_ = true;
        aquariumTelemetryOverlayRect_ = makeUiRect(0, 0, 0, 0);
        if (aquariumEmbeddedViewportVisible_)
        {
            renderAquariumDx12ViewportSurface(ctx, aquariumEmbeddedViewportRect_, snapshot.debugTruthEnabled);
            renderCameraSpeedControl(ctx, aquariumEmbeddedViewportRect_);
        }

        const auto primitives = aquariumSceneAdapter_.BuildPrimitives(aquariumController_, false);
        std::array<std::size_t, 7> primitiveCounts{};
        for (const auto& primitive : primitives)
        {
            const auto index = static_cast<std::size_t>(primitive.Kind);
            if (index < primitiveCounts.size()) ++primitiveCounts[index];
        }

        auto renderRows = [&](const char* nodeId, const std::vector<std::wstring>& rows)
        {
            const auto* node = geometry->findNode(nodeId);
            if (!node || node->content.empty()) return;
            const UiRect content = toUiRect(node->content).inset({12.0f, 10.0f, 12.0f, 8.0f});
            if (ctx.target) ctx.target->PushAxisAlignedClip(content.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            float top = content.top;
            for (const auto& row : rows)
            {
                if (top + 24.0f > content.bottom) break;
                D2DTextLayoutFoundation::Draw(ctx, row, FontRole::Small,
                    makeUiRect(content.left, top, content.right, top + 22.0f), ctx.brushes.textDim,
                    D2DTextOverflowMode::Ellipsis, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                top += 24.0f;
            }
            if (ctx.target) ctx.target->PopAxisAlignedClip();
        };

        renderRows("stack.outliner", {
            L"Scene",
            L"  Editor Camera",
            L"  Dynamic Meshes (1)",
            L"  Blocks (" + std::to_wstring(primitiveCounts[static_cast<std::size_t>(ace::aquarium_render::AceAqRenderPrimitiveKind::Block)]) + L")",
            L"  Tiles (" + std::to_wstring(primitiveCounts[static_cast<std::size_t>(ace::aquarium_render::AceAqRenderPrimitiveKind::Tile)]) + L")",
            L"  Grid lines (" + std::to_wstring(primitiveCounts[static_cast<std::size_t>(ace::aquarium_render::AceAqRenderPrimitiveKind::GridLine)]) + L")"
        });
        const auto cameraPosition = aquariumSingleHwndCamera_.Position();
        renderRows("stack.details", {
            L"Renderer    DX12 + D2D composite",
            L"Viewport    " + std::to_wstring(static_cast<int>(aquariumEmbeddedViewportRect_.width())) + L" x " +
                std::to_wstring(static_cast<int>(aquariumEmbeddedViewportRect_.height())),
            L"Primitives  " + std::to_wstring(primitives.size()),
            L"Camera X    " + std::to_wstring(cameraPosition.x),
            L"Camera Y    " + std::to_wstring(cameraPosition.y),
            L"Camera Z    " + std::to_wstring(cameraPosition.z),
            L"Move speed  " + std::to_wstring(aquariumSingleHwndCamera_.MoveSpeed())
        });

        if (const auto* contentNode = geometry->findNode("stack.content"))
            renderContentBrowser(ctx, toUiRect(contentNode->content));
        else
            contentBrowserRect_ = makeUiRect(0, 0, 0, 0);

        // Menus are a real overlay layer and therefore paint after the viewport
        // and dock panels. This mirrors Slate's menu stack instead of letting the
        // DX12 viewport erase a popup drawn earlier in the frame.
        renderEngineCommandSurface(ctx, topBarHeight);
    }

    void AceShellUi::renderAquariumFullScreen3DMode(D2DRenderContext& ctx)
    {
        const auto snapshot = aquariumController_.BuildSnapshot();
        aquarium3DPanelState_.detailsVisible = aquariumDetailsPanelVisible_;
        aquarium3DPanelState_.logsVisible = aquariumLogsPanelVisible_;
        environment3DMode_.ClampPanelState(aquarium3DPanelState_, ctx.width, ctx.height);
        const auto layout = environment3DMode_.Compute(ctx.width, ctx.height, aquarium3DPanelState_);
        environment3DMode_.MarkRepaint();
        ++aquariumUiLayoutPassCount_;
        ++aquariumUiRepaintCount_;

        auto toUiRect = [](const ace::aquarium_ui::AceEnvironment3DModeRect& r)
        {
            return am::ui::makeUiRect(r.left, r.top, r.right, r.bottom);
        };

        const UiRect mode = toUiRect(layout.mode);
        const UiRect topbar = toUiRect(layout.topOverlay);
        const UiRect viewportSurface = toUiRect(layout.dx12Surface);
        const UiRect viewportBackground = toUiRect(layout.viewport);
        environmentModalRect_ = mode;
        environmentModalCloseRect_ = toUiRect(layout.closeButton);
        aquariumDetailsToggleRect_ = toUiRect(layout.detailsToggle);
        aquariumLogsToggleRect_ = toUiRect(layout.logsToggle);
        aquariumDebugRect_ = toUiRect(layout.topbarDebugTruth);
        aquariumEngineModeRect_ = makeUiRect(topbar.left + 164.0f, topbar.top + 8.0f, topbar.left + 258.0f, topbar.bottom - 8.0f);
        aquariumLeftPanelRect_ = toUiRect(layout.leftPanel);
        aquariumRightLogsPanelRect_ = toUiRect(layout.rightLogsPanel);
        aquariumLeftResizeHandleRect_ = toUiRect(layout.leftResizeHandle);
        aquariumRightResizeHandleRect_ = toUiRect(layout.rightResizeHandle);
        aquariumLogsFocusRect_ = aquariumRightLogsPanelRect_;
        aquariumLastViewportRect_ = viewportSurface;
        aquariumLastLeftPanelRect_ = aquariumLeftPanelRect_;
        aquariumLastRightPanelRect_ = aquariumRightLogsPanelRect_;
        aquariumOpen3DRect_ = makeUiRect(0, 0, 0, 0);
        aquariumEmbeddedViewportVisible_ = true;
        aquariumContentScroll_.viewport = viewportSurface;

        // ACE-AQ3D6: deterministic 3D mode layout pass. Topbar, left panel,
        // right logs panel, content clips and resize handles are all computed
        // from AceEnvironment3DModeLayout before anything is painted. Hover/click
        // visual state does not structurally rebuild this layout.
        D2DWidgetUtils::fillRect(ctx, mode, ctx.brushes.panelDeep);
        D2DGlassEffects::drawGlassOverlay(ctx, mode, 0.10f);

        // The viewport remains the dominant background. The hosted DX12 child
        // surface is kept stable in dx12Surface so panels are not covered by
        // child-window z-order and the surface is not recreated during idle.
        D2DWidgetUtils::fillRounded(ctx, viewportBackground, 0.0f, ctx.brushes.panelDeep);
        renderAquariumDx12ViewportSurface(ctx, viewportSurface, snapshot.debugTruthEnabled);

        // ACE-UI12: the viewport HUD is a stable layer above the scene but below
        // chrome/popups. It computes its own safe rect instead of being glued to
        // the bottom-left corner and then acting shocked when the log console
        // docks there too.
        renderAquariumViewportHudLayer(ctx, viewportSurface, snapshot);
        renderCameraSpeedControl(ctx, viewportSurface);

        D2DGlassMaterial topGlass;
        topGlass.radius = 0.0f;
        topGlass.fillAlpha = 0.52f;
        topGlass.borderAlpha = 0.34f;
        topGlass.highlightAlpha = 0.14f;
        topGlass.glowAlpha = 0.08f;
        topGlass.shadowAlpha = 0.04f;
        topGlass.blurFallbackAlpha = 0.10f;
        topGlass.useCornerTicks = false;
        D2DGlassEffects::drawGlassPanel(ctx, topbar, topGlass);

        // ACE-AQ3D6: TopbarLayout left/center/right clusters exist and do not overlap.
        renderAquariumMiniButton(ctx, aquariumDetailsToggleRect_, aquariumDetailsPanelVisible_ ? L"Hide Details" : L"Details", aquariumDetailsPanelVisible_);
        renderAquariumMiniButton(ctx, aquariumLogsToggleRect_, aquariumLogsPanelVisible_ ? L"Hide Logs" : L"Logs", aquariumLogsPanelVisible_);
        renderAquariumMiniButton(ctx, aquariumEngineModeRect_, L"Engine", false);
        renderAquariumMiniButton(ctx, aquariumDebugRect_, snapshot.debugTruthEnabled ? L"Truth ON" : L"Truth", snapshot.debugTruthEnabled);
        renderAquariumMiniButton(ctx, environmentModalCloseRect_, L"X", false);

        UiRect statusRect = toUiRect(layout.topbarStatusClip);
        statusRect.left = std::min(statusRect.right, std::max(statusRect.left, aquariumEngineModeRect_.right + 14.0f));
        if (!statusRect.empty())
        {
            if (ctx.target)
            {
                ctx.target->PushAxisAlignedClip(statusRect.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            }
            D2DWidgetUtils::drawTextEx(
                ctx,
                widen(snapshot.scenarioName + " | " + snapshot.plannerName + " | step=" + std::to_string(snapshot.step)),
                FontRole::Small,
                statusRect,
                ctx.brushes.muted,
                DWRITE_TEXT_ALIGNMENT_CENTER,
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER
            );
            if (ctx.target)
            {
                ctx.target->PopAxisAlignedClip();
            }
        }

        if (snapshot.debugTruthEnabled)
        {
            const UiRect warningRect = toUiRect(layout.topbarWarningClip);
            if (!warningRect.empty())
            {
                if (ctx.target)
                {
                    ctx.target->PushAxisAlignedClip(warningRect.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                }
                D2DWidgetUtils::drawTextEx(
                    ctx,
                    L"DEBUG TRUTH - NOT AGENT INPUT",
                    FontRole::Small,
                    warningRect,
                    ctx.brushes.accentWarm,
                    DWRITE_TEXT_ALIGNMENT_TRAILING,
                    DWRITE_PARAGRAPH_ALIGNMENT_CENTER
                );
                if (ctx.target)
                {
                    ctx.target->PopAxisAlignedClip();
                }
            }
        }

        auto renderPanelShell = [&](UiRect rect, const std::wstring& title)
        {
            if (rect.empty())
            {
                return;
            }

            D2DGlassMaterial panelGlass;
            panelGlass.radius = 14.0f;
            panelGlass.fillAlpha = 0.48f;
            panelGlass.borderAlpha = 0.56f;
            panelGlass.highlightAlpha = 0.18f;
            panelGlass.glowAlpha = 0.16f;
            panelGlass.shadowAlpha = 0.08f;
            panelGlass.blurFallbackAlpha = 0.14f;
            panelGlass.useCornerTicks = false;
            D2DGlassEffects::drawGlassPanel(ctx, rect, panelGlass);
            D2DCyberEffects::drawBorderGlow(ctx, rect, 14.0f, 0.14f);
            D2DWidgetUtils::drawTextEx(ctx, title, FontRole::BodyStrong, makeUiRect(rect.left + 14.0f, rect.top + 8.0f, rect.right - 14.0f, rect.top + 34.0f), ctx.brushes.text);
        };

        auto clearLeftControlRects = [&]()
        {
            aquariumScenarioPrevRect_ = makeUiRect(0, 0, 0, 0);
            aquariumScenarioNextRect_ = makeUiRect(0, 0, 0, 0);
            aquariumPlannerPrevRect_ = makeUiRect(0, 0, 0, 0);
            aquariumPlannerNextRect_ = makeUiRect(0, 0, 0, 0);
            aquariumResetRect_ = makeUiRect(0, 0, 0, 0);
            aquariumStepRect_ = makeUiRect(0, 0, 0, 0);
            aquariumRunPauseRect_ = makeUiRect(0, 0, 0, 0);
            aquariumCameraResetRect_ = makeUiRect(0, 0, 0, 0);
            aquariumDetailsDebugRect_ = makeUiRect(0, 0, 0, 0);
            aquariumManualForwardRect_ = makeUiRect(0, 0, 0, 0);
            aquariumManualLeftRect_ = makeUiRect(0, 0, 0, 0);
            aquariumManualRightRect_ = makeUiRect(0, 0, 0, 0);
            aquariumManualWaitRect_ = makeUiRect(0, 0, 0, 0);
            aquariumManualTouchRect_ = makeUiRect(0, 0, 0, 0);
            aquariumManualConsumeRect_ = makeUiRect(0, 0, 0, 0);
            aquariumManualPushRect_ = makeUiRect(0, 0, 0, 0);
        };

        if (aquariumDetailsPanelVisible_ && !aquariumLeftPanelRect_.empty())
        {
            renderPanelShell(aquariumLeftPanelRect_, L"Details / Controls");
            const UiRect contentClip = toUiRect(layout.leftContentClip);
            aquariumScenarioPrevRect_ = toUiRect(layout.scenarioPrev);
            aquariumScenarioNextRect_ = toUiRect(layout.scenarioNext);
            aquariumPlannerPrevRect_ = toUiRect(layout.plannerPrev);
            aquariumPlannerNextRect_ = toUiRect(layout.plannerNext);
            aquariumResetRect_ = toUiRect(layout.reset);
            aquariumStepRect_ = toUiRect(layout.step);
            aquariumRunPauseRect_ = toUiRect(layout.runPause);
            aquariumCameraResetRect_ = toUiRect(layout.cameraReset);
            aquariumDetailsDebugRect_ = toUiRect(layout.debugTruth);
            aquariumManualForwardRect_ = toUiRect(layout.manualForward);
            aquariumManualLeftRect_ = toUiRect(layout.manualLeft);
            aquariumManualRightRect_ = toUiRect(layout.manualRight);
            aquariumManualWaitRect_ = toUiRect(layout.manualWait);
            aquariumManualTouchRect_ = toUiRect(layout.manualTouch);
            aquariumManualConsumeRect_ = toUiRect(layout.manualConsume);
            aquariumManualPushRect_ = toUiRect(layout.manualPush);

            // ACE-AQ3D6: panel_content_clipped_to_content_rect. Sections may be
            // clipped, but they never draw into the resize handle or into each other.
            if (ctx.target && !contentClip.empty())
            {
                ctx.target->PushAxisAlignedClip(contentClip.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            }

            renderAquariumLines(ctx, L"Runtime", {
                "scenario: " + snapshot.scenarioName,
                "planner: " + snapshot.plannerName,
                "step: " + std::to_string(snapshot.step),
                "state: " + std::string(snapshot.running ? "running" : "paused")
            }, toUiRect(layout.leftRuntimeSection), 5);

            const UiRect scenarioSection = toUiRect(layout.leftScenarioPlannerSection);
            renderAquariumLines(ctx, L"Scenario / Planner", {}, scenarioSection, 0);
            renderAquariumMiniButton(ctx, aquariumScenarioPrevRect_, L"<", false);
            renderAquariumMiniButton(ctx, aquariumScenarioNextRect_, L">", false);
            renderAquariumMiniButton(ctx, aquariumPlannerPrevRect_, L"<", false);
            renderAquariumMiniButton(ctx, aquariumPlannerNextRect_, L">", false);

            renderAquariumLines(ctx, L"Main Controls", {}, toUiRect(layout.leftMainControlsSection), 0);
            renderAquariumMiniButton(ctx, aquariumResetRect_, L"Reset", false);
            renderAquariumMiniButton(ctx, aquariumStepRect_, L"Step", false);
            renderAquariumMiniButton(ctx, aquariumRunPauseRect_, snapshot.running ? L"Pause" : L"Run", snapshot.running);
            renderAquariumMiniButton(ctx, aquariumCameraResetRect_, L"Camera Reset", false);
            renderAquariumMiniButton(ctx, aquariumDetailsDebugRect_, L"Debug Truth", snapshot.debugTruthEnabled);

            // ACE-AQ3D6: Manual Actions grid is deterministic and non-overlapping.
            renderAquariumLines(ctx, L"Manual Actions", {}, toUiRect(layout.leftManualActionsSection), 0);
            renderAquariumMiniButton(ctx, aquariumManualForwardRect_, L"Forward", false);
            renderAquariumMiniButton(ctx, aquariumManualLeftRect_, L"Turn L", false);
            renderAquariumMiniButton(ctx, aquariumManualRightRect_, L"Turn R", false);
            renderAquariumMiniButton(ctx, aquariumManualWaitRect_, L"Wait", false);
            renderAquariumMiniButton(ctx, aquariumManualTouchRect_, L"Touch", false);
            renderAquariumMiniButton(ctx, aquariumManualConsumeRect_, L"Consume", false);
            renderAquariumMiniButton(ctx, aquariumManualPushRect_, L"Push", false);

            renderAquariumLines(ctx, L"Inspector", {
                "Body / Agent",
                "Observation",
                "Planner Trace",
                "Counterfactual",
                "Self Model",
                "Delayed / Dynamic",
                "Metrics"
            }, toUiRect(layout.leftInspectorSection), 9);

            if (ctx.target && !contentClip.empty())
            {
                ctx.target->PopAxisAlignedClip();
            }

        }
        else
        {
            clearLeftControlRects();
        }

        if (aquariumLogsPanelVisible_ && !aquariumRightLogsPanelRect_.empty())
        {
            renderPanelShell(aquariumRightLogsPanelRect_, L"Logs / Episodes");
            const UiRect logsBody = toUiRect(layout.rightLogsContent);
            if (ctx.target && !logsBody.empty())
            {
                ctx.target->PushAxisAlignedClip(logsBody.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            }
            renderAquariumScrollableLines(ctx, L"Logs", snapshot.logLines, logsBody, aquarium3DLogsScroll_, 128);
            if (ctx.target && !logsBody.empty())
            {
                ctx.target->PopAxisAlignedClip();
            }
        }
    }


    bool AceShellUi::executeEngineStatsCommand(const std::wstring& commandId)
    {
        const std::wstring id = aceTrimCommand(commandId);
        std::string logLine;

        if (id == L"stat_coords")
        {
            appendEngineCommandOutput(L"STAT_COORDS", formatStatCoords(&logLine), "STAT_COORDS", logLine);
            statusBar_.setText(L"Command executed: stat_coords");
            return true;
        }
        if (id == L"stat_rhi")
        {
            appendEngineCommandOutput(L"STAT_RHI", formatStatRhi(&logLine), "STAT_RHI", logLine);
            statusBar_.setText(L"Command executed: stat_rhi");
            return true;
        }
        if (id == L"stat_fps")
        {
            appendEngineCommandOutput(L"STAT_FPS", formatStatFps(&logLine), "STAT_FPS", logLine);
            statusBar_.setText(L"Command executed: stat_fps");
            return true;
        }
        if (id == L"stat_ui")
        {
            appendEngineCommandOutput(L"STAT_UI", formatStatUi(&logLine), "STAT_UI", logLine);
            statusBar_.setText(L"Command executed: stat_ui");
            return true;
        }
        if (id == L"stat_help" || id == L"help")
        {
            const std::wstring help =
                L"Engine stats commands: stat_coords, stat_rhi, stat_fps, stat_ui, stat_help/help, clear_log. "
                L"Slash form also works: /stat_rhi. These are local-only and are not submitted to the backend.";
            appendEngineCommandOutput(L"STAT_HELP", help, "STAT_HELP", "commands=stat_coords,stat_rhi,stat_fps,stat_ui,stat_help,clear_log slash=enabled backend_submit=false");
            statusBar_.setText(L"Command executed: stat_help");
            return true;
        }
        if (id == L"clear_log")
        {
            std::wstring warning;
            bool ok = true;
            try
            {
                const auto path = AceEngineLogPath();
                std::filesystem::create_directories(path.parent_path());
                std::ofstream file(path, std::ios::trunc);
                ok = static_cast<bool>(file);
                if (!ok)
                {
                    warning = L"Could not truncate Build/Logs/ace_engine.log.";
                }
            }
            catch (...)
            {
                ok = false;
                warning = L"Could not clear Build/Logs/ace_engine.log.";
            }

            const std::wstring body = ok ? L"Build/Logs/ace_engine.log cleared." : warning;
            messageList_.addMessage(makeMessage(ok ? L"Tool" : L"Warning", body, false, false, ok ? ChatMessageKind::Tool : ChatMessageKind::Warning, L"engine log"));
            if (ok)
            {
                AceEngineAppendLog("CLEAR_LOG", "log_cleared=true", nullptr);
            }
            if (engineLogOverlayVisible_)
            {
                refreshEngineLogOverlayLines();
            }
            statusBar_.setText(ok ? L"Command executed: clear_log" : L"clear_log failed");
            return true;
        }

        return false;
    }

    AceEngineStatsSnapshot AceShellUi::buildEngineStatsSnapshot() const
    {
        AceEngineStatsSnapshot snapshot{};
        snapshot.activeRenderPath = aquariumActiveRenderPath_;
        snapshot.backend = aquariumGpuViewportRenderer_ ? L"DX12" : L"unknown";
        snapshot.adapterName = aquariumGpuViewportRenderer_ ? aquariumGpuViewportRenderer_->adapterName() : L"n/a";
        if (snapshot.adapterName.empty()) { snapshot.adapterName = L"n/a"; }
        if (aquariumGpuViewportRenderer_)
        {
            snapshot.viewportStats = aquariumGpuViewportRenderer_->stats();
            snapshot.rhiStats = aquariumGpuViewportRenderer_->gpuStats();
            snapshot.viewportTexture = aquariumGpuViewportRenderer_->viewportTextureResource();
            snapshot.viewportBridge = aquariumGpuViewportRenderer_->viewportBridgeStatus(aquariumActiveRenderPath_ == AceEngineRenderPath::Dx12D2DTextureBridge);
            if (aquariumActiveRenderPath_ == AceEngineRenderPath::Dx12D2DTextureBridge && aquariumBridgeUsesSharedIntermediate_)
            {
                snapshot.viewportBridge.requiredInterop = am::renderer::rhi::AceViewportGpuInteropKind::D3D11On12SharedD3D11Texture;
                snapshot.viewportBridge.fallbackReason = aquariumD2DBridgeSurfaceDiagnostics_.empty() ? "none" : aquariumD2DBridgeSurfaceDiagnostics_;
            }
            if (aquariumActiveRenderPath_ == AceEngineRenderPath::FailedD2DDeviceContext && !aquariumD2DBridgeLastError_.empty())
            {
                snapshot.viewportBridge = am::renderer::rhi::MakeAceViewportD2DDeviceContextFailure(
                    snapshot.viewportTexture,
                    aquariumD2DBridgeFatalStep_.empty() ? aquariumD2DBridgeLastError_ : aquariumD2DBridgeFatalStep_,
                    aquariumD2DBridgeFatalHresult_.empty() ? aquariumD2DBridgeLastError_ : aquariumD2DBridgeFatalHresult_);
            }
            else if (aquariumActiveRenderPath_ != AceEngineRenderPath::Dx12D2DTextureBridge && !aquariumD2DBridgeLastError_.empty())
            {
                snapshot.viewportBridge.fallbackReason = aquariumD2DBridgeLastError_;
            }
        }
        return snapshot;
    }

    std::wstring AceShellUi::formatStatCoords(std::string* logLine) const
    {
        const auto pos = aquariumSingleHwndCamera_.Position();
        const auto forward = aquariumSingleHwndCamera_.Forward();
        const auto right = aquariumSingleHwndCamera_.Right();
        const auto up = aquariumSingleHwndCamera_.Up();
        const auto& world = aquariumController_.Environment().World();
        const auto agent = world.AgentPosition();
        const auto dir = world.AgentDirection();
        const std::wstring path = AceEngineRenderPathToWide(aquariumActiveRenderPath_);
        const std::wstring scenario = aquariumControllerReady_ ? widen(aquariumController_.CurrentScenarioName()) : L"n/a";
        const std::wstring planner = aquariumControllerReady_ ? widen(aquariumController_.CurrentPlannerName()) : L"n/a";

        std::wstringstream ss;
        ss << L"path=" << path
           << L" | camera_pos=" << aceVec3Wide(pos)
           << L" yaw=" << aceFormatDouble(aquariumSingleHwndCamera_.Yaw(), 3)
           << L" pitch=" << aceFormatDouble(aquariumSingleHwndCamera_.Pitch(), 3)
           << L" | forward=" << aceVec3Wide(forward)
           << L" right=" << aceVec3Wide(right)
           << L" up=" << aceVec3Wide(up)
           << L" | speed=" << aceFormatDouble(aquariumSingleHwndCamera_.MoveSpeed(), 2)
           << L" | viewport=" << static_cast<int>(std::round(aquariumEmbeddedViewportRect_.width())) << L"x" << static_cast<int>(std::round(aquariumEmbeddedViewportRect_.height()))
           << L" | scenario=" << scenario
           << L" planner=" << planner
           << L" step=" << (aquariumControllerReady_ ? std::to_wstring(aquariumController_.StepIndex()) : L"n/a")
           << L" | agent=(" << agent.x << L", " << agent.y << L") dir=" << widen(ace::aquarium::ToString(dir));

        if (logLine)
        {
            std::ostringstream os;
            os << "path=" << AceEngineRenderPathToUtf8(aquariumActiveRenderPath_)
               << " camera=" << aceVec3Utf8(pos)
               << " yaw=" << aceFormatDoubleUtf8(aquariumSingleHwndCamera_.Yaw(), 3)
               << " pitch=" << aceFormatDoubleUtf8(aquariumSingleHwndCamera_.Pitch(), 3)
               << " viewport=" << static_cast<int>(std::round(aquariumEmbeddedViewportRect_.width())) << "x" << static_cast<int>(std::round(aquariumEmbeddedViewportRect_.height()))
               << " scenario=" << aquariumController_.CurrentScenarioName()
               << " planner=" << aquariumController_.CurrentPlannerName()
               << " step=" << aquariumController_.StepIndex()
               << " agent=(" << agent.x << "," << agent.y << ") dir=" << ace::aquarium::ToString(dir);
            *logLine = os.str();
        }
        return ss.str();
    }

    std::wstring AceShellUi::formatStatRhi(std::string* logLine) const
    {
        const auto snapshot = buildEngineStatsSnapshot();
        const auto& v = snapshot.viewportStats;
        const auto& r = snapshot.rhiStats;
        const bool gpuTextOverlayActive =
            snapshot.activeRenderPath == AceEngineRenderPath::Dx12GpuComposited &&
            r.gpuOverlayBakedFrames > 0;
        const bool d2dCompositionHudActive =
            snapshot.activeRenderPath == AceEngineRenderPath::Dx12GpuComposited &&
            aquariumD2DCompositionHudAttached_;
        const bool readbackActive = !(snapshot.activeRenderPath == AceEngineRenderPath::Dx12GpuComposited ||
                                      snapshot.activeRenderPath == AceEngineRenderPath::Dx12ZeroCopy ||
                                      snapshot.activeRenderPath == AceEngineRenderPath::Dx12CachedReadback ||
                                      snapshot.activeRenderPath == AceEngineRenderPath::Dx12D2DTextureBridge ||
                                      snapshot.activeRenderPath == AceEngineRenderPath::FailedD2DDeviceContext);

        std::wstringstream ss;
        ss << L"[STAT_RHI]\n"
           << L"  path: " << AceEngineRenderPathToWide(snapshot.activeRenderPath) << L"\n"
           << L"  backend: " << snapshot.backend << L"\n"
           << L"  adapter: " << snapshot.adapterName << L"\n"
           << L"  viewport_mode: " << AceEngineRenderPathToWide(snapshot.activeRenderPath) << L"\n"
           << L"  ui_layer: " << (d2dCompositionHudActive ? L"DX12_SCENE+D2D_COMPOSITION_HUD" : (gpuTextOverlayActive ? L"DX12_VIEWPORT_GPU_OVERLAY+D2D_CHROME" : L"D2D_RETAINED_OVERLAY")) << L"\n"
           << L"  quality: preserved\n"
           << L"  gpu_text_overlay: " << (gpuTextOverlayActive ? L"true" : L"false") << L"\n"
           << L"  viewport_texture_resource: " << widen(am::renderer::rhi::AceViewportTextureResourceKindToString(snapshot.viewportTexture.kind)) << L"\n"
           << L"  viewport_texture_bridge: " << widen(am::renderer::rhi::AceViewportTextureBridgeModeToString(snapshot.viewportBridge.mode)) << L"\n"
           << L"  viewport_ui_renderer: " << widen(am::renderer::rhi::AceViewportUiRendererKindToString(snapshot.viewportBridge.uiRenderer)) << L"\n"
           << L"  viewport_required_ui_renderer: " << widen(am::renderer::rhi::AceViewportUiRendererKindToString(snapshot.viewportBridge.requiredUiRenderer)) << L"\n"
           << L"  viewport_required_interop: " << widen(am::renderer::rhi::AceViewportGpuInteropKindToString(snapshot.viewportBridge.requiredInterop)) << L"\n"
           << L"  viewport_bridge_fallback_reason: " << widen(snapshot.viewportBridge.fallbackReason) << L"\n"
           << L"  legacy_fallback: " << (snapshot.viewportBridge.legacyFallbackUsed ? L"true" : L"false") << L"\n"
           << L"  fatal_bridge_step: " << widen(snapshot.viewportBridge.fatalBridgeStep.empty() ? std::string("none") : snapshot.viewportBridge.fatalBridgeStep) << L"\n"
           << L"  fatal_bridge_hresult: " << widen(snapshot.viewportBridge.fatalBridgeHresult.empty() ? std::string("none") : snapshot.viewportBridge.fatalBridgeHresult) << L"\n"
           << L"  readback_active: " << (readbackActive ? L"true" : L"false") << L"\n"
           << L"  viewportCacheHits: " << aquariumViewportCacheHitCount_ << L"\n"
           << L"  viewportCacheMisses: " << aquariumViewportCacheMissCount_ << L"\n"
           << L"  zeroCopyFrames: " << r.zeroCopyFrames << L" / " << v.zeroCopyFrames << L"\n"
           << L"  readbackFrames: " << r.readbackFrames << L" / " << v.readbackFrames << L"\n"
           << L"  combinedReadbackFrames: " << r.combinedRenderReadbackFrames << L" / " << v.combinedReadbackFrames << L"\n"
           << L"  gpuCompositedFrames: " << r.gpuCompositedFrames << L" / " << v.gpuCompositedFrames << L"\n"
           << L"  combinedGpuCompositionFrames: " << r.combinedGpuCompositionFrames << L"\n"
           << L"  d2dTextureBridgeFrames: " << r.d2dTextureBridgeFrames << L" / " << v.d2dTextureBridgeFrames << L"\n"
           << L"  d2dTextureBridgeAttempts: " << aquariumD2DBridgeAttemptCount_ << L"\n"
           << L"  d2dTextureBridgeSuccesses: " << aquariumD2DBridgeSuccessCount_ << L"\n"
           << L"  d2dTextureBridgeDirectSuccesses: " << aquariumD2DBridgeDirectSuccessCount_ << L"\n"
           << L"  d2dTextureBridgeSharedSuccesses: " << aquariumD2DBridgeSharedSuccessCount_ << L"\n"
           << L"  d2dSharedBufferCount: " << aquariumD2DBridgeSharedBufferCount_ << L"\n"
           << L"  d2dSharedWriteIndex: " << aquariumD2DBridgeSharedWriteIndexStat_ << L"\n"
           << L"  d2dSharedDrawIndex: " << aquariumD2DBridgeSharedDrawIndexStat_ << L"\n"
           << L"  d2dSharedBitmapRecreates: " << aquariumD2DBridgeSharedBitmapRecreateCount_ << L"\n"
           << L"  d2dSharedCopies: " << aquariumD2DBridgeSharedCopyCount_ << L"\n"
           << L"  d2dSharedMutexAcquires: " << aquariumD2DBridgeSharedMutexAcquireCount_ << L"\n"
           << L"  d2dSharedMutexContentions: " << aquariumD2DBridgeSharedMutexContentionCount_ << L"\n"
           << L"  d2dSharedD2DFlushes: " << aquariumD2DBridgeSharedD2DFlushCount_ << L"\n"
           << L"  d2dTextureBridgeSurfaceDiagnostics: " << widen(aquariumD2DBridgeSurfaceDiagnostics_.empty() ? std::string("none") : aquariumD2DBridgeSurfaceDiagnostics_) << L"\n"
           << L"  d2dFrameCompositor: " << d2dFrameCompositor_.WideDiagnostics() << L"\n"
           << L"  d2dFrameDiagnostics: " << d2dFrameDiagnostics_.WideDiagnostics() << L"\n"
           << L"  d2dPresentScheduler: " << d2dPresentScheduler_.WideDiagnostics() << L"\n"
           << L"  d2dViewportRuntime: " << d2dViewportBridgeRuntime_.WideDiagnostics() << L"\n"
           << L"  d2dViewportCopyScheduler: " << d2dViewportCopyScheduler_.WideDiagnostics() << L"\n"
           << L"  d2dViewportTextureCache: " << d2dViewportTextureCache_.WideDiagnostics() << L"\n"
           << L"  slateRendererPipeline: " << slateRendererPipeline_.WideDiagnostics() << L"\n"
           << L"  slateLayerTree: " << slateLayerTree_.WideDiagnostics() << L"\n"
           << L"  slatePaintJournal: " << slatePaintJournal_.WideDiagnostics() << L"\n"
           << L"  d2dCompositorAudit: " << d2dCompositorAudit_.WideDiagnostics() << L"\n"
           << L"  aquariumViewportInput: " << aquariumViewportInputDiagnosticsWide() << L"\n"
           << L"  gpuOverlayBakedFrames: " << r.gpuOverlayBakedFrames << L" / " << v.gpuOverlayBakedFrames << L"\n"
           << L"  gpuOverlayVertices: " << r.gpuOverlayVertices << L" / " << v.gpuOverlayVertexCount << L"\n"
           << L"  compositionFrames: " << r.compositionFrames << L"\n"
           << L"  compositionResizes: " << r.compositionResizes << L"\n"
           << L"  compositionPresentSkips: " << r.compositionPresentSkips << L"\n"
           << L"  compositionPacingSkips: " << r.compositionPacingSkips << L"\n"
           << L"  compositionRenderOnlyFrames: " << r.compositionRenderOnlyFrames << L"\n"
           << L"  renderThroughputFps: " << aceFormatDouble(r.compositionRenderIntervalAvgMs > 0.0 ? 1000.0 / r.compositionRenderIntervalAvgMs : 0.0) << L"\n"
           << L"  renderIntervalMs(last/avg/max): " << aceFormatDouble(r.compositionRenderIntervalLastMs) << L" / " << aceFormatDouble(r.compositionRenderIntervalAvgMs) << L" / " << aceFormatDouble(r.compositionRenderIntervalMaxMs) << L"\n"
           << L"  presentedFps: " << aceFormatDouble(r.compositionPresentIntervalAvgMs > 0.0 ? 1000.0 / r.compositionPresentIntervalAvgMs : 0.0) << L"\n"
           << L"  presentIntervalMs(last/avg/max): " << aceFormatDouble(r.compositionPresentIntervalLastMs) << L" / " << aceFormatDouble(r.compositionPresentIntervalAvgMs) << L" / " << aceFormatDouble(r.compositionPresentIntervalMaxMs) << L"\n"
           << L"  d2dCompositionHud: attached=" << (aquariumD2DCompositionHudAttached_ ? L"true" : L"false")
           << L" draws=" << aquariumD2DCompositionHudDrawCount_
           << L" presents=" << aquariumD2DCompositionHudPresentCount_ << L"\n"
           << L"  targetResizes: " << v.targetResizes << L"\n"
           << L"  framesRendered: " << v.framesRendered << L"\n"
           << L"  compositionPresentedFrames: " << v.compositionPresentedFrames << L"\n"
           << L"  geometryUploadFrames: " << v.geometryUploadFrames << L"\n"
           << L"  geometryReuseFrames: " << v.geometryReuseFrames << L"\n"
           << L"  lastPrimitiveCount: " << v.lastPrimitiveCount << L"\n"
           << L"  lastVertexCount: " << v.lastVertexCount << L"\n"
           << L"  lastExtent: " << v.lastExtent.width << L"x" << v.lastExtent.height << L"\n"
           << L"  uploadBytesAllocated: " << r.uploadBytesAllocated << L"\n"
           << L"  uploadAllocations: " << r.uploadAllocations << L"\n"
           << L"  mappedUploadBytes: " << r.mappedUploadBytes << L"\n"
           << L"  mappedUploadUpdates: " << r.mappedUploadUpdates << L"\n"
           << L"  nativeBuffers: " << r.nativeBuffers << L"\n"
           << L"  nativeTextures: " << r.nativeTextures << L"\n"
           << L"  nativePipelines: " << r.nativePipelines << L"\n"
           << L"  compiledShaders: " << r.compiledShaders << L"\n"
           << L"  descriptorAllocations: " << r.descriptorAllocations << L"\n"
           << L"  drawCallsExecuted: " << r.drawCallsExecuted << L"\n"
           << L"  submittedGpuCommandLists: " << r.submittedGpuCommandLists << L"\n"
           << L"  completedFenceValue: " << r.completedFenceValue << L"\n"
           << L"  blockingFenceWaits: " << r.blockingFenceWaits << L"\n"
           << L"  blockingFenceWaitMs: " << aceFormatDouble(r.blockingFenceWaitMs) << L"\n"
           << L"  readbackBytes: " << r.readbackBytes << L"\n"
           << L"  combinedRenderReadbackBytes: " << r.combinedRenderReadbackBytes << L"\n"
           << L"  readbackBufferReuses: " << r.readbackBufferReuses << L"\n"
           << L"  readbackBufferResizes: " << r.readbackBufferResizes << L"\n"
           << L"  offscreenSceneTargets: " << r.offscreenSceneTargets << L"\n"
           << L"  wvpConstantsUploaded: " << r.wvpConstantsUploaded;

        if (logLine)
        {
            std::ostringstream os;
            os << "path=" << AceEngineRenderPathToUtf8(snapshot.activeRenderPath)
               << " backend=" << (aquariumGpuViewportRenderer_ ? "DX12" : "unknown")
               << " viewport_mode=" << AceEngineRenderPathToUtf8(snapshot.activeRenderPath)
               << " ui_layer=" << (d2dCompositionHudActive ? "DX12_SCENE+D2D_COMPOSITION_HUD" : (gpuTextOverlayActive ? "DX12_VIEWPORT_GPU_OVERLAY+D2D_CHROME" : "D2D_RETAINED_OVERLAY"))
               << " quality=preserved gpu_text_overlay=" << (gpuTextOverlayActive ? "true" : "false")
               << " viewport_texture_resource=" << am::renderer::rhi::AceViewportTextureResourceKindToString(snapshot.viewportTexture.kind)
               << " viewport_texture_bridge=" << am::renderer::rhi::AceViewportTextureBridgeModeToString(snapshot.viewportBridge.mode)
               << " viewport_ui_renderer=" << am::renderer::rhi::AceViewportUiRendererKindToString(snapshot.viewportBridge.uiRenderer)
               << " viewport_required_ui_renderer=" << am::renderer::rhi::AceViewportUiRendererKindToString(snapshot.viewportBridge.requiredUiRenderer)
               << " viewport_required_interop=" << am::renderer::rhi::AceViewportGpuInteropKindToString(snapshot.viewportBridge.requiredInterop)
               << " viewport_bridge_fallback_reason=" << snapshot.viewportBridge.fallbackReason
               << " legacy_fallback=" << (snapshot.viewportBridge.legacyFallbackUsed ? "true" : "false")
               << " fatal_bridge_step=" << (snapshot.viewportBridge.fatalBridgeStep.empty() ? "none" : snapshot.viewportBridge.fatalBridgeStep)
               << " fatal_bridge_hresult=" << (snapshot.viewportBridge.fatalBridgeHresult.empty() ? "none" : snapshot.viewportBridge.fatalBridgeHresult)
               << " viewportTextureExports=" << v.viewportTextureExports
               << " viewportBridgeReadbackFallbacks=" << v.viewportBridgeReadbackFallbacks
               << " readback_active=" << (readbackActive ? "true" : "false")
               << " viewportCacheHits=" << aquariumViewportCacheHitCount_
               << " viewportCacheMisses=" << aquariumViewportCacheMissCount_
               << " gpuComposited=" << r.gpuCompositedFrames << "/" << v.gpuCompositedFrames
               << " gpuOverlayBaked=" << r.gpuOverlayBakedFrames << "/" << v.gpuOverlayBakedFrames
               << " gpuOverlayVerts=" << r.gpuOverlayVertices << "/" << v.gpuOverlayVertexCount
               << " combinedReadback=" << r.combinedRenderReadbackFrames << "/" << v.combinedReadbackFrames
               << " blockingFenceWaits=" << r.blockingFenceWaits
               << " blockingFenceWaitMs=" << aceFormatDoubleUtf8(r.blockingFenceWaitMs)
               << " frames=" << v.framesRendered
               << " compositionPresentedFrames=" << v.compositionPresentedFrames
               << " geometryUploadFrames=" << v.geometryUploadFrames
               << " geometryReuseFrames=" << v.geometryReuseFrames
               << " zeroCopy=" << v.zeroCopyFrames
               << " readback=" << v.readbackFrames
               << " compositionFrames=" << r.compositionFrames
               << " compositionRenderOnlyFrames=" << r.compositionRenderOnlyFrames
               << " renderThroughputFps=" << aceFormatDoubleUtf8(r.compositionRenderIntervalAvgMs > 0.0 ? 1000.0 / r.compositionRenderIntervalAvgMs : 0.0)
               << " renderIntervalMs=" << aceFormatDoubleUtf8(r.compositionRenderIntervalLastMs) << "/" << aceFormatDoubleUtf8(r.compositionRenderIntervalAvgMs) << "/" << aceFormatDoubleUtf8(r.compositionRenderIntervalMaxMs)
               << " presentedFps=" << aceFormatDoubleUtf8(r.compositionPresentIntervalAvgMs > 0.0 ? 1000.0 / r.compositionPresentIntervalAvgMs : 0.0)
               << " presentIntervalMs=" << aceFormatDoubleUtf8(r.compositionPresentIntervalLastMs) << "/" << aceFormatDoubleUtf8(r.compositionPresentIntervalAvgMs) << "/" << aceFormatDoubleUtf8(r.compositionPresentIntervalMaxMs)
               << " compositionResizes=" << r.compositionResizes
               << " targetResizes=" << v.targetResizes
               << " primitives=" << v.lastPrimitiveCount
               << " verts=" << v.lastVertexCount
               << " extent=" << v.lastExtent.width << "x" << v.lastExtent.height
               << " uploadBytesAllocated=" << r.uploadBytesAllocated
               << " uploadAllocations=" << r.uploadAllocations
               << " nativeBuffers=" << r.nativeBuffers
               << " nativeTextures=" << r.nativeTextures
               << " nativePipelines=" << r.nativePipelines
               << " compiledShaders=" << r.compiledShaders
               << " descriptorAllocations=" << r.descriptorAllocations
               << " drawCallsExecuted=" << r.drawCallsExecuted
               << " submittedGpuCommandLists=" << r.submittedGpuCommandLists
               << " completedFenceValue=" << r.completedFenceValue
               << " readbackBytes=" << r.readbackBytes
               << " offscreenSceneTargets=" << r.offscreenSceneTargets
               << " wvpConstantsUploaded=" << r.wvpConstantsUploaded
               << " mappedUploadBytes=" << r.mappedUploadBytes
               << " mappedUploadUpdates=" << r.mappedUploadUpdates
               << " readbackBufferReuses=" << r.readbackBufferReuses
               << " readbackBufferResizes=" << r.readbackBufferResizes
               << " combinedRenderReadbackBytes=" << r.combinedRenderReadbackBytes
               << " gpuCompositedFrames=" << r.gpuCompositedFrames
               << " combinedGpuCompositionFrames=" << r.combinedGpuCompositionFrames
               << " d2dTextureBridgeFrames=" << r.d2dTextureBridgeFrames << "/" << v.d2dTextureBridgeFrames
               << " d2dTextureBridgeAttempts=" << aquariumD2DBridgeAttemptCount_
               << " d2dTextureBridgeSuccesses=" << aquariumD2DBridgeSuccessCount_
               << " d2dTextureBridgeDirectSuccesses=" << aquariumD2DBridgeDirectSuccessCount_
               << " d2dTextureBridgeSharedSuccesses=" << aquariumD2DBridgeSharedSuccessCount_
               << " d2dSharedBufferCount=" << aquariumD2DBridgeSharedBufferCount_
               << " d2dSharedWriteIndex=" << aquariumD2DBridgeSharedWriteIndexStat_
               << " d2dSharedDrawIndex=" << aquariumD2DBridgeSharedDrawIndexStat_
               << " d2dSharedBitmapRecreates=" << aquariumD2DBridgeSharedBitmapRecreateCount_
               << " d2dSharedCopies=" << aquariumD2DBridgeSharedCopyCount_
               << " d2dSharedMutexAcquires=" << aquariumD2DBridgeSharedMutexAcquireCount_
               << " d2dSharedMutexContentions=" << aquariumD2DBridgeSharedMutexContentionCount_
               << " d2dSharedD2DFlushes=" << aquariumD2DBridgeSharedD2DFlushCount_
               << " d2dTextureBridgeSurfaceDiagnostics=" << (aquariumD2DBridgeSurfaceDiagnostics_.empty() ? "none" : aquariumD2DBridgeSurfaceDiagnostics_)
               << " d2dFrameCompositor=" << d2dFrameCompositor_.Diagnostics()
               << " d2dFrameDiagnostics=" << d2dFrameDiagnostics_.Diagnostics()
               << " d2dPresentScheduler=" << d2dPresentScheduler_.Diagnostics()
               << " d2dViewportRuntime=" << d2dViewportBridgeRuntime_.Diagnostics()
               << " d2dViewportCopyScheduler=" << d2dViewportCopyScheduler_.Diagnostics()
               << " d2dViewportTextureCache=" << d2dViewportTextureCache_.Diagnostics()
               << " slateRendererPipeline=" << slateRendererPipeline_.Diagnostics()
               << " d2dCompositorAudit=" << d2dCompositorAudit_.Diagnostics()
               << " aquariumViewportInput=" << aquariumViewportInputDiagnostics()
               << " gpuOverlayBakedFrames=" << r.gpuOverlayBakedFrames
               << " gpuOverlayVertices=" << r.gpuOverlayVertices
               << " compositionPresentSkips=" << r.compositionPresentSkips
               << " compositionPacingSkips=" << r.compositionPacingSkips
               << " d2dCompositionHudAttached=" << (aquariumD2DCompositionHudAttached_ ? "true" : "false")
               << " d2dCompositionHudDraws=" << aquariumD2DCompositionHudDrawCount_
               << " d2dCompositionHudPresents=" << aquariumD2DCompositionHudPresentCount_;
            *logLine = os.str();
        }
        return ss.str();
    }

    std::wstring AceShellUi::formatStatFps(std::string* logLine) const
    {
        const auto s = enginePerfStats_.Summary();
        const auto snapshot = buildEngineStatsSnapshot();
        const auto& r = snapshot.rhiStats;
        const bool gpuTextOverlayActive =
            snapshot.activeRenderPath == AceEngineRenderPath::Dx12GpuComposited &&
            r.gpuOverlayBakedFrames > 0;
        const bool d2dCompositionHudActive =
            snapshot.activeRenderPath == AceEngineRenderPath::Dx12GpuComposited &&
            aquariumD2DCompositionHudAttached_;
        const bool readbackActive = !(snapshot.activeRenderPath == AceEngineRenderPath::Dx12GpuComposited ||
                                      snapshot.activeRenderPath == AceEngineRenderPath::Dx12ZeroCopy ||
                                      snapshot.activeRenderPath == AceEngineRenderPath::Dx12CachedReadback ||
                                      snapshot.activeRenderPath == AceEngineRenderPath::Dx12D2DTextureBridge ||
                                      snapshot.activeRenderPath == AceEngineRenderPath::FailedD2DDeviceContext);

        std::wstringstream ss;
        ss << L"[STAT_FPS]\n"
           << L"  samples: " << s.count << L" / " << AceEnginePerfStats::kMaxSamples << L"\n"
           << L"  fps.last: " << aceFormatDouble(s.fpsLast) << L"\n"
           << L"  fps.avg: " << aceFormatDouble(s.fpsAvg) << L"\n"
           << L"  fps.min: " << aceFormatDouble(s.fpsMin) << L"\n"
           << L"  fps.max: " << aceFormatDouble(s.fpsMax) << L"\n"
           << L"  frame_ms.last: " << aceFormatDouble(s.frameMsLast) << L"\n"
           << L"  frame_ms.avg: " << aceFormatDouble(s.frameMsAvg) << L"\n"
           << L"  frame_ms.min: " << aceFormatDouble(s.frameMsMin) << L"\n"
           << L"  frame_ms.max: " << aceFormatDouble(s.frameMsMax) << L"\n"
           << L"  ui_ms: " << aceFormatDouble(s.uiMsLast) << L"\n"
           << L"  layout_ms: " << aceFormatDouble(s.layoutMsLast) << L"\n"
           << L"  aquarium_build_ms: " << aceFormatDouble(s.aquariumBuildMsLast) << L"\n"
           << L"  rhi_render_ms: " << aceFormatDouble(s.rhiRenderMsLast) << L"\n"
           << L"  present_or_composite_ms: " << aceFormatDouble(s.presentOrCompositeMsLast) << L"\n"
           << L"  fallback_path: " << AceEngineRenderPathToWide(s.fallbackPath) << L"\n"
           << L"  viewport_mode: " << AceEngineRenderPathToWide(snapshot.activeRenderPath) << L"\n"
           << L"  ui_layer: " << (d2dCompositionHudActive ? L"DX12_SCENE+D2D_COMPOSITION_HUD" : (gpuTextOverlayActive ? L"DX12_VIEWPORT_GPU_OVERLAY+D2D_CHROME" : L"D2D_RETAINED_OVERLAY")) << L"\n"
           << L"  quality: preserved\n"
           << L"  gpu_text_overlay: " << (gpuTextOverlayActive ? L"true" : L"false") << L"\n"
           << L"  viewport_texture_resource: " << widen(am::renderer::rhi::AceViewportTextureResourceKindToString(snapshot.viewportTexture.kind)) << L"\n"
           << L"  viewport_texture_bridge: " << widen(am::renderer::rhi::AceViewportTextureBridgeModeToString(snapshot.viewportBridge.mode)) << L"\n"
           << L"  viewport_ui_renderer: " << widen(am::renderer::rhi::AceViewportUiRendererKindToString(snapshot.viewportBridge.uiRenderer)) << L"\n"
           << L"  viewport_required_ui_renderer: " << widen(am::renderer::rhi::AceViewportUiRendererKindToString(snapshot.viewportBridge.requiredUiRenderer)) << L"\n"
           << L"  viewport_required_interop: " << widen(am::renderer::rhi::AceViewportGpuInteropKindToString(snapshot.viewportBridge.requiredInterop)) << L"\n"
           << L"  viewport_bridge_fallback_reason: " << widen(snapshot.viewportBridge.fallbackReason) << L"\n"
           << L"  legacy_fallback: " << (snapshot.viewportBridge.legacyFallbackUsed ? L"true" : L"false") << L"\n"
           << L"  fatal_bridge_step: " << widen(snapshot.viewportBridge.fatalBridgeStep.empty() ? std::string("none") : snapshot.viewportBridge.fatalBridgeStep) << L"\n"
           << L"  fatal_bridge_hresult: " << widen(snapshot.viewportBridge.fatalBridgeHresult.empty() ? std::string("none") : snapshot.viewportBridge.fatalBridgeHresult) << L"\n"
           << L"  readback_active: " << (readbackActive ? L"true" : L"false") << L"\n"
           << L"  viewport_cache_hits: " << aquariumViewportCacheHitCount_ << L"\n"
           << L"  viewport_cache_misses: " << aquariumViewportCacheMissCount_ << L"\n"
           << L"  gpu_composited_frames: " << r.gpuCompositedFrames << L"\n"
           << L"  combined_gpu_composition_frames: " << r.combinedGpuCompositionFrames << L"\n"
           << L"  d2d_texture_bridge_frames: " << r.d2dTextureBridgeFrames << L" / " << snapshot.viewportStats.d2dTextureBridgeFrames << L"\n"
           << L"  d2d_texture_bridge_attempts: " << aquariumD2DBridgeAttemptCount_ << L"\n"
           << L"  d2d_texture_bridge_successes: " << aquariumD2DBridgeSuccessCount_ << L"\n"
           << L"  d2d_texture_bridge_direct_successes: " << aquariumD2DBridgeDirectSuccessCount_ << L"\n"
           << L"  d2d_texture_bridge_shared_successes: " << aquariumD2DBridgeSharedSuccessCount_ << L"\n"
           << L"  d2d_shared_buffer_count: " << aquariumD2DBridgeSharedBufferCount_ << L"\n"
           << L"  d2d_shared_write_index: " << aquariumD2DBridgeSharedWriteIndexStat_ << L"\n"
           << L"  d2d_shared_draw_index: " << aquariumD2DBridgeSharedDrawIndexStat_ << L"\n"
           << L"  d2d_shared_bitmap_recreates: " << aquariumD2DBridgeSharedBitmapRecreateCount_ << L"\n"
           << L"  d2d_shared_copies: " << aquariumD2DBridgeSharedCopyCount_ << L"\n"
           << L"  d2d_shared_mutex_acquires: " << aquariumD2DBridgeSharedMutexAcquireCount_ << L"\n"
           << L"  d2d_shared_mutex_contentions: " << aquariumD2DBridgeSharedMutexContentionCount_ << L"\n"
           << L"  d2d_shared_d2d_flushes: " << aquariumD2DBridgeSharedD2DFlushCount_ << L"\n"
           << L"  d2d_texture_bridge_surface_diagnostics: " << widen(aquariumD2DBridgeSurfaceDiagnostics_.empty() ? std::string("none") : aquariumD2DBridgeSurfaceDiagnostics_) << L"\n"
           << L"  d2d_frame_compositor: " << d2dFrameCompositor_.WideDiagnostics() << L"\n"
           << L"  d2d_frame_diagnostics: " << d2dFrameDiagnostics_.WideDiagnostics() << L"\n"
           << L"  d2d_present_scheduler: " << d2dPresentScheduler_.WideDiagnostics() << L"\n"
           << L"  d2d_viewport_runtime: " << d2dViewportBridgeRuntime_.WideDiagnostics() << L"\n"
           << L"  d2d_viewport_copy_scheduler: " << d2dViewportCopyScheduler_.WideDiagnostics() << L"\n"
           << L"  d2d_viewport_texture_cache: " << d2dViewportTextureCache_.WideDiagnostics() << L"\n"
           << L"  slate_renderer_pipeline: " << slateRendererPipeline_.WideDiagnostics() << L"\n"
           << L"  slate_layer_tree: " << slateLayerTree_.WideDiagnostics() << L"\n"
           << L"  slate_paint_journal: " << slatePaintJournal_.WideDiagnostics() << L"\n"
           << L"  d2d_compositor_audit: " << d2dCompositorAudit_.WideDiagnostics() << L"\n"
           << L"  aquarium_viewport_input: " << aquariumViewportInputDiagnosticsWide() << L"\n"
           << L"  gpu_overlay_baked: " << r.gpuOverlayBakedFrames << L"\n"
           << L"  composition_present_skips: " << r.compositionPresentSkips << L"\n"
           << L"  composition_pacing_skips: " << r.compositionPacingSkips << L"\n"
           << L"  composition_render_only_frames: " << r.compositionRenderOnlyFrames << L"\n"
           << L"  render_throughput_fps: " << aceFormatDouble(r.compositionRenderIntervalAvgMs > 0.0 ? 1000.0 / r.compositionRenderIntervalAvgMs : 0.0) << L"\n"
           << L"  render_interval_ms_last_avg_max: " << aceFormatDouble(r.compositionRenderIntervalLastMs) << L" / " << aceFormatDouble(r.compositionRenderIntervalAvgMs) << L" / " << aceFormatDouble(r.compositionRenderIntervalMaxMs) << L"\n"
           << L"  presented_fps: " << aceFormatDouble(r.compositionPresentIntervalAvgMs > 0.0 ? 1000.0 / r.compositionPresentIntervalAvgMs : 0.0) << L"\n"
           << L"  present_interval_ms_last_avg_max: " << aceFormatDouble(r.compositionPresentIntervalLastMs) << L" / " << aceFormatDouble(r.compositionPresentIntervalAvgMs) << L" / " << aceFormatDouble(r.compositionPresentIntervalMaxMs) << L"\n"
           << L"  geometry_upload_frames: " << snapshot.viewportStats.geometryUploadFrames << L"\n"
           << L"  geometry_reuse_frames: " << snapshot.viewportStats.geometryReuseFrames << L"\n"
           << L"  d2d_composition_hud: attached=" << (aquariumD2DCompositionHudAttached_ ? L"true" : L"false")
           << L" draws=" << aquariumD2DCompositionHudDrawCount_
           << L" presents=" << aquariumD2DCompositionHudPresentCount_ << L"\n"
           << L"  combined_readback_frames: " << r.combinedRenderReadbackFrames << L"\n"
           << L"  readback_bytes: " << r.readbackBytes << L"\n"
           << L"  fence_waits: " << r.blockingFenceWaits << L"\n"
           << L"  fence_wait_ms: " << aceFormatDouble(r.blockingFenceWaitMs);

        if (logLine)
        {
            std::ostringstream os;
            os << "samples=" << s.count
               << " fps_last=" << aceFormatDoubleUtf8(s.fpsLast)
               << " fps_avg=" << aceFormatDoubleUtf8(s.fpsAvg)
               << " fps_min=" << aceFormatDoubleUtf8(s.fpsMin)
               << " fps_max=" << aceFormatDoubleUtf8(s.fpsMax)
               << " frame_last_ms=" << aceFormatDoubleUtf8(s.frameMsLast)
               << " frame_avg_ms=" << aceFormatDoubleUtf8(s.frameMsAvg)
               << " frame_min_ms=" << aceFormatDoubleUtf8(s.frameMsMin)
               << " frame_max_ms=" << aceFormatDoubleUtf8(s.frameMsMax)
               << " ui_ms=" << aceFormatDoubleUtf8(s.uiMsLast)
               << " layout_ms=" << aceFormatDoubleUtf8(s.layoutMsLast)
               << " aquarium_build_ms=" << aceFormatDoubleUtf8(s.aquariumBuildMsLast)
               << " rhi_render_ms=" << aceFormatDoubleUtf8(s.rhiRenderMsLast)
               << " present_or_composite_ms=" << aceFormatDoubleUtf8(s.presentOrCompositeMsLast)
               << " fallback_path=" << AceEngineRenderPathToUtf8(s.fallbackPath)
               << " viewport_mode=" << AceEngineRenderPathToUtf8(snapshot.activeRenderPath)
               << " ui_layer=" << (d2dCompositionHudActive ? "DX12_SCENE+D2D_COMPOSITION_HUD" : (gpuTextOverlayActive ? "DX12_VIEWPORT_GPU_OVERLAY+D2D_CHROME" : "D2D_RETAINED_OVERLAY"))
               << " quality=preserved gpu_text_overlay=" << (gpuTextOverlayActive ? "true" : "false")
               << " viewport_texture_resource=" << am::renderer::rhi::AceViewportTextureResourceKindToString(snapshot.viewportTexture.kind)
               << " viewport_texture_bridge=" << am::renderer::rhi::AceViewportTextureBridgeModeToString(snapshot.viewportBridge.mode)
               << " viewport_ui_renderer=" << am::renderer::rhi::AceViewportUiRendererKindToString(snapshot.viewportBridge.uiRenderer)
               << " viewport_required_ui_renderer=" << am::renderer::rhi::AceViewportUiRendererKindToString(snapshot.viewportBridge.requiredUiRenderer)
               << " viewport_required_interop=" << am::renderer::rhi::AceViewportGpuInteropKindToString(snapshot.viewportBridge.requiredInterop)
               << " viewport_bridge_fallback_reason=" << snapshot.viewportBridge.fallbackReason
               << " legacy_fallback=" << (snapshot.viewportBridge.legacyFallbackUsed ? "true" : "false")
               << " fatal_bridge_step=" << (snapshot.viewportBridge.fatalBridgeStep.empty() ? "none" : snapshot.viewportBridge.fatalBridgeStep)
               << " fatal_bridge_hresult=" << (snapshot.viewportBridge.fatalBridgeHresult.empty() ? "none" : snapshot.viewportBridge.fatalBridgeHresult)
               << " readback_active=" << (readbackActive ? "true" : "false")
               << " viewport_cache_hits=" << aquariumViewportCacheHitCount_
               << " viewport_cache_misses=" << aquariumViewportCacheMissCount_
               << " gpu_composited_frames=" << r.gpuCompositedFrames
               << " combined_gpu_composition_frames=" << r.combinedGpuCompositionFrames
               << " d2d_texture_bridge_frames=" << r.d2dTextureBridgeFrames << "/" << snapshot.viewportStats.d2dTextureBridgeFrames
               << " d2d_texture_bridge_attempts=" << aquariumD2DBridgeAttemptCount_
               << " d2d_texture_bridge_successes=" << aquariumD2DBridgeSuccessCount_
               << " d2d_texture_bridge_direct_successes=" << aquariumD2DBridgeDirectSuccessCount_
               << " d2d_texture_bridge_shared_successes=" << aquariumD2DBridgeSharedSuccessCount_
               << " d2d_shared_buffer_count=" << aquariumD2DBridgeSharedBufferCount_
               << " d2d_shared_write_index=" << aquariumD2DBridgeSharedWriteIndexStat_
               << " d2d_shared_draw_index=" << aquariumD2DBridgeSharedDrawIndexStat_
               << " d2d_shared_bitmap_recreates=" << aquariumD2DBridgeSharedBitmapRecreateCount_
               << " d2d_shared_copies=" << aquariumD2DBridgeSharedCopyCount_
               << " d2d_shared_mutex_acquires=" << aquariumD2DBridgeSharedMutexAcquireCount_
               << " d2d_shared_mutex_contentions=" << aquariumD2DBridgeSharedMutexContentionCount_
               << " d2d_shared_d2d_flushes=" << aquariumD2DBridgeSharedD2DFlushCount_
               << " d2d_texture_bridge_surface_diagnostics=" << (aquariumD2DBridgeSurfaceDiagnostics_.empty() ? "none" : aquariumD2DBridgeSurfaceDiagnostics_)
               << " d2d_frame_compositor=" << d2dFrameCompositor_.Diagnostics()
               << " d2d_frame_diagnostics=" << d2dFrameDiagnostics_.Diagnostics()
               << " d2d_present_scheduler=" << d2dPresentScheduler_.Diagnostics()
               << " d2d_viewport_runtime=" << d2dViewportBridgeRuntime_.Diagnostics()
               << " d2d_viewport_copy_scheduler=" << d2dViewportCopyScheduler_.Diagnostics()
               << " d2d_viewport_texture_cache=" << d2dViewportTextureCache_.Diagnostics()
               << " slate_renderer_pipeline=" << slateRendererPipeline_.Diagnostics()
               << " d2d_compositor_audit=" << d2dCompositorAudit_.Diagnostics()
               << " aquarium_viewport_input=" << aquariumViewportInputDiagnostics()
               << " gpu_overlay_baked=" << r.gpuOverlayBakedFrames
               << " composition_present_skips=" << r.compositionPresentSkips
               << " composition_pacing_skips=" << r.compositionPacingSkips
               << " composition_render_only_frames=" << r.compositionRenderOnlyFrames
               << " render_throughput_fps=" << aceFormatDoubleUtf8(r.compositionRenderIntervalAvgMs > 0.0 ? 1000.0 / r.compositionRenderIntervalAvgMs : 0.0)
               << " render_interval_ms_last_avg_max=" << aceFormatDoubleUtf8(r.compositionRenderIntervalLastMs) << "/" << aceFormatDoubleUtf8(r.compositionRenderIntervalAvgMs) << "/" << aceFormatDoubleUtf8(r.compositionRenderIntervalMaxMs)
               << " presented_fps=" << aceFormatDoubleUtf8(r.compositionPresentIntervalAvgMs > 0.0 ? 1000.0 / r.compositionPresentIntervalAvgMs : 0.0)
               << " present_interval_ms_last_avg_max=" << aceFormatDoubleUtf8(r.compositionPresentIntervalLastMs) << "/" << aceFormatDoubleUtf8(r.compositionPresentIntervalAvgMs) << "/" << aceFormatDoubleUtf8(r.compositionPresentIntervalMaxMs)
               << " geometry_upload_frames=" << snapshot.viewportStats.geometryUploadFrames
               << " geometry_reuse_frames=" << snapshot.viewportStats.geometryReuseFrames
               << " d2d_composition_hud_attached=" << (aquariumD2DCompositionHudAttached_ ? "true" : "false")
               << " d2d_composition_hud_draws=" << aquariumD2DCompositionHudDrawCount_
               << " d2d_composition_hud_presents=" << aquariumD2DCompositionHudPresentCount_
               << " combined_readback_frames=" << r.combinedRenderReadbackFrames
               << " readback_bytes=" << r.readbackBytes
               << " fence_waits=" << r.blockingFenceWaits
               << " fence_wait_ms=" << aceFormatDoubleUtf8(r.blockingFenceWaitMs);
            *logLine = os.str();
        }
        return ss.str();
    }

    std::wstring AceShellUi::formatStatUi(std::string* logLine) const
    {
        const auto textStats = D2DTextLayoutFoundation::Stats();
        const auto drawStats = uiDrawCommands_.Stats();
        const auto invalidationStats = uiInvalidation_.Snapshot();
        const auto retainedStats = uiRetainedLayout_.Stats();
        const auto styleStats = uiStyleSet_.Stats();
        const auto effectStats = D2DCachedEffects::stats();
        std::wstringstream ss;
        ss << L"text_draw=" << textStats.drawCount
           << L" ellipsis=" << textStats.ellipsisCount
           << L" retained_nodes=" << retainedStats.nodeCount
           << L" retained_passes=" << retainedStats.arrangePasses
           << L" draw_commands=" << drawStats.commandCount
           << L" draw_commands_max=" << drawStats.maxCommandCount
           << L" dirty_marks=" << invalidationStats.markCount
           << L" dirty_rects=" << invalidationStats.rects.size()
           << L" styles_panels=" << styleStats.panelStyleCount
           << L" styles_text=" << styleStats.textStyleCount
           << L" text_cache_hits=" << textCache_.hitCount()
           << L" text_cache_misses=" << textCache_.missCount()
           << L" layout_cache_hits=" << messageList_.layoutCacheHits()
           << L" layout_cache_misses=" << messageList_.layoutCacheMisses()
           << L" effect_cache_hits=" << effectStats.hitCount
           << L" effect_cache_misses=" << effectStats.missCount
           << L" viewport_layer_hold=" << aquariumParentCompositedHoldFrames_
           << L" viewport_layer_switches=" << aquariumViewportLayerModeSwitchCount_
           << L" fast_viewport_paints=" << aquariumFastViewportPaintCount_
           << L" full_viewport_paints=" << aquariumFullViewportPaintCount_;
        if (logLine)
        {
            std::ostringstream os;
            os << "text_draw=" << textStats.drawCount
               << " ellipsis=" << textStats.ellipsisCount
               << " retained_nodes=" << retainedStats.nodeCount
               << " retained_passes=" << retainedStats.arrangePasses
               << " draw_commands=" << drawStats.commandCount
               << " draw_commands_max=" << drawStats.maxCommandCount
               << " dirty_marks=" << invalidationStats.markCount
               << " dirty_rects=" << invalidationStats.rects.size()
               << " styles_panels=" << styleStats.panelStyleCount
               << " styles_text=" << styleStats.textStyleCount
               << " text_cache_hits=" << textCache_.hitCount()
               << " text_cache_misses=" << textCache_.missCount()
               << " layout_cache_hits=" << messageList_.layoutCacheHits()
               << " layout_cache_misses=" << messageList_.layoutCacheMisses()
               << " effect_cache_hits=" << effectStats.hitCount
               << " effect_cache_misses=" << effectStats.missCount
               << " viewport_layer_hold=" << aquariumParentCompositedHoldFrames_
               << " viewport_layer_switches=" << aquariumViewportLayerModeSwitchCount_
               << " fast_viewport_paints=" << aquariumFastViewportPaintCount_
               << " full_viewport_paints=" << aquariumFullViewportPaintCount_;
            *logLine = os.str();
        }
        return ss.str();
    }

    void AceShellUi::appendEngineCommandOutput(const std::wstring& title, const std::wstring& body, const std::string& logTag, const std::string& logLine)
    {
        std::wstring warning;
        const bool logged = AceEngineAppendLog(logTag, logLine, &warning);

        // PERF2R2: keep the compact one-line log entry for grepping, then append
        // the user-facing multi-line body as detail records. The D2D console reads
        // the log file tail, so without these detail lines the pretty console was
        // also a very stylish blindfold. Humanity, naturally, noticed.
        std::wstring detailWarning;
        const auto detailLines = aceDetailLogLinesFromBody(body);
        const bool loggedDetails = detailLines.empty() || AceEngineAppendLogLines(logTag + "_DETAIL", detailLines, &detailWarning);

        messageList_.addMessage(makeMessage(L"Tool", body, false, false, ChatMessageKind::Tool, title));
        if (!logged && !warning.empty())
        {
            messageList_.addMessage(makeMessage(L"Warning", warning, false, false, ChatMessageKind::Warning, L"engine log"));
        }
        if (!loggedDetails && !detailWarning.empty())
        {
            messageList_.addMessage(makeMessage(L"Warning", detailWarning, false, false, ChatMessageKind::Warning, L"engine log"));
        }
        if (engineLogOverlayVisible_)
        {
            refreshEngineLogOverlayLines();
        }
    }

    void AceShellUi::sendCurrentInput()
    {
        if (input_.empty())
        {
            statusBar_.setText(L"Input is empty.");
            return;
        }

        const auto submittedText = input_.takeText();

        if (handleLocalInputCommand(submittedText))
        {
            layout(width_, height_);
            invalidate();
            return;
        }

        commandHistory_.push(submittedText);
        messageList_.addMessage(makeMessage(L"You", submittedText, true, false, ChatMessageKind::User, L""));

        if (submitHandler_)
        {
            appendSubmitResult(submitHandler_(submittedText));
        }
        else
        {
            appendAssistantMockReply();
            statusBar_.setText(L"Input captured locally. No legacy AI backend is attached.");
        }

        saveActiveConversation();
        layout(width_, height_);
        invalidate();
    }

    bool AceShellUi::handleLocalInputCommand(const std::wstring& text)
    {
        const std::wstring normalized = aceTrimCommand(text);
        if (executeEngineStatsCommand(normalized))
        {
            return true;
        }

        const bool slashCommand = !text.empty() && text.front() == L'/';
        const std::wstring prefix = L"/rename ";
        if (text.rfind(prefix, 0) != 0)
        {
            if (slashCommand)
            {
                messageList_.addMessage(makeMessage(L"Warning", L"Unknown local command: " + text + L". Not submitted to backend.", false, false, ChatMessageKind::Warning, L"command"));
                AceEngineAppendLog("UNKNOWN_COMMAND", "backend_submit=false command=slash_unknown", nullptr);
                statusBar_.setText(L"Unknown local command. Not submitted.");
                return true;
            }
            return false;
        }

        std::wstring name = text.substr(prefix.size());
        const auto first = name.find_first_not_of(L" \t\r\n");
        const auto last = name.find_last_not_of(L" \t\r\n");

        if (first == std::wstring::npos || last == std::wstring::npos)
        {
            statusBar_.setText(L"Rename cancelled: empty name.");
            return true;
        }

        name = name.substr(first, last - first + 1);

        if (conversations_.empty())
        {
            conversations_.push_back(name);
            activeConversationIndex_ = 0;
            nextConversationNumber_ = 2;
        }
        else
        {
            activeConversationIndex_ = std::min(activeConversationIndex_, conversations_.size() - 1);
            conversations_[activeConversationIndex_] = name;
        }

        if (conversationMessages_.size() < conversations_.size())
        {
            conversationMessages_.resize(conversations_.size());
        }

        statusBar_.setText(L"Conversation renamed.");
        return true;
    }

    void AceShellUi::appendExternalMessage(ChatMessage message)
    {
        if (message.id == 0)
        {
            message.id = nextMessageId_++;
        }

        if (message.timestamp.empty())
        {
            message.timestamp = currentTimestamp();
        }

        message.fromUser = message.kind == ChatMessageKind::User ? true : message.fromUser;
        message.system = message.kind == ChatMessageKind::System ? true : message.system;
        messageList_.addMessage(std::move(message));
        saveActiveConversation();
    }

    void AceShellUi::appendSubmitResult(ChatSubmitResult result)
    {
        for (auto& message : result.messages)
        {
            appendExternalMessage(std::move(message));
        }

        if (!result.status.empty())
        {
            statusBar_.setText(std::move(result.status));
        }
        else
        {
            statusBar_.setText(result.ok ? L"Backend submit completed." : L"Backend submit failed.");
        }

        refreshSnapshot();
        refreshSuggestions();
        refreshDiagnostics();
    }

    void AceShellUi::appendAssistantMockReply()
    {
        messageList_.addMessage(makeMessage(
            L"Arhqen Cognition Engine",
            L"Input captured locally. No legacy AI backend is attached.",
            false,
            false,
            ChatMessageKind::Assistant,
            L"local"
        ));
        saveActiveConversation();
    }



    void AceShellUi::refreshSnapshot()
    {
        if (snapshotProvider_)
        {
            workspacePanel_.setSnapshot(snapshotProvider_());
        }
        else
        {
            am::core::AceUiSnapshot snapshot;
            snapshot.title = L"Arhqen Cognition Engine Workspace";
            snapshot.subtitle = L"Snapshot provider is not connected.";
            snapshot.metrics.push_back({L"Provider", L"missing", 3});
            workspacePanel_.setSnapshot(std::move(snapshot));
        }
    }

    void AceShellUi::refreshSuggestions()
    {
        if (!suggestionProvider_)
        {
            autocomplete_.close();
            return;
        }

        auto suggestions = suggestionProvider_(input_.text());
        autocomplete_.setSuggestions(std::move(suggestions));
    }

    void AceShellUi::acceptAutocomplete()
    {
        if (auto accepted = autocomplete_.takeAcceptedText())
        {
            input_.replaceAllText(*accepted);
            input_.setFocused(true);
            focus_.set(D2DFocusTarget::TextInput);
            refreshSuggestions();
            statusBar_.setText(L"Autocomplete accepted.");
        }
    }



    void AceShellUi::refreshInspector(am::core::AceUiSelection selection)
    {
        if (inspectorProvider_)
        {
            inspectorPanel_.setRecord(inspectorProvider_(selection));
            showToast(L"Inspector", L"Selected " + selection.label, D2DToastKind::Success);
        }
        else
        {
            am::core::AceUiInspectorRecord record;
            record.selection = selection;
            record.title = selection.label.empty() ? L"Inspector" : selection.label;
            record.subtitle = L"Inspector provider missing.";
            record.body = L"The UI selected an item, but no inspector provider is connected.";
            record.properties.push_back({L"provider", L"missing", 3});
            inspectorPanel_.setRecord(std::move(record));
        }
    }


    void AceShellUi::inspectLocalMessage(std::uint64_t messageId)
    {
        const ChatMessage* message = messageList_.findMessage(messageId);
        if (!message)
        {
            am::core::AceUiInspectorRecord record;
            record.title = L"Message missing";
            record.subtitle = L"Local UI message lookup failed.";
            record.body = L"The message was clicked, then disappeared. Naturally.";
            record.properties.push_back({L"id", std::to_wstring(messageId), 3});
            inspectorPanel_.setRecord(std::move(record));
            return;
        }

        inspectorPanel_.setRecord(D2DMessageInspector::makeRecord(*message));
        workspaceTabs_.setActive(L"inspect");
        statusBar_.setText(L"Inspecting message #" + std::to_wstring(messageId));
        showToast(L"Message inspector", L"Opened message #" + std::to_wstring(messageId), D2DToastKind::Success);
    }

    void AceShellUi::refreshDiagnostics()
    {
        D2DDiagnosticsSnapshot snapshot;
        snapshot.title = L"Arhqen Cognition Engine Diagnostics";
        snapshot.visible = diagnostics_.visible();

        snapshot.rows.push_back({L"window", std::to_wstring(width_) + L"x" + std::to_wstring(height_)});
        snapshot.rows.push_back({L"text cache", L"size=" + std::to_wstring(textCache_.size()) + L" cap=" + std::to_wstring(textCache_.capacity())});
        snapshot.rows.push_back({L"text cache hits", std::to_wstring(textCache_.hitCount())});
        snapshot.rows.push_back({L"text cache misses", std::to_wstring(textCache_.missCount())});
        snapshot.rows.push_back({L"message layout cache", L"size=" + std::to_wstring(messageList_.layoutCacheSize())});
        snapshot.rows.push_back({L"layout cache hits", std::to_wstring(messageList_.layoutCacheHits())});
        snapshot.rows.push_back({L"layout cache misses", std::to_wstring(messageList_.layoutCacheMisses())});
        snapshot.rows.push_back({L"messages", std::to_wstring(messageList_.size())});
        snapshot.rows.push_back({L"toolbar", L"active"});
        snapshot.rows.push_back({L"autocomplete", autocomplete_.active() ? L"visible" : L"hidden"});
        snapshot.rows.push_back({L"command palette", commandPalette_.active() ? L"visible" : L"hidden"});
        snapshot.rows.push_back({L"focus", focus_.name()});
        snapshot.rows.push_back({L"workspace ratio", std::to_wstring(layoutProfile_.workspaceRatio)});
        snapshot.rows.push_back({L"inspector ratio", std::to_wstring(layoutProfile_.inspectorRatio)});
        snapshot.rows.push_back({L"command history", std::to_wstring(commandHistory_.size())});
        const auto graphStats = workspacePanel_.graphStats();
        snapshot.rows.push_back({L"graph nodes", std::to_wstring(graphStats.nodeCount)});
        snapshot.rows.push_back({L"graph edges", std::to_wstring(graphStats.edgeCount)});
        snapshot.rows.push_back({L"graph zoom", std::to_wstring(graphStats.zoom)});
        snapshot.rows.push_back({L"AQ3D7 parent paints", std::to_wstring(parentPaintCount_)});
        snapshot.rows.push_back({L"AQ3D7 full invalidations", std::to_wstring(fullInvalidationCount_)});
        snapshot.rows.push_back({L"AQ3D7 partial invalidations", std::to_wstring(partialInvalidationCount_)});
        snapshot.rows.push_back({L"AQ3D7 hover invalidations", std::to_wstring(hoverInvalidationCount_)});
        snapshot.rows.push_back({L"AQ3D7 child sync", std::to_wstring(childSyncCount_)});
        snapshot.rows.push_back({L"AQ3D7 viewport init", std::to_wstring(aquariumEmbeddedDx12Viewport_.InitCount())});
        snapshot.rows.push_back({L"AQ3D7 viewport resize", std::to_wstring(aquariumEmbeddedDx12Viewport_.ResizeCount())});
        snapshot.rows.push_back({L"AQ3D7 renderer recreate", std::to_wstring(aquariumEmbeddedDx12Viewport_.RendererRecreateCount())});
        snapshot.rows.push_back({L"AQ3D14 single-HWND composite", aquariumUseSingleHwndCompositeViewport_ ? L"enabled" : L"disabled"});
        snapshot.rows.push_back({L"AQ3D14 composite frames", std::to_wstring(aquariumCompositeFrameCount_)});
        snapshot.rows.push_back({L"AQ3D14 resize cached frames", std::to_wstring(aquariumCompositeCachedResizeFrameCount_)});
        snapshot.rows.push_back({L"AQ3D14 legacy child suppressed", std::to_wstring(aquariumLegacyChildSuppressedCount_)});
        const auto textStats = D2DTextLayoutFoundation::Stats();
        const auto drawStats = uiDrawCommands_.Stats();
        const auto invalidationStats = uiInvalidation_.Snapshot();
        const auto retainedStats = uiRetainedLayout_.Stats();
        const auto styleStats = uiStyleSet_.Stats();
        const auto telemetryStats = aquariumTelemetryWidgets_.Stats();
        snapshot.rows.push_back({L"ACE-UI5 text draws", std::to_wstring(textStats.drawCount) + L" ellipsis=" + std::to_wstring(textStats.ellipsisCount)});
        snapshot.rows.push_back({L"ACE-UI6 retained nodes", std::to_wstring(retainedStats.nodeCount) + L" passes=" + std::to_wstring(retainedStats.arrangePasses)});
        snapshot.rows.push_back({L"ACE-UI7 draw commands", std::to_wstring(drawStats.commandCount) + L" max=" + std::to_wstring(drawStats.maxCommandCount)});
        snapshot.rows.push_back({L"ACE-UI8 dirty marks", std::to_wstring(invalidationStats.markCount) + L" rects=" + std::to_wstring(invalidationStats.rects.size())});
        snapshot.rows.push_back({L"ACE-UI9 overlay", uiDebugOverlay_.Visible() ? L"visible" : L"hidden"});
        snapshot.rows.push_back({L"ACE-UI11 styles", std::to_wstring(styleStats.panelStyleCount) + L" panels / " + std::to_wstring(styleStats.textStyleCount) + L" text"});
        snapshot.rows.push_back({L"ACE-AQUI1 telemetry", std::to_wstring(telemetryStats.renderCount) + L" renders"});

        snapshot.notes.push_back(L"F12 toggles this overlay.");
        snapshot.notes.push_back(L"Click a message bubble to inspect local message metadata.");
        snapshot.notes.push_back(L"Click workspace items to inspect backend records.");
        snapshot.notes.push_back(L"Toolbar actions are command-palette shortcuts with fewer human ceremonies.");

        diagnostics_.setSnapshot(std::move(snapshot));
    }


    void AceShellUi::refreshUiDebugOverlay()
    {
        D2DUiDebugOverlaySnapshot snapshot;
        snapshot.visible = uiDebugOverlay_.Visible();
        snapshot.title = L"Arhqen UI Debug / Slate-inspired";
        snapshot.layoutStats = uiRetainedLayout_.Stats();
        snapshot.dirty = uiInvalidation_.Snapshot();
        snapshot.drawStats = uiDrawCommands_.Stats();
        snapshot.textStats = D2DTextLayoutFoundation::Stats();
        snapshot.paintCount = parentPaintCount_;
        snapshot.repaintCount = aquariumUiRepaintCount_;
        snapshot.hoverId = aquariumHoverHotId_ >= 0 ? L"aquarium-hot-" + std::to_wstring(aquariumHoverHotId_) : L"shell";
        snapshot.focusId = focus_.name();
        snapshot.rects.push_back({L"app.topbar", appTopBarRect_, false});
        snapshot.rects.push_back({L"sidebar", sidebarRect_, false});
        snapshot.rects.push_back({L"main", mainRect_, false});
        if (environmentOpen_)
        {
            snapshot.rects.push_back({L"environment", environmentModalRect_, true});
            snapshot.rects.push_back({L"aq.viewport", aquariumEmbeddedViewportRect_, true});
            snapshot.rects.push_back({L"aq.left", aquariumLeftPanelRect_, false});
            snapshot.rects.push_back({L"aq.logs", aquariumRightLogsPanelRect_, false});
        }
        uiDebugOverlay_.SetSnapshot(std::move(snapshot));
    }

    void AceShellUi::renderUiDebugOverlay(D2DRenderContext& ctx)
    {
        if (!uiDebugOverlay_.Visible())
        {
            return;
        }

        refreshUiDebugOverlay();
        uiDebugOverlay_.Render(ctx);
    }

    void AceShellUi::toggleEngineLogOverlay()
    {
        engineLogOverlayVisible_ = !engineLogOverlayVisible_;
        ++engineLogOverlayToggleCount_;
        if (engineLogOverlayVisible_)
        {
            refreshEngineLogOverlayLines();
            engineLogOverlayInputFocused_ = true;
            engineLogTextFocused_ = false;
            engineLogOverlayInput_.setFocused(true);
            input_.setFocused(false);
            engineLogOverlayScroll_.autoScrollWhenAtBottom = true;
            if (!engineLogOverlayScroll_.userScrolled)
            {
                engineLogOverlayScroll_.offset = engineLogOverlayScroll_.maxScroll;
            }
        }
        else
        {
            engineLogOverlayInputFocused_ = false;
            engineLogTextFocused_ = false;
            clearEngineLogTextSelection();
            engineLogOverlayInput_.setFocused(false);
            input_.setFocused(focus_.is(D2DFocusTarget::TextInput));
        }

        requestParentCompositedViewportHold(engineLogOverlayVisible_ ? 30u : 20u, engineLogOverlayVisible_ ? L"engine-log-open" : L"engine-log-close");

        // ACE-UI12: no toast on console toggle. The console itself is the
        // feedback. Throwing a toast over a viewport overlay is how a UI becomes
        // a sandwich made of popups, and nobody ordered that.
        statusBar_.setText(engineLogOverlayVisible_ ? L"Engine log console opened." : L"Engine log console closed.");
        invalidate();
    }

    void AceShellUi::refreshEngineLogOverlayLines()
    {
        const bool wasAtBottom = engineLogOverlayScroll_.offset >= engineLogOverlayScroll_.maxScroll - 2.0f;
        clearEngineLogTextSelection();
        engineLogOverlayLines_.clear();
        engineLogOverlayLineLayouts_.clear();
        std::wstring warning;
        const auto rawLines = AceEngineReadLogTail(240, &warning);
        if (!warning.empty())
        {
            engineLogOverlayLines_.push_back(L"[warning] " + warning);
        }
        else if (rawLines.empty())
        {
            engineLogOverlayLines_.push_back(L"Build/Logs/ace_engine.log is empty or has not been created yet.");
            engineLogOverlayLines_.push_back(L"Run stat_rhi, stat_coords, stat_fps, stat_ui, or stat_help to write entries.");
        }
        else
        {
            for (const auto& line : rawLines)
            {
                const auto wrapped = aceWrapEngineConsoleLine(aceWideLossy(line));
                engineLogOverlayLines_.insert(engineLogOverlayLines_.end(), wrapped.begin(), wrapped.end());
            }
        }

        if (wasAtBottom || !engineLogOverlayScroll_.userScrolled)
        {
            engineLogOverlayScroll_.autoScrollWhenAtBottom = true;
            engineLogOverlayScroll_.userScrolled = false;
        }
        engineLogOverlayLineLayouts_.resize(engineLogOverlayLines_.size());
    }

    bool AceShellUi::submitEngineLogOverlayInput()
    {
        std::wstring command = engineLogOverlayInput_.text();
        const std::wstring normalized = aceTrimCommand(command);
        if (normalized.empty())
        {
            statusBar_.setText(L"Engine console input is empty.");
            return true;
        }

        engineLogOverlayInput_.setText(L"");
        if (executeEngineStatsCommand(normalized))
        {
            refreshEngineLogOverlayLines();
            engineLogOverlayScroll_.autoScrollWhenAtBottom = true;
            engineLogOverlayScroll_.userScrolled = false;
            return true;
        }

        const std::wstring body = L"Unknown engine console command: " + normalized + L". Not submitted to backend. Try stat_help.";
        messageList_.addMessage(makeMessage(L"Warning", body, false, false, ChatMessageKind::Warning, L"engine console"));
        std::wstring warning;
        AceEngineAppendLog("UNKNOWN_COMMAND", "source=engine_log_overlay command=" + aceNarrowLossy(normalized) + " backend_submit=false", &warning);
        if (!warning.empty())
        {
            messageList_.addMessage(makeMessage(L"Warning", warning, false, false, ChatMessageKind::Warning, L"engine log"));
        }
        refreshEngineLogOverlayLines();
        statusBar_.setText(L"Unknown engine console command. Not submitted to backend.");
        return true;
    }

    Microsoft::WRL::ComPtr<IDWriteTextLayout> AceShellUi::engineLogTextLayoutForLine(std::size_t line) const
    {
        if (line >= engineLogOverlayLines_.size())
        {
            return {};
        }
        if (engineLogOverlayLineLayouts_.size() != engineLogOverlayLines_.size())
        {
            engineLogOverlayLineLayouts_.resize(engineLogOverlayLines_.size());
        }

        auto& cached = engineLogOverlayLineLayouts_[line];
        if (!cached)
        {
            TextLayoutOptions options{};
            options.role = FontRole::Mono;
            options.width = 65536.0f;
            options.height = 19.0f;
            options.wrapping = DWRITE_WORD_WRAPPING_NO_WRAP;
            cached = fontEngine_.createLayout(engineLogOverlayLines_[line], options).layout;
        }
        return cached;
    }

    AceShellUi::EngineLogTextPosition AceShellUi::hitTestEngineLogText(float x, float y) const
    {
        EngineLogTextPosition hit{};
        if (!engineLogOverlayVisible_ || engineLogOverlayLines_.empty() || engineLogOverlayLogViewportRect_.empty())
        {
            return hit;
        }

        constexpr float kLineHeight = 19.0f;
        const float localY = std::max(0.0f, y - engineLogOverlayLogViewportRect_.top + engineLogOverlayScroll_.offset);
        const auto line = static_cast<std::size_t>(std::floor(localY / kLineHeight));
        hit.line = std::min(line, engineLogOverlayLines_.size() - 1);

        const std::wstring& text = engineLogOverlayLines_[hit.line];
        if (x <= engineLogOverlayLogViewportRect_.left)
        {
            hit.column = 0;
        }
        else
        {
            const float localX = x - engineLogOverlayLogViewportRect_.left;
            const auto layout = engineLogTextLayoutForLine(hit.line);
            if (layout)
            {
                BOOL trailing = FALSE;
                BOOL inside = FALSE;
                DWRITE_HIT_TEST_METRICS metrics{};
                if (SUCCEEDED(layout->HitTestPoint(localX, std::fmod(localY, kLineHeight), &trailing, &inside, &metrics)))
                {
                    const std::size_t position = static_cast<std::size_t>(metrics.textPosition) + (trailing ? 1u : 0u);
                    hit.column = std::min<std::size_t>(text.size(), position);
                }
            }
        }
        hit.valid = true;
        return hit;
    }

    bool AceShellUi::engineLogHasTextSelection() const
    {
        return engineLogSelectionAnchor_.valid &&
            engineLogSelectionActive_.valid &&
            (engineLogSelectionAnchor_.line != engineLogSelectionActive_.line ||
             engineLogSelectionAnchor_.column != engineLogSelectionActive_.column);
    }

    std::wstring AceShellUi::selectedEngineLogText() const
    {
        if (!engineLogHasTextSelection() || engineLogOverlayLines_.empty())
        {
            return L"";
        }

        EngineLogTextPosition first = engineLogSelectionAnchor_;
        EngineLogTextPosition last = engineLogSelectionActive_;
        if (std::tie(last.line, last.column) < std::tie(first.line, first.column))
        {
            std::swap(first, last);
        }

        first.line = std::min(first.line, engineLogOverlayLines_.size() - 1);
        last.line = std::min(last.line, engineLogOverlayLines_.size() - 1);

        std::wstringstream out;
        for (std::size_t line = first.line; line <= last.line && line < engineLogOverlayLines_.size(); ++line)
        {
            const std::wstring& text = engineLogOverlayLines_[line];
            const std::size_t start = line == first.line ? std::min(first.column, text.size()) : 0;
            const std::size_t end = line == last.line ? std::min(last.column, text.size()) : text.size();
            if (end > start)
            {
                out << text.substr(start, end - start);
            }
            if (line != last.line)
            {
                out << L"\r\n";
            }
        }
        return out.str();
    }

    void AceShellUi::clearEngineLogTextSelection()
    {
        engineLogTextSelecting_ = false;
        engineLogSelectionAnchor_ = {};
        engineLogSelectionActive_ = {};
    }

    void AceShellUi::selectAllEngineLogText()
    {
        if (engineLogOverlayLines_.empty())
        {
            clearEngineLogTextSelection();
            return;
        }

        engineLogSelectionAnchor_ = {0, 0, true};
        engineLogSelectionActive_ = {
            engineLogOverlayLines_.size() - 1,
            engineLogOverlayLines_.back().size(),
            true};
        engineLogTextFocused_ = true;
        engineLogTextSelecting_ = false;
    }

    bool AceShellUi::copyEngineLogTextSelectionToClipboard()
    {
        const std::wstring selected = selectedEngineLogText();
        if (selected.empty())
        {
            return false;
        }

        std::string error;
        if (!D2DClipboard::writeText(parent_, selected, &error))
        {
            statusBar_.setText(L"Engine log copy failed: " + widen(error));
            return true;
        }

        statusBar_.setText(L"Copied engine log selection.");
        return true;
    }

    void AceShellUi::renderEngineLogTextSelection(D2DRenderContext& ctx, float lineHeight) const
    {
        if (!ctx.target || !ctx.brushes.accentBlue || !engineLogHasTextSelection() || engineLogOverlayLines_.empty())
        {
            return;
        }

        EngineLogTextPosition first = engineLogSelectionAnchor_;
        EngineLogTextPosition last = engineLogSelectionActive_;
        if (std::tie(last.line, last.column) < std::tie(first.line, first.column))
        {
            std::swap(first, last);
        }

        const float oldOpacity = ctx.brushes.accentBlue->GetOpacity();
        for (std::size_t line = first.line; line <= last.line && line < engineLogOverlayLines_.size(); ++line)
        {
            const std::wstring& text = engineLogOverlayLines_[line];
            const std::size_t start = line == first.line ? std::min(first.column, text.size()) : 0;
            const std::size_t end = line == last.line ? std::min(last.column, text.size()) : text.size();
            if (end <= start)
            {
                continue;
            }

            const float y = engineLogOverlayScroll_.viewport.top - engineLogOverlayScroll_.offset + static_cast<float>(line) * lineHeight;
            if (y + lineHeight < engineLogOverlayScroll_.viewport.top || y > engineLogOverlayScroll_.viewport.bottom)
            {
                continue;
            }

            // PERF2R3.2: one source of truth for visual selection feedback.
            // The selected text span uses a single tint; do not layer a row-band
            // hover/selection color under a second word/span highlight.
            float startX = 0.0f;
            float endX = 0.0f;
            if (const auto layout = engineLogTextLayoutForLine(line))
            {
                FLOAT ignoredY = 0.0f;
                DWRITE_HIT_TEST_METRICS metrics{};
                layout->HitTestTextPosition(static_cast<UINT32>(start), FALSE, &startX, &ignoredY, &metrics);
                layout->HitTestTextPosition(static_cast<UINT32>(end), FALSE, &endX, &ignoredY, &metrics);
            }
            UiRect highlight = makeUiRect(
                engineLogOverlayScroll_.viewport.left + startX,
                y + 1.0f,
                std::min(engineLogOverlayScroll_.viewport.right, engineLogOverlayScroll_.viewport.left + endX + 1.0f),
                y + lineHeight - 1.0f);
            if (!highlight.empty())
            {
                ctx.brushes.accentBlue->SetOpacity(0.36f);
                ctx.target->FillRectangle(highlight.d2d(), ctx.brushes.accentBlue);
            }
        }
        ctx.brushes.accentBlue->SetOpacity(oldOpacity);
    }

    bool AceShellUi::handleEngineLogOverlayMouseDown(D2DRenderContext& ctx, float x, float y, unsigned clickCount)
    {
        if (!engineLogOverlayVisible_ || !engineLogOverlayRect_.contains(x, y))
        {
            return false;
        }

        if (engineLogOverlayInput_.onMouseDown(ctx, x, y))
        {
            engineLogOverlayInputFocused_ = true;
            engineLogTextFocused_ = false;
            engineLogTextSelecting_ = false;
            input_.setFocused(false);
            return true;
        }

        engineLogOverlayInputFocused_ = false;
        engineLogOverlayInput_.setFocused(false);

        if (engineLogOverlayScroll_.viewport.contains(x, y))
        {
            engineLogTextFocused_ = true;
            const EngineLogTextPosition hit = hitTestEngineLogText(x, y);

            if (clickCount >= 3 && hit.valid)
            {
                // Triple click selects the complete visual log row.
                const std::size_t line = std::min(hit.line, engineLogOverlayLines_.size() - 1);
                engineLogSelectionAnchor_ = {line, 0, true};
                engineLogSelectionActive_ = {line, engineLogOverlayLines_[line].size(), true};
                engineLogTextSelecting_ = false;
            }
            else if (clickCount == 2 && hit.valid)
            {
                // Double click selects one word (or one punctuation/space run),
                // using the same DirectWrite-derived position as drag selection.
                const std::size_t line = std::min(hit.line, engineLogOverlayLines_.size() - 1);
                const std::wstring& text = engineLogOverlayLines_[line];
                if (text.empty())
                {
                    engineLogSelectionAnchor_ = {line, 0, true};
                    engineLogSelectionActive_ = engineLogSelectionAnchor_;
                }
                else
                {
                    std::size_t at = std::min(hit.column, text.size());
                    if (at == text.size()) { --at; }
                    const auto characterClass = [](wchar_t ch)
                    {
                        if (std::iswalnum(ch) || ch == L'_') { return 0; }
                        if (std::iswspace(ch)) { return 1; }
                        return 2;
                    };
                    const int selectedClass = characterClass(text[at]);
                    std::size_t start = at;
                    std::size_t end = at + 1;
                    while (start > 0 && characterClass(text[start - 1]) == selectedClass) { --start; }
                    while (end < text.size() && characterClass(text[end]) == selectedClass) { ++end; }
                    engineLogSelectionAnchor_ = {line, start, true};
                    engineLogSelectionActive_ = {line, end, true};
                }
                engineLogTextSelecting_ = false;
            }
            else
            {
                engineLogTextSelecting_ = true;
                engineLogSelectionAnchor_ = hit;
                engineLogSelectionActive_ = hit;
            }
            input_.setFocused(false);
            return true;
        }

        if (engineLogOverlayScroll_.thumb.contains(x, y))
        {
            return beginAquariumScrollbarDrag(x, y);
        }

        if (engineLogOverlayScroll_.track.contains(x, y))
        {
            const float page = std::max(24.0f, engineLogOverlayScroll_.viewportHeight - 24.0f);
            engineLogOverlayScroll_.offset += y < engineLogOverlayScroll_.thumb.top ? -page : page;
            engineLogOverlayScroll_.userScrolled = true;
            clampAquariumScroll(engineLogOverlayScroll_);
            engineLogOverlayScroll_.autoScrollWhenAtBottom = engineLogOverlayScroll_.offset >= engineLogOverlayScroll_.maxScroll - 2.0f;
            return true;
        }

        return true;
    }

    bool AceShellUi::handleEngineLogOverlayMouseUp(D2DRenderContext& ctx, float x, float y)
    {
        if (!engineLogOverlayVisible_)
        {
            return false;
        }

        // The overlay owns mouse-up before the generic aquarium scrollbar path.
        // Finish its scrollbar drag here, otherwise the pointer remains live and
        // later mouse moves scroll the log even though LMB is no longer down.
        if (aquariumDraggingScroll_ == &engineLogOverlayScroll_)
        {
            endAquariumScrollbarDrag();
            return true;
        }

        if (engineLogTextSelecting_)
        {
            engineLogSelectionActive_ = hitTestEngineLogText(x, y);
            engineLogTextSelecting_ = false;
            engineLogTextFocused_ = true;
            return true;
        }

        if (engineLogOverlayInput_.onMouseUp(ctx, x, y))
        {
            return true;
        }

        return engineLogOverlayRect_.contains(x, y);
    }

    bool AceShellUi::handleEngineLogOverlayMouseMove(D2DRenderContext& ctx, float x, float y, bool leftButtonDown)
    {
        if (!engineLogOverlayVisible_)
        {
            return false;
        }

        if (engineLogTextSelecting_)
        {
            if (!leftButtonDown)
            {
                // Defensive event-state repair for capture loss or an LMB-up that
                // was consumed by another control. Autoscroll must never outlive drag.
                engineLogTextSelecting_ = false;
                return true;
            }

            const float oldOffset = engineLogOverlayScroll_.offset;
            if (y < engineLogOverlayScroll_.viewport.top)
            {
                const float distance = engineLogOverlayScroll_.viewport.top - y;
                engineLogOverlayScroll_.offset -= std::clamp(distance * 0.35f, 2.0f, 24.0f);
            }
            else if (y > engineLogOverlayScroll_.viewport.bottom)
            {
                const float distance = y - engineLogOverlayScroll_.viewport.bottom;
                engineLogOverlayScroll_.offset += std::clamp(distance * 0.35f, 2.0f, 24.0f);
            }
            clampAquariumScroll(engineLogOverlayScroll_);
            if (engineLogOverlayScroll_.offset != oldOffset)
            {
                engineLogOverlayScroll_.userScrolled = true;
                engineLogOverlayScroll_.autoScrollWhenAtBottom =
                    engineLogOverlayScroll_.offset >= engineLogOverlayScroll_.maxScroll - 2.0f;
            }
            engineLogSelectionActive_ = hitTestEngineLogText(x, y);
            return true;
        }

        if (engineLogOverlayInput_.onMouseMove(ctx, x, y))
        {
            return true;
        }

        return engineLogOverlayRect_.contains(x, y);
    }

    bool AceShellUi::handleEngineLogOverlayWheel(D2DRenderContext& ctx, float x, float y, int wheelDelta)
    {
        if (!engineLogOverlayVisible_ || !engineLogOverlayRect_.contains(x, y))
        {
            return false;
        }

        if (engineLogOverlayInput_.hitTest(x, y))
        {
            engineLogOverlayInput_.onMouseWheel(ctx, x, y, wheelDelta);
            return true;
        }

        if (engineLogOverlayScroll_.viewport.contains(x, y) || engineLogOverlayScroll_.track.contains(x, y) || engineLogOverlayScroll_.thumb.contains(x, y))
        {
            engineLogOverlayScroll_.offset += static_cast<float>(-wheelDelta) * 0.34f;
            engineLogOverlayScroll_.userScrolled = true;
            clampAquariumScroll(engineLogOverlayScroll_);
            engineLogOverlayScroll_.autoScrollWhenAtBottom = engineLogOverlayScroll_.offset >= engineLogOverlayScroll_.maxScroll - 2.0f;
            if (engineLogOverlayScroll_.autoScrollWhenAtBottom)
            {
                engineLogOverlayScroll_.userScrolled = false;
            }
        }

        return true;
    }

    bool AceShellUi::handleEngineLogOverlayChar(WPARAM wParam)
    {
        if (!engineLogOverlayVisible_)
        {
            return false;
        }

        if (engineLogOverlayInputFocused_)
        {
            return engineLogOverlayInput_.onChar(wParam);
        }

        // ACE-PERF2R3: when the log body owns focus, printable chars must not
        // leak into the main chat input underneath the overlay.
        return engineLogTextFocused_;
    }

    bool AceShellUi::handleEngineLogOverlayKeyDown(WPARAM wParam, const D2DKeyboardState& keyboard)
    {
        if (!engineLogOverlayVisible_)
        {
            return false;
        }

        if (wParam == VK_ESCAPE)
        {
            toggleEngineLogOverlay();
            return true;
        }

        if (wParam == VK_RETURN && !keyboard.shift && engineLogOverlayInputFocused_)
        {
            return submitEngineLogOverlayInput();
        }

        if (wParam == VK_PRIOR || wParam == VK_NEXT)
        {
            const float page = std::max(24.0f, engineLogOverlayScroll_.viewportHeight - 24.0f);
            engineLogOverlayScroll_.offset += (wParam == VK_PRIOR ? -page : page);
            engineLogOverlayScroll_.userScrolled = true;
            clampAquariumScroll(engineLogOverlayScroll_);
            engineLogOverlayScroll_.autoScrollWhenAtBottom = engineLogOverlayScroll_.offset >= engineLogOverlayScroll_.maxScroll - 2.0f;
            return true;
        }

        if (keyboard.ctrl && wParam == 'C')
        {
            if (engineLogHasTextSelection() && copyEngineLogTextSelectionToClipboard())
            {
                return true;
            }

            std::string error;
            if (engineLogOverlayInput_.copySelectionToClipboard(parent_, &error))
            {
                statusBar_.setText(L"Copied engine console selection.");
            }
            return true;
        }
        if (keyboard.ctrl && wParam == 'A')
        {
            if (engineLogOverlayInputFocused_)
            {
                engineLogOverlayInput_.selectAll();
            }
            else
            {
                selectAllEngineLogText();
            }
            return true;
        }
        if (keyboard.ctrl && wParam == 'X')
        {
            if (engineLogOverlayInputFocused_)
            {
                std::string error;
                if (engineLogOverlayInput_.cutSelectionToClipboard(parent_, &error))
                {
                    statusBar_.setText(L"Cut engine console selection.");
                }
            }
            return true;
        }
        if (keyboard.ctrl && wParam == 'V')
        {
            if (engineLogOverlayInputFocused_)
            {
                std::string error;
                if (engineLogOverlayInput_.pasteFromClipboard(parent_, &error))
                {
                    statusBar_.setText(L"Pasted into engine console.");
                }
            }
            return true;
        }

        if (!engineLogOverlayInputFocused_)
        {
            return true;
        }

        const bool handled = engineLogOverlayInput_.onKeyDown(wParam, keyboard.ctrl, keyboard.shift);
        if (handled && !keyboard.ctrl)
        {
            clearEngineLogTextSelection();
        }
        return handled;
    }

    void AceShellUi::renderAquariumViewportHudLayer(D2DRenderContext& ctx, UiRect viewportRect, const ace::aquarium_ui::AceAquariumUiSnapshot& snapshot)
    {
        if (!ctx.target || viewportRect.empty())
        {
            aquariumTelemetryOverlayRect_ = makeUiRect(0, 0, 0, 0);
            return;
        }

        const UiRect avoidRect = engineLogOverlayVisible_ ? computeEngineLogOverlayRect(viewportRect) : makeUiRect(0, 0, 0, 0);
        aquariumTelemetryOverlayRect_ = computeAquariumTelemetryOverlayRect(viewportRect, avoidRect);
        if (aquariumTelemetryOverlayRect_.empty())
        {
            return;
        }

        aquariumTelemetryWidgets_.RenderPanel(ctx, aquariumTelemetryOverlayRect_, snapshot);
    }

    UiRect AceShellUi::computeAquariumTelemetryOverlayRect(UiRect viewportRect, UiRect avoidRect) const
    {
        if (viewportRect.empty())
        {
            return makeUiRect(0, 0, 0, 0);
        }

        constexpr float margin = 16.0f;
        constexpr float minTopPad = 46.0f;
        constexpr float panelHeight = 122.0f;
        const float width = std::min(420.0f, std::max(310.0f, viewportRect.width() * 0.26f));
        const float left = viewportRect.left + margin;
        const float right = std::min(viewportRect.right - margin, left + width);

        float bottom = viewportRect.bottom - margin;
        const bool avoidOverlapsBottomHud = !avoidRect.empty() &&
            avoidRect.left < right + 8.0f && avoidRect.right > left - 8.0f &&
            avoidRect.bottom > viewportRect.top + minTopPad;
        if (avoidOverlapsBottomHud)
        {
            bottom = std::min(bottom, avoidRect.top - 12.0f);
        }

        float top = bottom - panelHeight;
        const float minTop = viewportRect.top + minTopPad;
        if (top < minTop)
        {
            top = minTop;
            bottom = top + panelHeight;
        }

        if (bottom > viewportRect.bottom - margin)
        {
            bottom = viewportRect.bottom - margin;
            top = bottom - panelHeight;
        }

        if (right <= left + 80.0f || bottom <= top + 60.0f)
        {
            return makeUiRect(0, 0, 0, 0);
        }

        return makeUiRect(left, top, right, bottom);
    }

    UiRect AceShellUi::computeEngineLogOverlayRect(UiRect anchor) const
    {
        if (anchor.empty())
        {
            return makeUiRect(0.0f, 0.0f, 0.0f, 0.0f);
        }

        constexpr float margin = 16.0f;
        const float overlayHeight = std::clamp(anchor.height() * 0.34f, 184.0f, 286.0f);
        const float left = anchor.left + margin;
        const float right = anchor.right - margin;
        const float bottom = anchor.bottom - margin;
        const float top = std::max(anchor.top + 58.0f, bottom - overlayHeight);
        return makeUiRect(left, top, std::max(left + 420.0f, right), bottom);
    }

    UiRect AceShellUi::computeAquariumNativeViewportRect(UiRect logicalRect) const
    {
        // ACE-PERF0R2: do not solve UI-over-viewport by resizing/clipping a child
        // HWND. UE's Slate path treats the viewport as a draw element inside the
        // window element list, then paints children/overlays on higher layers.
        // The legacy child HWND keeps its stable rect; overlay correctness is now
        // handled by switching the 3D scene to the parent-composited viewport
        // texture path when UI must be above it. Less z-order astrology, more sane
        // architecture.
        return logicalRect;
    }

    bool AceShellUi::isViewportLocalOverlayActive() const
    {
        if (!environmentOpen_ || !aquarium3DModeActive_)
        {
            return false;
        }

        return engineLogOverlayVisible_ ||
            cameraSpeedPopupOpen_ ||
            !engineOpenMenu_.empty() ||
            commandPalette_.active() ||
            settingsOpen_ ||
            shortcutHelp_.visible() ||
            diagnostics_.visible() ||
            uiDebugOverlay_.Visible();
    }

    void AceShellUi::requestParentCompositedViewportHold(std::uint32_t frames, const wchar_t* reason)
    {
        if (frames == 0)
        {
            return;
        }

        aquariumParentCompositedHoldFrames_ = std::max(aquariumParentCompositedHoldFrames_, frames);
        aquariumViewportLayerReason_ = reason && reason[0] ? reason : L"parent-composited-hold";
    }

    bool AceShellUi::shouldUseDirectCompositionForAquariumViewport() const
    {
        if (!aquariumUseSingleHwndCompositeViewport_ || !environmentOpen_ || !aquarium3DModeActive_)
        {
            return false;
        }

        if (windowLiveResizeActive_ || aquariumPanelResizeTarget_ != AquariumPanelResizeTarget::None || aquariumResizeQuarantineActive_)
        {
            return false;
        }

        // Keep viewport-local D2D/DWrite overlays correct by temporarily returning
        // to parent composition. Otherwise the scene owns an independent DComp
        // visual and must not wait for a full shell paint to advance.
        return !isViewportLocalOverlayActive() && aquariumParentCompositedHoldFrames_ == 0;
    }

    bool AceShellUi::ensureAquariumD2DCompositionHud(UiRect hudRect, std::string* error)
    {
        if (!uiDxgiFactory_ || !uiD3D11Device_ || !renderTarget_ || hudRect.empty())
        {
            if (error) { *error = "D2D composition HUD requires live UI DXGI/D3D11/D2D devices and a non-empty rect."; }
            return false;
        }

        const am::renderer::rhi::Extent2D extent{
            static_cast<am::renderer::rhi::U32>(std::max(1.0f, std::round(hudRect.width()))),
            static_cast<am::renderer::rhi::U32>(std::max(1.0f, std::round(hudRect.height())))
        };
        const bool resourceMatches =
            aquariumD2DCompositionHudSwapChain_ && aquariumD2DCompositionHudTarget_ &&
            aquariumD2DCompositionHudExtent_.width == extent.width &&
            aquariumD2DCompositionHudExtent_.height == extent.height;
        if (resourceMatches)
        {
            aquariumD2DCompositionHudRect_ = hudRect;
            return true;
        }

        resetAquariumD2DCompositionHud();

        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width = extent.width;
        desc.Height = extent.height;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.Stereo = FALSE;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 2;
        desc.Scaling = DXGI_SCALING_STRETCH;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

        HRESULT hr = uiDxgiFactory_->CreateSwapChainForComposition(
            uiD3D11Device_.Get(),
            &desc,
            nullptr,
            aquariumD2DCompositionHudSwapChain_.GetAddressOf());
        if (FAILED(hr) || !aquariumD2DCompositionHudSwapChain_)
        {
            if (error) { *error = hresultToString("CreateSwapChainForComposition D2D HUD", hr); }
            resetAquariumD2DCompositionHud();
            return false;
        }

        Microsoft::WRL::ComPtr<IDXGISurface> surface;
        hr = aquariumD2DCompositionHudSwapChain_->GetBuffer(0, IID_PPV_ARGS(surface.GetAddressOf()));
        if (FAILED(hr) || !surface)
        {
            if (error) { *error = hresultToString("GetBuffer D2D composition HUD", hr); }
            resetAquariumD2DCompositionHud();
            return false;
        }

        const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96.0f,
            96.0f);
        hr = renderTarget_->CreateBitmapFromDxgiSurface(
            surface.Get(),
            &props,
            aquariumD2DCompositionHudTarget_.GetAddressOf());
        if (FAILED(hr) || !aquariumD2DCompositionHudTarget_)
        {
            if (error) { *error = hresultToString("CreateBitmapFromDxgiSurface D2D composition HUD", hr); }
            resetAquariumD2DCompositionHud();
            return false;
        }

        aquariumD2DCompositionHudExtent_ = extent;
        aquariumD2DCompositionHudRect_ = hudRect;
        aquariumD2DCompositionHudCacheValid_ = false;
        aquariumD2DCompositionHudAttached_ = false;
        return true;
    }

    std::wstring AceShellUi::formatCameraSpeed(double speed) const
    {
        std::wostringstream out;
        if (speed < 0.01 || speed >= 10000.0)
        {
            out << std::scientific << std::setprecision(4) << speed;
        }
        else
        {
            out << std::fixed << std::setprecision(speed < 1.0 ? 4 : (speed < 100.0 ? 2 : 1)) << speed;
        }

        std::wstring result = out.str();
        const auto exponent = result.find_first_of(L"eE");
        if (exponent == std::wstring::npos)
        {
            const auto dot = result.find(L'.');
            if (dot != std::wstring::npos)
            {
                while (!result.empty() && result.back() == L'0') result.pop_back();
                if (!result.empty() && result.back() == L'.') result.pop_back();
            }
        }
        return result;
    }

    bool AceShellUi::handleCameraSpeedWheel(float x, float y, int wheelDelta)
    {
        if (!isAquariumSingleHwndViewportPoint(x, y) ||
            shouldAquariumViewportInputDeferToOverlay(x, y))
        {
            return false;
        }

        const double speed = cameraSpeedModel_.applyWheelDelta(wheelDelta, aceMonotonicSeconds());
        aquariumSingleHwndCamera_.SetMoveSpeed(static_cast<float>(speed));
        cameraSpeedFeedbackSeconds_ = 1.15f;
        aquariumEmbeddedViewportStatus_ = L"Camera speed " + formatCameraSpeed(speed) + L" m/s";
        requestParentCompositedViewportHold(10u, L"camera-speed-wheel");
        return true;
    }

    void AceShellUi::openCameraSpeedPopup()
    {
        cameraSpeedPopupOpen_ = true;
        cameraSpeedInputFocused_ = true;
        cameraSpeedInput_.replaceAllText(formatCameraSpeed(cameraSpeedModel_.speed()));
        cameraSpeedInput_.selectAll();
        requestParentCompositedViewportHold(30u, L"camera-speed-popup-open");
        SetFocus(parent_);
    }

    void AceShellUi::closeCameraSpeedPopup(bool commit)
    {
        if (commit && !commitCameraSpeedText())
        {
            return;
        }
        cameraSpeedPopupOpen_ = false;
        cameraSpeedInputFocused_ = false;
        cameraSpeedInput_.setFocused(false);
        requestParentCompositedViewportHold(12u, L"camera-speed-popup-close");
    }

    bool AceShellUi::commitCameraSpeedText()
    {
        std::wstring text = cameraSpeedInput_.text();
        const auto first = text.find_first_not_of(L" \t\r\n");
        const auto last = text.find_last_not_of(L" \t\r\n");
        if (first == std::wstring::npos)
        {
            cameraSpeedInput_.selectAll();
            showToast(L"Camera speed", L"Enter a value from 0.0001 to 100000.", D2DToastKind::Error);
            return false;
        }
        text = text.substr(first, last - first + 1);

        try
        {
            std::size_t consumed = 0;
            const double parsed = std::stod(text, &consumed);
            if (consumed != text.size() || !std::isfinite(parsed))
            {
                throw std::invalid_argument("camera speed");
            }
            const double speed = cameraSpeedModel_.setSpeed(parsed);
            aquariumSingleHwndCamera_.SetMoveSpeed(static_cast<float>(speed));
            cameraSpeedFeedbackSeconds_ = 1.15f;
            aquariumEmbeddedViewportStatus_ = L"Camera speed " + formatCameraSpeed(speed) + L" m/s";
            return true;
        }
        catch (const std::exception&)
        {
            cameraSpeedInput_.selectAll();
            showToast(L"Camera speed", L"Use a finite decimal value from 0.0001 to 100000.", D2DToastKind::Error);
            return false;
        }
    }

    bool AceShellUi::handleCameraSpeedMouseDown(D2DRenderContext& ctx, float x, float y)
    {
        if (cameraSpeedButtonRect_.contains(x, y))
        {
            if (cameraSpeedPopupOpen_) closeCameraSpeedPopup(false);
            else openCameraSpeedPopup();
            return true;
        }

        if (!cameraSpeedPopupOpen_)
        {
            return false;
        }

        if (cameraSpeedInputRect_.contains(x, y))
        {
            cameraSpeedInputFocused_ = true;
            cameraSpeedInput_.setFocused(true);
            cameraSpeedInput_.onMouseDown(ctx, x, y);
            return true;
        }

        if (cameraSpeedPopupRect_.contains(x, y))
        {
            return true;
        }

        closeCameraSpeedPopup(false);
        return false;
    }

    bool AceShellUi::handleCameraSpeedMouseUp(D2DRenderContext& ctx, float x, float y)
    {
        return cameraSpeedInput_.onMouseUp(ctx, x, y);
    }

    bool AceShellUi::handleCameraSpeedMouseMove(D2DRenderContext& ctx, float x, float y)
    {
        return cameraSpeedInput_.onMouseMove(ctx, x, y);
    }

    bool AceShellUi::handleCameraSpeedChar(WPARAM wParam)
    {
        return cameraSpeedInput_.onChar(wParam);
    }

    bool AceShellUi::handleCameraSpeedKeyDown(WPARAM wParam, const D2DKeyboardState& keyboard)
    {
        if (wParam == VK_ESCAPE)
        {
            closeCameraSpeedPopup(false);
            return true;
        }
        if (wParam == VK_RETURN && !keyboard.shift)
        {
            closeCameraSpeedPopup(true);
            return true;
        }
        if (keyboard.ctrl && wParam == 'C')
        {
            std::string ignored;
            cameraSpeedInput_.copySelectionToClipboard(parent_, &ignored);
            return true;
        }
        if (keyboard.ctrl && wParam == 'X')
        {
            std::string ignored;
            cameraSpeedInput_.cutSelectionToClipboard(parent_, &ignored);
            return true;
        }
        if (keyboard.ctrl && wParam == 'V')
        {
            std::string ignored;
            cameraSpeedInput_.pasteFromClipboard(parent_, &ignored);
            return true;
        }
        return cameraSpeedInput_.onKeyDown(wParam, keyboard.ctrl, keyboard.shift);
    }

    void AceShellUi::renderCameraSpeedControl(D2DRenderContext& ctx, UiRect viewportRect)
    {
        if (!ctx.target || viewportRect.empty())
        {
            cameraSpeedButtonRect_ = makeUiRect(0, 0, 0, 0);
            cameraSpeedPopupRect_ = makeUiRect(0, 0, 0, 0);
            cameraSpeedInputRect_ = makeUiRect(0, 0, 0, 0);
            return;
        }

        constexpr float margin = 14.0f;
        constexpr float buttonWidth = 166.0f;
        constexpr float buttonHeight = 38.0f;
        cameraSpeedButtonRect_ = makeUiRect(
            viewportRect.right - margin - buttonWidth,
            viewportRect.top + margin,
            viewportRect.right - margin,
            viewportRect.top + margin + buttonHeight);

        const bool hovered = cameraSpeedButtonRect_.contains(mouseX_, mouseY_);
        D2DWidgetUtils::fillRounded(ctx, cameraSpeedButtonRect_, 8.0f,
            hovered || cameraSpeedPopupOpen_ ? ctx.brushes.panelSoft : ctx.brushes.panelElevated,
            cameraSpeedPopupOpen_ ? ctx.brushes.accent : ctx.brushes.border, 1.0f);

        // Code-native camera glyph: crisp at every DPI and no font-fallback gamble.
        const UiRect body = makeUiRect(cameraSpeedButtonRect_.left + 10.0f, cameraSpeedButtonRect_.top + 11.0f,
            cameraSpeedButtonRect_.left + 31.0f, cameraSpeedButtonRect_.top + 27.0f);
        D2DWidgetUtils::fillRounded(ctx, body, 3.0f, ctx.brushes.accentBlue);
        D2DWidgetUtils::fillRounded(ctx, makeUiRect(body.left + 4.0f, body.top - 3.0f, body.left + 12.0f, body.top + 2.0f),
            2.0f, ctx.brushes.accentBlue);
        if (ctx.brushes.panelDeep)
            ctx.target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(body.left + 13.0f, body.top + 8.0f), 4.0f, 4.0f), ctx.brushes.panelDeep);

        D2DTextLayoutFoundation::Draw(ctx, L"Speed  " + formatCameraSpeed(cameraSpeedModel_.speed()), FontRole::Small,
            makeUiRect(body.right + 8.0f, cameraSpeedButtonRect_.top, cameraSpeedButtonRect_.right - 8.0f, cameraSpeedButtonRect_.bottom),
            ctx.brushes.text, D2DTextOverflowMode::Ellipsis, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        const float progressWidth = static_cast<float>(cameraSpeedModel_.normalizedLogPosition()) * (cameraSpeedButtonRect_.width() - 16.0f);
        D2DWidgetUtils::fillRounded(ctx,
            makeUiRect(cameraSpeedButtonRect_.left + 8.0f, cameraSpeedButtonRect_.bottom - 3.0f,
                cameraSpeedButtonRect_.left + 8.0f + progressWidth, cameraSpeedButtonRect_.bottom - 1.0f),
            1.0f, ctx.brushes.accent);

        if (!cameraSpeedPopupOpen_)
        {
            cameraSpeedPopupRect_ = makeUiRect(0, 0, 0, 0);
            cameraSpeedInputRect_ = makeUiRect(0, 0, 0, 0);
            return;
        }

        constexpr float popupWidth = 300.0f;
        constexpr float popupHeight = 116.0f;
        float popupTop = cameraSpeedButtonRect_.bottom + 8.0f;
        if (popupTop + popupHeight > viewportRect.bottom - margin)
            popupTop = cameraSpeedButtonRect_.top - popupHeight - 8.0f;
        const float popupLeft = std::max(viewportRect.left + margin, cameraSpeedButtonRect_.right - popupWidth);
        cameraSpeedPopupRect_ = makeUiRect(popupLeft, popupTop, popupLeft + popupWidth, popupTop + popupHeight);
        cameraSpeedInputRect_ = makeUiRect(popupLeft + 12.0f, popupTop + 38.0f, popupLeft + popupWidth - 12.0f, popupTop + 80.0f);

        D2DWidgetUtils::fillRounded(ctx, cameraSpeedPopupRect_, 10.0f, ctx.brushes.panelElevated, ctx.brushes.accent, 1.0f);
        D2DTextLayoutFoundation::Draw(ctx, L"Camera speed (m/s)", FontRole::Small,
            makeUiRect(popupLeft + 14.0f, popupTop + 8.0f, popupLeft + popupWidth - 14.0f, popupTop + 34.0f),
            ctx.brushes.text, D2DTextOverflowMode::Ellipsis, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        cameraSpeedInput_.setRect(cameraSpeedInputRect_);
        cameraSpeedInput_.setPlaceholder(L"0.0001 - 100000");
        cameraSpeedInput_.setFocused(cameraSpeedInputFocused_);
        cameraSpeedInput_.render(ctx);
        D2DTextLayoutFoundation::Draw(ctx, L"Enter apply  |  Esc cancel  |  wheel adapts to cadence", FontRole::Small,
            makeUiRect(popupLeft + 14.0f, popupTop + 84.0f, popupLeft + popupWidth - 14.0f, popupTop + 108.0f),
            ctx.brushes.muted, D2DTextOverflowMode::Ellipsis, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    bool AceShellUi::renderAquariumD2DCompositionHud(
        const ace::aquarium_ui::AceAquariumUiSnapshot& snapshot,
        std::string* error)
    {
        if (!aquariumGpuViewportRenderer_ || !renderTarget_ || aquariumEmbeddedViewportRect_.empty())
        {
            if (error) { *error = "D2D composition HUD requires a live viewport renderer."; }
            return false;
        }

        const UiRect hudRect = computeAquariumTelemetryOverlayRect(
            aquariumEmbeddedViewportRect_,
            makeUiRect(0, 0, 0, 0));
        const UiRect previousRect = aquariumD2DCompositionHudRect_;
        if (!ensureAquariumD2DCompositionHud(hudRect, error))
        {
            return false;
        }

        const bool placementChanged =
            previousRect.left != hudRect.left || previousRect.top != hudRect.top ||
            previousRect.right != hudRect.right || previousRect.bottom != hudRect.bottom;
        const bool valuesChanged = !aquariumD2DCompositionHudCacheValid_ ||
            std::fabs(aquariumD2DCompositionHudHydration_ - snapshot.body.hydration) > 0.000001 ||
            std::fabs(aquariumD2DCompositionHudNutrition_ - snapshot.body.nutrition) > 0.000001 ||
            std::fabs(aquariumD2DCompositionHudIntegrity_ - snapshot.body.integrity) > 0.000001;

        if (valuesChanged)
        {
            Microsoft::WRL::ComPtr<ID2D1Image> previousTarget;
            renderTarget_->GetTarget(previousTarget.GetAddressOf());
            D2D1_MATRIX_3X2_F previousTransform{};
            renderTarget_->GetTransform(&previousTransform);

            renderTarget_->SetTarget(aquariumD2DCompositionHudTarget_.Get());
            renderTarget_->SetTransform(D2D1::Matrix3x2F::Identity());
            renderTarget_->SetDpi(96.0f, 96.0f);
            renderTarget_->BeginDraw();
            renderTarget_->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

            auto ctx = makeContext();
            ctx.width = static_cast<float>(aquariumD2DCompositionHudExtent_.width);
            ctx.height = static_cast<float>(aquariumD2DCompositionHudExtent_.height);
            aquariumTelemetryWidgets_.RenderPanel(
                ctx,
                makeUiRect(0.0f, 0.0f, ctx.width, ctx.height),
                snapshot);

            const HRESULT drawHr = renderTarget_->EndDraw();
            renderTarget_->SetTarget(previousTarget.Get());
            renderTarget_->SetTransform(previousTransform);
            applyPixelAlignedD2DTargetDpi();
            ++aquariumD2DCompositionHudDrawCount_;
            if (FAILED(drawHr))
            {
                if (error) { *error = hresultToString("ID2D1DeviceContext::EndDraw composition HUD", drawHr); }
                resetAquariumD2DCompositionHud();
                return false;
            }

            const HRESULT presentHr = aquariumD2DCompositionHudSwapChain_->Present(0, DXGI_PRESENT_DO_NOT_WAIT);
            if (presentHr == DXGI_ERROR_WAS_STILL_DRAWING)
            {
                aquariumD2DCompositionHudCacheValid_ = false;
            }
            else if (FAILED(presentHr))
            {
                if (error) { *error = hresultToString("IDXGISwapChain1::Present composition HUD", presentHr); }
                resetAquariumD2DCompositionHud();
                return false;
            }
            else
            {
                ++aquariumD2DCompositionHudPresentCount_;
                aquariumD2DCompositionHudHydration_ = snapshot.body.hydration;
                aquariumD2DCompositionHudNutrition_ = snapshot.body.nutrition;
                aquariumD2DCompositionHudIntegrity_ = snapshot.body.integrity;
                aquariumD2DCompositionHudCacheValid_ = true;
            }
        }

        if (!aquariumD2DCompositionHudAttached_ || placementChanged)
        {
            if (!aquariumGpuViewportRenderer_->setCompositionOverlay(
                    aquariumD2DCompositionHudSwapChain_.Get(),
                    hudRect.left,
                    hudRect.top,
                    aquariumD2DCompositionHudExtent_,
                    error))
            {
                return false;
            }
            aquariumD2DCompositionHudAttached_ = true;
        }

        aquariumTelemetryOverlayRect_ = hudRect;
        return true;
    }

    void AceShellUi::resetAquariumD2DCompositionHud()
    {
        aquariumD2DCompositionHudTarget_.Reset();
        aquariumD2DCompositionHudSwapChain_.Reset();
        aquariumD2DCompositionHudExtent_ = {};
        aquariumD2DCompositionHudRect_ = {};
        aquariumD2DCompositionHudAttached_ = false;
        aquariumD2DCompositionHudCacheValid_ = false;
    }

    void AceShellUi::resetAquariumDirectCompositionIfActive()
    {
        if (!aquariumDirectCompositionActive_)
        {
            return;
        }

        if (aquariumGpuViewportRenderer_)
        {
            aquariumGpuViewportRenderer_->resetCompositionHost();
        }
        resetAquariumD2DCompositionHud();
        aquariumDirectCompositionActive_ = false;
    }

    bool AceShellUi::AquariumViewportCacheKey::operator==(const AquariumViewportCacheKey& other) const
    {
        return width == other.width &&
            height == other.height &&
            step == other.step &&
            cameraX == other.cameraX &&
            cameraY == other.cameraY &&
            cameraZ == other.cameraZ &&
            yaw == other.yaw &&
            pitch == other.pitch &&
            debugTruth == other.debugTruth &&
            scenario == other.scenario;
    }

    AceShellUi::AquariumViewportCacheKey AceShellUi::makeAquariumViewportCacheKey(
        am::renderer::rhi::U32 width,
        am::renderer::rhi::U32 height,
        bool debugTruth) const
    {
        auto quantize = [](float value, float scale) -> int
        {
            return static_cast<int>(std::lround(value * scale));
        };

        const auto pos = aquariumSingleHwndCamera_.Position();
        AquariumViewportCacheKey key{};
        key.width = width;
        key.height = height;
        key.step = aquariumController_.StepIndex();
        key.cameraX = quantize(pos.x, 1000.0f);
        key.cameraY = quantize(pos.y, 1000.0f);
        key.cameraZ = quantize(pos.z, 1000.0f);
        key.yaw = quantize(aquariumSingleHwndCamera_.Yaw(), 100000.0f);
        key.pitch = quantize(aquariumSingleHwndCamera_.Pitch(), 100000.0f);
        key.debugTruth = debugTruth;
        key.scenario = aquariumController_.CurrentScenarioName();
        return key;
    }

    bool AceShellUi::canReuseAquariumViewportBitmap(const AquariumViewportCacheKey& key) const
    {
        return aquariumViewportCacheKeyValid_ &&
            aquariumSlateViewportBitmap_ &&
            aquariumSlateViewportBitmapExtent_.width == key.width &&
            aquariumSlateViewportBitmapExtent_.height == key.height &&
            aquariumViewportCacheKey_ == key &&
            !aquariumController_.IsRunning() &&
            !windowLiveResizeActive_;
    }

    void AceShellUi::resetAquariumD2DTextureBridgeResourceObjects()
    {
        aquariumBridgeSharedBitmap_.Reset();
        aquariumBridgeSurfaceBitmap_.Reset();
        aquariumBridgeInteropSharedTexture_.Reset();
        aquariumBridgeUiSharedTexture_.Reset();
        for (auto& slot : aquariumBridgeSharedSlots_)
        {
            slot.surfaceBitmap.Reset();
            slot.interopMutex.Reset();
            slot.uiMutex.Reset();
            slot.interopTexture.Reset();
            slot.uiTexture.Reset();
            // IDXGIResource::GetSharedHandle returns a legacy DXGI resource
            // handle, not an NT handle. It is tied to the shared texture
            // lifetime and must not be closed with CloseHandle. Tiny API
            // landmine, naturally.
            slot.sharedHandle = nullptr;
            slot.ready = false;
            slot.keyedMutex = false;
        }
        aquariumBridgeWrappedResource_.Reset();
        // IDXGIResource::GetSharedHandle returns a legacy DXGI resource handle,
        // not an NT handle. It is tied to the shared texture lifetime and must
        // not be closed with CloseHandle. Tiny API landmine, naturally.
        aquariumBridgeSharedHandle_ = nullptr;
        aquariumBridgeNativeResource_ = nullptr;
        aquariumBridgeBitmapExtent_ = {};
        aquariumBridgeUsesSharedIntermediate_ = false;
        aquariumBridgeSharedWriteIndex_ = 0;
        aquariumBridgeSharedReadyIndex_ = 0;
        aquariumBridgeSharedHasReadyFrame_ = false;
        d2dViewportBridgeRuntime_.Reset();
        d2dViewportCopyScheduler_.Reset();
        d2dViewportTextureCache_.Reset();
    }

    void AceShellUi::resetAquariumD2DTextureBridge()
    {
        resetAquariumD2DTextureBridgeResourceObjects();
        aquariumD2DBridgeSurfaceDiagnostics_.clear();
        aquariumD2DBridgeDisabled_ = false;
    }

    void AceShellUi::setAquariumD2DBridgeFatalError(const std::string& error)
    {
        aquariumD2DBridgeLastError_ = error.empty() ? "D2D DeviceContext viewport bridge failed." : error;
        aquariumD2DBridgeFatalStep_ = aquariumD2DBridgeLastError_;
        aquariumD2DBridgeFatalHresult_.clear();

        const std::string failedMarker = " failed.";
        const std::size_t failedPos = aquariumD2DBridgeLastError_.find(failedMarker);
        if (failedPos != std::string::npos)
        {
            aquariumD2DBridgeFatalStep_ = aquariumD2DBridgeLastError_.substr(0, failedPos);
        }

        const std::string hrMarker = "HRESULT=";
        const std::size_t hrPos = aquariumD2DBridgeLastError_.find(hrMarker);
        if (hrPos != std::string::npos)
        {
            aquariumD2DBridgeFatalHresult_ = aquariumD2DBridgeLastError_.substr(hrPos + hrMarker.size());
        }
    }

    bool AceShellUi::ensureAquariumD2DInteropDevices(std::string* error)
    {
        // ACE-VTBRIDGE4: the UI target itself is now ID2D1DeviceContext-backed.
        // This helper remains as a validator-stable interop checkpoint, but it no
        // longer creates a second D2D context and no longer tries to share a bitmap
        // back into a legacy ID2D1HwndRenderTarget. CreateSharedBitmap is a banned
        // bridge endpoint here, not a fallback strategy wearing a fake mustache.
        // VTBRIDGE3 probe breadcrumb only: __uuidof(ID2D1Bitmap),
        // CreateSharedBitmap from D2D device-context bitmap, D3D11On12SharedBitmap.
        if (!renderTarget_)
        {
            if (error) { *error = "D2D DeviceContext viewport bridge requires the modern UI DeviceContext target."; }
            return false;
        }
        return true;
    }

    bool AceShellUi::ensureAquariumD2DTextureBridge(const am::renderer::rhi::AceViewportTextureResource& resource, std::string* error)
    {
        if (!renderTarget_ || !uiD3D11Device_ || !aquariumGpuViewportRenderer_ || !resource.nativeResource || resource.extent.width == 0 || resource.extent.height == 0)
        {
            if (error) { *error = "D2D DeviceContext viewport bridge requires live UI DeviceContext/D3D11 target and native DX12 viewport texture."; }
            return false;
        }

        auto* d3d12Device = static_cast<ID3D12Device*>(aquariumGpuViewportRenderer_->nativeD3D12Device());
        auto* d3d12Queue = static_cast<ID3D12CommandQueue*>(aquariumGpuViewportRenderer_->nativeD3D12GraphicsQueue());
        auto* d3d12Resource = static_cast<ID3D12Resource*>(resource.nativeResource);
        if (!d3d12Device || !d3d12Queue || !d3d12Resource)
        {
            if (error) { *error = "D2D DeviceContext viewport bridge missing native D3D12 device/queue/resource."; }
            return false;
        }

        if (!aquariumBridgeD3D11On12_)
        {
            UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
            flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
            IUnknown* queues[] = { static_cast<IUnknown*>(d3d12Queue) };
            HRESULT hr = D3D11On12CreateDevice(
                d3d12Device,
                flags,
                nullptr,
                0,
                queues,
                1,
                0,
                aquariumBridgeD3D11Device_.GetAddressOf(),
                aquariumBridgeD3D11Context_.GetAddressOf(),
                nullptr);
#if defined(_DEBUG)
            if (FAILED(hr))
            {
                flags &= ~D3D11_CREATE_DEVICE_DEBUG;
                hr = D3D11On12CreateDevice(
                    d3d12Device,
                    flags,
                    nullptr,
                    0,
                    queues,
                    1,
                    0,
                    aquariumBridgeD3D11Device_.GetAddressOf(),
                    aquariumBridgeD3D11Context_.GetAddressOf(),
                    nullptr);
            }
#endif
            if (FAILED(hr) || !aquariumBridgeD3D11Device_)
            {
                if (error) { *error = hresultToString("D3D11On12CreateDevice viewport bridge", hr); }
                aquariumBridgeD3D11Device_.Reset();
                aquariumBridgeD3D11Context_.Reset();
                aquariumBridgeD3D11On12_.Reset();
                return false;
            }

            hr = aquariumBridgeD3D11Device_.As(&aquariumBridgeD3D11On12_);
            if (FAILED(hr) || !aquariumBridgeD3D11On12_)
            {
                if (error) { *error = hresultToString("Query ID3D11On12Device viewport bridge", hr); }
                aquariumBridgeD3D11Device_.Reset();
                aquariumBridgeD3D11Context_.Reset();
                aquariumBridgeD3D11On12_.Reset();
                return false;
            }
        }

        if (!ensureAquariumD2DInteropDevices(error))
        {
            return false;
        }

        const bool resourceChanged = aquariumBridgeNativeResource_ != resource.nativeResource ||
            aquariumBridgeBitmapExtent_.width != resource.extent.width ||
            aquariumBridgeBitmapExtent_.height != resource.extent.height ||
            !aquariumBridgeWrappedResource_ ||
            !aquariumBridgeSurfaceBitmap_;
        if (!resourceChanged)
        {
            return true;
        }

        resetAquariumD2DTextureBridgeResourceObjects();

        std::ostringstream diagnostics;
        diagnostics << "vtbridge4r1";
        const D3D12_RESOURCE_DESC d3d12Desc = d3d12Resource->GetDesc();
        diagnostics << ";" << aceD3D12ResourceDescText(d3d12Desc);

        D3D11_RESOURCE_FLAGS wrappedFlags{};
        wrappedFlags.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        HRESULT hr = aquariumBridgeD3D11On12_->CreateWrappedResource(
            d3d12Resource,
            &wrappedFlags,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            IID_PPV_ARGS(aquariumBridgeWrappedResource_.GetAddressOf()));
        if (FAILED(hr) || !aquariumBridgeWrappedResource_)
        {
            diagnostics << ";wrap=" << hresultToString("CreateWrappedResource viewport bridge", hr);
            aquariumD2DBridgeSurfaceDiagnostics_ = diagnostics.str();
            if (error) { *error = diagnostics.str(); }
            resetAquariumD2DTextureBridgeResourceObjects();
            return false;
        }

        Microsoft::WRL::ComPtr<IDXGISurface> directSurface;
        hr = aquariumBridgeWrappedResource_.As(&directSurface);
        if (FAILED(hr) || !directSurface)
        {
            diagnostics << ";direct_surface=" << hresultToString("Query IDXGISurface viewport bridge", hr);
            aquariumD2DBridgeSurfaceDiagnostics_ = diagnostics.str();
            if (error) { *error = diagnostics.str(); }
            resetAquariumD2DTextureBridgeResourceObjects();
            return false;
        }

        DXGI_SURFACE_DESC directSurfaceDesc{};
        hr = directSurface->GetDesc(&directSurfaceDesc);
        if (SUCCEEDED(hr))
        {
            diagnostics << ";" << aceDxgiSurfaceDescText(directSurfaceDesc);
        }
        else
        {
            diagnostics << ";direct_getdesc=" << hresultToString("IDXGISurface::GetDesc viewport bridge", hr);
        }

        // ACE-VTBRIDGE4 validator breadcrumb: ID2D1DeviceContext::CreateBitmapFromDxgiSurface viewport bridge.
        auto tryCreateD2DBitmap = [&](IDXGISurface* surface, const D2D1_BITMAP_PROPERTIES1& props, const char* label, Microsoft::WRL::ComPtr<ID2D1Bitmap1>& outBitmap) -> HRESULT
        {
            outBitmap.Reset();
            const HRESULT bitmapHr = renderTarget_->CreateBitmapFromDxgiSurface(surface, &props, outBitmap.GetAddressOf());
            diagnostics << ";" << label << "=0x" << std::hex << static_cast<unsigned long>(bitmapHr) << std::dec;
            if (SUCCEEDED(bitmapHr) && outBitmap)
            {
                diagnostics << "(ok)";
            }
            return bitmapHr;
        };

        Microsoft::WRL::ComPtr<ID2D1Bitmap1> directBitmap;
        const D2D1_BITMAP_PROPERTIES1 directPropsExact = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_NONE,
            D2D1::PixelFormat(directSurfaceDesc.Format == DXGI_FORMAT_UNKNOWN ? DXGI_FORMAT_B8G8R8A8_UNORM : directSurfaceDesc.Format, D2D1_ALPHA_MODE_IGNORE),
            96.0f,
            96.0f);
        hr = tryCreateD2DBitmap(directSurface.Get(), directPropsExact, "direct_exact_ignore", directBitmap);
        if (FAILED(hr))
        {
            const D2D1_BITMAP_PROPERTIES1 directPropsUnknown = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_NONE,
                D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE),
                96.0f,
                96.0f);
            hr = tryCreateD2DBitmap(directSurface.Get(), directPropsUnknown, "direct_unknown_ignore", directBitmap);
        }
        if (FAILED(hr))
        {
            const D2D1_BITMAP_PROPERTIES1 directPropsPremul = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_NONE,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                96.0f,
                96.0f);
            hr = tryCreateD2DBitmap(directSurface.Get(), directPropsPremul, "direct_bgra_premul", directBitmap);
        }
        if (SUCCEEDED(hr) && directBitmap)
        {
            aquariumBridgeSurfaceBitmap_ = directBitmap;
            aquariumBridgeNativeResource_ = resource.nativeResource;
            aquariumBridgeBitmapExtent_ = resource.extent;
            aquariumBridgeUsesSharedIntermediate_ = false;
            aquariumD2DBridgeSurfaceDiagnostics_ = diagnostics.str() + ";selected=direct_wrapped_surface";
            return true;
        }

        // ACE-VTBRIDGE5: Direct2D refused the D3D11On12 wrapped DXGI surface.
        // Keep the R1 GPU-only shared D3D11 intermediate. Prefer an explicit keyed
        // writer/reader handoff so the current GPU frame can be sampled coherently;
        // if the driver cannot expose keyed mutexes, fall back to the double-buffered
        // last-good texture path with one frame of latency.
        AceD2DViewportBridgePolicyInput bridgePolicyInput{};
        bridgePolicyInput.width = static_cast<std::uint32_t>(resource.extent.width);
        bridgePolicyInput.height = static_cast<std::uint32_t>(resource.extent.height);
        bridgePolicyInput.format = directSurfaceDesc.Format == DXGI_FORMAT_UNKNOWN ? DXGI_FORMAT_B8G8R8A8_UNORM : directSurfaceDesc.Format;
        bridgePolicyInput.flipModelSwapChain = uiSwapChain_ != nullptr;
        bridgePolicyInput.fullFrameRedraw = d2dFrameCompositor_.RequiresFullFrameRedraw();
        bridgePolicyInput.localOverlayActive = isViewportLocalOverlayActive();
        bridgePolicyInput.liveResize = windowLiveResizeActive_ || aquariumResizeQuarantineActive_;
        bridgePolicyInput.directSurfaceRejected = true;
        bridgePolicyInput.frameNumber = d2dFrameCompositor_.Stats().frameNumber;
        bridgePolicyInput.previousContentionCount = aquariumD2DBridgeSharedMutexContentionCount_;
        const AceD2DViewportBridgePolicyDecision bridgeDecision = d2dViewportBridgePolicy_.Decide(bridgePolicyInput);
        diagnostics << ";" << bridgeDecision.reason;

        D3D11_TEXTURE2D_DESC sharedDesc{};
        sharedDesc.Width = std::max<UINT>(1u, static_cast<UINT>(resource.extent.width));
        sharedDesc.Height = std::max<UINT>(1u, static_cast<UINT>(resource.extent.height));
        sharedDesc.MipLevels = 1;
        sharedDesc.ArraySize = 1;
        sharedDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        sharedDesc.SampleDesc.Count = 1;
        sharedDesc.SampleDesc.Quality = 0;
        sharedDesc.Usage = D3D11_USAGE_DEFAULT;
        sharedDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        sharedDesc.CPUAccessFlags = 0;
        sharedDesc.MiscFlags = bridgeDecision.miscFlag;
        diagnostics << ";intermediate_desc=" << aceTexture2DDescText(sharedDesc)
            << ";shared_buffer_count=" << kAquariumD2DSharedBridgeSlotCount;

        auto createSharedSlot = [&](std::size_t slotIndex, bool keyedMutex) -> HRESULT
        {
            auto& slot = aquariumBridgeSharedSlots_[slotIndex];
            slot = AquariumD2DSharedBridgeSlot{};

            D3D11_TEXTURE2D_DESC slotDesc = sharedDesc;
            slotDesc.MiscFlags = keyedMutex ? D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX : D3D11_RESOURCE_MISC_SHARED;

            HRESULT slotHr = uiD3D11Device_->CreateTexture2D(&slotDesc, nullptr, slot.uiTexture.GetAddressOf());
            if (FAILED(slotHr) || !slot.uiTexture)
            {
                diagnostics << ";slot" << slotIndex << "_ui_shared_create="
                    << hresultToString("ID3D11Device::CreateTexture2D UI shared viewport bridge", slotHr);
                return slotHr;
            }

            Microsoft::WRL::ComPtr<IDXGIResource> uiDxgiResource;
            slotHr = slot.uiTexture.As(&uiDxgiResource);
            if (FAILED(slotHr) || !uiDxgiResource)
            {
                diagnostics << ";slot" << slotIndex << "_ui_shared_query_dxgi="
                    << hresultToString("Query IDXGIResource UI shared viewport bridge", slotHr);
                return slotHr;
            }

            slotHr = uiDxgiResource->GetSharedHandle(&slot.sharedHandle);
            if (FAILED(slotHr) || !slot.sharedHandle)
            {
                diagnostics << ";slot" << slotIndex << "_ui_shared_handle="
                    << hresultToString("IDXGIResource::GetSharedHandle viewport bridge", slotHr);
                return slotHr;
            }

            slotHr = aquariumBridgeD3D11Device_->OpenSharedResource(
                slot.sharedHandle,
                IID_PPV_ARGS(slot.interopTexture.GetAddressOf()));
            if (FAILED(slotHr) || !slot.interopTexture)
            {
                diagnostics << ";slot" << slotIndex << "_interop_open_shared="
                    << hresultToString("ID3D11Device::OpenSharedResource viewport bridge", slotHr);
                return slotHr;
            }

            if (keyedMutex)
            {
                const HRESULT uiMutexHr = slot.uiTexture.As(&slot.uiMutex);
                const HRESULT interopMutexHr = slot.interopTexture.As(&slot.interopMutex);
                slot.keyedMutex = SUCCEEDED(uiMutexHr) && slot.uiMutex && SUCCEEDED(interopMutexHr) && slot.interopMutex;
                diagnostics << ";slot" << slotIndex << "_keyed_mutex="
                    << (slot.keyedMutex ? "ok" : "unavailable")
                    << "(ui=0x" << std::hex << static_cast<unsigned long>(uiMutexHr)
                    << ",interop=0x" << static_cast<unsigned long>(interopMutexHr) << std::dec << ")";
            }
            else
            {
                slot.keyedMutex = false;
                diagnostics << ";slot" << slotIndex << "_keyed_mutex=disabled";
            }

            Microsoft::WRL::ComPtr<IDXGISurface> uiSurface;
            slotHr = slot.uiTexture.As(&uiSurface);
            if (FAILED(slotHr) || !uiSurface)
            {
                diagnostics << ";slot" << slotIndex << "_ui_shared_surface="
                    << hresultToString("Query IDXGISurface UI shared viewport bridge", slotHr);
                return slotHr;
            }

            if (slotIndex == 0)
            {
                DXGI_SURFACE_DESC uiSurfaceDesc{};
                const HRESULT descHr = uiSurface->GetDesc(&uiSurfaceDesc);
                if (SUCCEEDED(descHr))
                {
                    diagnostics << ";ui_" << aceDxgiSurfaceDescText(uiSurfaceDesc);
                }
                else
                {
                    diagnostics << ";ui_getdesc=" << hresultToString("IDXGISurface::GetDesc UI shared viewport bridge", descHr);
                }
            }

            const D2D1_BITMAP_PROPERTIES1 uiSurfaceProps = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_NONE,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
                96.0f,
                96.0f);
            slotHr = tryCreateD2DBitmap(uiSurface.Get(), uiSurfaceProps,
                slotIndex == 0 ? "ui_shared_bgra_ignore" : "ui_shared_bgra_ignore_slot", slot.surfaceBitmap);
            if (FAILED(slotHr) || !slot.surfaceBitmap)
            {
                const D2D1_BITMAP_PROPERTIES1 uiSurfaceUnknownProps = D2D1::BitmapProperties1(
                    D2D1_BITMAP_OPTIONS_NONE,
                    D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE),
                    96.0f,
                    96.0f);
                slotHr = tryCreateD2DBitmap(uiSurface.Get(), uiSurfaceUnknownProps,
                    slotIndex == 0 ? "ui_shared_unknown_ignore" : "ui_shared_unknown_ignore_slot", slot.surfaceBitmap);
            }
            if (FAILED(slotHr) || !slot.surfaceBitmap)
            {
                diagnostics << ";slot" << slotIndex << "_bitmap=failed";
                return slotHr;
            }

            slot.ready = false;
            return S_OK;
        };

        bool slotsReady = true;
        bool keyedSlots = bridgeDecision.allowKeyedMutex;
        for (std::size_t i = 0; i < kAquariumD2DSharedBridgeSlotCount; ++i)
        {
            hr = createSharedSlot(i, bridgeDecision.allowKeyedMutex);
            if (FAILED(hr))
            {
                slotsReady = false;
                break;
            }
            keyedSlots = keyedSlots && aquariumBridgeSharedSlots_[i].keyedMutex;
        }

        if ((FAILED(hr) || !slotsReady || (bridgeDecision.allowKeyedMutex && !keyedSlots)) && bridgeDecision.allowKeyedMutex)
        {
            for (auto& slot : aquariumBridgeSharedSlots_)
            {
                slot = AquariumD2DSharedBridgeSlot{};
            }
            sharedDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
            diagnostics << ";keyed_mutex_experimental_failed_try_flush_only_ring=1";
            slotsReady = true;
            keyedSlots = false;
            for (std::size_t i = 0; i < kAquariumD2DSharedBridgeSlotCount; ++i)
            {
                hr = createSharedSlot(i, false);
                if (FAILED(hr))
                {
                    slotsReady = false;
                    break;
                }
            }
        }

        if (FAILED(hr) || !slotsReady)
        {
            diagnostics << ";selected=failed";
            aquariumD2DBridgeSurfaceDiagnostics_ = diagnostics.str();
            d2dViewportBridgePolicy_.RecordFailure(bridgeDecision, hr);
            if (error)
            {
                *error = hresultToString("Create shared D3D11 texture ring for D2D viewport bridge", hr) +
                    " diagnostics=" + aquariumD2DBridgeSurfaceDiagnostics_;
            }
            resetAquariumD2DTextureBridgeResourceObjects();
            return false;
        }

        aquariumBridgeUiSharedTexture_ = aquariumBridgeSharedSlots_[0].uiTexture;
        aquariumBridgeInteropSharedTexture_ = aquariumBridgeSharedSlots_[0].interopTexture;
        aquariumBridgeSurfaceBitmap_ = aquariumBridgeSharedSlots_[0].surfaceBitmap;
        aquariumBridgeSharedHandle_ = aquariumBridgeSharedSlots_[0].sharedHandle;
        aquariumBridgeNativeResource_ = resource.nativeResource;
        aquariumBridgeBitmapExtent_ = resource.extent;
        aquariumBridgeUsesSharedIntermediate_ = true;
        aquariumBridgeSharedWriteIndex_ = 0;
        aquariumBridgeSharedReadyIndex_ = 0;
        aquariumBridgeSharedHasReadyFrame_ = false;
        ++aquariumD2DBridgeSharedBitmapRecreateCount_;
        aquariumD2DBridgeSharedBufferCount_ = kAquariumD2DSharedBridgeSlotCount;
        d2dViewportBridgePolicy_.RecordSuccess(bridgeDecision);
        aquariumD2DBridgeSurfaceDiagnostics_ = diagnostics.str() +
            ";selected=" + bridgeDecision.selectedPath +
            (keyedSlots ? ";keyed_mutex=experimental_active" : ";keyed_mutex=disabled_default") +
            ";bridge_policy=" + d2dViewportBridgePolicy_.Diagnostics();
        return true;
    }

    bool AceShellUi::drawAquariumGpuTextureWithD2DDeviceContext(
        D2DRenderContext& ctx,
        UiRect rect,
        const am::renderer::scene::AceAquariumGpuViewportSnapshot& snapshot,
        const AquariumViewportCacheKey& key,
        std::string* error)
    {
        ++aquariumD2DBridgeAttemptCount_;
        if (!ctx.target || rect.empty() || !snapshot.valid || !snapshot.d2dTextureBridgeReady)
        {
            if (error) { *error = "D2D DeviceContext bridge draw requires a valid GPU-texture snapshot."; }
            return false;
        }
        if (!ensureAquariumD2DTextureBridge(snapshot.viewportTexture, error))
        {
            if (error && !error->empty()) { setAquariumD2DBridgeFatalError(*error); }
            return false;
        }

        ID3D11Resource* wrapped[] = { aquariumBridgeWrappedResource_.Get() };
        if (aquariumBridgeD3D11On12_ && wrapped[0])
        {
            aquariumBridgeD3D11On12_->AcquireWrappedResources(wrapped, 1);
        }

        ID2D1Bitmap1* bitmapToDraw = aquariumBridgeSurfaceBitmap_.Get();
        bool sharedDraw = false;
        bool sharedKeyed = false;
        std::size_t writeIndex = aquariumBridgeSharedWriteIndex_ % kAquariumD2DSharedBridgeSlotCount;
        std::size_t drawIndex = aquariumBridgeSharedHasReadyFrame_ ?
            (aquariumBridgeSharedReadyIndex_ % kAquariumD2DSharedBridgeSlotCount) : writeIndex;
        AceD2DViewportBridgeFrameDecision bridgeRuntimeDecision{};
        AceD2DViewportCopySchedule bridgeCopySchedule{};
        const AceD2DViewportTextureKey textureCacheKey = AceD2DViewportTextureKeyFromResource(
            snapshot.viewportTexture,
            static_cast<std::uint32_t>(DXGI_FORMAT_B8G8R8A8_UNORM),
            aquariumD2DBridgeSharedBitmapRecreateCount_,
            aquariumD2DBridgeSharedBitmapRecreateCount_);

        if (aquariumBridgeUsesSharedIntermediate_)
        {
            if (!aquariumBridgeD3D11Context_ || kAquariumD2DSharedBridgeSlotCount == 0)
            {
                if (aquariumBridgeD3D11On12_ && wrapped[0])
                {
                    aquariumBridgeD3D11On12_->ReleaseWrappedResources(wrapped, 1);
                }
                if (error) { *error = "D2D DeviceContext shared intermediate bridge missing D3D11 copy context."; }
                return false;
            }

            AceD2DViewportBridgeFrameInput bridgeRuntimeInput{};
            bridgeRuntimeInput.frameNumber = d2dFrameCompositor_.Stats().frameNumber;
            bridgeRuntimeInput.resourceEpoch = aquariumD2DBridgeSharedBitmapRecreateCount_;
            bridgeRuntimeInput.width = snapshot.viewportTexture.extent.width;
            bridgeRuntimeInput.height = snapshot.viewportTexture.extent.height;
            bridgeRuntimeInput.fullFrameRedraw = d2dFrameCompositor_.RequiresFullFrameRedraw();
            bridgeRuntimeInput.liveResize = windowLiveResizeActive_ || aquariumResizeQuarantineActive_;
            bridgeRuntimeInput.bridgeUsesSharedIntermediate = true;
            bridgeRuntimeInput.keyedMutexEnabled = aquariumBridgeSharedSlots_[writeIndex].keyedMutex;
            bridgeRuntimeInput.firstFrameMayUseSameSlot = true;
            bridgeRuntimeInput.slotCount = static_cast<std::uint32_t>(kAquariumD2DSharedBridgeSlotCount);
            bridgeRuntimeDecision = d2dViewportBridgeRuntime_.BeginFrame(bridgeRuntimeInput);
            if (bridgeRuntimeDecision.valid)
            {
                writeIndex = bridgeRuntimeDecision.writeSlot % kAquariumD2DSharedBridgeSlotCount;
                drawIndex = bridgeRuntimeDecision.drawSlot % kAquariumD2DSharedBridgeSlotCount;
            }
            AceD2DViewportCopyScheduleInput copyScheduleInput{};
            copyScheduleInput.bridgeDecision = bridgeRuntimeDecision;
            copyScheduleInput.textureKey = textureCacheKey;
            copyScheduleInput.fullFrame = d2dFrameCompositor_.RequiresFullFrameRedraw();
            copyScheduleInput.liveResize = windowLiveResizeActive_ || aquariumResizeQuarantineActive_;
            copyScheduleInput.allowOneFrameLatency = !bridgeRuntimeInput.keyedMutexEnabled;
            copyScheduleInput.keyedMutexEnabled = bridgeRuntimeInput.keyedMutexEnabled;
            copyScheduleInput.frameNumber = d2dFrameCompositor_.Stats().frameNumber;
            copyScheduleInput.resizeEpoch = aquariumD2DBridgeSharedBitmapRecreateCount_;
            bridgeCopySchedule = d2dViewportCopyScheduler_.BuildSchedule(copyScheduleInput);

            auto* writeSlot = &aquariumBridgeSharedSlots_[writeIndex];
            if (!writeSlot->interopTexture || !writeSlot->surfaceBitmap)
            {
                if (aquariumBridgeD3D11On12_ && wrapped[0])
                {
                    aquariumBridgeD3D11On12_->ReleaseWrappedResources(wrapped, 1);
                }
                if (error) { *error = "D2D DeviceContext shared intermediate bridge missing ring-buffer slot."; }
                return false;
            }

            sharedKeyed = writeSlot->keyedMutex && writeSlot->interopMutex && writeSlot->uiMutex;
            bool writerMutexHeld = false;
            if (sharedKeyed)
            {
                const HRESULT acquireHr = writeSlot->interopMutex->AcquireSync(0, 2);
                ++aquariumD2DBridgeSharedMutexAcquireCount_;
                if (acquireHr == WAIT_TIMEOUT)
                {
                    ++aquariumD2DBridgeSharedMutexContentionCount_;
                    const std::size_t alternateIndex = (writeIndex + 1) % kAquariumD2DSharedBridgeSlotCount;
                    auto* alternateSlot = &aquariumBridgeSharedSlots_[alternateIndex];
                    if (alternateSlot->keyedMutex && alternateSlot->interopMutex && alternateSlot->uiMutex && alternateSlot->interopTexture && alternateSlot->surfaceBitmap)
                    {
                        const HRESULT alternateAcquireHr = alternateSlot->interopMutex->AcquireSync(0, 2);
                        ++aquariumD2DBridgeSharedMutexAcquireCount_;
                        if (SUCCEEDED(alternateAcquireHr))
                        {
                            writeIndex = alternateIndex;
                            writeSlot = alternateSlot;
                            writerMutexHeld = true;
                        }
                        else
                        {
                            if (aquariumBridgeD3D11On12_ && wrapped[0])
                            {
                                aquariumBridgeD3D11On12_->ReleaseWrappedResources(wrapped, 1);
                            }
                            if (error) { *error = hresultToString("IDXGIKeyedMutex::AcquireSync D2D shared viewport bridge", alternateAcquireHr); }
                            return false;
                        }
                    }
                    else
                    {
                        if (aquariumBridgeD3D11On12_ && wrapped[0])
                        {
                            aquariumBridgeD3D11On12_->ReleaseWrappedResources(wrapped, 1);
                        }
                        if (error) { *error = hresultToString("IDXGIKeyedMutex::AcquireSync D2D shared viewport bridge", acquireHr); }
                        return false;
                    }
                }
                else if (FAILED(acquireHr))
                {
                    if (aquariumBridgeD3D11On12_ && wrapped[0])
                    {
                        aquariumBridgeD3D11On12_->ReleaseWrappedResources(wrapped, 1);
                    }
                    if (error) { *error = hresultToString("IDXGIKeyedMutex::AcquireSync D2D shared viewport bridge", acquireHr); }
                    return false;
                }
                else
                {
                    writerMutexHeld = true;
                }
            }

            d2dViewportBridgeRuntime_.MarkCopyStarted(writeIndex);
            aquariumBridgeD3D11Context_->CopyResource(writeSlot->interopTexture.Get(), aquariumBridgeWrappedResource_.Get());
            d2dViewportBridgeRuntime_.MarkCopyCompleted(writeIndex);
            ++aquariumD2DBridgeSharedCopyCount_;

            if (aquariumBridgeD3D11On12_ && wrapped[0])
            {
                aquariumBridgeD3D11On12_->ReleaseWrappedResources(wrapped, 1);
                wrapped[0] = nullptr;
            }
            aquariumBridgeD3D11Context_->Flush();

            if (writerMutexHeld && writeSlot->interopMutex)
            {
                const HRESULT releaseHr = writeSlot->interopMutex->ReleaseSync(1);
                if (FAILED(releaseHr))
                {
                    if (error) { *error = hresultToString("IDXGIKeyedMutex::ReleaseSync D2D shared viewport bridge", releaseHr); }
                    return false;
                }
            }

            writeSlot->ready = true;
            // Keyed slots are synchronized as a write-then-read handoff in the
            // same frame: writer releases key 1, UI acquires key 1, D2D draws,
            // then UI releases key 0 for the next writer. Non-keyed slots keep
            // the one-frame-late ring path so D2D does not sample the texture
            // being updated by CopyResource this frame.
            if (sharedKeyed || (!aquariumBridgeSharedHasReadyFrame_ && !bridgeRuntimeDecision.valid))
            {
                drawIndex = writeIndex;
            }
            auto* drawSlot = &aquariumBridgeSharedSlots_[drawIndex];
            if (!drawSlot->ready || !drawSlot->surfaceBitmap)
            {
                drawIndex = writeIndex;
                drawSlot = writeSlot;
            }

            bool readerMutexHeld = false;
            if (drawSlot->keyedMutex && drawSlot->uiMutex)
            {
                const HRESULT readAcquireHr = drawSlot->uiMutex->AcquireSync(1, 2);
                ++aquariumD2DBridgeSharedMutexAcquireCount_;
                if (readAcquireHr == WAIT_TIMEOUT)
                {
                    ++aquariumD2DBridgeSharedMutexContentionCount_;
                    if (error) { *error = hresultToString("IDXGIKeyedMutex::AcquireSync UI D2D shared viewport bridge", readAcquireHr); }
                    return false;
                }
                if (FAILED(readAcquireHr))
                {
                    if (error) { *error = hresultToString("IDXGIKeyedMutex::AcquireSync UI D2D shared viewport bridge", readAcquireHr); }
                    return false;
                }
                readerMutexHeld = true;
            }

            bitmapToDraw = drawSlot->surfaceBitmap.Get();
            aquariumBridgeUiSharedTexture_ = drawSlot->uiTexture;
            aquariumBridgeInteropSharedTexture_ = writeSlot->interopTexture;
            aquariumBridgeSurfaceBitmap_ = drawSlot->surfaceBitmap;
            aquariumBridgeSharedHandle_ = drawSlot->sharedHandle;
            aquariumBridgeSharedWriteIndex_ = (writeIndex + 1) % kAquariumD2DSharedBridgeSlotCount;
            aquariumBridgeSharedReadyIndex_ = writeIndex;
            aquariumBridgeSharedHasReadyFrame_ = !sharedKeyed;
            if (sharedKeyed)
            {
                writeSlot->ready = false;
            }
            aquariumD2DBridgeSharedWriteIndexStat_ = static_cast<std::uint64_t>(writeIndex);
            aquariumD2DBridgeSharedDrawIndexStat_ = static_cast<std::uint64_t>(drawIndex);
            sharedDraw = true;

            if (!bitmapToDraw)
            {
                if (readerMutexHeld && drawSlot->uiMutex)
                {
                    drawSlot->uiMutex->ReleaseSync(0);
                }
                if (error) { *error = "D2D DeviceContext shared intermediate bridge has no bitmap to draw."; }
                return false;
            }

            d2dViewportBridgeRuntime_.MarkDrawStarted(drawIndex);
            ctx.target->DrawBitmap(
                bitmapToDraw,
                rect.d2d(),
                1.0f,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
            d2dViewportBridgeRuntime_.MarkDrawCompleted(drawIndex);

            if (readerMutexHeld && drawSlot->uiMutex)
            {
                D2D1_TAG tag1 = 0;
                D2D1_TAG tag2 = 0;
                const HRESULT flushHr = ctx.target->Flush(&tag1, &tag2);
                ++aquariumD2DBridgeSharedD2DFlushCount_;
                const HRESULT releaseHr = drawSlot->uiMutex->ReleaseSync(0);
                if (FAILED(flushHr))
                {
                    if (error) { *error = hresultToString("ID2D1DeviceContext::Flush D2D shared viewport bridge", flushHr); }
                    return false;
                }
                if (FAILED(releaseHr))
                {
                    if (error) { *error = hresultToString("IDXGIKeyedMutex::ReleaseSync UI D2D shared viewport bridge", releaseHr); }
                    return false;
                }
            }
        }

        if (aquariumBridgeD3D11On12_ && wrapped[0])
        {
            aquariumBridgeD3D11On12_->ReleaseWrappedResources(wrapped, 1);
        }
        if (aquariumBridgeD3D11Context_ && !aquariumBridgeUsesSharedIntermediate_)
        {
            aquariumBridgeD3D11Context_->Flush();
        }

        if (!sharedDraw)
        {
            if (!bitmapToDraw)
            {
                if (error) { *error = "D2D DeviceContext bridge has no bitmap to draw."; }
                return false;
            }
            ctx.target->DrawBitmap(
                bitmapToDraw,
                rect.d2d(),
                1.0f,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        }

        if (aquariumBridgeUsesSharedIntermediate_)
        {
            d2dViewportCopyScheduler_.RecordExecuted(bridgeCopySchedule, true, true);
            d2dViewportTextureCache_.RecordSharedTextureAccepted(textureCacheKey, d2dViewportBridgeRuntime_.Diagnostics());
        }
        d2dFrameDiagnostics_.RecordViewportDraw(true, aquariumD2DBridgeSharedBitmapRecreateCount_);

        aquariumViewportCacheKey_ = key;
        aquariumViewportCacheKeyValid_ = false;
        aquariumD2DBridgeLastError_.clear();
        aquariumD2DBridgeFatalStep_.clear();
        aquariumD2DBridgeFatalHresult_.clear();
        aquariumD2DBridgeDisabled_ = false;
        ++aquariumD2DBridgeSuccessCount_;
        if (aquariumBridgeUsesSharedIntermediate_)
        {
            ++aquariumD2DBridgeSharedSuccessCount_;
        }
        else
        {
            ++aquariumD2DBridgeDirectSuccessCount_;
        }
        return true;
    }

    bool AceShellUi::drawCachedAquariumSlateViewportElement(D2DRenderContext& ctx, UiRect rect, const AquariumViewportCacheKey& key)
    {
        if (!ctx.target || rect.empty() || !canReuseAquariumViewportBitmap(key))
        {
            return false;
        }

        ctx.target->DrawBitmap(
            aquariumSlateViewportBitmap_.Get(),
            rect.d2d(),
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        ++aquariumViewportCacheHitCount_;
        return true;
    }

    bool AceShellUi::drawAquariumGpuSnapshotAsSlateViewportElement(
        D2DRenderContext& ctx,
        UiRect rect,
        const am::renderer::scene::AceAquariumGpuViewportSnapshot& snapshot,
        const AquariumViewportCacheKey& key,
        std::string* error)
    {
        if (!ctx.target || rect.empty() || snapshot.bgraPixels.empty() || snapshot.extent.width == 0 || snapshot.extent.height == 0)
        {
            if (error) { *error = "Slate-style viewport texture draw requires a valid BGRA snapshot."; }
            return false;
        }

        const UINT32 width = static_cast<UINT32>(snapshot.extent.width);
        const UINT32 height = static_cast<UINT32>(snapshot.extent.height);
        const UINT32 pitch = width * 4u;
        const bool extentChanged =
            !aquariumSlateViewportBitmap_ ||
            aquariumSlateViewportBitmapExtent_.width != snapshot.extent.width ||
            aquariumSlateViewportBitmapExtent_.height != snapshot.extent.height;

        if (extentChanged)
        {
            aquariumSlateViewportBitmap_.Reset();
            aquariumSlateViewportBitmapExtent_ = {};

            const D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
                static_cast<FLOAT>(ctx.dpiX),
                static_cast<FLOAT>(ctx.dpiY));

            const HRESULT hr = ctx.target->CreateBitmap(
                D2D1::SizeU(width, height),
                snapshot.bgraPixels.data(),
                pitch,
                props,
                aquariumSlateViewportBitmap_.GetAddressOf());

            if (FAILED(hr) || !aquariumSlateViewportBitmap_)
            {
                if (error) { *error = hresultToString("CreateBitmap viewport element", hr); }
                return false;
            }

            aquariumSlateViewportBitmapExtent_ = snapshot.extent;
        }
        else
        {
            const HRESULT hr = aquariumSlateViewportBitmap_->CopyFromMemory(nullptr, snapshot.bgraPixels.data(), pitch);
            if (FAILED(hr))
            {
                aquariumSlateViewportBitmap_.Reset();
                aquariumSlateViewportBitmapExtent_ = {};
                if (error) { *error = hresultToString("CopyFromMemory viewport element", hr); }
                return false;
            }
            ++aquariumSlateViewportBitmapReuseCount_;
        }

        ctx.target->DrawBitmap(
            aquariumSlateViewportBitmap_.Get(),
            rect.d2d(),
            1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        aquariumViewportCacheKey_ = key;
        aquariumViewportCacheKeyValid_ = true;
        ++aquariumViewportCacheMissCount_;
        return true;
    }

    void AceShellUi::renderEngineLogOverlay(D2DRenderContext& ctx)
    {
        if (!engineLogOverlayVisible_ || !ctx.target)
        {
            return;
        }

        UiRect anchor = mainRect_;
        if (environmentOpen_ && aquarium3DModeActive_ && !aquariumEmbeddedViewportRect_.empty())
        {
            anchor = aquariumEmbeddedViewportRect_;
        }
        else if (environmentOpen_ && !environmentModalRect_.empty())
        {
            anchor = environmentModalRect_.inset({24.0f, 56.0f, 24.0f, 24.0f});
        }

        engineLogOverlayRect_ = computeEngineLogOverlayRect(anchor);
        if (engineLogOverlayRect_.empty())
        {
            return;
        }

        const float oldPanelOpacity = ctx.brushes.panelDeep ? ctx.brushes.panelDeep->GetOpacity() : 1.0f;
        const float oldBorderOpacity = ctx.brushes.border ? ctx.brushes.border->GetOpacity() : 1.0f;
        const float oldAccentOpacity = ctx.brushes.accentBlue ? ctx.brushes.accentBlue->GetOpacity() : 1.0f;
        if (ctx.brushes.panelDeep) { ctx.brushes.panelDeep->SetOpacity(0.94f); }
        if (ctx.brushes.border) { ctx.brushes.border->SetOpacity(0.78f); }
        D2DWidgetUtils::fillRounded(ctx, engineLogOverlayRect_, 14.0f, ctx.brushes.panelDeep, ctx.brushes.border, 1.1f);

        UiRect accent = engineLogOverlayRect_;
        accent.bottom = accent.top + 3.0f;
        if (ctx.brushes.accentBlue) { ctx.brushes.accentBlue->SetOpacity(0.76f); }
        D2DWidgetUtils::fillRounded(ctx, accent, 2.0f, ctx.brushes.accentBlue);

        if (ctx.brushes.panelDeep) { ctx.brushes.panelDeep->SetOpacity(oldPanelOpacity); }
        if (ctx.brushes.border) { ctx.brushes.border->SetOpacity(oldBorderOpacity); }
        if (ctx.brushes.accentBlue) { ctx.brushes.accentBlue->SetOpacity(oldAccentOpacity); }

        const UiRect header = makeUiRect(engineLogOverlayRect_.left + 14.0f, engineLogOverlayRect_.top + 10.0f, engineLogOverlayRect_.right - 14.0f, engineLogOverlayRect_.top + 32.0f);
        D2DWidgetUtils::drawText(ctx, L"ACE Engine Log Console", ctx.fonts.bodyStrong, header, ctx.brushes.text);
        D2DWidgetUtils::drawText(ctx, L"Enter = run local command | PgUp/PgDn = scroll | ` / Esc = close", ctx.fonts.small, header, ctx.brushes.muted, DWRITE_TEXT_ALIGNMENT_TRAILING);

        const UiRect meta = makeUiRect(engineLogOverlayRect_.left + 14.0f, engineLogOverlayRect_.top + 34.0f, engineLogOverlayRect_.right - 14.0f, engineLogOverlayRect_.top + 52.0f);
        D2DWidgetUtils::drawText(ctx, L"Build/Logs/ace_engine.log | local-only commands: stat_rhi, stat_coords, stat_fps, stat_ui, clear_log", ctx.fonts.small, meta, ctx.brushes.muted);

        const float inputHeight = 42.0f;
        engineLogOverlayInputRect_ = makeUiRect(engineLogOverlayRect_.left + 14.0f, engineLogOverlayRect_.bottom - inputHeight - 12.0f, engineLogOverlayRect_.right - 14.0f, engineLogOverlayRect_.bottom - 12.0f);
        UiRect logBody = makeUiRect(engineLogOverlayRect_.left + 14.0f, engineLogOverlayRect_.top + 58.0f, engineLogOverlayRect_.right - 14.0f, engineLogOverlayInputRect_.top - 10.0f);
        D2DWidgetUtils::fillRounded(ctx, logBody, 10.0f, ctx.brushes.panel, ctx.brushes.borderDim, 1.0f);

        const float lineHeight = 19.0f;
        const float scrollBarWidth = 8.0f;
        engineLogOverlayLogViewportRect_ = makeUiRect(logBody.left + 10.0f, logBody.top + 8.0f, logBody.right - 16.0f - scrollBarWidth, logBody.bottom - 8.0f);
        engineLogOverlayScroll_.viewport = engineLogOverlayLogViewportRect_;
        engineLogOverlayScroll_.track = makeUiRect(logBody.right - 12.0f, logBody.top + 9.0f, logBody.right - 6.0f, logBody.bottom - 9.0f);
        const float previousMaxScroll = engineLogOverlayScroll_.maxScroll;
        const bool wasAtBottom = engineLogOverlayScroll_.offset >= previousMaxScroll - 2.0f;
        engineLogOverlayScroll_.viewportHeight = std::max(0.0f, engineLogOverlayScroll_.viewport.height());
        engineLogOverlayScroll_.lineCount = engineLogOverlayLines_.size();
        engineLogOverlayScroll_.contentHeight = static_cast<float>(engineLogOverlayLines_.size()) * lineHeight;
        engineLogOverlayScroll_.maxScroll = std::max(0.0f, engineLogOverlayScroll_.contentHeight - engineLogOverlayScroll_.viewportHeight);
        if (engineLogOverlayScroll_.autoScrollWhenAtBottom && (!engineLogOverlayScroll_.userScrolled || wasAtBottom))
        {
            engineLogOverlayScroll_.offset = engineLogOverlayScroll_.maxScroll;
            engineLogOverlayScroll_.userScrolled = false;
        }
        clampAquariumScroll(engineLogOverlayScroll_);

        ctx.target->PushAxisAlignedClip(engineLogOverlayScroll_.viewport.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        renderEngineLogTextSelection(ctx, lineHeight);
        float y = engineLogOverlayScroll_.viewport.top - engineLogOverlayScroll_.offset;
        for (std::size_t lineIndex = 0; lineIndex < engineLogOverlayLines_.size(); ++lineIndex)
        {
            if (y + lineHeight >= engineLogOverlayScroll_.viewport.top && y <= engineLogOverlayScroll_.viewport.bottom)
            {
                if (const auto layout = engineLogTextLayoutForLine(lineIndex))
                {
                    ctx.target->DrawTextLayout(
                        D2D1::Point2F(engineLogOverlayScroll_.viewport.left, y),
                        layout.Get(),
                        ctx.brushes.textDim,
                        D2D1_DRAW_TEXT_OPTIONS_CLIP);
                }
            }
            y += lineHeight;
        }
        ctx.target->PopAxisAlignedClip();
        renderAquariumScrollbar(ctx, engineLogOverlayScroll_);

        engineLogOverlayInput_.setRect(engineLogOverlayInputRect_);
        engineLogOverlayInput_.setFocused(engineLogOverlayInputFocused_);
        engineLogOverlayInput_.render(ctx);
    }

    void AceShellUi::showToast(std::wstring title, std::wstring body, D2DToastKind kind)
    {
        toastCenter_.push(std::move(title), std::move(body), kind, 4.0f);
    }


    void AceShellUi::saveLayoutProfile()
    {
        if (layoutProfilePath_.empty())
        {
            showToast(L"Layout", L"No layout path configured.", D2DToastKind::Warning);
            return;
        }

        std::string error;
        layoutProfile_.activeWorkspaceTab = workspaceTabs_.active();
        layoutProfile_.aquariumDetailsWidth = aquarium3DPanelState_.detailsWidth;
        layoutProfile_.aquariumDetailsHeight = aquarium3DPanelState_.detailsHeight;
        layoutProfile_.aquariumLogsWidth = aquarium3DPanelState_.logsWidth;
        layoutProfile_.aquariumLogsHeight = aquarium3DPanelState_.logsHeight;
        layoutProfile_.aquariumDetailsVisible = aquariumDetailsPanelVisible_;
        layoutProfile_.aquariumLogsVisible = aquariumLogsPanelVisible_;

        if (D2DLayoutPersistence::save(layoutProfilePath_, layoutProfile_, &error))
        {
            showToast(L"Layout saved", layoutProfilePath_.wstring(), D2DToastKind::Success);
            statusBar_.setText(L"Layout profile saved.");
        }
        else
        {
            showToast(L"Layout save failed", std::wstring(error.begin(), error.end()), D2DToastKind::Error);
            statusBar_.setText(L"Layout profile save failed.");
        }
    }

    void AceShellUi::loadLayoutProfile()
    {
        if (layoutProfilePath_.empty())
        {
            return;
        }

        std::string error;
        if (auto loaded = D2DLayoutPersistence::load(layoutProfilePath_, &error))
        {
            layoutProfile_ = *loaded;
            layoutProfile_.clamp();
            aquarium3DPanelState_.detailsWidth = layoutProfile_.aquariumDetailsWidth;
            aquarium3DPanelState_.detailsHeight = layoutProfile_.aquariumDetailsHeight;
            aquarium3DPanelState_.logsWidth = layoutProfile_.aquariumLogsWidth;
            aquarium3DPanelState_.logsHeight = layoutProfile_.aquariumLogsHeight;
            aquariumDetailsPanelVisible_ = layoutProfile_.aquariumDetailsVisible;
            aquariumLogsPanelVisible_ = layoutProfile_.aquariumLogsVisible;
            aquarium3DPanelState_.detailsVisible = aquariumDetailsPanelVisible_;
            aquarium3DPanelState_.logsVisible = aquariumLogsPanelVisible_;

            if (!layoutProfile_.activeWorkspaceTab.empty())
            {
                workspaceTabs_.setActive(layoutProfile_.activeWorkspaceTab);
            }

            refreshLayoutOverlay();
            statusBar_.setText(L"Layout profile loaded.");
        }
    }

    void AceShellUi::resetLayoutProfile()
    {
        layoutProfile_ = D2DDockLayoutProfile::defaults();
        workspaceTabs_.setActive(layoutProfile_.activeWorkspaceTab);
        layout(width_, height_);
        refreshLayoutOverlay();
        showToast(L"Layout reset", L"Default dock profile restored.", D2DToastKind::Success);
        statusBar_.setText(L"Layout profile reset.");
    }

    void AceShellUi::refreshLayoutOverlay()
    {
        layoutProfile_.activeWorkspaceTab = workspaceTabs_.active().empty() ? layoutProfile_.activeWorkspaceTab : workspaceTabs_.active();
        layoutProfile_.clamp();
        layoutOverlay_.setProfile(layoutProfile_);
    }


    void AceShellUi::startNewConversation()
    {
        saveActiveConversation();

        const std::wstring title = L"Conversation " + std::to_wstring(nextConversationNumber_++);
        conversations_.push_back(title);
        conversationMessages_.push_back({});
        activeConversationIndex_ = conversations_.empty() ? 0 : conversations_.size() - 1;

        messageList_.setMessages({});
        input_.setText(L"");
        input_.setFocused(true);
        focus_.set(D2DFocusTarget::TextInput);
        statusBar_.setText(L"New conversation.");
        layout(width_, height_);
    }

    void AceShellUi::selectConversation(std::size_t index)
    {
        if (index >= conversations_.size())
        {
            return;
        }

        saveActiveConversation();
        activeConversationIndex_ = index;

        if (conversationMessages_.size() < conversations_.size())
        {
            conversationMessages_.resize(conversations_.size());
        }

        messageList_.setMessages(conversationMessages_[activeConversationIndex_]);
        input_.setText(L"");
        input_.setFocused(true);
        focus_.set(D2DFocusTarget::TextInput);
        statusBar_.setText(L"Selected " + conversations_[index] + L".");
        layout(width_, height_);
        invalidate();
    }

    void AceShellUi::prefillRenameConversation(std::size_t index)
    {
        if (index >= conversations_.size())
        {
            return;
        }

        selectConversation(index);
        input_.setText(L"/rename " + conversations_[index]);
        input_.setFocused(true);
        focus_.set(D2DFocusTarget::TextInput);
        statusBar_.setText(L"Edit the name in the input, then press Enter.");
        layout(width_, height_);
    }

    void AceShellUi::saveActiveConversation()
    {
        if (conversations_.empty())
        {
            conversations_.push_back(L"Conversation 1");
            activeConversationIndex_ = 0;
            nextConversationNumber_ = std::max<std::uint64_t>(nextConversationNumber_, 2);
        }

        if (conversationMessages_.size() < conversations_.size())
        {
            conversationMessages_.resize(conversations_.size());
        }

        activeConversationIndex_ = std::min(activeConversationIndex_, conversations_.size() - 1);
        conversationMessages_[activeConversationIndex_] = messageList_.messages();
    }

    void AceShellUi::clearInspector()
    {
        am::core::AceUiInspectorRecord record;
        record.title = L"Inspector";
        record.subtitle = L"Select an item.";
        record.body = L"Click a workspace item to inspect its backend data.";
        record.properties.push_back({L"state", L"idle", 0});
        inspectorPanel_.setRecord(std::move(record));
    }


    void AceShellUi::executeCommand(const std::wstring& commandId)
    {
        if (commandId == L"palette")
        {
            openCommandPalette();
        }
        else if (commandId == L"summary")
        {
            if (submitHandler_)
            {
                appendSubmitResult(submitHandler_(L"/summary"));
            }
            statusBar_.setText(L"Toolbar executed: summary");
        }
        else if (commandId == L"store")
        {
            if (submitHandler_)
            {
                appendSubmitResult(submitHandler_(L"/summary"));
            }
            statusBar_.setText(L"Store summary appended to chat.");
        }
        else if (commandId == L"ledger")
        {
            refreshInspector(am::core::AceUiSelection{am::core::AceUiItemKind::Ledger, 0, L"Belief Ledger", L"toolbar"});
            statusBar_.setText(L"Toolbar inspected ledger.");
        }
        else if (commandId == L"diagnostics")
        {
            diagnostics_.toggle();
            toolbar_.setToggled(L"diagnostics", diagnostics_.visible());
            refreshDiagnostics();
            showToast(L"Diagnostics", diagnostics_.visible() ? L"Diagnostics overlay opened." : L"Diagnostics overlay closed.", D2DToastKind::Info);
            statusBar_.setText(diagnostics_.visible() ? L"Diagnostics opened." : L"Diagnostics closed.");
        }
        else if (commandId == L"shortcuts")
        {
            shortcutHelp_.toggle();
            showToast(L"Help", shortcutHelp_.visible() ? L"Shortcut help opened." : L"Shortcut help closed.", D2DToastKind::Info);
            statusBar_.setText(shortcutHelp_.visible() ? L"Shortcut help opened." : L"Shortcut help closed.");
        }
        else if (commandId == L"graph_fit")
        {
            messageList_.addMessage(makeMessage(L"Tools", L"Graph tools are hidden until there is a real environment graph view.", false, true, ChatMessageKind::System, L"graph"));
            showToast(L"Graph", L"Graph tools hidden.", D2DToastKind::Info);
            statusBar_.setText(L"Graph tool command acknowledged.");
        }
        else if (commandId == L"graph_layout")
        {
            messageList_.addMessage(makeMessage(L"Tools", L"Graph auto-layout is reserved for a future environment graph view.", false, true, ChatMessageKind::System, L"graph"));
            showToast(L"Graph", L"Graph auto-layout acknowledged.", D2DToastKind::Info);
            statusBar_.setText(L"Graph auto-layout command acknowledged.");
        }
        else if (commandId == L"layout" || commandId == L"layout_overlay")
        {
            layoutOverlay_.toggle();
            refreshLayoutOverlay();
            showToast(L"Layout", layoutOverlay_.visible() ? L"Layout overlay opened." : L"Layout overlay closed.", D2DToastKind::Info);
            statusBar_.setText(layoutOverlay_.visible() ? L"Layout overlay opened." : L"Layout overlay closed.");
        }
        else if (commandId == L"save_layout")
        {
            saveLayoutProfile();
        }
        else if (commandId == L"load_layout")
        {
            loadLayoutProfile();
            layout(width_, height_);
            showToast(L"Layout loaded", L"Saved layout profile applied.", D2DToastKind::Success);
        }
        else if (commandId == L"reset_layout")
        {
            resetLayoutProfile();
        }
        else if (commandId == L"clear_chat")
        {
            messageList_.clear();
            messageList_.addMessage(makeMessage(L"System", L"Workspace entries cleared by command palette.", false, true, ChatMessageKind::System, L"command"));
            statusBar_.setText(L"Command executed: clear workspace");
        }
        else if (commandId == L"seed_demo")
        {
            seedDemoMessages();
            statusBar_.setText(L"Command executed: seed_demo");
        }
        else if (commandId == L"focus_input")
        {
            focus_.set(D2DFocusTarget::TextInput);
            input_.setFocused(true);
            statusBar_.setText(L"Command executed: focus_input");
        }
        else if (commandId == L"insert_template")
        {
            input_.setFocused(true);
            focus_.set(D2DFocusTarget::TextInput);
            input_.setText(L"Analyze this claim: liquids boil when vapor pressure equals external pressure. Generate hypotheses, evidence needs, and possible counterexamples.");
            statusBar_.setText(L"Command executed: insert_template");
        }
        else if (commandId == L"backend_summary")
        {
            if (submitHandler_)
            {
                appendSubmitResult(submitHandler_(L"/summary"));
            }
            else
            {
                messageList_.addMessage(makeMessage(L"Warning", L"Submit handler is not connected.", false, false, ChatMessageKind::Warning, L"backend"));
            }
            statusBar_.setText(L"Command executed: backend_summary");
        }
        else if (commandId == L"backend_help")
        {
            if (submitHandler_)
            {
                appendSubmitResult(submitHandler_(L"/help"));
            }
            else
            {
                messageList_.addMessage(makeMessage(L"Warning", L"Submit handler is not connected.", false, false, ChatMessageKind::Warning, L"backend"));
            }
            statusBar_.setText(L"Command executed: backend_help");
        }
        else if (commandId == L"sample_concept")
        {
            input_.setFocused(true);
            focus_.set(D2DFocusTarget::TextInput);
            input_.setText(L"concept vapor_pressure | pressure exerted by vapor in equilibrium with liquid");
            statusBar_.setText(L"Inserted concept parser command.");
        }
        else if (commandId == L"sample_hypothesis")
        {
            input_.setFocused(true);
            focus_.set(D2DFocusTarget::TextInput);
            input_.setText(L"Liquids with higher external pressure need higher temperature to boil.");
            statusBar_.setText(L"Inserted natural-language hypothesis.");
        }
        else if (commandId == L"refresh_workspace")
        {
            refreshSnapshot();
            statusBar_.setText(L"Workspace snapshot refreshed.");
        }
        else if (commandId == L"show_concepts")
        {
            if (submitHandler_)
            {
                appendSubmitResult(submitHandler_(L"/concepts"));
                refreshSnapshot();
            }
            else
            {
                messageList_.addMessage(makeMessage(L"Warning", L"Submit handler is not connected.", false, false, ChatMessageKind::Warning, L"backend"));
            }
            statusBar_.setText(L"Command executed: show_concepts");
        }
        else if (commandId == L"inspect_store")
        {
            refreshInspector(am::core::AceUiSelection{am::core::AceUiItemKind::Store, 0, L"Cognitive Store", L"command"});
            statusBar_.setText(L"Inspector opened: store");
        }
        else if (commandId == L"inspect_ledger")
        {
            refreshInspector(am::core::AceUiSelection{am::core::AceUiItemKind::Ledger, 0, L"Belief Ledger", L"command"});
            statusBar_.setText(L"Inspector opened: ledger");
        }
        else if (commandId == L"ui_debug")
        {
            uiDebugOverlay_.Toggle();
            ++aceUi9DebugOverlayToggleCount_;
            refreshUiDebugOverlay();
            showToast(L"UI Debug", uiDebugOverlay_.Visible() ? L"UI debug overlay opened." : L"UI debug overlay closed.", D2DToastKind::Info);
            statusBar_.setText(uiDebugOverlay_.Visible() ? L"UI debug overlay opened." : L"UI debug overlay closed.");
        }
        else if (commandId == L"ui_stats")
        {
            const auto textStats = D2DTextLayoutFoundation::Stats();
            const auto drawStats = uiDrawCommands_.Stats();
            const auto invalidationStats = uiInvalidation_.Snapshot();
            const auto retainedStats = uiRetainedLayout_.Stats();
            const auto styleStats = uiStyleSet_.Stats();
            const auto telemetryStats = aquariumTelemetryWidgets_.Stats();
            std::wstring stats =
                L"UI5 text: draw=" + std::to_wstring(textStats.drawCount) +
                L" ellipsis=" + std::to_wstring(textStats.ellipsisCount) +
                L" | UI6 retained: nodes=" + std::to_wstring(retainedStats.nodeCount) +
                L" passes=" + std::to_wstring(retainedStats.arrangePasses) +
                L" | UI7 draw: commands=" + std::to_wstring(drawStats.commandCount) +
                L" max=" + std::to_wstring(drawStats.maxCommandCount) +
                L" | UI8 dirty: marks=" + std::to_wstring(invalidationStats.markCount) +
                L" rects=" + std::to_wstring(invalidationStats.rects.size()) +
                L" | UI11 styles: panels=" + std::to_wstring(styleStats.panelStyleCount) +
                L" text=" + std::to_wstring(styleStats.textStyleCount) +
                L" | AQUI1 telemetry=" + std::to_wstring(telemetryStats.renderCount);
            messageList_.addMessage(makeMessage(L"Tool", stats, false, false, ChatMessageKind::Tool, L"ui stats"));
            statusBar_.setText(L"Command executed: ui_stats");
        }
        else if (commandId == L"cache_stats")
        {
            std::wstring stats =
                L"Text cache: size=" + std::to_wstring(textCache_.size()) +
                L" capacity=" + std::to_wstring(textCache_.capacity()) +
                L" hits=" + std::to_wstring(textCache_.hitCount()) +
                L" misses=" + std::to_wstring(textCache_.missCount()) +
                L" | message layout cache: size=" + std::to_wstring(messageList_.layoutCacheSize()) +
                L" hits=" + std::to_wstring(messageList_.layoutCacheHits()) +
                L" misses=" + std::to_wstring(messageList_.layoutCacheMisses());

            const auto effectStats = D2DCachedEffects::stats();
            stats += L" | effect cache: entries=" + std::to_wstring(effectStats.cachedEntryCount) +
                L" hits=" + std::to_wstring(effectStats.hitCount) +
                L" misses=" + std::to_wstring(effectStats.missCount) +
                L" | display: dpi=" + std::to_wstring(displayMetrics_.nearestMonitor.dpiX) +
                L" scale=" + std::to_wstring(static_cast<int>(dpiScale() * 100.0f)) + L"%" +
                L" | d2d pixel-dpi fix=" + std::to_wstring(d2dPixelDpiFixApplyCount_);

            messageList_.addMessage(makeMessage(L"Tool", stats, false, false, ChatMessageKind::Tool, L"font cache"));
            statusBar_.setText(L"Command executed: cache_stats");
        }
        else if (commandId == L"help")
        {
            messageList_.addMessage(makeMessage(
                L"System",
                L"Shortcuts: Ctrl+K/Ctrl+P command palette, Ctrl+A select input, Ctrl+C copy, Ctrl+X cut, Ctrl+V paste, Tab focus, mouse wheel scroll, Enter send.",
                false,
                true,
                ChatMessageKind::System,
                L"help"
            ));
            statusBar_.setText(L"Command executed: help");
        }
        else
        {
            messageList_.addMessage(makeMessage(L"Warning", L"Unknown command: " + commandId, false, false, ChatMessageKind::Warning, L"command"));
            statusBar_.setText(L"Unknown command.");
        }

        invalidate();
    }

    void AceShellUi::openCommandPalette()
    {
        commandPalette_.open();
        focus_.set(D2DFocusTarget::CommandPalette);
        input_.setFocused(false);
        statusBar_.setText(L"Command palette open. Type to filter; Enter executes; Esc closes.");
    }

    void AceShellUi::seedDemoMessages()
    {
        messageList_.addMessage(makeMessage(L"System", L"ACE shell demo seed inserted for layout testing.", false, true, ChatMessageKind::System, L"demo"));
        messageList_.addMessage(makeMessage(L"Tool", L"ACE shell backend placeholder.applyEvidence(claim=boiling_rule, delta=+0.11)", false, false, ChatMessageKind::Tool, L"backend trace"));
        messageList_.addMessage(makeMessage(L"Arhqen Cognition Engine", L"This message is intentionally longer so the message list has to measure text, wrap it, estimate bubble height, and scroll like a civilized UI instead of guessing with a cursed char counter.", false, false, ChatMessageKind::Assistant, L"layout"));
        messageList_.addMessage(makeMessage(L"You", L"Good. Keep making the UI less embarrassing.", true, false, ChatMessageKind::User, L"demo user"));
    }


    void AceShellUi::handleResize(int width, int height)
    {
        if (width <= 0 || height <= 0)
        {
            return;
        }

        if (engineWorkspaceController_.draggingSplitter())
        {
            std::string ignored;
            engineWorkspaceController_.cancelPointerInteraction(&ignored);
            if (mouseCaptured_ && parent_ && GetCapture() == parent_) ReleaseCapture();
            mouseCaptured_ = false;
        }

        refreshDisplayMetrics("resize");

        if (windowLiveResizeActive_)
        {
            // ACE-AQ3D9: live resize still updates the D2D parent target/layout so
            // the window paints at the current client size, but the embedded DX12
            // child HWND remains hidden/frozen until WM_EXITSIZEMOVE.
            pendingLiveResizeWidth_ = width;
            pendingLiveResizeHeight_ = height;
            pendingResizeAfterLiveDrag_ = true;
            aquariumEmbeddedViewportSyncNeeded_ = true;
            ++liveResizeDeferredSizeCount_;
            layoutForLiveResize(width, height);
            return;
        }

        layout(width, height);

        if (environmentOpen_ && aquarium3DModeActive_ && aquariumEmbeddedViewportVisible_)
        {
            aquariumResizeQuarantineActive_ = true;
            aquariumResizeQuarantineDelaySeconds_ = 0.12f;
            ++aquariumResizeQuarantineEnterCount_;
            aquariumEmbeddedDx12Viewport_.SetResizeApplySuspended(true);
            aquariumEmbeddedViewportStatus_ = L"Real DX12 3D resize freeze active";
            aquariumEmbeddedViewportSyncNeeded_ = true;
        }
    }

    void AceShellUi::layoutForLiveResize(int width, int height)
    {
        if (width <= 0 || height <= 0)
        {
            return;
        }

        ++d2dResizeDuringLiveResizeCount_;
        gradientsDirty_ = true;
        layout(width, height);

        if (environmentOpen_ && aquarium3DModeActive_ && engineEditorModeActive_)
        {
            std::string ignored;
            engineWorkspaceController_.arrange({0.0, 70.0, static_cast<double>(width), static_cast<double>(height)}, {}, &ignored);
            if (const auto* geometry = engineWorkspaceController_.geometry())
            {
                if (const auto* viewport = geometry->findNode("stack.viewport"))
                    aquariumEmbeddedViewportRect_ = makeUiRect(
                        static_cast<float>(viewport->content.left), static_cast<float>(viewport->content.top),
                        static_cast<float>(viewport->content.right), static_cast<float>(viewport->content.bottom));
            }
            aquariumNativeViewportRect_ = computeAquariumNativeViewportRect(aquariumEmbeddedViewportRect_);
            aquariumPendingViewportRect_ = aquariumNativeViewportRect_;
            aquariumPendingViewportValid_ = true;
            hideAquariumResizeShieldWindow();
        }
        else if (environmentOpen_ && aquarium3DModeActive_)
        {
            aquarium3DPanelState_.detailsVisible = aquariumDetailsPanelVisible_;
            aquarium3DPanelState_.logsVisible = aquariumLogsPanelVisible_;
            environment3DMode_.ClampPanelState(aquarium3DPanelState_, static_cast<float>(width), static_cast<float>(height));
            const auto liveLayout = environment3DMode_.Compute(static_cast<float>(width), static_cast<float>(height), aquarium3DPanelState_);
            aquariumEmbeddedViewportRect_ = makeUiRect(liveLayout.dx12Surface.left, liveLayout.dx12Surface.top, liveLayout.dx12Surface.right, liveLayout.dx12Surface.bottom);
            aquariumNativeViewportRect_ = computeAquariumNativeViewportRect(aquariumEmbeddedViewportRect_);
            aquariumPendingViewportRect_ = aquariumNativeViewportRect_;
            aquariumPendingViewportValid_ = true;
            // ACE-AQ3D11R2: parent D2D proxy covers resize; no popup shield.
            hideAquariumResizeShieldWindow();
        }

        forceLiveResizeProxyRepaintNow();
    }

    void AceShellUi::beginWindowLiveResize()
    {
        if (windowLiveResizeActive_)
        {
            return;
        }

        windowLiveResizeActive_ = true;
        pendingResizeAfterLiveDrag_ = false;
        pendingLiveResizeWidth_ = width_;
        pendingLiveResizeHeight_ = height_;
        gradientsDirty_ = true;
        aquariumEmbeddedViewportSyncNeeded_ = true;
        aquariumResizeQuarantineActive_ = true;
        aquariumResizeQuarantineDelaySeconds_ = 0.18f;
        ++aquariumResizeQuarantineEnterCount_;
        captureFrozenNativeResizeRect();
        aquariumEmbeddedDx12Viewport_.SetResizeApplySuspended(true);

        aquariumViewportWasVisibleBeforeLiveResize_ = environmentOpen_ && aquarium3DModeActive_ && aquariumEmbeddedDx12Viewport_.IsVisible();
        aquariumViewportHiddenForLiveResize_ = false;

        if (environmentOpen_ && aquarium3DModeActive_)
        {
            // ACE-AQ3D11R5: no proxy swap and no hide/show loop. The child DX12
            // surface stays visible at its last stable rect while resize settles.
            aquariumEmbeddedViewportStatus_ = L"Real DX12 3D resize freeze active";
            hideAquariumResizeShieldWindow();
        }

        ++liveResizeEnterCount_;
    }

    bool AceShellUi::shouldFreezeNativeLiveResize() const
    {
        // ACE-AQ3D11R2: do not freeze the native top-level window during resize.
        // The real DX12 child viewport is hidden and the parent paints the stable
        // single-HWND D2D proxy until WM_EXITSIZEMOVE restores renderFrame3D.
        return false;
    }

    void AceShellUi::captureFrozenNativeResizeRect()
    {
        if (!parent_ || !shouldFreezeNativeLiveResize())
        {
            return;
        }

        RECT rect{};
        if (!GetWindowRect(parent_, &rect))
        {
            return;
        }

        frozenNativeLiveResizeRect_ = rect;
        frozenNativeLiveResizeRectValid_ = true;
        frozenNativeLiveResizeActive_ = true;
        pendingNativeLiveResizeRectValid_ = false;
        ++nativeResizeFreezeEnterCount_;
    }

    bool AceShellUi::handleFrozenNativeResizeSizing(LPARAM lParam)
    {
        if (!parent_ || !shouldFreezeNativeLiveResize() || !lParam)
        {
            return false;
        }

        if (!frozenNativeLiveResizeRectValid_)
        {
            captureFrozenNativeResizeRect();
        }

        if (!frozenNativeLiveResizeRectValid_)
        {
            return false;
        }

        RECT* proposed = reinterpret_cast<RECT*>(lParam);
        pendingNativeLiveResizeRect_ = *proposed;
        pendingNativeLiveResizeRectValid_ = true;
        ++nativeResizeFrozenSizingMessageCount_;

        // Do not let USER32 continuously change the actual top-level client area
        // while the DX12 child swapchain exists. The proposed RECT is committed
        // once in applyFrozenNativeResizeCommit().
        *proposed = frozenNativeLiveResizeRect_;
        ++nativeResizePreventedClientResizeCount_;

        pendingResizeAfterLiveDrag_ = true;
        aquariumEmbeddedViewportSyncNeeded_ = true;
        updateAquariumResizeShieldWindow(aquariumEmbeddedViewportRect_);
        return true;
    }

    void AceShellUi::applyFrozenNativeResizeCommit()
    {
        if (!parent_ || !frozenNativeLiveResizeActive_)
        {
            frozenNativeLiveResizeRectValid_ = false;
            pendingNativeLiveResizeRectValid_ = false;
            return;
        }

        frozenNativeLiveResizeActive_ = false;
        frozenNativeLiveResizeRectValid_ = false;

        if (!pendingNativeLiveResizeRectValid_)
        {
            return;
        }

        const RECT finalRect = pendingNativeLiveResizeRect_;
        pendingNativeLiveResizeRectValid_ = false;

        const int finalWidth = std::max<int>(1, static_cast<int>(finalRect.right - finalRect.left));
        const int finalHeight = std::max<int>(1, static_cast<int>(finalRect.bottom - finalRect.top));
        ++nativeResizeFinalCommitCount_;

        SetWindowPos(
            parent_,
            nullptr,
            finalRect.left,
            finalRect.top,
            finalWidth,
            finalHeight,
            SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER
        );

        RECT client{};
        if (GetClientRect(parent_, &client))
        {
            width_ = std::max<int>(1, static_cast<int>(client.right - client.left));
            height_ = std::max<int>(1, static_cast<int>(client.bottom - client.top));
            pendingLiveResizeWidth_ = width_;
            pendingLiveResizeHeight_ = height_;
            pendingResizeAfterLiveDrag_ = true;
        }
    }

    void AceShellUi::endWindowLiveResize()
    {
        if (!windowLiveResizeActive_)
        {
            return;
        }

        windowLiveResizeActive_ = false;
        ++liveResizeExitCount_;

        // ACE-AQ3D11R3: do not restore the child swapchain immediately at
        // WM_EXITSIZEMOVE. Keep a short resize quarantine so DWM/layout/parent D2D
        // settle before the embedded DX12 child HWND comes back.
        aquariumResizeQuarantineActive_ = true;
        aquariumResizeQuarantineDelaySeconds_ = 0.18f;
        aquariumEmbeddedDx12Viewport_.SetResizeApplySuspended(true);
        applyFrozenNativeResizeCommit();
        applyDeferredLiveResize();
    }

    void AceShellUi::applyDeferredLiveResize()
    {
        if (!pendingResizeAfterLiveDrag_)
        {
            // Still force a final paint/sync point so maximize/restore and native
            // resize edge cases leave the viewport in the correct state. If the
            // child HWND was hidden for resize, keep the D2D proxy visible until
            // syncAquariumEmbeddedViewportWindow restores the child at the final rect.
            aquariumEmbeddedViewportSyncNeeded_ = true;
            invalidate();
            forceLiveResizeProxyRepaintNow();
            return;
        }

        const int finalWidth = std::max(1, pendingLiveResizeWidth_);
        const int finalHeight = std::max(1, pendingLiveResizeHeight_);

        pendingResizeAfterLiveDrag_ = false;
        gradientsDirty_ = true;
        layout(finalWidth, finalHeight);
        aquariumEmbeddedViewportSyncNeeded_ = true;
        ++liveResizeAppliedFinalCount_;
        invalidate();
        forceLiveResizeProxyRepaintNow();
    }

    void AceShellUi::forceLiveResizeProxyRepaintNow()
    {
        if (!parent_)
        {
            return;
        }

        ++liveResizeSynchronousProxyPaintCount_;
        hideAquariumResizeShieldWindow();
        // ACE-AQ3D10/AQ3D11R2: live resize must not wait for a later idle tick/paint.
        // Force a no-erase synchronous parent repaint so the D2D proxy fills the
        // viewport rectangle immediately after each WM_SIZE, before the desktop
        // compositor can expose stale child/DX12 contents.
        RedrawWindow(parent_, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE | RDW_NOCHILDREN);
    }

    void AceShellUi::restoreEmbeddedViewportAfterLiveResize()
    {
        if (!environmentOpen_ || !aquarium3DModeActive_)
        {
            return;
        }

        if (aquariumResizeQuarantineActive_ ||
            windowLiveResizeActive_ ||
            aquariumPanelResizeTarget_ != AquariumPanelResizeTarget::None)
        {
            aquariumEmbeddedDx12Viewport_.SetResizeApplySuspended(true);
            aquariumEmbeddedViewportStatus_ = L"Real DX12 3D resize freeze active";
            return;
        }

        if (aquariumUseSingleHwndCompositeViewport_)
        {
            hideAquariumResizeShieldWindow();
            aquariumEmbeddedViewportSyncNeeded_ = false;
            aquariumEmbeddedViewportVisible_ = false;
            if (aquariumEmbeddedDx12Viewport_.IsVisible())
            {
                aquariumEmbeddedDx12Viewport_.Hide();
                ++aquariumLegacyChildSuppressedCount_;
            }
            invalidateRect(aquariumEmbeddedViewportRect_.empty() ? mainRect_ : aquariumEmbeddedViewportRect_);
            return;
        }

        syncAquariumEmbeddedViewportWindow();

        if (aquariumControllerReady_ && aquariumEmbeddedViewportVisible_ && aquariumEmbeddedDx12Viewport_.IsVisible())
        {
            std::string viewportError;
            if (!aquariumEmbeddedDx12Viewport_.RenderFrame(aquariumController_, aquariumSceneAdapter_, aquariumController_.DebugTruthEnabled(), 1.0f / 60.0f, &viewportError))
            {
                aquariumEmbeddedViewportStatus_ = L"Single-HWND 3D renderer unavailable; check logs.";
            }
        }

        hideAquariumResizeShieldWindow();
    }

    bool AceShellUi::ensureAquariumResizeShieldWindow()
    {
        if (!parent_)
        {
            return false;
        }

        constexpr wchar_t kShieldClassName[] = L"ArhqenCognitionEngineAquariumResizeShieldPopup";
        HINSTANCE instance = GetModuleHandleW(nullptr);

        if (!aquariumResizeShieldClassRegistered_)
        {
            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.style = 0;
            wc.lpfnWndProc = &AceShellUi::AquariumResizeShieldWindowProc;
            wc.hInstance = instance;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hbrBackground = nullptr;
            wc.lpszClassName = kShieldClassName;

            if (RegisterClassExW(&wc) || GetLastError() == ERROR_CLASS_ALREADY_EXISTS)
            {
                aquariumResizeShieldClassRegistered_ = true;
            }
            else
            {
                return false;
            }
        }

        if (aquariumResizeShieldHwnd_)
        {
            return true;
        }

        aquariumResizeShieldHwnd_ = CreateWindowExW(
            WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            kShieldClassName,
            L"",
            WS_POPUP,
            -32000,
            -32000,
            1,
            1,
            parent_,
            nullptr,
            instance,
            this
        );

        if (!aquariumResizeShieldHwnd_)
        {
            return false;
        }

        ++resizeShieldCreateCount_;
        return true;
    }

    void AceShellUi::updateAquariumResizeShieldWindow(UiRect rect)
    {
        if (!windowLiveResizeActive_ || !environmentOpen_ || !aquarium3DModeActive_ || rect.empty())
        {
            return;
        }

        if (!ensureAquariumResizeShieldWindow())
        {
            return;
        }

        POINT topLeft{static_cast<LONG>(std::floor(rect.left)), static_cast<LONG>(std::floor(rect.top))};
        POINT bottomRight{static_cast<LONG>(std::ceil(rect.right)), static_cast<LONG>(std::ceil(rect.bottom))};
        ClientToScreen(parent_, &topLeft);
        ClientToScreen(parent_, &bottomRight);

        const int x = topLeft.x;
        const int y = topLeft.y;
const int w = std::max(1, static_cast<int>(bottomRight.x - topLeft.x));
const int h = std::max(1, static_cast<int>(bottomRight.y - topLeft.y));

        aquariumResizeShieldRect_ = rect;
        ++resizeShieldMoveCount_;
        SetWindowPos(
            aquariumResizeShieldHwnd_,
            HWND_TOP,
            x,
            y,
            w,
            h,
            SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW
        );

        if (!aquariumResizeShieldVisible_)
        {
            ++resizeShieldShowCount_;
        }
        aquariumResizeShieldVisible_ = true;
        RedrawWindow(aquariumResizeShieldHwnd_, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
    }

    void AceShellUi::hideAquariumResizeShieldWindow()
    {
        if (aquariumResizeShieldHwnd_ && aquariumResizeShieldVisible_)
        {
            ++resizeShieldHideCount_;
            ShowWindow(aquariumResizeShieldHwnd_, SW_HIDE);
        }
        aquariumResizeShieldVisible_ = false;
    }

    void AceShellUi::destroyAquariumResizeShieldWindow()
    {
        if (aquariumResizeShieldHwnd_)
        {
            DestroyWindow(aquariumResizeShieldHwnd_);
            aquariumResizeShieldHwnd_ = nullptr;
        }
        aquariumResizeShieldVisible_ = false;
    }

    LRESULT CALLBACK AceShellUi::AquariumResizeShieldWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        AceShellUi* self = nullptr;
        if (message == WM_NCCREATE)
        {
            const auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            self = static_cast<AceShellUi*>(cs ? cs->lpCreateParams : nullptr);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        else
        {
            self = reinterpret_cast<AceShellUi*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        switch (message)
        {
        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
        {
            if (self)
            {
                ++self->resizeShieldPaintCount_;
            }

            PAINTSTRUCT ps{};
            HDC dc = BeginPaint(hwnd, &ps);
            RECT rc{};
            GetClientRect(hwnd, &rc);

            HBRUSH background = CreateSolidBrush(RGB(3, 9, 18));
            FillRect(dc, &rc, background);
            DeleteObject(background);

            HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(18, 60, 70));
            HPEN oldPen = static_cast<HPEN>(SelectObject(dc, gridPen));
            for (int x = 0; x < rc.right; x += 32)
            {
                MoveToEx(dc, x, 0, nullptr);
                LineTo(dc, x, rc.bottom);
            }
            for (int y = 0; y < rc.bottom; y += 32)
            {
                MoveToEx(dc, 0, y, nullptr);
                LineTo(dc, rc.right, y);
            }
            SelectObject(dc, oldPen);
            DeleteObject(gridPen);

            HBRUSH agentBrush = CreateSolidBrush(RGB(80, 230, 210));
            const int cx = (rc.right - rc.left) / 2;
            const int cy = (rc.bottom - rc.top) / 2;
            RECT agent{cx - 7, cy - 7, cx + 7, cy + 7};
            FillRect(dc, &agent, agentBrush);
            DeleteObject(agentBrush);

            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(125, 210, 210));
            RECT textRect{18, 14, std::max<LONG>(18, rc.right - 18), 42};
            DrawTextW(dc, L"Resizing viewport...", -1, &textRect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

            EndPaint(hwnd, &ps);
            return 0;
        }

        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }

    UiRect AceShellUi::inflateRect(UiRect rect, float padding)
    {
        if (rect.empty())
        {
            return rect;
        }

        return am::ui::makeUiRect(rect.left - padding, rect.top - padding, rect.right + padding, rect.bottom + padding);
    }

    void AceShellUi::invalidateRect(UiRect rect)
    {
        if (!parent_ || rect.empty())
        {
            return;
        }

        RECT r{};
        r.left = static_cast<LONG>(std::floor(rect.left));
        r.top = static_cast<LONG>(std::floor(rect.top));
        r.right = static_cast<LONG>(std::ceil(rect.right));
        r.bottom = static_cast<LONG>(std::ceil(rect.bottom));

        ++partialInvalidationCount_;
        ++d2dFrameInvalidationSerial_;
        uiInvalidation_.MarkRect(rect, AceUiDirtyReason::Paint);
        ++aceUi8InvalidationFrameCount_;
        // ACE-AQ3D7: dirty rect invalidation excludes child HWNDs and never
        // triggers background erase. Hover/press updates use this path.
        RedrawWindow(parent_, &r, nullptr, RDW_INVALIDATE | RDW_NOERASE | RDW_NOCHILDREN);
    }

    void AceShellUi::invalidateAquariumChrome()
    {
        if (!environmentOpen_)
        {
            invalidate();
            return;
        }

        // ACE-AQ3D7: repaint panels/topbar/logs, not the DX12 child viewport.
        invalidateRect(inflateRect(aquariumDetailsToggleRect_, 8.0f));
        invalidateRect(inflateRect(aquariumLogsToggleRect_, 8.0f));
        invalidateRect(inflateRect(aquariumDebugRect_, 8.0f));
        invalidateRect(inflateRect(environmentModalCloseRect_, 8.0f));
        invalidateRect(inflateRect(aquariumLeftPanelRect_, 10.0f));
        invalidateRect(inflateRect(aquariumRightLogsPanelRect_, 10.0f));
        if (!aquarium3DModeActive_)
        {
            invalidateRect(inflateRect(environmentModalRect_, 10.0f));
        }
    }

    void AceShellUi::invalidate()
    {
        if (parent_)
        {
            ++fullInvalidationCount_;
            ++d2dFrameInvalidationSerial_;
            uiInvalidation_.Mark(AceUiDirtyReason::All);
            ++aceUi8InvalidationFrameCount_;
            // ACE-AQ3D7: full invalidation is reserved for major layout/mode changes.
            // It still avoids erase and child invalidation to keep DX12 stable.
            RedrawWindow(parent_, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE | RDW_NOCHILDREN);
        }
    }

    void AceShellUi::updateCursor()
    {
        POINT p{};
        GetCursorPos(&p);
        ScreenToClient(parent_, &p);

        const float x = static_cast<float>(p.x);
        const float y = static_cast<float>(p.y);

        if (shortcutHelp_.visible())
        {
            SetCursor(LoadCursorW(nullptr, IDC_ARROW));
            return;
        }

        if (cameraSpeedPopupOpen_ && cameraSpeedInputRect_.contains(x, y))
        {
            SetCursor(LoadCursorW(nullptr, IDC_IBEAM));
            return;
        }
        if (cameraSpeedButtonRect_.contains(x, y))
        {
            SetCursor(LoadCursorW(nullptr, IDC_HAND));
            return;
        }

        if (engineEditorModeActive_)
        {
            const auto cursor = engineWorkspaceController_.cursorAt(x, y);
            if (cursor == am::editor::WorkspaceCursor::ResizeHorizontal)
            {
                SetCursor(LoadCursorW(nullptr, IDC_SIZEWE));
                return;
            }
            if (cursor == am::editor::WorkspaceCursor::ResizeVertical)
            {
                SetCursor(LoadCursorW(nullptr, IDC_SIZENS));
                return;
            }
            bool overMenuRow = false;
            for (const auto& row : engineMenuRows_) overMenuRow = overMenuRow || row.second.contains(x, y);
            if (engineMenuWindowRect_.contains(x, y) || engineMenuViewRect_.contains(x, y) ||
                engineMenuHelpRect_.contains(x, y) || engineBackToAiRect_.contains(x, y) ||
                engineCameraResetRect_.contains(x, y) || engineConsoleToggleRect_.contains(x, y) ||
                engineOutlinerToggleRect_.contains(x, y) || engineDetailsToggleRect_.contains(x, y) ||
                engineShortcutHelpRect_.contains(x, y) || overMenuRow)
                SetCursor(LoadCursorW(nullptr, IDC_HAND));
            else
                SetCursor(LoadCursorW(nullptr, IDC_ARROW));
            return;
        }

        PanelResizeEdge resizeEdges = aquariumPanelResizeTarget_ != AquariumPanelResizeTarget::None
            ? aquariumPanelResizeEdges_
            : PanelResizeEdge::None;
        if (resizeEdges == PanelResizeEdge::None && environmentOpen_ && aquarium3DModeActive_)
        {
            resizeEdges = PanelResizePolicy::hitTest(
                aquariumLeftPanelRect_, x, y, PanelResizeEdge::Right | PanelResizeEdge::Bottom);
            if (resizeEdges == PanelResizeEdge::None)
            {
                resizeEdges = PanelResizePolicy::hitTest(
                    aquariumRightLogsPanelRect_, x, y, PanelResizeEdge::Left | PanelResizeEdge::Bottom);
            }
        }
        const PanelResizeCursor resizeCursor = PanelResizePolicy::cursor(resizeEdges);
        if (resizeCursor != PanelResizeCursor::Arrow)
        {
            LPCWSTR cursorId = IDC_SIZEWE;
            if (resizeCursor == PanelResizeCursor::Vertical) cursorId = IDC_SIZENS;
            else if (resizeCursor == PanelResizeCursor::DiagonalNorthWestSouthEast) cursorId = IDC_SIZENWSE;
            else if (resizeCursor == PanelResizeCursor::DiagonalNorthEastSouthWest) cursorId = IDC_SIZENESW;
            SetCursor(LoadCursorW(nullptr, cursorId));
        }
        else if (commandPalette_.active())
        {
            SetCursor(LoadCursorW(nullptr, IDC_ARROW));
        }
        else if (engineLogOverlayVisible_ &&
            (engineLogOverlayInputRect_.contains(x, y) || engineLogOverlayLogViewportRect_.contains(x, y)))
        {
            SetCursor(LoadCursorW(nullptr, IDC_IBEAM));
        }
        else if (input_.hitTest(static_cast<float>(p.x), static_cast<float>(p.y)))
        {
            SetCursor(LoadCursorW(nullptr, IDC_IBEAM));
        }
        else if (sendButton_.rect().contains(static_cast<float>(p.x), static_cast<float>(p.y)))
        {
            SetCursor(LoadCursorW(nullptr, IDC_HAND));
        }
        else
        {
            SetCursor(LoadCursorW(nullptr, IDC_ARROW));
        }
    }


    ChatMessage AceShellUi::makeMessage(std::wstring author, std::wstring text, bool fromUser, bool system, ChatMessageKind kind, std::wstring metadata)
    {
        ChatMessage message;
        message.author = std::move(author);
        message.text = std::move(text);
        message.fromUser = fromUser;
        message.system = system;
        message.kind = kind;
        message.id = nextMessageId_++;
        message.timestamp = currentTimestamp();
        message.metadata = std::move(metadata);
        return message;
    }

    std::wstring AceShellUi::currentTimestamp() const
    {
        std::time_t now = std::time(nullptr);
        std::tm localTime{};
        localtime_s(&localTime, &now);

        std::wostringstream out;
        out << std::setfill(L'0') << std::setw(2) << localTime.tm_hour
            << L":" << std::setw(2) << localTime.tm_min
            << L":" << std::setw(2) << localTime.tm_sec;
        return out.str();
    }


    std::string AceShellUi::hresultToString(const char* label, HRESULT hr)
    {
        std::ostringstream out;
        out << label << " failed. HRESULT=0x" << std::hex << static_cast<unsigned long>(hr);
        return out.str();
    }
}
