#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"

#include <wrl/client.h>

#include <array>
#include <optional>
#include <string>
#include <unordered_map>

namespace am::ui
{
    enum class FontRole
    {
        Title = 0,
        Subtitle,
        Body,
        BodyStrong,
        Small,
        Mono,
        Button,
        Count
    };

    enum class TextVerticalAlign
    {
        Top,
        Center,
        Bottom
    };

    struct TextLayoutOptions
    {
        FontRole role = FontRole::Body;
        float width = 0.0f;
        float height = 0.0f;
        DWRITE_TEXT_ALIGNMENT horizontal = DWRITE_TEXT_ALIGNMENT_LEADING;
        DWRITE_PARAGRAPH_ALIGNMENT vertical = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
        DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_WRAP;
        bool trimEnd = false;
    };

    struct TextLayoutResult
    {
        Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
        DWRITE_TEXT_METRICS metrics{};
        bool valid = false;
    };

    struct TextHitResult
    {
        std::uint32_t textPosition = 0;
        bool trailingHit = false;
        bool inside = false;
        float caretX = 0.0f;
        float caretY = 0.0f;
        float caretHeight = 0.0f;
    };

    struct FontRoleSpec
    {
        const wchar_t* family = L"Segoe UI";
        float size = 16.0f;
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;
        DWRITE_FONT_STYLE style = DWRITE_FONT_STYLE_NORMAL;
        DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL;
        const wchar_t* locale = L"ro-RO";
    };

    class DWriteFontEngine
    {
    public:
        DWriteFontEngine() = default;

        bool initialize(IDWriteFactory* factory, std::string* error);
        bool initialized() const;

        IDWriteTextFormat* format(FontRole role) const;
        TextLayoutResult createLayout(const std::wstring& text, const TextLayoutOptions& options) const;
        DWRITE_TEXT_METRICS measure(const std::wstring& text, const TextLayoutOptions& options) const;

        TextHitResult hitTestPoint(const std::wstring& text, const TextLayoutOptions& options, float x, float y) const;
        TextHitResult hitTestTextPosition(const std::wstring& text, const TextLayoutOptions& options, std::uint32_t position, bool trailing = false) const;

        void drawText(
            ID2D1RenderTarget* target,
            const std::wstring& text,
            const TextLayoutOptions& options,
            D2D1_POINT_2F origin,
            ID2D1Brush* brush
        ) const;

        void drawTextInRect(
            ID2D1RenderTarget* target,
            const std::wstring& text,
            const TextLayoutOptions& options,
            UiRect rect,
            ID2D1Brush* brush
        ) const;

        std::wstring roleName(FontRole role) const;

    private:
        bool createDefaultFormats(std::string* error);
        bool createFormat(FontRole role, const FontRoleSpec& spec, std::string* error);
        static std::size_t roleIndex(FontRole role);
        static std::string hresultToString(const char* label, HRESULT hr);

        Microsoft::WRL::ComPtr<IDWriteFactory> factory_;
        std::array<Microsoft::WRL::ComPtr<IDWriteTextFormat>, static_cast<std::size_t>(FontRole::Count)> formats_{};
        bool initialized_ = false;
    };
}
