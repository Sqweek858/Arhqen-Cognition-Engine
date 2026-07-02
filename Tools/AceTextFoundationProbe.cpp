#include "ArhqenCognitionEngine/Ui/Core/AceUiStyleSet.h"
#include "ArhqenCognitionEngine/Ui/D2D/DWriteTextCache.h"

#include <dwrite.h>
#include <wrl/client.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

namespace
{
    int failures = 0;

    void check(bool condition, const char* name)
    {
        std::cout << (condition ? "PASS|" : "FAIL|") << name << '\n';
        if (!condition)
        {
            ++failures;
        }
    }
}

int main()
{
    using Microsoft::WRL::ComPtr;
    using namespace am::ui;

    ComPtr<IDWriteFactory> factory;
    const HRESULT factoryResult = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(factory.GetAddressOf()));
    check(SUCCEEDED(factoryResult) && factory, "directwrite_factory_created");
    if (!factory)
    {
        return 1;
    }

    DWriteFontEngine engine;
    std::string error;
    check(engine.initialize(factory.Get(), &error), "font_engine_initialized");
    check(engine.generation() == 1, "font_generation_starts_at_one");

    TextLayoutOptions options;
    options.role = FontRole::Body;
    options.width = 120.0f;
    options.height = 32.0f;
    options.wrapping = DWRITE_WORD_WRAPPING_NO_WRAP;

    DWriteTextCache cache(8);
    const std::wstring unicodeText = L"Știință, țară și Unicode \U0001F680";
    const auto first = cache.getOrCreate(engine, unicodeText, options);
    const auto second = cache.getOrCreate(engine, unicodeText, options);
    check(first.valid && first.layout, "unicode_layout_created");
    check(second.valid && cache.hitCount() == 1 && cache.missCount() == 1, "identical_layout_hits_cache");

    options.trimEnd = true;
    const auto trimmed = cache.getOrCreate(engine, unicodeText, options);
    check(trimmed.valid && cache.missCount() == 2 && cache.size() == 2, "trimming_has_distinct_cache_identity");

    DWRITE_TRIMMING trimming{};
    ComPtr<IDWriteInlineObject> trimmingSign;
    const HRESULT trimmingResult = trimmed.layout->GetTrimming(&trimming, &trimmingSign);
    check(SUCCEEDED(trimmingResult) &&
        trimming.granularity == DWRITE_TRIMMING_GRANULARITY_CHARACTER && trimmingSign,
        "native_ellipsis_trimming_sign_installed");

    TextLayoutOptions invalidDimensions = options;
    invalidDimensions.width = std::numeric_limits<float>::quiet_NaN();
    invalidDimensions.height = -200.0f;
    const auto normalized = engine.createLayout(L"safe dimensions", invalidDimensions);
    check(normalized.valid && std::isfinite(normalized.metrics.width), "invalid_dimensions_are_normalized");

    const auto hit = engine.hitTestTextPosition(unicodeText, options, 2);
    check(hit.inside && hit.caretHeight > 0.0f, "unicode_caret_hit_test_works");

    options.trimEnd = false;
    check(engine.initialize(factory.Get(), &error) && engine.generation() == 2, "font_reinitialization_advances_generation");
    const std::size_t missesBeforeRecreate = cache.missCount();
    const auto recreated = cache.getOrCreate(engine, unicodeText, options);
    check(recreated.valid && cache.missCount() == missesBeforeRecreate + 1, "font_recreation_invalidates_cache_identity");

    const std::size_t entriesBeforeFailure = cache.size();
    DWriteFontEngine unavailableEngine;
    const auto unavailable = cache.getOrCreate(unavailableEngine, L"not cacheable", options);
    check(!unavailable.valid && cache.size() == entriesBeforeFailure, "failed_layout_is_not_cached");

    TextCacheKey plainKey;
    plainKey.text = L"key";
    TextCacheKey trimmedKey = plainKey;
    trimmedKey.trimEnd = true;
    TextCacheKey regeneratedKey = plainKey;
    regeneratedKey.fontGeneration = 4;
    check(!(plainKey == trimmedKey) && !(plainKey == regeneratedKey), "cache_key_tracks_trim_and_generation");

    auto styles = AceUiStyleSet::MakeDefaultArhqen();
    const auto initialStats = styles.Stats();
    check(styles.Generation() == initialStats.colorTokenCount + initialStats.panelStyleCount + initialStats.textStyleCount,
        "style_generation_tracks_default_registration");
    const auto previousGeneration = styles.Generation();
    styles.RegisterColor(L"accent.test", {0.2f, 0.3f, 0.4f, 1.0f});
    check(styles.Generation() == previousGeneration + 1, "style_mutation_advances_generation");
    const auto color = styles.Color(L"accent.test");
    check(color.r == 0.2f && color.a == 1.0f, "registered_style_resolves");

    DWriteTextCache boundedCache(2);
    options.width = 300.0f;
    boundedCache.getOrCreate(engine, L"one", options);
    boundedCache.getOrCreate(engine, L"two", options);
    boundedCache.getOrCreate(engine, L"three", options);
    check(boundedCache.size() == 2, "text_cache_honors_lru_capacity");

    std::cout << (failures == 0 ? "PASS|" : "FAIL|") << "ace_text_foundation_probe\n";
    return failures == 0 ? 0 : 1;
}
