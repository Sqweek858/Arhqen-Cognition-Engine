#pragma once

#include "ArhqenCognitionEngine/Ui/Slate/AceSlateFramePrimitives.h"

#include <cstdint>
#include <string>
#include <vector>

namespace am::ui::slate
{
    enum class AceSlateViewportColorSpace : std::uint8_t
    {
        Linear,
        SRgb,
        HDR10Reserved
    };

    enum class AceSlateViewportCompositeMode : std::uint8_t
    {
        Opaque,
        IgnoreAlpha,
        PremultipliedAlpha,
        AdditiveDebugReserved
    };

    struct AceSlateViewportElementInput
    {
        UiRect localRect{};
        UiRect clipRect{};
        AceSlateViewportDescriptor descriptor{};
        AceSlateColor tint = AceSlateColor::White(1.0f);
        AceSlateDrawEffect effects = AceSlateDrawEffect::None;
        AceSlateViewportColorSpace colorSpace = AceSlateViewportColorSpace::Linear;
        AceSlateViewportCompositeMode compositeMode = AceSlateViewportCompositeMode::IgnoreAlpha;
        std::uint32_t layer = 0;
        std::uint64_t frameNumber = 0;
        std::string debugName;
    };

    struct AceSlateViewportElementValidation
    {
        bool valid = false;
        bool visible = false;
        bool requiresTexture = true;
        bool ignoreAlpha = true;
        bool pixelSnapped = true;
        bool resourceEpochValid = false;
        bool sizeMatchesRect = false;
        UiRect snappedRect{};
        std::string reason;
    };

    struct AceSlateViewportBatchKey
    {
        std::uint64_t resourceEpoch = 0;
        std::uint32_t layer = 0;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        AceSlateViewportCompositeMode compositeMode = AceSlateViewportCompositeMode::Opaque;
        AceSlateViewportColorSpace colorSpace = AceSlateViewportColorSpace::Linear;
        bool operator==(const AceSlateViewportBatchKey& rhs) const;
    };

    struct AceSlateViewportBatchRecord
    {
        AceSlateViewportBatchKey key{};
        UiRect rect{};
        UiRect clip{};
        AceSlateColor tint{};
        std::uint64_t frameNumber = 0;
        bool valid = false;
        std::string debugName;
    };

    class AceSlateViewportElementBuilder
    {
    public:
        AceSlateViewportElementValidation Validate(const AceSlateViewportElementInput& input) const;
        AceSlateElement BuildDrawElement(const AceSlateViewportElementInput& input, const AceSlateViewportElementValidation& validation) const;
        AceSlateViewportBatchRecord BuildBatchRecord(const AceSlateViewportElementInput& input, const AceSlateViewportElementValidation& validation) const;
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
        std::uint64_t BuiltCount() const { return builtCount_; }
        std::uint64_t InvalidCount() const { return invalidCount_; }
    private:
        UiRect SnapRect(UiRect rect) const;
        mutable std::uint64_t builtCount_ = 0;
        mutable std::uint64_t invalidCount_ = 0;
        mutable std::string lastReason_;
    };

    const char* AceSlateViewportColorSpaceName(AceSlateViewportColorSpace value);
    const char* AceSlateViewportCompositeModeName(AceSlateViewportCompositeMode value);
}
