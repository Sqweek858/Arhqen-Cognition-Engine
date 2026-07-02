#include "ArhqenCognitionEngine/Ui/Core/AceUiStyleSet.h"
#include <utility>

namespace am::ui
{
    AceUiStyleSet AceUiStyleSet::MakeDefaultArhqen()
    {
        AceUiStyleSet set;
        const auto tokens = makeDefaultCyberpunkThemeTokens();
        set.RegisterColor(L"text.primary", tokens.palette.text);
        set.RegisterColor(L"text.muted", tokens.palette.textMuted);
        set.RegisterColor(L"accent.cyan", tokens.palette.accentCyan);
        set.RegisterColor(L"accent.warn", tokens.palette.warning);
        set.RegisterColor(L"panel.deep", tokens.palette.panel0);
        set.RegisterColor(L"panel.elevated", tokens.palette.panel2);
        set.RegisterPanel({L"panel.acrylic", tokens.palette.panel0, tokens.palette.borderActive, 16.0f, 1.0f, 0.16f, 0.14f});
        set.RegisterPanel({L"panel.dark", tokens.palette.background1, tokens.palette.borderSoft, 12.0f, 1.0f, 0.08f, 0.06f});
        set.RegisterText({L"text.body", tokens.palette.text, 16.0f, false});
        set.RegisterText({L"text.section", tokens.palette.text, 17.0f, true});
        set.RegisterText({L"text.muted", tokens.palette.textMuted, 14.0f, false});
        return set;
    }

    void AceUiStyleSet::RegisterColor(std::wstring key, UiColor color)
    {
        colors_[std::move(key)] = color;
        ++generation_;
    }

    void AceUiStyleSet::RegisterPanel(AceUiPanelStyle style)
    {
        panels_[style.name] = std::move(style);
        ++generation_;
    }

    void AceUiStyleSet::RegisterText(AceUiTextStyle style)
    {
        texts_[style.name] = std::move(style);
        ++generation_;
    }

    UiColor AceUiStyleSet::Color(const std::wstring& key, UiColor fallback) const
    {
        ++lookupCount_;
        const auto it = colors_.find(key);
        if (it == colors_.end())
        {
            ++missCount_;
            return fallback;
        }
        return it->second;
    }

    AceUiPanelStyle AceUiStyleSet::Panel(const std::wstring& key) const
    {
        ++lookupCount_;
        const auto it = panels_.find(key);
        if (it == panels_.end())
        {
            ++missCount_;
            return {};
        }
        return it->second;
    }

    AceUiTextStyle AceUiStyleSet::Text(const std::wstring& key) const
    {
        ++lookupCount_;
        const auto it = texts_.find(key);
        if (it == texts_.end())
        {
            ++missCount_;
            return {};
        }
        return it->second;
    }

    AceUiStyleStats AceUiStyleSet::Stats() const
    {
        AceUiStyleStats stats;
        stats.panelStyleCount = panels_.size();
        stats.textStyleCount = texts_.size();
        stats.colorTokenCount = colors_.size();
        stats.lookupCount = lookupCount_;
        stats.missCount = missCount_;
        return stats;
    }

    std::uint64_t AceUiStyleSet::Generation() const
    {
        return generation_;
    }
}
