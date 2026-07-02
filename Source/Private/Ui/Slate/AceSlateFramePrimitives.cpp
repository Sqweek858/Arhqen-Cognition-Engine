#include "ArhqenCognitionEngine/Ui/Slate/AceSlateFramePrimitives.h"

#include <iomanip>
#include <cmath>

namespace am::ui::slate
{
    bool AceSlateBatchKey::compatibleWith(const AceSlateBatchKey& other) const
    {
        if (type != other.type) { return false; }
        if (layer != other.layer) { return false; }
        if (resource != other.resource) { return false; }
        if (effects != other.effects) { return false; }
        if (clipEnabled != other.clipEnabled) { return false; }
        if (clipEnabled)
        {
            return clipRect.left == other.clipRect.left &&
                clipRect.top == other.clipRect.top &&
                clipRect.right == other.clipRect.right &&
                clipRect.bottom == other.clipRect.bottom;
        }
        return true;
    }

    std::string AceSlateBatchKey::toString() const
    {
        std::ostringstream oss;
        oss << "type=" << ElementTypeName(type)
            << ";layer=" << layer
            << ";resource=0x" << std::hex << reinterpret_cast<std::uintptr_t>(resource) << std::dec
            << ";effects=" << EffectMaskText(effects)
            << ";clip=" << (clipEnabled ? RectText(clipRect) : std::string("none"));
        return oss.str();
    }

    void AceSlateRenderBatch::clear()
    {
        key = {};
        range = {};
        vertices.clear();
        indices.clear();
        sourceElementFirst = 0;
        sourceElementLast = 0;
    }

    void AceSlateRenderBatch::reserveQuad()
    {
        vertices.reserve(vertices.size() + 4);
        indices.reserve(indices.size() + 6);
    }

    void AceSlateRenderBatch::addQuad(UiRect rect, AceSlateColor color, bool snap)
    {
        if (snap)
        {
            rect = PixelSnapRect(rect);
        }
        if (rect.empty())
        {
            return;
        }
        const std::uint32_t base = static_cast<std::uint32_t>(vertices.size());
        vertices.push_back({rect.left, rect.top, 0.0f, 0.0f, color});
        vertices.push_back({rect.right, rect.top, 1.0f, 0.0f, color});
        vertices.push_back({rect.left, rect.bottom, 0.0f, 1.0f, color});
        vertices.push_back({rect.right, rect.bottom, 1.0f, 1.0f, color});
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        indices.push_back(base + 3);
        range.vertexCount = static_cast<std::uint32_t>(vertices.size());
        range.indexCount = static_cast<std::uint32_t>(indices.size());
    }

    std::string AceSlateRenderBatch::summary() const
    {
        std::ostringstream oss;
        oss << key.toString()
            << ";verts=" << vertices.size()
            << ";indices=" << indices.size()
            << ";elements=" << sourceElementFirst << ".." << sourceElementLast;
        return oss.str();
    }

    bool AceSlateElement::isDrawable() const
    {
        switch (type)
        {
        case AceSlateElementType::Box:
        case AceSlateElementType::RoundedBox:
        case AceSlateElementType::Border:
        case AceSlateElementType::Line:
        case AceSlateElementType::Text:
        case AceSlateElementType::Viewport:
        case AceSlateElementType::Custom:
        case AceSlateElementType::DebugOverlay:
            return !geometry.empty();
        case AceSlateElementType::ClipPush:
        case AceSlateElementType::ClipPop:
        case AceSlateElementType::None:
        default:
            return false;
        }
    }

    bool AceSlateElement::shouldCull(UiRect cullingRect) const
    {
        if (!isDrawable())
        {
            return true;
        }
        if (cullingRect.empty())
        {
            return false;
        }
        return !RectIntersects(geometry.transformedRect(), cullingRect);
    }

    void AceSlateFrameStats::resetForFrame(std::uint64_t nextFrame)
    {
        *this = {};
        frameNumber = nextFrame;
        endDrawHr = S_OK;
        presentHr = S_OK;
    }

    std::string AceSlateFrameStats::compact() const
    {
        std::ostringstream oss;
        oss << "frame=" << frameNumber
            << ";elements=" << elementCount
            << ";viewports=" << viewportElementCount
            << ";culled=" << culledElementCount
            << ";batches=" << batchCount
            << ";verts=" << vertexCount
            << ";indices=" << indexCount
            << ";full=" << fullFramePasses
            << ";partialRejected=" << partialFrameRejected
            << ";vsync=" << (requiresVSync ? "true" : "false")
            << ";presented=" << (presented ? "true" : "false")
            << ";endDraw=" << HResultHex(endDrawHr)
            << ";present=" << HResultHex(presentHr);
        if (!diagnostics.empty())
        {
            oss << ";diag=" << diagnostics;
        }
        return oss.str();
    }

    AceSlateFramePolicy AceSlateFramePolicy::FlipModelFullFrame(std::uint32_t width, std::uint32_t height)
    {
        AceSlateFramePolicy policy{};
        policy.flipSwapChain = true;
        policy.allowDirtyRectPresentation = false;
        policy.forceFullFrameRedraw = true;
        policy.allowRetainedContentsAssumption = false;
        policy.drawLastGoodViewportWhenCurrentUnavailable = true;
        policy.deferViewportUntilResourceReady = true;
        policy.clearOutputEveryFrame = true;
        policy.useViewportAsNormalElement = true;
        policy.collectBatchDiagnostics = true;
        policy.backBufferCount = 2;
        policy.outputWidth = width;
        policy.outputHeight = height;
        return policy;
    }

    AceSlateFramePolicy AceSlateFramePolicy::FlipModelDirtyRect(std::uint32_t width, std::uint32_t height)
    {
        AceSlateFramePolicy policy = FlipModelFullFrame(width, height);
        policy.allowDirtyRectPresentation = true;
        policy.forceFullFrameRedraw = false;
        policy.allowRetainedContentsAssumption = true;
        policy.clearOutputEveryFrame = false;
        return policy;
    }

    bool AceSlateFramePolicy::IsFullFrameRequired() const
    {
        if (forceFullFrameRedraw) { return true; }
        if (flipSwapChain && !allowDirtyRectPresentation) { return true; }
        if (!allowRetainedContentsAssumption) { return true; }
        return false;
    }

    std::string AceSlateFramePolicy::Explain() const
    {
        std::ostringstream oss;
        oss << "flip=" << (flipSwapChain ? "true" : "false")
            << ";dirty_rects=" << (allowDirtyRectPresentation ? "true" : "false")
            << ";full_frame=" << (forceFullFrameRedraw ? "true" : "false")
            << ";retain=" << (allowRetainedContentsAssumption ? "true" : "false")
            << ";last_good_viewport=" << (drawLastGoodViewportWhenCurrentUnavailable ? "true" : "false")
            << ";clear=" << (clearOutputEveryFrame ? "true" : "false")
            << ";buffers=" << backBufferCount
            << ";extent=" << outputWidth << "x" << outputHeight
            << ";epoch=" << resizeEpoch;
        return oss.str();
    }

    void AceSlateFrameDiagnostics::Begin(std::uint64_t frameNumber)
    {
        frameNumber_ = frameNumber;
        items_.clear();
    }

    void AceSlateFrameDiagnostics::Add(std::string_view key, std::string_view value)
    {
        items_.push_back({std::string(key), std::string(value)});
    }

    void AceSlateFrameDiagnostics::Add(std::string_view key, std::uint64_t value)
    {
        items_.push_back({std::string(key), std::to_string(value)});
    }

    void AceSlateFrameDiagnostics::Add(std::string_view key, HRESULT hr)
    {
        items_.push_back({std::string(key), HResultHex(hr)});
    }

    void AceSlateFrameDiagnostics::Merge(std::string_view prefix, const AceSlateFrameStats& stats)
    {
        Add(std::string(prefix) + ".frame", stats.frameNumber);
        Add(std::string(prefix) + ".elements", stats.elementCount);
        Add(std::string(prefix) + ".viewports", stats.viewportElementCount);
        Add(std::string(prefix) + ".batches", stats.batchCount);
        Add(std::string(prefix) + ".presented", stats.presented ? "true" : "false");
        Add(std::string(prefix) + ".endDraw", stats.endDrawHr);
        Add(std::string(prefix) + ".present", stats.presentHr);
    }

    std::string AceSlateFrameDiagnostics::Text() const
    {
        std::ostringstream oss;
        oss << "frame=" << frameNumber_;
        for (const auto& kv : items_)
        {
            oss << ";" << kv.first << "=" << kv.second;
        }
        return oss.str();
    }

    void AceSlateFrameDiagnostics::Clear()
    {
        frameNumber_ = 0;
        items_.clear();
    }

    UiRect IntersectRect(UiRect a, UiRect b)
    {
        return makeUiRect(std::max(a.left, b.left), std::max(a.top, b.top), std::min(a.right, b.right), std::min(a.bottom, b.bottom));
    }

    UiRect UnionRect(UiRect a, UiRect b)
    {
        if (a.empty()) { return b; }
        if (b.empty()) { return a; }
        return makeUiRect(std::min(a.left, b.left), std::min(a.top, b.top), std::max(a.right, b.right), std::max(a.bottom, b.bottom));
    }

    bool RectIntersects(UiRect a, UiRect b)
    {
        if (a.empty() || b.empty()) { return false; }
        return a.right > b.left && a.left < b.right && a.bottom > b.top && a.top < b.bottom;
    }

    UiRect PixelSnapRect(UiRect r)
    {
        return makeUiRect(std::floor(r.left + 0.5f), std::floor(r.top + 0.5f), std::floor(r.right + 0.5f), std::floor(r.bottom + 0.5f));
    }

    const char* ElementTypeName(AceSlateElementType type)
    {
        switch (type)
        {
        case AceSlateElementType::None: return "None";
        case AceSlateElementType::Box: return "Box";
        case AceSlateElementType::RoundedBox: return "RoundedBox";
        case AceSlateElementType::Border: return "Border";
        case AceSlateElementType::Line: return "Line";
        case AceSlateElementType::Text: return "Text";
        case AceSlateElementType::Viewport: return "Viewport";
        case AceSlateElementType::ClipPush: return "ClipPush";
        case AceSlateElementType::ClipPop: return "ClipPop";
        case AceSlateElementType::Custom: return "Custom";
        case AceSlateElementType::DebugOverlay: return "DebugOverlay";
        default: return "Unknown";
        }
    }

    std::string EffectMaskText(AceSlateDrawEffect effects)
    {
        if (effects == AceSlateDrawEffect::None) { return "None"; }
        std::ostringstream oss;
        bool first = true;
        auto append = [&](AceSlateDrawEffect bit, const char* name)
        {
            if (HasEffect(effects, bit))
            {
                if (!first) { oss << "|"; }
                oss << name;
                first = false;
            }
        };
        append(AceSlateDrawEffect::DisabledEffect, "DisabledEffect");
        append(AceSlateDrawEffect::IgnoreTextureAlpha, "IgnoreTextureAlpha");
        append(AceSlateDrawEffect::NoGamma, "NoGamma");
        append(AceSlateDrawEffect::ReverseGamma, "ReverseGamma");
        append(AceSlateDrawEffect::NoBlending, "NoBlending");
        append(AceSlateDrawEffect::PreMultipliedAlpha, "PreMultipliedAlpha");
        append(AceSlateDrawEffect::PixelSnap, "PixelSnap");
        append(AceSlateDrawEffect::NoPixelSnap, "NoPixelSnap");
        append(AceSlateDrawEffect::AllowScaling, "AllowScaling");
        append(AceSlateDrawEffect::RequiresVSync, "RequiresVSync");
        append(AceSlateDrawEffect::ForceOpaque, "ForceOpaque");
        append(AceSlateDrawEffect::HDR, "HDR");
        return oss.str();
    }

    std::string RectText(UiRect rect)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1)
            << rect.left << "," << rect.top << "," << rect.right << "," << rect.bottom;
        return oss.str();
    }

    std::string HResultHex(HRESULT hr)
    {
        std::ostringstream oss;
        oss << "0x" << std::hex << std::setw(8) << std::setfill('0') << static_cast<unsigned long>(hr);
        return oss.str();
    }
}
