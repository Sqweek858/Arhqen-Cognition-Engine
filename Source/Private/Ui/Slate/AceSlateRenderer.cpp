#include "ArhqenCognitionEngine/Ui/Slate/AceSlateRenderer.h"

#include <iomanip>
#include <sstream>

namespace am::ui::slate
{
    namespace
    {
        bool SameColor(AceSlateColor a, AceSlateColor b)
        {
            return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
        }

        std::string ElementDebugLabel(const AceSlateElement& element)
        {
            std::ostringstream oss;
            oss << ElementTypeName(element.type)
                << "#" << element.serial
                << ";layer=" << element.layer
                << ";rect=" << RectText(element.geometry.transformedRect());
            if (!element.debugName.empty())
            {
                oss << ";name=" << element.debugName;
            }
            return oss.str();
        }
    }

    void AceSlateRendererStats::ResetFrameTransient()
    {
        elementsVisited = 0;
        drawableElements = 0;
        viewportElements = 0;
        viewportDraws = 0;
        viewportFallbacks = 0;
        boxDraws = 0;
        roundedBoxDraws = 0;
        borderDraws = 0;
        textDraws = 0;
        clipPushes = 0;
        clipPops = 0;
        failedViewportDraws = 0;
        fullFrameClears = 0;
        framePolicyRejects = 0;
        lastHr = S_OK;
        lastDiagnostics.clear();
    }

    std::string AceSlateRendererStats::Summary() const
    {
        std::ostringstream oss;
        oss << "frames=" << framesRendered
            << ";visited=" << elementsVisited
            << ";drawable=" << drawableElements
            << ";viewports=" << viewportElements
            << ";viewport_draws=" << viewportDraws
            << ";viewport_fallbacks=" << viewportFallbacks
            << ";boxes=" << boxDraws
            << ";rounded=" << roundedBoxDraws
            << ";borders=" << borderDraws
            << ";text=" << textDraws
            << ";clips=" << clipPushes << "/" << clipPops
            << ";brush_creates=" << brushCreates
            << ";brush_hits=" << brushReuseHits
            << ";brush_fail=" << failedBrushCreates
            << ";policy_rejects=" << framePolicyRejects
            << ";hr=" << HResultHex(lastHr);
        if (!lastDiagnostics.empty())
        {
            oss << ";diag=" << lastDiagnostics;
        }
        return oss.str();
    }

    bool AceD2DSlateRenderer::Render(D2DRenderContext& ctx, const AceSlateWindowElementList& list, const AceSlateRendererOptions& options, std::string* error)
    {
        if (!ctx.target)
        {
            if (error) { *error = "AceD2DSlateRenderer requires a live D2D target."; }
            return false;
        }
        stats_.ResetFrameTransient();
        ++stats_.framesRendered;
        if (options.validateLayers && !ValidateLayerOrder(list, error))
        {
            ++stats_.framePolicyRejects;
            return false;
        }
        if (options.clearFullFrame)
        {
            const UiRect full = makeUiRect(0.0f, 0.0f, ctx.width, ctx.height);
            AceSlateElement clear{};
            clear.type = AceSlateElementType::Box;
            clear.geometry.rect = full;
            clear.tint = AceSlateColor::Black(1.0f);
            if (!DrawBox(ctx, clear, options, error))
            {
                return false;
            }
            ++stats_.fullFrameClears;
        }
        for (const auto& element : list.Elements())
        {
            ++stats_.elementsVisited;
            if (element.type == AceSlateElementType::ClipPush)
            {
                BeginClip(ctx, element.geometry.rect);
                continue;
            }
            if (element.type == AceSlateElementType::ClipPop)
            {
                EndClip(ctx);
                continue;
            }
            if (!element.isDrawable())
            {
                continue;
            }
            ++stats_.drawableElements;
            if (element.type == AceSlateElementType::Viewport)
            {
                ++stats_.viewportElements;
            }
            if (!ValidateElementForFlipModel(element, options, error))
            {
                ++stats_.framePolicyRejects;
                return false;
            }
            if (!DrawElement(ctx, element, options, error))
            {
                return false;
            }
            if (options.drawDebugElementOutlines)
            {
                DrawDebugOutline(ctx, element, nullptr);
            }
        }
        while (!clipStack_.empty())
        {
            EndClip(ctx);
        }
        return true;
    }

    bool AceD2DSlateRenderer::RenderBatches(D2DRenderContext& ctx, const std::vector<AceSlateRenderBatch>& batches, const AceSlateRendererOptions& options, std::string* error)
    {
        if (!ctx.target)
        {
            if (error) { *error = "AceD2DSlateRenderer batch render requires a D2D target."; }
            return false;
        }
        stats_.ResetFrameTransient();
        ++stats_.framesRendered;
        for (const auto& batch : batches)
        {
            if (batch.empty())
            {
                continue;
            }
            ++stats_.elementsVisited;
            for (std::size_t i = 0; i + 3 < batch.vertices.size(); i += 4)
            {
                const auto& v0 = batch.vertices[i + 0];
                const auto& v3 = batch.vertices[i + 3];
                AceSlateElement box{};
                box.type = batch.key.type == AceSlateElementType::Viewport ? AceSlateElementType::Viewport : AceSlateElementType::Box;
                box.layer = batch.key.layer;
                box.geometry.rect = makeUiRect(v0.x, v0.y, v3.x, v3.y);
                box.tint = v0.color;
                box.effects = batch.key.effects;
                if (batch.key.type == AceSlateElementType::Viewport)
                {
                    box.viewport.d2dBitmap = static_cast<ID2D1Bitmap1*>(batch.key.resource);
                    box.viewport.valid = box.viewport.d2dBitmap != nullptr;
                }
                if (!DrawElement(ctx, box, options, error))
                {
                    return false;
                }
            }
        }
        return true;
    }

    void AceD2DSlateRenderer::ResetResources()
    {
        for (auto& slot : brushCache_)
        {
            slot = {};
        }
        clipStack_.clear();
    }

    std::string AceD2DSlateRenderer::Diagnostics() const
    {
        return stats_.Summary();
    }

    ID2D1SolidColorBrush* AceD2DSlateRenderer::BrushFor(D2DRenderContext& ctx, AceSlateColor color, std::string* error)
    {
        for (auto& slot : brushCache_)
        {
            if (slot.valid && SameColor(slot.color, color) && slot.brush)
            {
                ++stats_.brushReuseHits;
                return slot.brush.Get();
            }
        }
        BrushSlot* targetSlot = nullptr;
        for (auto& slot : brushCache_)
        {
            if (!slot.valid)
            {
                targetSlot = &slot;
                break;
            }
        }
        if (!targetSlot)
        {
            targetSlot = &brushCache_[stats_.brushCreates % brushCache_.size()];
            targetSlot->brush.Reset();
            targetSlot->valid = false;
        }
        const HRESULT hr = ctx.target->CreateSolidColorBrush(color.d2d(), targetSlot->brush.GetAddressOf());
        stats_.lastHr = hr;
        if (FAILED(hr) || !targetSlot->brush)
        {
            ++stats_.failedBrushCreates;
            if (error) { *error = "CreateSolidColorBrush failed for Slate renderer: " + HResultHex(hr); }
            return nullptr;
        }
        targetSlot->color = color;
        targetSlot->valid = true;
        ++stats_.brushCreates;
        return targetSlot->brush.Get();
    }

    void AceD2DSlateRenderer::BeginClip(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.target || rect.empty())
        {
            return;
        }
        ctx.target->PushAxisAlignedClip(rect.d2d(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        clipStack_.push_back(rect);
        ++stats_.clipPushes;
    }

    void AceD2DSlateRenderer::EndClip(D2DRenderContext& ctx)
    {
        if (!ctx.target || clipStack_.empty())
        {
            return;
        }
        ctx.target->PopAxisAlignedClip();
        clipStack_.pop_back();
        ++stats_.clipPops;
    }

    bool AceD2DSlateRenderer::DrawElement(D2DRenderContext& ctx, const AceSlateElement& element, const AceSlateRendererOptions& options, std::string* error)
    {
        switch (element.type)
        {
        case AceSlateElementType::Box: return DrawBox(ctx, element, options, error);
        case AceSlateElementType::RoundedBox: return DrawRoundedBox(ctx, element, options, error);
        case AceSlateElementType::Border: return DrawBorder(ctx, element, options, error);
        case AceSlateElementType::Text: return DrawText(ctx, element, options, error);
        case AceSlateElementType::Viewport: return DrawViewport(ctx, element, options, error);
        case AceSlateElementType::DebugOverlay: return DrawDebugOutline(ctx, element, error);
        default: return true;
        }
    }

    bool AceD2DSlateRenderer::DrawBox(D2DRenderContext& ctx, const AceSlateElement& element, const AceSlateRendererOptions&, std::string* error)
    {
        auto* brush = BrushFor(ctx, element.tint, error);
        if (!brush) { return false; }
        ctx.target->FillRectangle(element.geometry.transformedRect().d2d(), brush);
        ++stats_.boxDraws;
        return true;
    }

    bool AceD2DSlateRenderer::DrawRoundedBox(D2DRenderContext& ctx, const AceSlateElement& element, const AceSlateRendererOptions& options, std::string* error)
    {
        if (!options.allowRoundedGeometry)
        {
            return DrawBox(ctx, element, options, error);
        }
        auto* brush = BrushFor(ctx, element.tint, error);
        if (!brush) { return false; }
        const auto rect = element.geometry.transformedRect().d2d();
        ctx.target->FillRoundedRectangle(D2D1::RoundedRect(rect, element.radius, element.radius), brush);
        ++stats_.roundedBoxDraws;
        return true;
    }

    bool AceD2DSlateRenderer::DrawBorder(D2DRenderContext& ctx, const AceSlateElement& element, const AceSlateRendererOptions&, std::string* error)
    {
        auto* brush = BrushFor(ctx, element.tint, error);
        if (!brush) { return false; }
        ctx.target->DrawRectangle(element.geometry.transformedRect().d2d(), brush, std::max(0.5f, element.strokeWidth));
        ++stats_.borderDraws;
        return true;
    }

    bool AceD2DSlateRenderer::DrawText(D2DRenderContext& ctx, const AceSlateElement& element, const AceSlateRendererOptions& options, std::string* error)
    {
        if (!options.allowText || !ctx.fonts.body)
        {
            return DrawBox(ctx, element, options, error);
        }
        auto* brush = BrushFor(ctx, element.tint, error);
        if (!brush) { return false; }
        ctx.target->DrawTextW(element.text.c_str(), static_cast<UINT32>(element.text.size()), ctx.fonts.body, element.geometry.transformedRect().d2d(), brush);
        ++stats_.textDraws;
        return true;
    }

    bool AceD2DSlateRenderer::DrawViewport(D2DRenderContext& ctx, const AceSlateElement& element, const AceSlateRendererOptions& options, std::string* error)
    {
        if (!element.viewport.canDraw())
        {
            ++stats_.failedViewportDraws;
            if (options.drawViewportFallbackBox)
            {
                AceSlateElement fallback = element;
                fallback.type = AceSlateElementType::Box;
                fallback.tint = options.fallbackViewportColor;
                ++stats_.viewportFallbacks;
                return DrawBox(ctx, fallback, options, error);
            }
            if (error) { *error = "Viewport element has no drawable D2D bitmap: " + ElementDebugLabel(element); }
            return false;
        }
        ApplyViewportDrawPolicy(element.viewport.d2dBitmap, element.geometry.transformedRect(), ctx, element);
        ++stats_.viewportDraws;
        return true;
    }

    bool AceD2DSlateRenderer::DrawDebugOutline(D2DRenderContext& ctx, const AceSlateElement& element, std::string* error)
    {
        AceSlateElement border{};
        border.type = AceSlateElementType::Border;
        border.geometry = element.geometry;
        border.strokeWidth = 1.0f;
        border.tint = AceSlateColor{0.0f, 0.8f, 1.0f, 0.50f};
        return DrawBorder(ctx, border, {}, error);
    }

    bool AceD2DSlateRenderer::ValidateElementForFlipModel(const AceSlateElement& element, const AceSlateRendererOptions& options, std::string* error)
    {
        if (!options.validateNoRetainedContentAssumption)
        {
            return true;
        }
        if (element.type == AceSlateElementType::Viewport && element.geometry.rect.empty())
        {
            if (error) { *error = "Viewport element has an empty rect in flip-model renderer."; }
            return false;
        }
        return true;
    }

    bool AceD2DSlateRenderer::ValidateLayerOrder(const AceSlateWindowElementList& list, std::string* error)
    {
        int lastLayer = -2147483647;
        for (const auto& element : list.Elements())
        {
            if (element.layer < lastLayer)
            {
                if (error) { *error = "Slate element list is not stable-sorted by layer before render."; }
                return false;
            }
            lastLayer = std::max(lastLayer, element.layer);
        }
        return true;
    }

    void AceD2DSlateRenderer::ApplyViewportDrawPolicy(ID2D1Bitmap1* bitmap, UiRect rect, D2DRenderContext& ctx, const AceSlateElement& element)
    {
        D2D1_BITMAP_INTERPOLATION_MODE interpolation = HasEffect(element.effects, AceSlateDrawEffect::AllowScaling)
            ? D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
            : D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
        ctx.target->DrawBitmap(bitmap, rect.d2d(), std::clamp(element.tint.a, 0.0f, 1.0f), interpolation);
    }
}
