#pragma once

#include "ArhqenCognitionEngine/Ui/Core/UiStyleTokens.h"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace am::ui
{
    struct AceUiPanelStyle
    {
        std::wstring name;
        UiColor fill{};
        UiColor border{};
        float radius = 14.0f;
        float borderWidth = 1.0f;
        float glowAlpha = 0.12f;
        float blurAlpha = 0.10f;
    };

    struct AceUiTextStyle
    {
        std::wstring name;
        UiColor color{};
        float size = 16.0f;
        bool strong = false;
    };

    struct AceUiStyleStats
    {
        std::size_t panelStyleCount = 0;
        std::size_t textStyleCount = 0;
        std::size_t colorTokenCount = 0;
        std::uint64_t lookupCount = 0;
        std::uint64_t missCount = 0;
    };

    class AceUiStyleSet
    {
    public:
        static AceUiStyleSet MakeDefaultArhqen();

        void RegisterColor(std::wstring key, UiColor color);
        void RegisterPanel(AceUiPanelStyle style);
        void RegisterText(AceUiTextStyle style);
        UiColor Color(const std::wstring& key, UiColor fallback = {}) const;
        AceUiPanelStyle Panel(const std::wstring& key) const;
        AceUiTextStyle Text(const std::wstring& key) const;
        AceUiStyleStats Stats() const;
        std::uint64_t Generation() const;

    private:
        mutable std::uint64_t lookupCount_ = 0;
        mutable std::uint64_t missCount_ = 0;
        std::uint64_t generation_ = 0;
        std::unordered_map<std::wstring, UiColor> colors_;
        std::unordered_map<std::wstring, AceUiPanelStyle> panels_;
        std::unordered_map<std::wstring, AceUiTextStyle> texts_;
    };
}
