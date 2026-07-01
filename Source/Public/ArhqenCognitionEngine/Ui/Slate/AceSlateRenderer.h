#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DRenderContext.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateElementList.h"

#include <wrl/client.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace am::ui::slate
{
    struct AceSlateRendererStats
    {
        std::uint64_t framesRendered = 0;
        std::uint64_t elementsVisited = 0;
        std::uint64_t drawableElements = 0;
        std::uint64_t viewportElements = 0;
        std::uint64_t viewportDraws = 0;
        std::uint64_t viewportFallbacks = 0;
        std::uint64_t boxDraws = 0;
        std::uint64_t roundedBoxDraws = 0;
        std::uint64_t borderDraws = 0;
        std::uint64_t textDraws = 0;
        std::uint64_t clipPushes = 0;
        std::uint64_t clipPops = 0;
        std::uint64_t brushCreates = 0;
        std::uint64_t brushReuseHits = 0;
        std::uint64_t failedBrushCreates = 0;
        std::uint64_t failedViewportDraws = 0;
        std::uint64_t fullFrameClears = 0;
        std::uint64_t framePolicyRejects = 0;
        HRESULT lastHr = S_OK;
        std::string lastDiagnostics;

        void ResetFrameTransient();
        std::string Summary() const;
    };

    struct AceSlateRendererOptions
    {
        bool clearFullFrame = true;
        bool drawViewportFallbackBox = true;
        bool honorClipStack = true;
        bool forceIgnoreViewportAlpha = true;
        bool drawDebugElementOutlines = false;
        bool allowText = true;
        bool allowRoundedGeometry = true;
        bool validateLayers = true;
        bool validateNoRetainedContentAssumption = true;
        AceSlateColor fallbackViewportColor = AceSlateColor::Black(1.0f);
    };

    class AceD2DSlateRenderer
    {
    public:
        AceD2DSlateRenderer() = default;

        bool Render(D2DRenderContext& ctx, const AceSlateWindowElementList& list, const AceSlateRendererOptions& options, std::string* error = nullptr);
        bool RenderBatches(D2DRenderContext& ctx, const std::vector<AceSlateRenderBatch>& batches, const AceSlateRendererOptions& options, std::string* error = nullptr);
        void ResetResources();
        const AceSlateRendererStats& Stats() const { return stats_; }
        std::string Diagnostics() const;

    private:
        struct BrushSlot
        {
            AceSlateColor color{};
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush{};
            bool valid = false;
        };

        ID2D1SolidColorBrush* BrushFor(D2DRenderContext& ctx, AceSlateColor color, std::string* error);
        void BeginClip(D2DRenderContext& ctx, UiRect rect);
        void EndClip(D2DRenderContext& ctx);
        bool DrawElement(D2DRenderContext& ctx, const AceSlateElement& element, const AceSlateRendererOptions& options, std::string* error);
        bool DrawBox(D2DRenderContext& ctx, const AceSlateElement& element, const AceSlateRendererOptions& options, std::string* error);
        bool DrawRoundedBox(D2DRenderContext& ctx, const AceSlateElement& element, const AceSlateRendererOptions& options, std::string* error);
        bool DrawBorder(D2DRenderContext& ctx, const AceSlateElement& element, const AceSlateRendererOptions& options, std::string* error);
        bool DrawText(D2DRenderContext& ctx, const AceSlateElement& element, const AceSlateRendererOptions& options, std::string* error);
        bool DrawViewport(D2DRenderContext& ctx, const AceSlateElement& element, const AceSlateRendererOptions& options, std::string* error);
        bool DrawDebugOutline(D2DRenderContext& ctx, const AceSlateElement& element, std::string* error);
        bool ValidateElementForFlipModel(const AceSlateElement& element, const AceSlateRendererOptions& options, std::string* error);
        bool ValidateLayerOrder(const AceSlateWindowElementList& list, std::string* error);
        void ApplyViewportDrawPolicy(ID2D1Bitmap1* bitmap, UiRect rect, D2DRenderContext& ctx, const AceSlateElement& element);

        std::array<BrushSlot, 32> brushCache_{};
        std::vector<UiRect> clipStack_{};
        AceSlateRendererStats stats_{};
    };
}
