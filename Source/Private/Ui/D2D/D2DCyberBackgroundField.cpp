#include "ArhqenCognitionEngine/Ui/D2D/D2DCyberBackgroundField.h"

#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

#include <algorithm>
#include <cmath>

namespace am::ui
{
    namespace
    {
        constexpr float kPi = 3.14159265358979323846f;

        float fract(float value)
        {
            return value - std::floor(value);
        }

        float hash01(int n)
        {
            return fract(std::sin(static_cast<float>(n) * 12.9898f) * 43758.5453f);
        }

        float wrapPositive(float value, float limit)
        {
            limit = std::max(1.0f, limit);
            float wrapped = std::fmod(value, limit);
            if (wrapped < 0.0f)
            {
                wrapped += limit;
            }

            return wrapped;
        }

        void setOpacity(ID2D1Brush* brush, float opacity)
        {
            if (brush)
            {
                brush->SetOpacity(std::clamp(opacity, 0.0f, 1.0f));
            }
        }

        void restoreOpacity(ID2D1Brush* brush)
        {
            if (brush)
            {
                brush->SetOpacity(1.0f);
            }
        }

        D2D1_ELLIPSE ellipse(float x, float y, float radiusX, float radiusY)
        {
            return D2D1::Ellipse(D2D1::Point2F(x, y), radiusX, radiusY);
        }
    }

    void D2DCyberBackgroundField::resize(float width, float height)
    {
        if (!seeded_ || std::abs(width - width_) > 0.5f || std::abs(height - height_) > 0.5f)
        {
            seed(width, height);
        }
    }

    void D2DCyberBackgroundField::update(float dtSeconds)
    {
        time_ += std::clamp(dtSeconds, 0.0f, 0.10f) * 1.75f;
    }

    void D2DCyberBackgroundField::setMouse(float x, float y)
    {
        mouseX_ = x;
        mouseY_ = y;
    }

    void D2DCyberBackgroundField::render(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.target)
        {
            return;
        }

        resize(rect.width(), rect.height());

        renderDarkContrastMasks(ctx, rect);
        renderBoostedZones(ctx, rect);
        renderChaoticFog(ctx, rect);
        renderHugeCoreField(ctx, rect);
        renderHighContrastPlumes(ctx, rect);
        renderLightSource(ctx, rect);
        renderEnergySurges(ctx, rect);
        renderRibbons(ctx, rect);
        renderCoreShards(ctx, rect);
        renderNeuralVeins(ctx, rect);
        renderCircuitWisps(ctx, rect);
        renderFastPulseBursts(ctx, rect);
        renderDots(ctx, rect);
        renderTextureDust(ctx, rect);
        renderOuterVignette(ctx, rect);
    }


    void D2DCyberBackgroundField::renderOuterVignette(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.panelDeep)
        {
            return;
        }

        const float t = time_ * 0.18f;
        const float pulse = 0.5f + 0.5f * std::sin(t * 1.2f);

        // ACE-CLEAN0: balanced full-canvas vignette.
        // Still keeps the exterior darker, but leaves the screen a little lighter overall.
        struct EdgeBand
        {
            float thickness;
            float alpha;
        };

        const EdgeBand bands[] = {
            // ACE-UI1R9: bigger and softer vignette.
            // More spread, lighter opacity, more steps -> less "black bar" feeling.
            {72.0f, 0.34f},
            {138.0f, 0.21f},
            {214.0f, 0.12f},
            {302.0f, 0.070f},
            {392.0f, 0.035f},
        };

        for (const auto& band : bands)
        {
            const float alpha = band.alpha + pulse * 0.012f;
            setOpacity(ctx.brushes.panelDeep, alpha);

            ctx.target->FillRectangle(D2D1::RectF(rect.left, rect.top, rect.right, rect.top + band.thickness), ctx.brushes.panelDeep);
            ctx.target->FillRectangle(D2D1::RectF(rect.left, rect.bottom - band.thickness, rect.right, rect.bottom), ctx.brushes.panelDeep);
            ctx.target->FillRectangle(D2D1::RectF(rect.left, rect.top, rect.left + band.thickness, rect.bottom), ctx.brushes.panelDeep);
            ctx.target->FillRectangle(D2D1::RectF(rect.right - band.thickness, rect.top, rect.right, rect.bottom), ctx.brushes.panelDeep);
        }

        struct DarkOrb
        {
            float x;
            float y;
            float rx;
            float ry;
            float alpha;
        };

        const DarkOrb orbs[] = {
            {-0.12f,  0.06f, 0.78f, 0.78f, 0.30f},
            { 1.12f,  0.02f, 0.72f, 0.74f, 0.28f},
            { 0.50f, -0.24f, 0.88f, 0.56f, 0.25f},
            { 0.50f,  1.20f, 1.02f, 0.58f, 0.30f},
            {-0.18f,  0.96f, 0.84f, 0.56f, 0.24f},
            { 1.14f,  0.98f, 0.76f, 0.54f, 0.24f},
        };

        for (const auto& orb : orbs)
        {
            const float cx = rect.left + rect.width() * orb.x + std::sin(t + orb.x * 3.0f) * 18.0f;
            const float cy = rect.top + rect.height() * orb.y + std::cos(t + orb.y * 2.0f) * 14.0f;
            const float rx = rect.width() * orb.rx;
            const float ry = rect.height() * orb.ry;
            const float alpha = orb.alpha + pulse * 0.022f;

            for (int layer = 5; layer >= 1; --layer)
            {
                const float scale = 0.68f + static_cast<float>(layer) * 0.13f;
                setOpacity(ctx.brushes.panelDeep, alpha * (0.20f + static_cast<float>(6 - layer) * 0.095f));
                ctx.target->FillEllipse(ellipse(cx, cy, rx * scale, ry * scale), ctx.brushes.panelDeep);
            }
        }

        restoreOpacity(ctx.brushes.panelDeep);
    }

    float D2DCyberBackgroundField::time() const
    {
        return time_;
    }

    void D2DCyberBackgroundField::seed(float width, float height)
    {
        width_ = std::max(1.0f, width);
        height_ = std::max(1.0f, height);
        seeded_ = true;

        for (std::size_t i = 0; i < blobs_.size(); ++i)
        {
            const float a = hash01(static_cast<int>(i) * 31 + 1);
            const float b = hash01(static_cast<int>(i) * 31 + 2);

            blobs_[i].baseX = a * width_;
            blobs_[i].baseY = b * height_;
            blobs_[i].radius = 130.0f + hash01(static_cast<int>(i) * 31 + 3) * 250.0f;
            blobs_[i].phase = hash01(static_cast<int>(i) * 31 + 4) * kPi * 2.0f;
            blobs_[i].speed = 0.11f + hash01(static_cast<int>(i) * 31 + 5) * 0.13f;
            blobs_[i].alpha = 0.075f + hash01(static_cast<int>(i) * 31 + 6) * 0.105f;
            blobs_[i].driftX = 18.0f + hash01(static_cast<int>(i) * 31 + 7) * 46.0f;
            blobs_[i].driftY = 14.0f + hash01(static_cast<int>(i) * 31 + 8) * 38.0f;
        }

        for (std::size_t i = 0; i < dots_.size(); ++i)
        {
            dots_[i].baseX = hash01(static_cast<int>(i) * 47 + 1) * width_;
            dots_[i].baseY = hash01(static_cast<int>(i) * 47 + 2) * height_;
            dots_[i].phase = hash01(static_cast<int>(i) * 47 + 3) * kPi * 2.0f;
            dots_[i].speed = 0.15f + hash01(static_cast<int>(i) * 47 + 4) * 0.30f;
            dots_[i].radius = 1.0f + hash01(static_cast<int>(i) * 47 + 5) * 2.3f;
            dots_[i].alpha = 0.220f + hash01(static_cast<int>(i) * 47 + 6) * 0.420f;
        }

        for (std::size_t i = 0; i < arcs_.size(); ++i)
        {
            arcs_[i].cx = hash01(static_cast<int>(i) * 59 + 1) * width_;
            arcs_[i].cy = hash01(static_cast<int>(i) * 59 + 2) * height_;
            arcs_[i].radius = 80.0f + hash01(static_cast<int>(i) * 59 + 3) * 210.0f;
            arcs_[i].phase = hash01(static_cast<int>(i) * 59 + 4) * kPi * 2.0f;
            arcs_[i].alpha = 0.070f + hash01(static_cast<int>(i) * 59 + 5) * 0.120f;
        }

        for (std::size_t i = 0; i < ribbons_.size(); ++i)
        {
            ribbons_[i].x = hash01(static_cast<int>(i) * 71 + 1) * width_;
            ribbons_[i].y = hash01(static_cast<int>(i) * 71 + 2) * height_;
            ribbons_[i].length = 180.0f + hash01(static_cast<int>(i) * 71 + 3) * 420.0f;
            ribbons_[i].angle = -0.75f + hash01(static_cast<int>(i) * 71 + 4) * 1.20f;
            ribbons_[i].phase = hash01(static_cast<int>(i) * 71 + 5) * kPi * 2.0f;
            ribbons_[i].speed = 0.045f + hash01(static_cast<int>(i) * 71 + 6) * 0.080f;
            ribbons_[i].alpha = 0.030f + hash01(static_cast<int>(i) * 71 + 7) * 0.075f;
        }

        for (std::size_t i = 0; i < glyphRings_.size(); ++i)
        {
            glyphRings_[i].cx = hash01(static_cast<int>(i) * 83 + 1) * width_;
            glyphRings_[i].cy = hash01(static_cast<int>(i) * 83 + 2) * height_;
            glyphRings_[i].radius = 70.0f + hash01(static_cast<int>(i) * 83 + 3) * 230.0f;
            glyphRings_[i].phase = hash01(static_cast<int>(i) * 83 + 4) * kPi * 2.0f;
            glyphRings_[i].speed = 0.035f + hash01(static_cast<int>(i) * 83 + 5) * 0.070f;
            glyphRings_[i].alpha = 0.055f + hash01(static_cast<int>(i) * 83 + 6) * 0.125f;
        }

        primaryCore_.x = 0.74f;
        primaryCore_.y = 0.23f;
        primaryCore_.radius = std::max(220.0f, std::min(width_, height_) * 0.36f);
        primaryCore_.phase = 0.35f;
        primaryCore_.alpha = 0.28f;

        secondaryCore_.x = 0.16f;
        secondaryCore_.y = 0.72f;
        secondaryCore_.radius = std::max(160.0f, std::min(width_, height_) * 0.26f);
        secondaryCore_.phase = 2.10f;
        secondaryCore_.alpha = 0.16f;

        // M27R5: push the main background from "subtle wallpaper" into an obvious asymmetric core.
        primaryCore_.x = 0.70f;
        primaryCore_.y = 0.27f;
        primaryCore_.radius = std::max(390.0f, std::min(width_, height_) * 0.58f);
        primaryCore_.alpha = 0.52f;

        secondaryCore_.x = 0.14f;
        secondaryCore_.y = 0.66f;
        secondaryCore_.radius = std::max(260.0f, std::min(width_, height_) * 0.38f);
        secondaryCore_.alpha = 0.24f;

        for (std::size_t i = 0; i < energySurges_.size(); ++i)
        {
            const float coreBias = static_cast<float>(i % 4) * 0.055f;
            energySurges_[i].baseX = (0.48f + coreBias + hash01(static_cast<int>(i) * 131 + 1) * 0.42f) * width_;
            energySurges_[i].baseY = (0.05f + hash01(static_cast<int>(i) * 131 + 2) * 0.82f) * height_;
            energySurges_[i].length = 420.0f + hash01(static_cast<int>(i) * 131 + 3) * 760.0f;
            energySurges_[i].width = 34.0f + hash01(static_cast<int>(i) * 131 + 4) * 92.0f;
            energySurges_[i].angle = -0.72f + hash01(static_cast<int>(i) * 131 + 5) * 1.10f;
            energySurges_[i].phase = hash01(static_cast<int>(i) * 131 + 6) * kPi * 2.0f;
            energySurges_[i].speed = 0.110f + hash01(static_cast<int>(i) * 131 + 7) * 0.180f;
            energySurges_[i].alpha = 0.140f + hash01(static_cast<int>(i) * 131 + 8) * 0.260f;
        }

        for (std::size_t i = 0; i < coreShards_.size(); ++i)
        {
            coreShards_[i].angle = static_cast<float>(i) / static_cast<float>(coreShards_.size()) * kPi * 2.0f + hash01(static_cast<int>(i) * 149 + 1) * 0.45f;
            coreShards_[i].radius = primaryCore_.radius * (0.32f + hash01(static_cast<int>(i) * 149 + 2) * 0.56f);
            coreShards_[i].length = 24.0f + hash01(static_cast<int>(i) * 149 + 3) * 98.0f;
            coreShards_[i].phase = hash01(static_cast<int>(i) * 149 + 4) * kPi * 2.0f;
            coreShards_[i].alpha = 0.12f + hash01(static_cast<int>(i) * 149 + 5) * 0.24f;
        }

        for (std::size_t i = 0; i < fogLayers_.size(); ++i)
        {
            fogLayers_[i].baseX = hash01(static_cast<int>(i) * 97 + 1) * width_;
            fogLayers_[i].baseY = hash01(static_cast<int>(i) * 97 + 2) * height_;
            fogLayers_[i].radiusX = 240.0f + hash01(static_cast<int>(i) * 97 + 3) * 560.0f;
            fogLayers_[i].radiusY = 90.0f + hash01(static_cast<int>(i) * 97 + 4) * 300.0f;
            fogLayers_[i].phase = hash01(static_cast<int>(i) * 97 + 5) * kPi * 2.0f;
            fogLayers_[i].speed = 0.020f + hash01(static_cast<int>(i) * 97 + 6) * 0.060f;
            fogLayers_[i].alpha = 0.075f + hash01(static_cast<int>(i) * 97 + 7) * 0.135f;
        }

        for (std::size_t i = 0; i < neuralVeins_.size(); ++i)
        {
            const bool coreBiased = i < neuralVeins_.size() / 2;
            neuralVeins_[i].x = (coreBiased ? (0.56f + hash01(static_cast<int>(i) * 109 + 1) * 0.38f) : hash01(static_cast<int>(i) * 109 + 1)) * width_;
            neuralVeins_[i].y = (coreBiased ? (0.08f + hash01(static_cast<int>(i) * 109 + 2) * 0.44f) : hash01(static_cast<int>(i) * 109 + 2)) * height_;
            neuralVeins_[i].length = 120.0f + hash01(static_cast<int>(i) * 109 + 3) * 360.0f;
            neuralVeins_[i].angle = -0.90f + hash01(static_cast<int>(i) * 109 + 4) * 1.80f;
            neuralVeins_[i].phase = hash01(static_cast<int>(i) * 109 + 5) * kPi * 2.0f;
            neuralVeins_[i].alpha = 0.130f + hash01(static_cast<int>(i) * 109 + 6) * 0.260f;
            neuralVeins_[i].branches = 2 + static_cast<int>(hash01(static_cast<int>(i) * 109 + 7) * 4.0f);
        }
    }




    void D2DCyberBackgroundField::renderDarkContrastMasks(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.panelDeep)
        {
            return;
        }

        const float t = time_ * 0.22f;
        const float pulse = 0.5f + 0.5f * std::sin(t * 1.9f);

        // Negative-space zones. Without this, everything melts into one blue soup.
        setOpacity(ctx.brushes.panelDeep, 0.34f + pulse * 0.10f);
        ctx.target->FillEllipse(ellipse(rect.left + rect.width() * 0.20f, rect.top + rect.height() * 0.22f, rect.width() * 0.36f, rect.height() * 0.28f), ctx.brushes.panelDeep);

        setOpacity(ctx.brushes.panelDeep, 0.28f);
        ctx.target->FillEllipse(ellipse(rect.left + rect.width() * 0.50f, rect.top + rect.height() * 0.78f, rect.width() * 0.50f, rect.height() * 0.18f), ctx.brushes.panelDeep);

        restoreOpacity(ctx.brushes.panelDeep);
    }

    void D2DCyberBackgroundField::renderBoostedZones(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.accent || !ctx.brushes.accentBlue || !ctx.brushes.panelSoft)
        {
            return;
        }

        const float t = time_ * 0.34f;
        const float fastPulse = 0.50f + 0.50f * std::sin(time_ * 3.8f);
        const float shortPulse = 0.50f + 0.50f * std::sin(time_ * 6.2f);

        // Zone A: left-lower filled organic mass. Not a ring, not a dead outline, a real region.
        setOpacity(ctx.brushes.accent, 0.18f + fastPulse * 0.16f);
        ctx.target->FillEllipse(ellipse(rect.left + rect.width() * 0.13f + std::sin(t) * 26.0f, rect.top + rect.height() * 0.70f + std::cos(t * 0.8f) * 18.0f, rect.width() * 0.34f, rect.height() * 0.30f), ctx.brushes.accent);

        setOpacity(ctx.brushes.panelSoft, 0.20f + shortPulse * 0.10f);
        ctx.target->FillEllipse(ellipse(rect.left + rect.width() * 0.31f, rect.top + rect.height() * 0.58f, rect.width() * 0.25f, rect.height() * 0.22f), ctx.brushes.panelSoft);

        // Zone B: right hot reactor cloud. This should be visible in screenshots.
        setOpacity(ctx.brushes.accentBlue, 0.34f + fastPulse * 0.20f);
        ctx.target->FillEllipse(ellipse(rect.left + rect.width() * 0.79f + std::sin(t * 1.1f) * 32.0f, rect.top + rect.height() * 0.24f + std::cos(t * 0.9f) * 18.0f, rect.width() * 0.32f, rect.height() * 0.28f), ctx.brushes.accentBlue);

        setOpacity(ctx.brushes.accent, 0.24f + shortPulse * 0.18f);
        ctx.target->FillEllipse(ellipse(rect.left + rect.width() * 0.70f, rect.top + rect.height() * 0.33f, rect.width() * 0.26f, rect.height() * 0.18f), ctx.brushes.accent);

        // Zone C: central cold blue body, intentionally solid and contrasty.
        setOpacity(ctx.brushes.accentBlue, 0.22f + fastPulse * 0.12f);
        ctx.target->FillEllipse(ellipse(rect.left + rect.width() * 0.52f, rect.top + rect.height() * 0.48f, rect.width() * 0.33f, rect.height() * 0.26f), ctx.brushes.accentBlue);

        restoreOpacity(ctx.brushes.accent);
        restoreOpacity(ctx.brushes.accentBlue);
        restoreOpacity(ctx.brushes.panelSoft);
    }

    void D2DCyberBackgroundField::renderHighContrastPlumes(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.accent || !ctx.brushes.accentBlue)
        {
            return;
        }

        const float t = time_ * 0.62f;
        const float pulse = 0.50f + 0.50f * std::sin(time_ * 4.8f);

        // Fast, shorter, more visible diagonal energy. Screenshot should not look identical now.
        for (int i = 0; i < 9; ++i)
        {
            const float lane = static_cast<float>(i) / 8.0f;
            const float x = rect.left + rect.width() * (0.18f + lane * 0.74f) + std::sin(t + i) * 34.0f;
            const float y = rect.top + rect.height() * (0.76f - lane * 0.62f) + std::cos(t * 1.4f + i) * 28.0f;
            const float length = 160.0f + static_cast<float>(i % 4) * 70.0f;
            const float angle = -0.38f + std::sin(t * 0.7f + i) * 0.10f;
            const float dx = std::cos(angle);
            const float dy = std::sin(angle);
            ID2D1SolidColorBrush* brush = (i % 2 == 0) ? ctx.brushes.accent : ctx.brushes.accentBlue;

            setOpacity(brush, 0.18f + pulse * 0.22f);
            ctx.target->DrawLine(
                D2D1::Point2F(x - dx * length * 0.5f, y - dy * length * 0.5f),
                D2D1::Point2F(x + dx * length * 0.5f, y + dy * length * 0.5f),
                brush,
                2.0f + static_cast<float>(i % 3) * 1.6f
            );

            setOpacity(brush, 0.08f + pulse * 0.10f);
            ctx.target->DrawLine(
                D2D1::Point2F(x - dx * length * 0.7f, y - dy * length * 0.7f),
                D2D1::Point2F(x + dx * length * 0.7f, y + dy * length * 0.7f),
                brush,
                8.0f + static_cast<float>(i % 2) * 5.0f
            );
        }

        restoreOpacity(ctx.brushes.accent);
        restoreOpacity(ctx.brushes.accentBlue);
    }

    void D2DCyberBackgroundField::renderFastPulseBursts(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.accent || !ctx.brushes.accentBlue)
        {
            return;
        }

        const float t = time_ * 0.46f;

        for (int i = 0; i < 18; ++i)
        {
            const float seedX = hash01(8000 + i * 13);
            const float seedY = hash01(8100 + i * 17);
            const bool rightSide = i % 3 != 0;
            const float baseX = rightSide ? (0.58f + seedX * 0.40f) : (0.05f + seedX * 0.34f);
            const float baseY = rightSide ? (0.08f + seedY * 0.62f) : (0.36f + seedY * 0.48f);
            const float phase = hash01(8200 + i * 19) * kPi * 2.0f;
            const float pulse = 0.5f + 0.5f * std::sin(t * (0.70f + hash01(8300 + i) * 1.10f) + phase);
            const float burst = pulse * pulse * pulse;
            const float x = rect.left + baseX * rect.width() + std::sin(t + i) * 14.0f;
            const float y = rect.top + baseY * rect.height() + std::cos(t * 0.7f + i) * 12.0f;
            const float r = 3.0f + burst * (9.0f + hash01(8400 + i) * 18.0f);
            ID2D1SolidColorBrush* brush = (i % 2 == 0) ? ctx.brushes.accent : ctx.brushes.accentBlue;

            setOpacity(brush, 0.16f + burst * 0.42f);
            ctx.target->FillEllipse(ellipse(x, y, r, r * (0.72f + hash01(8500 + i) * 0.45f)), brush);
        }

        restoreOpacity(ctx.brushes.accent);
        restoreOpacity(ctx.brushes.accentBlue);
    }

    void D2DCyberBackgroundField::renderHugeCoreField(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.accentBlue || !ctx.brushes.accent || !ctx.brushes.panelSoft)
        {
            return;
        }

        const float t = time_ * 0.28f + primaryCore_.phase;
        const float cx = rect.left + primaryCore_.x * rect.width() + std::sin(t) * 56.0f;
        const float cy = rect.top + primaryCore_.y * rect.height() + std::cos(t * 0.73f) * 44.0f;
        const float pulse = 0.50f + 0.50f * std::sin(t * 2.20f);
        const float r = primaryCore_.radius * (0.92f + pulse * 0.12f);

        // Giant cyan-blue atmosphere. This is intentionally much more visible than M27.
        for (int layer = 12; layer >= 1; --layer)
        {
            const float scale = 0.30f + static_cast<float>(layer) * 0.105f;
            const float wobble = 0.78f + 0.10f * std::sin(t + layer * 0.7f);
            ID2D1SolidColorBrush* brush = (layer % 3 == 0) ? ctx.brushes.accent : ((layer % 3 == 1) ? ctx.brushes.accentBlue : ctx.brushes.panelSoft);
            setOpacity(brush, primaryCore_.alpha * (0.040f + static_cast<float>(13 - layer) * 0.018f));
            ctx.target->FillEllipse(ellipse(cx, cy, r * scale * 1.35f, r * scale * wobble), brush);
        }

        // Non-perfect central mass.
        for (int ring = 0; ring < 6; ++ring)
        {
            const float a = t * (0.18f + ring * 0.032f);
            const float ox = std::sin(a * 1.7f) * (18.0f + ring * 9.0f);
            const float oy = std::cos(a * 1.3f) * (14.0f + ring * 7.0f);
            const float rr = r * (0.18f + ring * 0.080f);
            ID2D1SolidColorBrush* brush = (ring % 2 == 0) ? ctx.brushes.accent : ctx.brushes.accentBlue;
            setOpacity(brush, 0.08f + pulse * 0.04f);
            ctx.target->FillEllipse(ellipse(cx + ox, cy + oy, rr * 1.22f, rr * 0.70f), brush);
        }

        restoreOpacity(ctx.brushes.accent);
        restoreOpacity(ctx.brushes.accentBlue);
        restoreOpacity(ctx.brushes.panelSoft);
    }

    void D2DCyberBackgroundField::renderEnergySurges(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.accent || !ctx.brushes.accentBlue)
        {
            return;
        }

        for (std::size_t i = 0; i < energySurges_.size(); ++i)
        {
            const auto& surge = energySurges_[i];
            const float t = time_ * surge.speed + surge.phase;
            const float x = rect.left + wrapPositive(surge.baseX + std::sin(t) * 90.0f, width_);
            const float y = rect.top + wrapPositive(surge.baseY + std::cos(t * 0.77f) * 80.0f, height_);
            const float angle = surge.angle + std::sin(t * 0.9f) * 0.18f;
            const float dx = std::cos(angle);
            const float dy = std::sin(angle);
            const float px = -dy;
            const float py = dx;
            const float half = surge.length * 0.5f;
            const float pulse = 0.50f + 0.50f * std::sin(t * 2.4f);
            ID2D1SolidColorBrush* brush = (i % 2 == 0) ? ctx.brushes.accentBlue : ctx.brushes.accent;

            for (int layer = 6; layer >= 1; --layer)
            {
                const float offset = (static_cast<float>(layer) - 3.5f) * surge.width * 0.10f + std::sin(t * 1.6f + layer) * 16.0f;
                const float width = 0.8f + static_cast<float>(layer) * 0.65f;
                const float x0 = x - dx * half + px * offset;
                const float y0 = y - dy * half + py * offset;
                const float x1 = x + dx * half - px * offset * 0.35f;
                const float y1 = y + dy * half - py * offset * 0.35f;

                setOpacity(brush, surge.alpha * (0.08f + static_cast<float>(7 - layer) * 0.055f) * (0.75f + pulse * 0.35f));
                ctx.target->DrawLine(D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1), brush, width);
            }
        }

        restoreOpacity(ctx.brushes.accent);
        restoreOpacity(ctx.brushes.accentBlue);
    }

    void D2DCyberBackgroundField::renderCoreShards(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.accent || !ctx.brushes.accentBlue)
        {
            return;
        }

        const float t = time_ * 0.18f + primaryCore_.phase;
        const float cx = rect.left + primaryCore_.x * rect.width() + std::sin(t) * 56.0f;
        const float cy = rect.top + primaryCore_.y * rect.height() + std::cos(t * 0.73f) * 44.0f;

        for (std::size_t i = 0; i < coreShards_.size(); ++i)
        {
            const auto& shard = coreShards_[i];
            const float phase = t + shard.phase;
            const float a = shard.angle + phase * (0.10f + static_cast<float>(i % 5) * 0.014f);
            const float r = shard.radius * (0.92f + 0.08f * std::sin(phase * 1.8f));
            const float x0 = cx + std::cos(a) * r;
            const float y0 = cy + std::sin(a) * r * 0.70f;
            const float x1 = cx + std::cos(a) * (r + shard.length);
            const float y1 = cy + std::sin(a) * (r + shard.length) * 0.70f;
            const float pulse = 0.55f + 0.45f * std::sin(phase * 2.9f);
            ID2D1SolidColorBrush* brush = (i % 2 == 0) ? ctx.brushes.accent : ctx.brushes.accentBlue;

            setOpacity(brush, shard.alpha * (0.55f + pulse * 0.45f));
            ctx.target->DrawLine(D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1), brush, 0.8f + pulse * 1.1f);

            if (i % 3 == 0)
            {
                ctx.target->FillEllipse(ellipse(x1, y1, 2.0f + pulse * 1.6f, 2.0f + pulse * 1.6f), brush);
            }
        }

        restoreOpacity(ctx.brushes.accent);
        restoreOpacity(ctx.brushes.accentBlue);
    }

    void D2DCyberBackgroundField::renderChaoticFog(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.accentBlue || !ctx.brushes.panelSoft || !ctx.brushes.accent)
        {
            return;
        }

        for (std::size_t i = 0; i < fogLayers_.size(); ++i)
        {
            const auto& fog = fogLayers_[i];
            const float t = time_ * fog.speed + fog.phase;
            const float x = rect.left + wrapPositive(fog.baseX + std::sin(t * 0.91f) * 95.0f + std::sin(t * 0.27f) * 45.0f, width_);
            const float y = rect.top + wrapPositive(fog.baseY + std::cos(t * 0.83f) * 70.0f + std::cos(t * 0.31f) * 34.0f, height_);
            const float pulse = 0.55f + 0.45f * std::sin(t * 2.0f);
            ID2D1SolidColorBrush* brush = (i % 3 == 0) ? ctx.brushes.accent : ((i % 3 == 1) ? ctx.brushes.accentBlue : ctx.brushes.panelSoft);

            for (int layer = 5; layer >= 1; --layer)
            {
                const float scale = 0.55f + static_cast<float>(layer) * 0.18f;
                const float wobble = 0.86f + 0.14f * std::sin(t + static_cast<float>(layer));
                setOpacity(brush, fog.alpha * (0.055f + static_cast<float>(6 - layer) * 0.026f) * (0.75f + pulse * 0.35f));
                ctx.target->FillEllipse(ellipse(x, y, fog.radiusX * scale, fog.radiusY * scale * wobble), brush);
            }

            restoreOpacity(brush);
        }
    }

    void D2DCyberBackgroundField::renderNeuralCore(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.accent || !ctx.brushes.accentBlue)
        {
            return;
        }

        const D2DCyberCore cores[] = {primaryCore_, secondaryCore_};

        for (std::size_t i = 0; i < 2; ++i)
        {
            const auto& core = cores[i];
            const float t = time_ * (i == 0 ? 0.18f : 0.11f) + core.phase;
            const float cx = rect.left + core.x * rect.width() + std::sin(t) * (i == 0 ? 36.0f : 22.0f);
            const float cy = rect.top + core.y * rect.height() + std::cos(t * 0.82f) * (i == 0 ? 30.0f : 18.0f);
            const float pulse = 0.50f + 0.50f * std::sin(t * 2.3f);
            const float r = core.radius * (0.94f + pulse * 0.10f);

            ID2D1SolidColorBrush* mainBrush = i == 0 ? ctx.brushes.accentBlue : ctx.brushes.accent;

            for (int layer = 8; layer >= 1; --layer)
            {
                const float scale = 0.30f + static_cast<float>(layer) * 0.16f;
                setOpacity(mainBrush, core.alpha * (0.020f + static_cast<float>(9 - layer) * 0.018f));
                ctx.target->FillEllipse(ellipse(cx, cy, r * scale * 1.18f, r * scale * 0.82f), mainBrush);
            }

            setOpacity(mainBrush, core.alpha * (0.40f + pulse * 0.28f));
            ctx.target->DrawEllipse(ellipse(cx, cy, r * 0.52f, r * 0.36f), mainBrush, 1.4f);
            ctx.target->DrawEllipse(ellipse(cx + std::sin(t * 1.7f) * 16.0f, cy + std::cos(t * 1.3f) * 12.0f, r * 0.22f, r * 0.16f), mainBrush, 1.0f);

            const int spokes = i == 0 ? 22 : 14;
            for (int s = 0; s < spokes; ++s)
            {
                if ((s + static_cast<int>(i)) % 4 == 0)
                {
                    continue;
                }

                const float a = t * 0.28f + static_cast<float>(s) / static_cast<float>(spokes) * kPi * 2.0f;
                const float inner = r * (0.34f + 0.08f * std::sin(t + s));
                const float outer = r * (0.52f + 0.12f * std::cos(t * 1.4f + s));
                const float x0 = cx + std::cos(a) * inner;
                const float y0 = cy + std::sin(a) * inner * 0.72f;
                const float x1 = cx + std::cos(a) * outer;
                const float y1 = cy + std::sin(a) * outer * 0.72f;

                setOpacity(mainBrush, core.alpha * 0.32f);
                ctx.target->DrawLine(D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1), mainBrush, 0.9f);
            }

            restoreOpacity(mainBrush);
        }
    }

    void D2DCyberBackgroundField::renderNeuralVeins(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.accent || !ctx.brushes.accentBlue)
        {
            return;
        }

        for (std::size_t i = 0; i < neuralVeins_.size(); ++i)
        {
            const auto& vein = neuralVeins_[i];
            const float t = time_ * 0.10f + vein.phase;
            const float x = rect.left + wrapPositive(vein.x + std::sin(t) * 38.0f, width_);
            const float y = rect.top + wrapPositive(vein.y + std::cos(t * 0.77f) * 32.0f, height_);
            const float angle = vein.angle + std::sin(t * 0.5f) * 0.20f;
            const float dx = std::cos(angle);
            const float dy = std::sin(angle);
            ID2D1SolidColorBrush* brush = (i % 2 == 0) ? ctx.brushes.accent : ctx.brushes.accentBlue;
            const float pulse = 0.45f + 0.55f * std::sin(t * 2.4f);

            setOpacity(brush, vein.alpha * (0.50f + pulse * 0.50f));
            float lastX = x;
            float lastY = y;

            const int segments = 5;
            for (int s = 1; s <= segments; ++s)
            {
                const float f = static_cast<float>(s) / static_cast<float>(segments);
                const float bend = std::sin(t * 1.9f + static_cast<float>(s) * 1.7f) * 22.0f;
                const float nx = x + dx * vein.length * f - dy * bend;
                const float ny = y + dy * vein.length * f + dx * bend * 0.64f;
                ctx.target->DrawLine(D2D1::Point2F(lastX, lastY), D2D1::Point2F(nx, ny), brush, 0.8f + pulse * 0.6f);
                lastX = nx;
                lastY = ny;

                if (s <= vein.branches)
                {
                    const float branchAngle = angle + ((s % 2 == 0) ? 0.82f : -0.72f);
                    const float bx = nx + std::cos(branchAngle) * vein.length * 0.16f;
                    const float by = ny + std::sin(branchAngle) * vein.length * 0.11f;
                    setOpacity(brush, vein.alpha * 0.34f);
                    ctx.target->DrawLine(D2D1::Point2F(nx, ny), D2D1::Point2F(bx, by), brush, 0.65f);
                    setOpacity(brush, vein.alpha * (0.50f + pulse * 0.50f));
                }
            }

            restoreOpacity(brush);
        }
    }

    void D2DCyberBackgroundField::renderLightSource(D2DRenderContext& ctx, UiRect rect)
    {
        const float lightX = rect.left + rect.width() * 0.70f + std::sin(time_ * 0.09f) * 32.0f;
        const float lightY = rect.top + rect.height() * 0.24f + std::cos(time_ * 0.07f) * 24.0f;

        if (ctx.brushes.accentBlue)
        {
            for (int i = 6; i >= 1; --i)
            {
                const float radius = 70.0f + static_cast<float>(i) * 52.0f;
                const float alpha = 0.018f + 0.016f * static_cast<float>(7 - i);
                setOpacity(ctx.brushes.accentBlue, alpha);
                ctx.target->FillEllipse(ellipse(lightX, lightY, radius * 1.22f, radius), ctx.brushes.accentBlue);
            }
            restoreOpacity(ctx.brushes.accentBlue);
        }
    }

    void D2DCyberBackgroundField::renderBlobs(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.accent || !ctx.brushes.accentBlue)
        {
            return;
        }

        const float parallaxX = (mouseX_ / std::max(1.0f, width_) - 0.5f) * 12.0f;
        const float parallaxY = (mouseY_ / std::max(1.0f, height_) - 0.5f) * 10.0f;

        for (std::size_t i = 0; i < blobs_.size(); ++i)
        {
            const auto& blob = blobs_[i];
            const float t = time_ * blob.speed + blob.phase;
            const float pulse = 0.5f + 0.5f * std::sin(t * 1.7f);
            const float x = rect.left + wrapPositive(blob.baseX + std::sin(t) * blob.driftX + parallaxX * (1.0f + static_cast<float>(i) * 0.18f), width_);
            const float y = rect.top + wrapPositive(blob.baseY + std::cos(t * 0.9f) * blob.driftY + parallaxY * (1.0f + static_cast<float>(i) * 0.12f), height_);
            const float r = blob.radius * (0.96f + pulse * 0.08f);
            ID2D1SolidColorBrush* brush = (i % 2 == 0) ? ctx.brushes.accent : ctx.brushes.accentBlue;

            for (int layer = 4; layer >= 1; --layer)
            {
                const float scale = 0.58f + static_cast<float>(layer) * 0.20f;
                setOpacity(brush, blob.alpha * (0.15f + 0.12f * static_cast<float>(5 - layer)));
                ctx.target->FillEllipse(ellipse(x, y, r * scale, r * scale * (0.72f + 0.10f * std::sin(t + layer))), brush);
            }

            setOpacity(brush, blob.alpha * 0.75f);
            ctx.target->DrawEllipse(ellipse(x, y, r * 0.70f, r * 0.52f), brush, 1.2f);
            ctx.target->DrawEllipse(ellipse(x + std::sin(t) * 8.0f, y + std::cos(t) * 8.0f, r * 0.38f, r * 0.32f), brush, 0.8f);
            restoreOpacity(brush);
        }
    }

    void D2DCyberBackgroundField::renderDots(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.accent || !ctx.brushes.border)
        {
            return;
        }

        setOpacity(ctx.brushes.border, 0.060f);
        for (std::size_t i = 1; i < dots_.size(); i += 3)
        {
            const auto& a = dots_[i - 1];
            const auto& b = dots_[i];
            const float ax = rect.left + wrapPositive(a.baseX + std::sin(time_ * a.speed + a.phase) * 18.0f, width_);
            const float ay = rect.top + wrapPositive(a.baseY + std::cos(time_ * a.speed + a.phase) * 16.0f, height_);
            const float bx = rect.left + wrapPositive(b.baseX + std::sin(time_ * b.speed + b.phase) * 18.0f, width_);
            const float by = rect.top + wrapPositive(b.baseY + std::cos(time_ * b.speed + b.phase) * 16.0f, height_);

            const float dx = bx - ax;
            const float dy = by - ay;
            if (dx * dx + dy * dy < 36000.0f)
            {
                ctx.target->DrawLine(D2D1::Point2F(ax, ay), D2D1::Point2F(bx, by), ctx.brushes.border, 0.6f);
            }
        }
        restoreOpacity(ctx.brushes.border);

        for (std::size_t i = 0; i < dots_.size(); ++i)
        {
            const auto& dot = dots_[i];
            const float t = time_ * dot.speed + dot.phase;
            const bool coreCluster = (i % 4) != 0;
            const float clusterX = coreCluster ? (primaryCore_.x * width_ + std::sin(t * 0.41f) * primaryCore_.radius * 0.62f) : dot.baseX;
            const float clusterY = coreCluster ? (primaryCore_.y * height_ + std::cos(t * 0.37f) * primaryCore_.radius * 0.42f) : dot.baseY;
            const float x = rect.left + wrapPositive((dot.baseX * 0.42f + clusterX * 0.58f) + std::sin(t) * 26.0f + std::sin(t * 0.37f) * 12.0f, width_);
            const float y = rect.top + wrapPositive((dot.baseY * 0.42f + clusterY * 0.58f) + std::cos(t * 0.92f) * 22.0f + std::cos(t * 0.43f) * 9.0f, height_);
            const float pulse = 0.60f + 0.40f * std::sin(t * 1.9f);
            const float r = dot.radius * (0.82f + pulse * 0.28f);

            setOpacity(ctx.brushes.accent, dot.alpha * (0.72f + pulse * 0.35f));
            ctx.target->FillEllipse(ellipse(x, y, r, r), ctx.brushes.accent);
        }

        restoreOpacity(ctx.brushes.accent);
    }


    void D2DCyberBackgroundField::renderRibbons(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.accentBlue || !ctx.brushes.accent)
        {
            return;
        }

        for (std::size_t i = 0; i < ribbons_.size(); ++i)
        {
            const auto& ribbon = ribbons_[i];
            const float t = time_ * ribbon.speed + ribbon.phase;
            const float cx = rect.left + wrapPositive(ribbon.x + std::sin(t) * 90.0f, width_);
            const float cy = rect.top + wrapPositive(ribbon.y + std::cos(t * 0.73f) * 60.0f, height_);
            const float angle = ribbon.angle + std::sin(t * 0.41f) * 0.18f;
            const float dx = std::cos(angle);
            const float dy = std::sin(angle);
            const float px = -dy;
            const float py = dx;
            const float half = ribbon.length * 0.5f;
            ID2D1SolidColorBrush* brush = (i % 2 == 0) ? ctx.brushes.accentBlue : ctx.brushes.accent;

            for (int layer = 0; layer < 4; ++layer)
            {
                const float side = (static_cast<float>(layer) - 1.5f) * 10.0f;
                const float wave = std::sin(t * 2.0f + layer) * 9.0f;
                const float x0 = cx - dx * half + px * (side + wave);
                const float y0 = cy - dy * half + py * (side + wave);
                const float x1 = cx + dx * half + px * (side - wave);
                const float y1 = cy + dy * half + py * (side - wave);

                setOpacity(brush, ribbon.alpha * (0.22f + static_cast<float>(layer) * 0.12f));
                ctx.target->DrawLine(D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1), brush, 1.0f + static_cast<float>(layer) * 0.5f);
            }

            restoreOpacity(brush);
        }
    }

    void D2DCyberBackgroundField::renderGlyphRings(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.accent || !ctx.brushes.accentBlue)
        {
            return;
        }

        for (std::size_t i = 0; i < glyphRings_.size(); ++i)
        {
            const auto& ring = glyphRings_[i];
            const float t = time_ * ring.speed + ring.phase;
            const float x = rect.left + wrapPositive(ring.cx + std::sin(t) * 36.0f, width_);
            const float y = rect.top + wrapPositive(ring.cy + std::cos(t * 0.8f) * 28.0f, height_);
            const float radius = ring.radius * (0.96f + 0.04f * std::sin(t * 2.1f));
            ID2D1SolidColorBrush* brush = (i % 2 == 0) ? ctx.brushes.accent : ctx.brushes.accentBlue;

            setOpacity(brush, ring.alpha * 0.38f);
            ctx.target->DrawEllipse(ellipse(x, y, radius, radius * 0.70f), brush, 0.8f);

            const int ticks = 18;
            for (int tick = 0; tick < ticks; ++tick)
            {
                if ((tick + static_cast<int>(i)) % 3 == 1)
                {
                    continue;
                }

                const float a = t * 0.32f + (static_cast<float>(tick) / static_cast<float>(ticks)) * kPi * 2.0f;
                const float rx = radius;
                const float ry = radius * 0.70f;
                const float x0 = x + std::cos(a) * rx;
                const float y0 = y + std::sin(a) * ry;
                const float x1 = x + std::cos(a) * (rx + 12.0f);
                const float y1 = y + std::sin(a) * (ry + 8.0f);
                const float pulse = 0.55f + 0.45f * std::sin(t * 3.0f + tick);

                setOpacity(brush, ring.alpha * (0.28f + pulse * 0.42f));
                ctx.target->DrawLine(D2D1::Point2F(x0, y0), D2D1::Point2F(x1, y1), brush, 0.8f + pulse * 0.5f);
            }

            restoreOpacity(brush);
        }
    }

    void D2DCyberBackgroundField::renderCircuitWisps(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.border || !ctx.brushes.accent)
        {
            return;
        }

        for (int i = 0; i < 11; ++i)
        {
            const float base = hash01(900 + i * 17);
            const float y = rect.top + wrapPositive(base * height_ + std::sin(time_ * 0.09f + i) * 42.0f, height_);
            const float x = rect.left + wrapPositive(hash01(1200 + i * 19) * width_ + time_ * (8.0f + static_cast<float>(i % 3) * 3.0f), width_);
            const float lenA = 60.0f + hash01(1400 + i) * 130.0f;
            const float lenB = 34.0f + hash01(1500 + i) * 90.0f;
            const float step = (i % 2 == 0) ? 1.0f : -1.0f;

            setOpacity(ctx.brushes.border, 0.08f + hash01(1600 + i) * 0.06f);
            ctx.target->DrawLine(D2D1::Point2F(x, y), D2D1::Point2F(x + step * lenA, y), ctx.brushes.border, 0.8f);
            ctx.target->DrawLine(D2D1::Point2F(x + step * lenA, y), D2D1::Point2F(x + step * lenA, y + step * lenB), ctx.brushes.border, 0.8f);
            ctx.target->DrawLine(D2D1::Point2F(x + step * lenA, y + step * lenB), D2D1::Point2F(x + step * (lenA + lenB), y + step * lenB), ctx.brushes.border, 0.8f);

            setOpacity(ctx.brushes.accent, 0.12f);
            ctx.target->FillEllipse(ellipse(x + step * lenA, y, 2.0f, 2.0f), ctx.brushes.accent);
            ctx.target->FillEllipse(ellipse(x + step * (lenA + lenB), y + step * lenB, 1.6f, 1.6f), ctx.brushes.accent);
        }

        restoreOpacity(ctx.brushes.border);
        restoreOpacity(ctx.brushes.accent);
    }

    void D2DCyberBackgroundField::renderTextureDust(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.muted)
        {
            return;
        }

        for (int i = 0; i < 90; ++i)
        {
            const float x = rect.left + hash01(3000 + i * 11) * rect.width();
            const float y = rect.top + wrapPositive(hash01(3200 + i * 13) * rect.height() + std::sin(time_ * 0.11f + i) * 8.0f, rect.height());
            const float pulse = 0.5f + 0.5f * std::sin(time_ * 0.8f + hash01(3400 + i) * kPi * 2.0f);
            const float alpha = 0.025f + pulse * 0.035f;
            const float r = 0.45f + hash01(3600 + i) * 0.80f;

            setOpacity(ctx.brushes.muted, alpha);
            ctx.target->FillEllipse(ellipse(x, y, r, r), ctx.brushes.muted);
        }

        restoreOpacity(ctx.brushes.muted);
    }

    void D2DCyberBackgroundField::renderArcs(D2DRenderContext& ctx, UiRect rect)
    {
        if (!ctx.brushes.accentBlue)
        {
            return;
        }

        for (std::size_t i = 0; i < arcs_.size(); ++i)
        {
            const auto& arc = arcs_[i];
            const float t = time_ * (0.04f + static_cast<float>(i) * 0.015f) + arc.phase;
            const float x = rect.left + wrapPositive(arc.cx + std::sin(t) * 24.0f, width_);
            const float y = rect.top + wrapPositive(arc.cy + std::cos(t) * 20.0f, height_);
            const float sweepPulse = 0.7f + 0.3f * std::sin(t * 2.0f);

            setOpacity(ctx.brushes.accentBlue, arc.alpha * sweepPulse);
            ctx.target->DrawEllipse(ellipse(x, y, arc.radius, arc.radius * 0.62f), ctx.brushes.accentBlue, 0.9f);
            ctx.target->DrawLine(D2D1::Point2F(x - arc.radius * 0.55f, y), D2D1::Point2F(x - arc.radius * 0.18f, y - arc.radius * 0.18f), ctx.brushes.accentBlue, 0.8f);
            ctx.target->DrawLine(D2D1::Point2F(x + arc.radius * 0.22f, y + arc.radius * 0.12f), D2D1::Point2F(x + arc.radius * 0.58f, y + arc.radius * 0.02f), ctx.brushes.accentBlue, 0.8f);
        }

        restoreOpacity(ctx.brushes.accentBlue);
    }
}
