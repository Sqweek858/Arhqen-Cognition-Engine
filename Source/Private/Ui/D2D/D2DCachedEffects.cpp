#include "ArhqenCognitionEngine/Ui/D2D/D2DCachedEffects.h"

#include "ArhqenCognitionEngine/Ui/D2D/D2DCyberEffects.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DWidgetUtils.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>

namespace am::ui
{
    namespace
    {
        struct CacheEntry
        {
            std::vector<D2DFrostedLayer> layers;
        };

        struct CacheKey
        {
            int widthBucket = 0;
            int heightBucket = 0;
            int radiusBucket = 0;
            int alphaBucket = 0;

            bool operator<(const CacheKey& other) const
            {
                return std::tie(widthBucket, heightBucket, radiusBucket, alphaBucket) <
                    std::tie(other.widthBucket, other.heightBucket, other.radiusBucket, other.alphaBucket);
            }
        };

        std::map<CacheKey, CacheEntry> gBlurLayerCache;
        D2DEffectCacheStats gStats;

        int bucket(float value, float quantum)
        {
            return static_cast<int>(std::round(value / quantum));
        }

        void setOpacity(ID2D1Brush* brush, float alpha)
        {
            if (brush)
            {
                brush->SetOpacity(std::clamp(alpha, 0.0f, 1.0f));
            }
        }

        void restoreOpacity(ID2D1Brush* brush)
        {
            if (brush)
            {
                brush->SetOpacity(1.0f);
            }
        }

        const CacheEntry& findOrBuild(UiRect rect, float radius, float alpha)
        {
            ++gStats.lookupCount;
            const CacheKey key{
                bucket(rect.width(), 8.0f),
                bucket(rect.height(), 8.0f),
                bucket(radius, 2.0f),
                bucket(alpha, 0.02f)
            };

            auto found = gBlurLayerCache.find(key);
            if (found != gBlurLayerCache.end())
            {
                ++gStats.hitCount;
                return found->second;
            }

            ++gStats.missCount;
            CacheEntry entry;
            entry.layers.reserve(5);
            for (int i = 5; i >= 1; --i)
            {
                const float spread = static_cast<float>(i) * 3.5f;
                D2DFrostedLayer layer;
                layer.rect = makeUiRect(-spread, -spread * 0.45f, rect.width() + spread, rect.height() + spread * 0.75f);
                layer.radius = radius + spread;
                layer.alpha = alpha * (0.035f + 0.018f * static_cast<float>(6 - i));
                layer.useBlueAccent = (i % 2) != 0;
                entry.layers.push_back(layer);
            }

            if (gBlurLayerCache.size() > 192)
            {
                gBlurLayerCache.clear();
                ++gStats.resetCount;
            }

            auto [it, inserted] = gBlurLayerCache.emplace(key, std::move(entry));
            (void)inserted;
            gStats.cachedEntryCount = gBlurLayerCache.size();
            return it->second;
        }
    }

    void D2DCachedEffects::drawCachedBlurFallback(D2DRenderContext& ctx, UiRect rect, float radius, float alpha)
    {
        if (!ctx.target || !ctx.brushes.accentBlue || !ctx.brushes.accent || rect.empty() || alpha <= 0.0f)
        {
            return;
        }

        const CacheEntry& entry = findOrBuild(rect, radius, alpha);
        for (const D2DFrostedLayer& layer : entry.layers)
        {
            UiRect drawRect = makeUiRect(
                rect.left + layer.rect.left,
                rect.top + layer.rect.top,
                rect.left + layer.rect.right,
                rect.top + layer.rect.bottom
            );
            ID2D1SolidColorBrush* brush = layer.useBlueAccent ? ctx.brushes.accentBlue : ctx.brushes.accent;
            setOpacity(brush, layer.alpha);
            ctx.target->FillRoundedRectangle(D2DWidgetUtils::rounded(drawRect, layer.radius), brush);
        }

        restoreOpacity(ctx.brushes.accent);
        restoreOpacity(ctx.brushes.accentBlue);
    }

    void D2DCachedEffects::drawCachedAcrylicPanel(D2DRenderContext& ctx, UiRect rect, float radius, float fillAlpha, float glowAlpha)
    {
        if (!ctx.target || rect.empty())
        {
            return;
        }

        drawCachedBlurFallback(ctx, rect.inset(1.0f), radius, glowAlpha * 0.58f);
        if (ctx.brushes.panel)
        {
            setOpacity(ctx.brushes.panel, fillAlpha);
            D2DWidgetUtils::fillRounded(ctx, rect, radius, ctx.brushes.panel);
            restoreOpacity(ctx.brushes.panel);
        }
        D2DCyberEffects::drawBorderGlow(ctx, rect, radius, glowAlpha);
    }

    void D2DCachedEffects::reset()
    {
        gBlurLayerCache.clear();
        ++gStats.resetCount;
        gStats.cachedEntryCount = 0;
    }

    D2DEffectCacheStats D2DCachedEffects::stats()
    {
        gStats.cachedEntryCount = gBlurLayerCache.size();
        return gStats;
    }
}
