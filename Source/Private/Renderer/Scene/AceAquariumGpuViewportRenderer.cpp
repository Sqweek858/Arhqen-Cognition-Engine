#include "ArhqenCognitionEngine/Renderer/Scene/AceAquariumGpuViewportRenderer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>

namespace am::renderer::scene
{
    using namespace am::renderer::rhi;
    using ace::aquarium_render::AceAqRenderPrimitive;
    using ace::aquarium_render::AceAqRenderPrimitiveKind;

    namespace
    {
        constexpr U32 kMaxGpuViewportWidth = 1600;
        constexpr U32 kMaxGpuViewportHeight = 900;
        constexpr std::size_t kInitialVertexBufferBytes = 4u * 1024u * 1024u;

        U32 clampViewportDimension(U32 value, U32 maxValue)
        {
            return std::clamp<U32>(value, 64, maxValue);
        }

        float clamp01(float value)
        {
            return std::clamp(value, 0.0f, 1.0f);
        }

        std::array<float, 16> identityWvp()
        {
            return {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            };
        }

        struct Bounds2D
        {
            float minX = 0.0f;
            float maxX = 8.0f;
            float minY = 0.0f;
            float maxY = 8.0f;
            bool valid = false;
        };

        Bounds2D computeBounds(const std::vector<AceAqRenderPrimitive>& primitives)
        {
            Bounds2D bounds{};
            for (const auto& primitive : primitives)
            {
                if (primitive.Kind == AceAqRenderPrimitiveKind::DebugLabel)
                {
                    continue;
                }

                const float x0 = primitive.X;
                const float x1 = primitive.X + std::max(0.04f, primitive.SizeX);
                const float y0 = primitive.Y;
                const float y1 = primitive.Y + std::max(0.04f, primitive.SizeY);

                if (!bounds.valid)
                {
                    bounds.minX = std::min(x0, x1);
                    bounds.maxX = std::max(x0, x1);
                    bounds.minY = std::min(y0, y1);
                    bounds.maxY = std::max(y0, y1);
                    bounds.valid = true;
                }
                else
                {
                    bounds.minX = std::min(bounds.minX, std::min(x0, x1));
                    bounds.maxX = std::max(bounds.maxX, std::max(x0, x1));
                    bounds.minY = std::min(bounds.minY, std::min(y0, y1));
                    bounds.maxY = std::max(bounds.maxY, std::max(y0, y1));
                }
            }

            if (!bounds.valid)
            {
                bounds = {-1.0f, 9.0f, -1.0f, 9.0f, true};
            }

            bounds.minX = std::floor(bounds.minX) - 1.0f;
            bounds.maxX = std::ceil(bounds.maxX) + 1.0f;
            bounds.minY = std::floor(bounds.minY) - 1.0f;
            bounds.maxY = std::ceil(bounds.maxY) + 1.0f;
            return bounds;
        }

        float worldX(float aquariumX)
        {
            // The default real camera starts around x=6.5 and looks forward into +X/+Z.
            // Offset the board into that view instead of putting half of it behind the camera.
            return aquariumX + 4.0f;
        }

        float worldZ(float aquariumY)
        {
            return aquariumY;
        }

        void pushVertex(
            std::vector<AceAquariumGpuViewportRenderer::Vertex>& vertices,
            float x, float y, float z,
            float r, float g, float b, float a)
        {
            vertices.push_back({x, y, z, clamp01(r), clamp01(g), clamp01(b), clamp01(a)});
        }

        void pushTri(
            std::vector<AceAquariumGpuViewportRenderer::Vertex>& vertices,
            float x0, float y0, float z0,
            float x1, float y1, float z1,
            float x2, float y2, float z2,
            float r, float g, float b, float a)
        {
            pushVertex(vertices, x0, y0, z0, r, g, b, a);
            pushVertex(vertices, x1, y1, z1, r, g, b, a);
            pushVertex(vertices, x2, y2, z2, r, g, b, a);
        }

        void pushQuad(
            std::vector<AceAquariumGpuViewportRenderer::Vertex>& vertices,
            float x0, float y0, float z0,
            float x1, float y1, float z1,
            float x2, float y2, float z2,
            float x3, float y3, float z3,
            float r, float g, float b, float a)
        {
            pushTri(vertices, x0, y0, z0, x1, y1, z1, x2, y2, z2, r, g, b, a);
            pushTri(vertices, x0, y0, z0, x2, y2, z2, x3, y3, z3, r, g, b, a);
        }

        void pushBox(
            std::vector<AceAquariumGpuViewportRenderer::Vertex>& vertices,
            float x, float y, float z,
            float w, float h, float d,
            float r, float g, float b, float a)
        {
            const float x0 = x;
            const float x1 = x + w;
            const float y0 = y;
            const float y1 = y + h;
            const float z0 = z;
            const float z1 = z + d;

            // Top
            pushQuad(vertices, x0, y1, z0, x1, y1, z0, x1, y1, z1, x0, y1, z1,
                r * 1.12f, g * 1.12f, b * 1.12f, a);
            // Front
            pushQuad(vertices, x0, y0, z0, x0, y1, z0, x1, y1, z0, x1, y0, z0,
                r * 0.94f, g * 0.94f, b * 0.94f, a);
            // Right
            pushQuad(vertices, x1, y0, z0, x1, y1, z0, x1, y1, z1, x1, y0, z1,
                r * 0.82f, g * 0.82f, b * 0.82f, a);
            // Back
            pushQuad(vertices, x1, y0, z1, x1, y1, z1, x0, y1, z1, x0, y0, z1,
                r * 0.70f, g * 0.70f, b * 0.70f, a);
            // Left
            pushQuad(vertices, x0, y0, z1, x0, y1, z1, x0, y1, z0, x0, y0, z0,
                r * 0.78f, g * 0.78f, b * 0.78f, a);
        }

        void pushFlatRect(
            std::vector<AceAquariumGpuViewportRenderer::Vertex>& vertices,
            float x, float z,
            float w, float d,
            float y,
            float r, float g, float b, float a)
        {
            pushQuad(vertices,
                x, y, z,
                x + w, y, z,
                x + w, y, z + d,
                x, y, z + d,
                r, g, b, a);
        }

        void pushLinePrism(
            std::vector<AceAquariumGpuViewportRenderer::Vertex>& vertices,
            float x0, float z0,
            float x1, float z1,
            float thickness,
            float y,
            float r, float g, float b, float a)
        {
            const float dx = x1 - x0;
            const float dz = z1 - z0;
            const float len = std::max(0.0001f, std::sqrt(dx * dx + dz * dz));
            const float nx = -dz / len * thickness;
            const float nz = dx / len * thickness;

            // Oriented two-triangle strip on the X/Z plane.
            const float ax = x0 + nx;
            const float az = z0 + nz;
            const float bx = x1 + nx;
            const float bz = z1 + nz;
            const float cx = x1 - nx;
            const float cz = z1 - nz;
            const float dxp = x0 - nx;
            const float dzp = z0 - nz;

            pushQuad(vertices, ax, y, az, bx, y, bz, cx, y, cz, dxp, y, dzp, r, g, b, a);
        }


        std::array<U8, 7> aceGpuGlyph(char raw)
        {
            const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(raw)));
            switch (c)
            {
            case '0': return {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E};
            case '1': return {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E};
            case '2': return {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F};
            case '3': return {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E};
            case '4': return {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02};
            case '5': return {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E};
            case '6': return {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E};
            case '7': return {0x1F,0x01,0x02,0x04,0x08,0x08,0x08};
            case '8': return {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E};
            case '9': return {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C};
            case 'A': return {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11};
            case 'B': return {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E};
            case 'C': return {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E};
            case 'D': return {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E};
            case 'E': return {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F};
            case 'F': return {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10};
            case 'G': return {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F};
            case 'H': return {0x11,0x11,0x11,0x1F,0x11,0x11,0x11};
            case 'I': return {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E};
            case 'J': return {0x07,0x02,0x02,0x02,0x12,0x12,0x0C};
            case 'K': return {0x11,0x12,0x14,0x18,0x14,0x12,0x11};
            case 'L': return {0x10,0x10,0x10,0x10,0x10,0x10,0x1F};
            case 'M': return {0x11,0x1B,0x15,0x15,0x11,0x11,0x11};
            case 'N': return {0x11,0x19,0x15,0x13,0x11,0x11,0x11};
            case 'O': return {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E};
            case 'P': return {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10};
            case 'Q': return {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D};
            case 'R': return {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11};
            case 'S': return {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E};
            case 'T': return {0x1F,0x04,0x04,0x04,0x04,0x04,0x04};
            case 'U': return {0x11,0x11,0x11,0x11,0x11,0x11,0x0E};
            case 'V': return {0x11,0x11,0x11,0x11,0x11,0x0A,0x04};
            case 'W': return {0x11,0x11,0x11,0x15,0x15,0x15,0x0A};
            case 'X': return {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11};
            case 'Y': return {0x11,0x11,0x0A,0x04,0x04,0x04,0x04};
            case 'Z': return {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F};
            case '[': return {0x0E,0x08,0x08,0x08,0x08,0x08,0x0E};
            case ']': return {0x0E,0x02,0x02,0x02,0x02,0x02,0x0E};
            case '(': return {0x02,0x04,0x08,0x08,0x08,0x04,0x02};
            case ')': return {0x08,0x04,0x02,0x02,0x02,0x04,0x08};
            case ':': return {0x00,0x04,0x04,0x00,0x04,0x04,0x00};
            case '.': return {0x00,0x00,0x00,0x00,0x00,0x04,0x04};
            case ',': return {0x00,0x00,0x00,0x00,0x04,0x04,0x08};
            case '-': return {0x00,0x00,0x00,0x1F,0x00,0x00,0x00};
            case '_': return {0x00,0x00,0x00,0x00,0x00,0x00,0x1F};
            case '=': return {0x00,0x00,0x1F,0x00,0x1F,0x00,0x00};
            case '+': return {0x00,0x04,0x04,0x1F,0x04,0x04,0x00};
            case '/': return {0x01,0x02,0x02,0x04,0x08,0x08,0x10};
            case '\\': return {0x10,0x08,0x08,0x04,0x02,0x02,0x01};
            case '|': return {0x04,0x04,0x04,0x04,0x04,0x04,0x04};
            case '>': return {0x10,0x08,0x04,0x02,0x04,0x08,0x10};
            case '<': return {0x01,0x02,0x04,0x08,0x04,0x02,0x01};
            case '!': return {0x04,0x04,0x04,0x04,0x04,0x00,0x04};
            case '?': return {0x0E,0x11,0x01,0x02,0x04,0x00,0x04};
            default: return {0x00,0x00,0x00,0x00,0x00,0x00,0x00};
            }
        }

        void pushOverlayRectPx(
            std::vector<AceAquariumGpuViewportRenderer::Vertex>& vertices,
            Extent2D extent,
            float x, float y, float w, float h,
            float r, float g, float b, float a)
        {
            if (w <= 0.0f || h <= 0.0f || extent.width == 0 || extent.height == 0)
            {
                return;
            }
            const float iw = 1.0f / static_cast<float>(extent.width);
            const float ih = 1.0f / static_cast<float>(extent.height);
            const float x0 = -1.0f + 2.0f * x * iw;
            const float y0 =  1.0f - 2.0f * y * ih;
            const float x1 = -1.0f + 2.0f * (x + w) * iw;
            const float y1 =  1.0f - 2.0f * (y + h) * ih;
            pushQuad(vertices,
                x0, y0, 0.0f,
                x1, y0, 0.0f,
                x1, y1, 0.0f,
                x0, y1, 0.0f,
                r, g, b, a);
        }

        void pushOverlayTextPx(
            std::vector<AceAquariumGpuViewportRenderer::Vertex>& vertices,
            Extent2D extent,
            const AceAquariumGpuOverlayText& text)
        {
            const float scale = std::clamp(text.scale, 1.0f, 5.0f);
            float penX = text.x;
            const float cell = scale;
            const float advance = 6.0f * scale;
            const std::size_t maxChars = static_cast<std::size_t>(std::max(1.0f, (static_cast<float>(extent.width) - text.x - 8.0f) / std::max(1.0f, advance)));
            std::size_t emitted = 0;
            for (char ch : text.text)
            {
                if (emitted++ >= maxChars)
                {
                    break;
                }
                if (ch == ' ')
                {
                    penX += advance;
                    continue;
                }
                const auto rows = aceGpuGlyph(ch);
                for (int row = 0; row < 7; ++row)
                {
                    for (int col = 0; col < 5; ++col)
                    {
                        if ((rows[static_cast<std::size_t>(row)] & (1 << (4 - col))) == 0)
                        {
                            continue;
                        }
                        pushOverlayRectPx(vertices, extent,
                            penX + static_cast<float>(col) * cell,
                            text.y + static_cast<float>(row) * cell,
                            cell, cell,
                            text.r, text.g, text.b, text.a);
                    }
                }
                penX += advance;
            }
        }
    }

    AceAquariumGpuViewportRenderer::AceAquariumGpuViewportRenderer() = default;

    AceAquariumGpuViewportRenderer::~AceAquariumGpuViewportRenderer()
    {
        shutdown();
    }

    bool AceAquariumGpuViewportRenderer::initialize(std::string* error)
    {
        if (device_ && device_->initialized())
        {
            return true;
        }

        if (!Dx12RuntimeAvailable())
        {
            if (error) { *error = "DX12 runtime unavailable for Aquarium GPU viewport."; }
            return false;
        }

        device_ = CreateDx12DeviceConcrete();
        DeviceDesc desc{};
        desc.backend = Backend::Dx12;
        desc.name = Name{"AceAquariumGpuViewportRenderer"};
        desc.framesInFlight = 2;

        if (!device_->initialize(desc, error))
        {
            device_.reset();
            return false;
        }

        if (!hasWorldToClip_)
        {
            worldToClip_ = identityWvp();
        }

        return true;
    }

    void AceAquariumGpuViewportRenderer::shutdown()
    {
        if (device_)
        {
            device_->shutdown();
            device_.reset();
        }

        sceneColor_ = {};
        sceneDepth_ = {};
        vertexBuffer_ = {};
        vertexShader_ = {};
        pipeline_ = {};
        overlayPipeline_ = {};
        targetExtent_ = {};
        vertexBufferCapacityBytes_ = 0;
    }

    void AceAquariumGpuViewportRenderer::setWorldToClipMatrix(const std::array<float, 16>& matrix)
    {
        worldToClip_ = matrix;
        hasWorldToClip_ = true;
    }

    void AceAquariumGpuViewportRenderer::resetWorldToClipMatrix()
    {
        worldToClip_ = identityWvp();
        hasWorldToClip_ = false;
    }


    void AceAquariumGpuViewportRenderer::resetCompositionHost()
    {
        if (device_)
        {
            device_->resetCompositionHost();
        }
    }

    am::renderer::rhi::Dx12GpuAllocationStats AceAquariumGpuViewportRenderer::gpuStats() const
    {
        return device_ ? device_->gpuStats() : am::renderer::rhi::Dx12GpuAllocationStats{};
    }

    Extent2D AceAquariumGpuViewportRenderer::viewportTextureExtent() const
    {
        return targetExtent_;
    }

    AceViewportTextureResource AceAquariumGpuViewportRenderer::viewportTextureResource() const
    {
        if (!sceneColor_.valid() || targetExtent_.width == 0 || targetExtent_.height == 0)
        {
            return {};
        }

        auto resource = MakeAceRhiViewportTextureResource(
            sceneColor_,
            targetExtent_,
            Format::BGRA8,
            Backend::Dx12,
            "AceAquarium.SceneColor");
        if (resource.validGpuTexture() && device_)
        {
            resource.nativeResource = device_->nativeD3D12TextureResource(sceneColor_);
            if (resource.nativeResource)
            {
                resource.kind = AceViewportTextureResourceKind::NativeGpuResource;
            }
        }
        return resource;
    }

    AceViewportTextureBridgeStatus AceAquariumGpuViewportRenderer::viewportBridgeStatus(bool uiGpuTextureSamplingAvailable) const
    {
        return EvaluateAceViewportTextureBridge(viewportTextureResource(), uiGpuTextureSamplingAvailable);
    }

    void* AceAquariumGpuViewportRenderer::nativeD3D12Device() const
    {
        return device_ ? device_->nativeD3D12Device() : nullptr;
    }

    void* AceAquariumGpuViewportRenderer::nativeD3D12GraphicsQueue() const
    {
        return device_ ? device_->nativeD3D12GraphicsQueue() : nullptr;
    }

    bool AceAquariumGpuViewportRenderer::initialized() const
    {
        return device_ && device_->initialized();
    }

    bool AceAquariumGpuViewportRenderer::ensureTargets(Extent2D extent, std::string* error)
    {
        if (!initialize(error))
        {
            return false;
        }

        extent.width = clampViewportDimension(extent.width, kMaxGpuViewportWidth);
        extent.height = clampViewportDimension(extent.height, kMaxGpuViewportHeight);

        if (sceneColor_.valid() && sceneDepth_.valid() &&
            targetExtent_.width == extent.width &&
            targetExtent_.height == extent.height)
        {
            return true;
        }

        if (!device_->createOffscreenSceneTargets(extent, &sceneColor_, &sceneDepth_, error))
        {
            return false;
        }

        targetExtent_ = extent;
        ++stats_.targetResizes;
        return true;
    }

    bool AceAquariumGpuViewportRenderer::ensurePipeline(std::string* error)
    {
        if (pipeline_.valid())
        {
            return true;
        }

        ShaderDesc vs{};
        vs.name = Name{"AceRhi7_AquariumWvpVS"};
        vs.stage = ShaderStage::Vertex;
        vertexShader_ = device_->resources().create(vs, error);
        if (!vertexShader_.valid())
        {
            return false;
        }

        GraphicsPipelineDesc pso{};
        pso.name = Name{"AceRhi7_AquariumWvpBasicColor"};
        pso.vs = vertexShader_;
        pso.topology = Topology::TriangleList;
        pso.colorFormats.push_back(Format::BGRA8);
        pso.depthFormat = Format::D32F;
        pipeline_ = device_->resources().create(pso, error);
        return pipeline_.valid();
    }

    bool AceAquariumGpuViewportRenderer::ensureOverlayPipeline(std::string* error)
    {
        if (overlayPipeline_.valid())
        {
            return true;
        }

        if (!ensurePipeline(error))
        {
            return false;
        }

        GraphicsPipelineDesc pso{};
        pso.name = Name{"AcePerf2_GpuOverlayNdcBasicColor"};
        pso.vs = vertexShader_;
        pso.topology = Topology::TriangleList;
        pso.colorFormats.push_back(Format::BGRA8);
        pso.depthFormat = Format::Unknown;
        overlayPipeline_ = device_->resources().create(pso, error);
        return overlayPipeline_.valid();
    }

    bool AceAquariumGpuViewportRenderer::ensureVertexBuffer(std::size_t requiredBytes, std::string* error)
    {
        if (vertexBuffer_.valid() && vertexBufferCapacityBytes_ >= requiredBytes)
        {
            return true;
        }

        const std::size_t capacity = std::max(requiredBytes, kInitialVertexBufferBytes);
        BufferDesc desc{};
        desc.name = Name{"AceRhi7_Aquarium3DVertexBuffer"};
        desc.size = static_cast<U64>(capacity);
        desc.stride = sizeof(Vertex);
        // ACE-PERF1: mirror UE/Slate's dynamic element upload model. The
        // Aquarium debug scene emits a tiny transient vertex stream, so keeping
        // it persistently mapped avoids a per-frame CopyBuffer command list and
        // fence wait. Bigger world geometry can move back to GPU-only buffers
        // once ACE has a proper render thread/resource lifetime model.
        desc.usage = Usage::Vertex;
        desc.memory = Memory::Upload;
        desc.persistentMap = true;

        vertexBuffer_ = device_->resources().create(desc, error);
        if (!vertexBuffer_.valid())
        {
            vertexBufferCapacityBytes_ = 0;
            return false;
        }

        vertexBufferCapacityBytes_ = capacity;
        return true;
    }

    void AceAquariumGpuViewportRenderer::buildVertices(
        const std::vector<AceAqRenderPrimitive>& primitives,
        bool debugTruthEnabled,
        std::vector<Vertex>& outVertices) const
    {
        outVertices.clear();
        outVertices.reserve(primitives.size() * 48 + 512);

        const Bounds2D bounds = computeBounds(primitives);

        // ACE-RHI7: real 3D grid on the X/Z plane, depth-tested by DX12.
        for (int y = static_cast<int>(bounds.minY); y <= static_cast<int>(bounds.maxY); ++y)
        {
            pushLinePrism(outVertices, worldX(bounds.minX), worldZ(static_cast<float>(y)),
                worldX(bounds.maxX), worldZ(static_cast<float>(y)), 0.012f, 0.015f,
                0.07f, 0.44f, 0.58f, 0.42f);
        }

        for (int x = static_cast<int>(bounds.minX); x <= static_cast<int>(bounds.maxX); ++x)
        {
            pushLinePrism(outVertices, worldX(static_cast<float>(x)), worldZ(bounds.minY),
                worldX(static_cast<float>(x)), worldZ(bounds.maxY), 0.012f, 0.015f,
                0.07f, 0.44f, 0.58f, 0.42f);
        }

        for (const auto& primitive : primitives)
        {
            const float x = worldX(primitive.X);
            const float z = worldZ(primitive.Y);
            const float w = std::max(0.05f, primitive.SizeX);
            const float d = std::max(0.05f, primitive.SizeY);
            const float r = clamp01(primitive.R);
            const float g = clamp01(primitive.G);
            const float b = clamp01(primitive.B);
            const float a = clamp01(primitive.A);

            switch (primitive.Kind)
            {
            case AceAqRenderPrimitiveKind::Tile:
                pushFlatRect(outVertices, x, z, w, d, 0.02f, r * 0.65f, g * 0.65f, b * 0.65f, std::max(0.35f, a));
                break;

            case AceAqRenderPrimitiveKind::Block:
                pushBox(outVertices, x, 0.04f, z, w, std::max(0.70f, primitive.SizeZ), d, r, g, b, std::max(0.70f, a));
                pushBox(outVertices, x + w * 0.18f, std::max(0.72f, primitive.SizeZ), z + d * 0.18f,
                    w * 0.64f, 0.08f, d * 0.20f,
                    std::min(1.0f, r + 0.16f), std::min(1.0f, g + 0.16f), std::min(1.0f, b + 0.16f), 0.92f);
                break;

            case AceAqRenderPrimitiveKind::Agent:
                pushBox(outVertices, x - 0.12f, 0.04f, z - 0.12f, w + 0.24f, 0.18f, d + 0.24f, 0.02f, 0.42f, 0.58f, 0.35f);
                pushBox(outVertices, x, 0.05f, z, w, 1.25f, d, 0.06f, 0.92f, 1.0f, 0.98f);
                break;

            case AceAqRenderPrimitiveKind::DirectionArrow:
                pushLinePrism(outVertices, x, z, x + primitive.SizeX, z + primitive.SizeY, 0.045f, 0.18f, 1.0f, 0.84f, 0.16f, 0.98f);
                pushBox(outVertices, x + primitive.SizeX - 0.08f, 0.12f, z + primitive.SizeY - 0.08f,
                    0.16f, 0.16f, 0.16f, 1.0f, 0.84f, 0.16f, 0.98f);
                break;

            case AceAqRenderPrimitiveKind::Highlight:
                pushFlatRect(outVertices, x, z, w, d, 0.10f, 1.0f, 0.82f, 0.16f, 0.48f);
                break;

            case AceAqRenderPrimitiveKind::DebugLabel:
                if (debugTruthEnabled)
                {
                    pushBox(outVertices, x, 0.10f, z, 0.26f, 0.12f, 0.08f, 1.0f, 0.50f, 0.12f, 0.86f);
                }
                break;

            case AceAqRenderPrimitiveKind::GridLine:
                break;
            }
        }
    }

    void AceAquariumGpuViewportRenderer::appendOverlayVertices(
        const AceAquariumGpuViewportOverlay& overlay,
        Extent2D extent,
        std::vector<Vertex>& outVertices) const
    {
        if (!overlay.enabled)
        {
            return;
        }

        for (const auto& rect : overlay.rects)
        {
            pushOverlayRectPx(outVertices, extent, rect.x, rect.y, rect.w, rect.h, rect.r, rect.g, rect.b, rect.a);
        }
        for (const auto& text : overlay.texts)
        {
            pushOverlayTextPx(outVertices, extent, text);
        }
    }

    bool AceAquariumGpuViewportRenderer::render(
        const std::vector<AceAqRenderPrimitive>& primitives,
        U32 requestedWidth,
        U32 requestedHeight,
        bool debugTruthEnabled,
        AceAquariumGpuViewportSnapshot* snapshot,
        std::string* error,
        void* compositionHwnd,
        float compositionLeft,
        float compositionTop)
    {
        return render(primitives, requestedWidth, requestedHeight, debugTruthEnabled, snapshot, error, nullptr, compositionHwnd, compositionLeft, compositionTop, false);
    }

    bool AceAquariumGpuViewportRenderer::render(
        const std::vector<AceAqRenderPrimitive>& primitives,
        U32 requestedWidth,
        U32 requestedHeight,
        bool debugTruthEnabled,
        AceAquariumGpuViewportSnapshot* snapshot,
        std::string* error,
        const AceAquariumGpuViewportOverlay* overlay,
        void* compositionHwnd,
        float compositionLeft,
        float compositionTop,
        bool preferD2DTextureBridge)
    {
        if (!snapshot)
        {
            if (error) { *error = "Aquarium GPU viewport render requires snapshot output."; }
            return false;
        }

        snapshot->valid = false;
        snapshot->gpuRendered = false;
        snapshot->zeroCopyPresented = false;
        snapshot->gpuComposited = false;
        snapshot->overlayBaked = false;
        snapshot->combinedRenderReadback = false;
        snapshot->d2dTextureBridgeReady = false;
        snapshot->viewportTexture = {};
        snapshot->viewportBridge = {};
        snapshot->status.clear();

        Extent2D extent{
            clampViewportDimension(requestedWidth, kMaxGpuViewportWidth),
            clampViewportDimension(requestedHeight, kMaxGpuViewportHeight)
        };

        if (!ensureTargets(extent, error) || !ensurePipeline(error))
        {
            return false;
        }

        snapshot->viewportTexture = viewportTextureResource();
        snapshot->viewportBridge = viewportBridgeStatus(false);
        if (snapshot->viewportTexture.validGpuTexture())
        {
            ++stats_.viewportTextureExports;
        }

        std::vector<Vertex> vertices;
        buildVertices(primitives, debugTruthEnabled, vertices);
        if (vertices.empty())
        {
            if (error) { *error = "Aquarium GPU viewport generated no vertices."; }
            return false;
        }

        const U32 sceneVertexCount = static_cast<U32>(vertices.size());
        const bool bakeOverlay = overlay && overlay->enabled;
        if (bakeOverlay)
        {
            if (!ensureOverlayPipeline(error))
            {
                return false;
            }
            appendOverlayVertices(*overlay, extent, vertices);
        }
        const U32 overlayFirstVertex = sceneVertexCount;
        const U32 overlayVertexCount = static_cast<U32>(vertices.size()) - sceneVertexCount;

        const std::size_t uploadBytes = vertices.size() * sizeof(Vertex);
        if (!ensureVertexBuffer(uploadBytes, error))
        {
            return false;
        }

        if (!device_->upload(vertexBuffer_, vertices.data(), static_cast<U64>(uploadBytes), error))
        {
            return false;
        }

        CommandList list;
        RenderPassDesc pass{};
        pass.name = Name{"AceRhi7_AquariumDepthTested3DPass"};
        pass.extent = extent;
        pass.colors.push_back({sceneColor_, LoadOp::Clear, StoreOp::Store, {{0.006f, 0.012f, 0.025f, 1.0f}, 1.0f, 0}});
        pass.depth = DepthAttachment{sceneDepth_, LoadOp::Clear, StoreOp::Store, 1.0f, 0};

        list.begin(pass);
        list.viewport({0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f});
        list.scissor({0, 0, extent.width, extent.height});
        list.pipeline(pipeline_);
        list.wvp(worldToClip_);
        list.vertexBuffer(0, vertexBuffer_, 0);
        list.draw(sceneVertexCount);
        list.end();

        if (overlayVertexCount > 0)
        {
            RenderPassDesc overlayPass{};
            overlayPass.name = Name{"AcePerf2_ViewportGpuOverlayPass"};
            overlayPass.extent = extent;
            overlayPass.colors.push_back({sceneColor_, LoadOp::Load, StoreOp::Store, {{0.0f, 0.0f, 0.0f, 0.0f}, 1.0f, 0}});

            list.begin(overlayPass);
            list.viewport({0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f});
            list.scissor({0, 0, extent.width, extent.height});
            list.pipeline(overlayPipeline_);
            list.wvp(identityWvp());
            list.vertexBuffer(0, vertexBuffer_, 0);
            list.draw(overlayVertexCount, 1, overlayFirstVertex);
            list.end();
        }

        if (!device_->beginFrame(stats_.framesRendered + 1, 1.0f / 60.0f, error))
        {
            return false;
        }

        if (!compositionHwnd && preferD2DTextureBridge)
        {
            list.barrier(sceneColor_, Access::RenderTarget, Access::ShaderRead);
            const bool submitOk = device_->submit({Queue::Graphics, &list}, error);
            if (!device_->endFrame(submitOk ? error : nullptr) || !submitOk)
            {
                return false;
            }

            snapshot->valid = true;
            snapshot->gpuRendered = true;
            snapshot->d2dTextureBridgeReady = true;
            snapshot->viewportTexture = viewportTextureResource();
            snapshot->viewportBridge = viewportBridgeStatus(true);
            snapshot->extent = extent;
            snapshot->status = "DX12 SceneColor rendered as GPU texture for D2D shared-bitmap bridge; no CPU readback requested";
            ++stats_.framesRendered;
            ++stats_.d2dTextureBridgeFrames;
            device_->noteD2DTextureBridgeFrame();
            stats_.verticesUploaded += static_cast<U64>(vertices.size());
            stats_.lastPrimitiveCount = static_cast<U32>(primitives.size());
            stats_.lastVertexCount = static_cast<U32>(vertices.size());
            stats_.lastExtent = extent;
            return true;
        }

        if (!compositionHwnd)
        {
            // ACE-PERF1R1: this is the hot path visible in the app as DX12_READBACK.
            // Keep the UE/Slate-style parent composition model, but stop submitting
            // render and readback as two separately blocking GPU jobs.
            const bool combinedOk = device_->submitAndReadbackBgra8(
                {Queue::Graphics, &list},
                sceneColor_,
                &snapshot->bgraPixels,
                &snapshot->extent,
                error);

            if (!device_->endFrame(combinedOk ? error : nullptr) || !combinedOk)
            {
                return false;
            }

            snapshot->valid = true;
            snapshot->gpuRendered = true;
            snapshot->combinedRenderReadback = true;
            snapshot->overlayBaked = overlayVertexCount > 0;
            snapshot->status = overlayVertexCount > 0 ?
                "DX12 SceneColor+GPU overlay rendered then copied through combined render+readback command list" :
                "DX12 SceneColor rendered and copied through combined render+readback command list";
            ++stats_.framesRendered;
            ++stats_.readbackFrames;
            ++stats_.combinedReadbackFrames;
            if (snapshot->viewportBridge.readbackFallbackRequired)
            {
                ++stats_.viewportBridgeReadbackFallbacks;
            }
            stats_.verticesUploaded += static_cast<U64>(vertices.size());
            if (overlayVertexCount > 0)
            {
                ++stats_.gpuOverlayBakedFrames;
                stats_.gpuOverlayVertexCount += overlayVertexCount;
            }
            stats_.lastPrimitiveCount = static_cast<U32>(primitives.size());
            stats_.lastVertexCount = static_cast<U32>(vertices.size());
            stats_.lastExtent = snapshot->extent;
            return true;
        }

        const bool presentedThroughGpuComposition = device_->submitAndPresentBgra8ToComposition(
            {Queue::Graphics, &list},
            sceneColor_,
            compositionHwnd,
            compositionLeft,
            compositionTop,
            extent,
            nullptr);

        if (presentedThroughGpuComposition)
        {
            if (!device_->endFrame(error))
            {
                return false;
            }
            device_->noteGpuViewportComposition(overlayVertexCount);
            snapshot->valid = true;
            snapshot->gpuRendered = true;
            snapshot->zeroCopyPresented = true;
            snapshot->gpuComposited = true;
            snapshot->overlayBaked = overlayVertexCount > 0;
            snapshot->extent = extent;
            snapshot->status = overlayVertexCount > 0 ?
                "DX12 SceneColor + viewport UI baked into GPU texture and presented through one combined GPU composition command list" :
                "DX12 SceneColor presented through one combined DirectComposition GPU command list";
            ++stats_.framesRendered;
            ++stats_.zeroCopyFrames;
            ++stats_.gpuCompositedFrames;
            stats_.verticesUploaded += static_cast<U64>(vertices.size());
            if (overlayVertexCount > 0)
            {
                ++stats_.gpuOverlayBakedFrames;
                stats_.gpuOverlayVertexCount += overlayVertexCount;
            }
            stats_.lastPrimitiveCount = static_cast<U32>(primitives.size());
            stats_.lastVertexCount = static_cast<U32>(vertices.size());
            stats_.lastExtent = extent;
            return true;
        }

        device_->endFrame(nullptr);

        // If DirectComposition setup/present failed before the scene command list
        // executed, do not read stale pixels from a previous SceneColor. Re-run
        // the exact scene+overlay command list through the verified combined
        // readback fallback. Fallback is allowed; stale fallback frames are not.
        if (!device_->beginFrame(stats_.framesRendered + 1, 1.0f / 60.0f, error))
        {
            return false;
        }

        const bool fallbackReadbackOk = device_->submitAndReadbackBgra8(
            {Queue::Graphics, &list},
            sceneColor_,
            &snapshot->bgraPixels,
            &snapshot->extent,
            error);

        if (!device_->endFrame(fallbackReadbackOk ? error : nullptr) || !fallbackReadbackOk)
        {
            return false;
        }

        snapshot->valid = true;
        snapshot->gpuRendered = true;
        snapshot->combinedRenderReadback = true;
        snapshot->overlayBaked = overlayVertexCount > 0;
        snapshot->status = overlayVertexCount > 0 ?
            "GPU SceneColor+overlay rendered through combined readback fallback after DirectComposition present failed" :
            "GPU SceneColor/SceneDepth rendered through combined readback fallback after DirectComposition present failed";
        ++stats_.framesRendered;
        ++stats_.readbackFrames;
        ++stats_.combinedReadbackFrames;
        if (snapshot->viewportBridge.readbackFallbackRequired)
        {
            ++stats_.viewportBridgeReadbackFallbacks;
        }
        stats_.verticesUploaded += static_cast<U64>(vertices.size());
        if (overlayVertexCount > 0)
        {
            ++stats_.gpuOverlayBakedFrames;
            stats_.gpuOverlayVertexCount += overlayVertexCount;
        }
        stats_.lastPrimitiveCount = static_cast<U32>(primitives.size());
        stats_.lastVertexCount = static_cast<U32>(vertices.size());
        stats_.lastExtent = snapshot->extent;
        return true;
    }
}
