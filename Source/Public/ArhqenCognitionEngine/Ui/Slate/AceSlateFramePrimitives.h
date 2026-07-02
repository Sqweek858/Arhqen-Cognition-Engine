#pragma once

#if !defined(_WIN32)
#error Arhqen Cognition Engine Slate-frame primitives are Windows-only because the current renderer is D2D/DXGI.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"

#include <Windows.h>
#include <d2d1_1.h>
#include <dxgi1_2.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace am::ui::slate
{
    enum class AceSlateElementType : std::uint8_t
    {
        None,
        Box,
        RoundedBox,
        Border,
        Line,
        Text,
        Viewport,
        ClipPush,
        ClipPop,
        Custom,
        DebugOverlay
    };

    enum class AceSlateDrawEffect : std::uint32_t
    {
        None = 0,
        DisabledEffect = 1u << 0,
        IgnoreTextureAlpha = 1u << 1,
        NoGamma = 1u << 2,
        ReverseGamma = 1u << 3,
        NoBlending = 1u << 4,
        PreMultipliedAlpha = 1u << 5,
        PixelSnap = 1u << 6,
        NoPixelSnap = 1u << 7,
        AllowScaling = 1u << 8,
        RequiresVSync = 1u << 9,
        ForceOpaque = 1u << 10,
        HDR = 1u << 11
    };

    constexpr AceSlateDrawEffect operator|(AceSlateDrawEffect a, AceSlateDrawEffect b)
    {
        return static_cast<AceSlateDrawEffect>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
    }

    constexpr AceSlateDrawEffect operator&(AceSlateDrawEffect a, AceSlateDrawEffect b)
    {
        return static_cast<AceSlateDrawEffect>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
    }

    inline AceSlateDrawEffect& operator|=(AceSlateDrawEffect& a, AceSlateDrawEffect b)
    {
        a = a | b;
        return a;
    }

    constexpr bool HasEffect(AceSlateDrawEffect effects, AceSlateDrawEffect bit)
    {
        return (static_cast<std::uint32_t>(effects & bit) != 0u);
    }

    struct AceSlateColor
    {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;

        static AceSlateColor White(float alpha = 1.0f) { return {1.0f, 1.0f, 1.0f, alpha}; }
        static AceSlateColor Black(float alpha = 1.0f) { return {0.0f, 0.0f, 0.0f, alpha}; }

        D2D1_COLOR_F d2d() const
        {
            return D2D1::ColorF(std::clamp(r, 0.0f, 1.0f), std::clamp(g, 0.0f, 1.0f), std::clamp(b, 0.0f, 1.0f), std::clamp(a, 0.0f, 1.0f));
        }
    };

    struct AceSlatePaintGeometry
    {
        UiRect rect{};
        float scale = 1.0f;
        float angleRadians = 0.0f;
        UiPoint translation{};

        bool empty() const { return rect.empty() || scale <= 0.0f; }
        UiRect transformedRect() const
        {
            return makeUiRect(rect.left + translation.x, rect.top + translation.y, rect.right + translation.x, rect.bottom + translation.y);
        }
    };

    struct AceSlateClipState
    {
        UiRect rect{};
        bool enabled = false;
        std::uint32_t depth = 0;

        bool contains(UiRect r) const
        {
            if (!enabled) { return true; }
            return r.right >= rect.left && r.left <= rect.right && r.bottom >= rect.top && r.top <= rect.bottom;
        }
    };

    struct AceSlateViewportDescriptor
    {
        void* nativeResource = nullptr;
        ID2D1Bitmap1* d2dBitmap = nullptr;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        bool valid = false;
        bool allowScaling = true;
        bool requiresVSync = false;
        bool ignoreAlpha = true;
        bool noGamma = false;
        bool premultipliedAlpha = false;
        std::uint64_t resourceEpoch = 0;
        std::uint64_t frameNumber = 0;
        std::string diagnostics;

        bool canDraw() const
        {
            return valid && d2dBitmap != nullptr && width > 0 && height > 0;
        }
    };

    struct AceSlateVertex
    {
        float x = 0.0f;
        float y = 0.0f;
        float u = 0.0f;
        float v = 0.0f;
        AceSlateColor color{};
    };

    struct AceSlateIndexRange
    {
        std::uint32_t firstIndex = 0;
        std::uint32_t indexCount = 0;
        std::uint32_t firstVertex = 0;
        std::uint32_t vertexCount = 0;
    };

    struct AceSlateBatchKey
    {
        AceSlateElementType type = AceSlateElementType::None;
        int layer = 0;
        void* resource = nullptr;
        AceSlateDrawEffect effects = AceSlateDrawEffect::None;
        bool clipEnabled = false;
        UiRect clipRect{};

        bool compatibleWith(const AceSlateBatchKey& other) const;
        std::string toString() const;
    };

    struct AceSlateRenderBatch
    {
        AceSlateBatchKey key{};
        AceSlateIndexRange range{};
        std::vector<AceSlateVertex> vertices;
        std::vector<std::uint32_t> indices;
        std::uint64_t sourceElementFirst = 0;
        std::uint64_t sourceElementLast = 0;

        bool empty() const { return vertices.empty() || indices.empty(); }
        void clear();
        void reserveQuad();
        void addQuad(UiRect rect, AceSlateColor color, bool snap);
        std::string summary() const;
    };

    struct AceSlateElement
    {
        AceSlateElementType type = AceSlateElementType::None;
        int layer = 0;
        AceSlatePaintGeometry geometry{};
        AceSlateDrawEffect effects = AceSlateDrawEffect::None;
        AceSlateColor tint{};
        AceSlateViewportDescriptor viewport{};
        std::wstring text;
        float radius = 0.0f;
        float strokeWidth = 1.0f;
        AceSlateClipState clip{};
        std::uint64_t serial = 0;
        std::string debugName;

        bool isDrawable() const;
        bool isViewport() const { return type == AceSlateElementType::Viewport; }
        bool shouldCull(UiRect cullingRect) const;
    };

    struct AceSlateFrameStats
    {
        std::uint64_t frameNumber = 0;
        std::uint64_t elementCount = 0;
        std::uint64_t viewportElementCount = 0;
        std::uint64_t culledElementCount = 0;
        std::uint64_t batchCount = 0;
        std::uint64_t vertexCount = 0;
        std::uint64_t indexCount = 0;
        std::uint64_t clipPushCount = 0;
        std::uint64_t clipPopCount = 0;
        std::uint64_t fullFramePasses = 0;
        std::uint64_t partialFrameRejected = 0;
        std::uint64_t viewportFallbackBoxes = 0;
        bool requiresVSync = false;
        bool presented = false;
        HRESULT endDrawHr = S_OK;
        HRESULT presentHr = S_OK;
        std::string diagnostics;

        void resetForFrame(std::uint64_t nextFrame);
        std::string compact() const;
    };

    struct AceSlateFramePolicy
    {
        bool flipSwapChain = true;
        bool allowDirtyRectPresentation = false;
        bool forceFullFrameRedraw = true;
        bool allowRetainedContentsAssumption = false;
        bool drawLastGoodViewportWhenCurrentUnavailable = true;
        bool deferViewportUntilResourceReady = true;
        bool clearOutputEveryFrame = true;
        bool useViewportAsNormalElement = true;
        bool collectBatchDiagnostics = true;
        std::uint32_t backBufferCount = 2;
        std::uint32_t resizeEpoch = 0;
        std::uint32_t outputWidth = 0;
        std::uint32_t outputHeight = 0;

        static AceSlateFramePolicy FlipModelFullFrame(std::uint32_t width, std::uint32_t height);
        static AceSlateFramePolicy FlipModelDirtyRect(std::uint32_t width, std::uint32_t height);
        bool IsFullFrameRequired() const;
        std::string Explain() const;
    };

    class AceSlateFrameDiagnostics
    {
    public:
        void Begin(std::uint64_t frameNumber);
        void Add(std::string_view key, std::string_view value);
        void Add(std::string_view key, std::uint64_t value);
        void Add(std::string_view key, HRESULT hr);
        void Merge(std::string_view prefix, const AceSlateFrameStats& stats);
        std::string Text() const;
        void Clear();

    private:
        std::uint64_t frameNumber_ = 0;
        std::vector<std::pair<std::string, std::string>> items_;
    };

    UiRect IntersectRect(UiRect a, UiRect b);
    UiRect UnionRect(UiRect a, UiRect b);
    bool RectIntersects(UiRect a, UiRect b);
    UiRect PixelSnapRect(UiRect r);
    const char* ElementTypeName(AceSlateElementType type);
    std::string EffectMaskText(AceSlateDrawEffect effects);
    std::string RectText(UiRect rect);
    std::string HResultHex(HRESULT hr);
}
