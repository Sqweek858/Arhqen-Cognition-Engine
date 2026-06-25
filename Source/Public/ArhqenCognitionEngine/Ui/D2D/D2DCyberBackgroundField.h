#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DRenderContext.h"

#include <array>
#include <cstdint>

namespace am::ui
{
    struct D2DCyberBlob
    {
        float baseX = 0.0f;
        float baseY = 0.0f;
        float radius = 160.0f;
        float phase = 0.0f;
        float speed = 1.0f;
        float alpha = 0.14f;
        float driftX = 20.0f;
        float driftY = 18.0f;
    };

    struct D2DCyberDot
    {
        float baseX = 0.0f;
        float baseY = 0.0f;
        float phase = 0.0f;
        float speed = 1.0f;
        float radius = 1.8f;
        float alpha = 0.22f;
    };

    struct D2DCyberArc
    {
        float cx = 0.0f;
        float cy = 0.0f;
        float radius = 120.0f;
        float phase = 0.0f;
        float alpha = 0.16f;
    };

    struct D2DCyberRibbon
    {
        float x = 0.0f;
        float y = 0.0f;
        float length = 260.0f;
        float angle = 0.0f;
        float phase = 0.0f;
        float speed = 0.10f;
        float alpha = 0.10f;
    };

    struct D2DCyberGlyphRing
    {
        float cx = 0.0f;
        float cy = 0.0f;
        float radius = 120.0f;
        float phase = 0.0f;
        float speed = 0.08f;
        float alpha = 0.18f;
    };

    struct D2DCyberFogLayer
    {
        float baseX = 0.0f;
        float baseY = 0.0f;
        float radiusX = 320.0f;
        float radiusY = 180.0f;
        float phase = 0.0f;
        float speed = 0.06f;
        float alpha = 0.10f;
    };

    struct D2DCyberNeuralVein
    {
        float x = 0.0f;
        float y = 0.0f;
        float length = 280.0f;
        float angle = 0.0f;
        float phase = 0.0f;
        float alpha = 0.12f;
        int branches = 3;
    };

    struct D2DCyberCore
    {
        float x = 0.72f;
        float y = 0.24f;
        float radius = 260.0f;
        float phase = 0.0f;
        float alpha = 0.24f;
    };

    struct D2DCyberEnergySurge
    {
        float baseX = 0.0f;
        float baseY = 0.0f;
        float length = 520.0f;
        float width = 90.0f;
        float angle = -0.25f;
        float phase = 0.0f;
        float speed = 0.08f;
        float alpha = 0.18f;
    };

    struct D2DCyberCoreShard
    {
        float angle = 0.0f;
        float radius = 160.0f;
        float length = 70.0f;
        float phase = 0.0f;
        float alpha = 0.24f;
    };


    class D2DCyberBackgroundField
    {
    public:
        void resize(float width, float height);
        void update(float dtSeconds);
        void setMouse(float x, float y);
        void render(D2DRenderContext& ctx, UiRect rect);

        float time() const;

    private:
        void seed(float width, float height);
        void renderLightSource(D2DRenderContext& ctx, UiRect rect);
        void renderBoostedZones(D2DRenderContext& ctx, UiRect rect);
        void renderHighContrastPlumes(D2DRenderContext& ctx, UiRect rect);
        void renderFastPulseBursts(D2DRenderContext& ctx, UiRect rect);
        void renderOuterVignette(D2DRenderContext& ctx, UiRect rect);
        void renderDarkContrastMasks(D2DRenderContext& ctx, UiRect rect);
        void renderHugeCoreField(D2DRenderContext& ctx, UiRect rect);
        void renderNeuralCore(D2DRenderContext& ctx, UiRect rect);
        void renderEnergySurges(D2DRenderContext& ctx, UiRect rect);
        void renderCoreShards(D2DRenderContext& ctx, UiRect rect);
        void renderChaoticFog(D2DRenderContext& ctx, UiRect rect);
        void renderNeuralVeins(D2DRenderContext& ctx, UiRect rect);
        void renderBlobs(D2DRenderContext& ctx, UiRect rect);
        void renderDots(D2DRenderContext& ctx, UiRect rect);
        void renderArcs(D2DRenderContext& ctx, UiRect rect);
        void renderRibbons(D2DRenderContext& ctx, UiRect rect);
        void renderGlyphRings(D2DRenderContext& ctx, UiRect rect);
        void renderCircuitWisps(D2DRenderContext& ctx, UiRect rect);
        void renderTextureDust(D2DRenderContext& ctx, UiRect rect);

        std::array<D2DCyberBlob, 5> blobs_ {};
        std::array<D2DCyberDot, 78> dots_ {};
        std::array<D2DCyberArc, 7> arcs_ {};
        std::array<D2DCyberRibbon, 9> ribbons_ {};
        std::array<D2DCyberGlyphRing, 6> glyphRings_ {};
        std::array<D2DCyberFogLayer, 11> fogLayers_ {};
        std::array<D2DCyberNeuralVein, 14> neuralVeins_ {};
        D2DCyberCore primaryCore_ {};
        D2DCyberCore secondaryCore_ {0.18f, 0.72f, 190.0f, 1.7f, 0.14f};
        std::array<D2DCyberEnergySurge, 10> energySurges_ {};
        std::array<D2DCyberCoreShard, 26> coreShards_ {};
        float width_ = 0.0f;
        float height_ = 0.0f;
        float time_ = 0.0f;
        float mouseX_ = 0.0f;
        float mouseY_ = 0.0f;
        bool seeded_ = false;
    };
}
