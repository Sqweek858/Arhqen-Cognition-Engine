#include "ArhqenCognitionEngine/Ui/D2D/DWriteFontEngine.h"

#include <sstream>

namespace am::ui
{
    bool DWriteFontEngine::initialize(IDWriteFactory* factory, std::string* error)
    {
        if (!factory)
        {
            if (error)
            {
                *error = "DWriteFontEngine::initialize received null IDWriteFactory.";
            }
            return false;
        }

        factory_ = factory;
        initialized_ = createDefaultFormats(error);
        return initialized_;
    }

    bool DWriteFontEngine::initialized() const
    {
        return initialized_;
    }

    IDWriteTextFormat* DWriteFontEngine::format(FontRole role) const
    {
        const auto index = roleIndex(role);
        if (index >= formats_.size())
        {
            return nullptr;
        }

        return formats_[index].Get();
    }

    TextLayoutResult DWriteFontEngine::createLayout(const std::wstring& text, const TextLayoutOptions& options) const
    {
        TextLayoutResult result;

        if (!initialized_ || !factory_)
        {
            return result;
        }

        IDWriteTextFormat* selectedFormat = format(options.role);
        if (!selectedFormat)
        {
            return result;
        }

        Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
        const HRESULT hr = factory_->CreateTextLayout(
            text.c_str(),
            static_cast<UINT32>(text.size()),
            selectedFormat,
            options.width,
            options.height,
            &layout
        );

        if (FAILED(hr) || !layout)
        {
            return result;
        }

        layout->SetTextAlignment(options.horizontal);
        layout->SetParagraphAlignment(options.vertical);
        layout->SetWordWrapping(options.wrapping);

        if (options.trimEnd)
        {
            DWRITE_TRIMMING trimming{};
            trimming.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
            layout->SetTrimming(&trimming, nullptr);
        }

        DWRITE_TEXT_METRICS metrics{};
        layout->GetMetrics(&metrics);

        result.layout = layout;
        result.metrics = metrics;
        result.valid = true;
        return result;
    }

    DWRITE_TEXT_METRICS DWriteFontEngine::measure(const std::wstring& text, const TextLayoutOptions& options) const
    {
        return createLayout(text, options).metrics;
    }

    TextHitResult DWriteFontEngine::hitTestPoint(const std::wstring& text, const TextLayoutOptions& options, float x, float y) const
    {
        TextHitResult result;
        const auto layoutResult = createLayout(text, options);
        if (!layoutResult.valid || !layoutResult.layout)
        {
            return result;
        }

        BOOL trailing = FALSE;
        BOOL inside = FALSE;
        DWRITE_HIT_TEST_METRICS metrics{};

        const HRESULT hr = layoutResult.layout->HitTestPoint(x, y, &trailing, &inside, &metrics);
        if (FAILED(hr))
        {
            return result;
        }

        result.textPosition = metrics.textPosition;
        result.trailingHit = trailing != FALSE;
        result.inside = inside != FALSE;
        result.caretX = metrics.left + (result.trailingHit ? metrics.width : 0.0f);
        result.caretY = metrics.top;
        result.caretHeight = metrics.height;
        return result;
    }

    TextHitResult DWriteFontEngine::hitTestTextPosition(const std::wstring& text, const TextLayoutOptions& options, std::uint32_t position, bool trailing) const
    {
        TextHitResult result;
        const auto layoutResult = createLayout(text, options);
        if (!layoutResult.valid || !layoutResult.layout)
        {
            return result;
        }

        FLOAT pointX = 0.0f;
        FLOAT pointY = 0.0f;
        DWRITE_HIT_TEST_METRICS metrics{};

        const HRESULT hr = layoutResult.layout->HitTestTextPosition(
            position,
            trailing ? TRUE : FALSE,
            &pointX,
            &pointY,
            &metrics
        );

        if (FAILED(hr))
        {
            return result;
        }

        result.textPosition = position;
        result.trailingHit = trailing;
        result.inside = true;
        result.caretX = pointX;
        result.caretY = pointY;
        result.caretHeight = metrics.height;
        return result;
    }

    void DWriteFontEngine::drawText(
        ID2D1RenderTarget* target,
        const std::wstring& text,
        const TextLayoutOptions& options,
        D2D1_POINT_2F origin,
        ID2D1Brush* brush) const
    {
        if (!target || !brush || text.empty())
        {
            return;
        }

        const auto layoutResult = createLayout(text, options);
        if (!layoutResult.valid || !layoutResult.layout)
        {
            return;
        }

        target->DrawTextLayout(origin, layoutResult.layout.Get(), brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }

    void DWriteFontEngine::drawTextInRect(
        ID2D1RenderTarget* target,
        const std::wstring& text,
        const TextLayoutOptions& options,
        UiRect rect,
        ID2D1Brush* brush) const
    {
        TextLayoutOptions localOptions = options;
        localOptions.width = rect.width();
        localOptions.height = rect.height();
        drawText(target, text, localOptions, D2D1::Point2F(rect.left, rect.top), brush);
    }

    std::wstring DWriteFontEngine::roleName(FontRole role) const
    {
        switch (role)
        {
        case FontRole::Title:
            return L"Title";
        case FontRole::Subtitle:
            return L"Subtitle";
        case FontRole::Body:
            return L"Body";
        case FontRole::BodyStrong:
            return L"BodyStrong";
        case FontRole::Small:
            return L"Small";
        case FontRole::Mono:
            return L"Mono";
        case FontRole::Button:
            return L"Button";
        case FontRole::Count:
            return L"Count";
        default:
            return L"Unknown";
        }
    }

    bool DWriteFontEngine::createDefaultFormats(std::string* error)
    {
        const std::array<FontRoleSpec, static_cast<std::size_t>(FontRole::Count)> specs =
        {
            FontRoleSpec{L"Segoe UI", 30.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD},
            FontRoleSpec{L"Segoe UI", 15.0f, DWRITE_FONT_WEIGHT_NORMAL},
            FontRoleSpec{L"Segoe UI", 18.0f, DWRITE_FONT_WEIGHT_NORMAL},
            FontRoleSpec{L"Segoe UI", 18.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD},
            FontRoleSpec{L"Segoe UI", 14.0f, DWRITE_FONT_WEIGHT_NORMAL},
            FontRoleSpec{L"Cascadia Mono", 15.0f, DWRITE_FONT_WEIGHT_NORMAL},
            FontRoleSpec{L"Segoe UI", 18.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD}
        };

        for (std::size_t i = 0; i < specs.size(); ++i)
        {
            if (!createFormat(static_cast<FontRole>(i), specs[i], error))
            {
                return false;
            }
        }

        return true;
    }

    bool DWriteFontEngine::createFormat(FontRole role, const FontRoleSpec& spec, std::string* error)
    {
        if (!factory_)
        {
            if (error)
            {
                *error = "DWriteFontEngine::createFormat called without factory.";
            }
            return false;
        }

        Microsoft::WRL::ComPtr<IDWriteTextFormat> created;
        const HRESULT hr = factory_->CreateTextFormat(
            spec.family,
            nullptr,
            spec.weight,
            spec.style,
            spec.stretch,
            spec.size,
            spec.locale,
            &created
        );

        if (FAILED(hr))
        {
            if (error)
            {
                *error = hresultToString("IDWriteFactory::CreateTextFormat", hr);
            }
            return false;
        }

        created->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
        created->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        created->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

        if (role == FontRole::Button)
        {
            created->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            created->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        formats_[roleIndex(role)] = created;
        return true;
    }

    std::size_t DWriteFontEngine::roleIndex(FontRole role)
    {
        return static_cast<std::size_t>(role);
    }

    std::string DWriteFontEngine::hresultToString(const char* label, HRESULT hr)
    {
        std::ostringstream out;
        out << label << " failed. HRESULT=0x" << std::hex << static_cast<unsigned long>(hr);
        return out.str();
    }
}
