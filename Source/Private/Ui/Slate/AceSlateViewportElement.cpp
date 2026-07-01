#include "ArhqenCognitionEngine/Ui/Slate/AceSlateViewportElement.h"

#include <cmath>
#include <sstream>

namespace am::ui::slate
{
    bool AceSlateViewportBatchKey::operator==(const AceSlateViewportBatchKey& rhs) const
    {
        return resourceEpoch == rhs.resourceEpoch &&
            layer == rhs.layer &&
            width == rhs.width &&
            height == rhs.height &&
            compositeMode == rhs.compositeMode &&
            colorSpace == rhs.colorSpace;
    }

    AceSlateViewportElementValidation AceSlateViewportElementBuilder::Validate(const AceSlateViewportElementInput& input) const
    {
        AceSlateViewportElementValidation validation{};
        validation.visible = !input.localRect.empty() && !input.clipRect.empty();
        validation.requiresTexture = true;
        validation.ignoreAlpha = input.compositeMode == AceSlateViewportCompositeMode::IgnoreAlpha || input.descriptor.ignoreAlpha;
        validation.pixelSnapped = HasEffect(input.effects, AceSlateDrawEffect::PixelSnap);
        validation.resourceEpochValid = input.descriptor.resourceEpoch > 0;
        validation.snappedRect = validation.pixelSnapped ? SnapRect(input.localRect) : input.localRect;
        const std::uint32_t rectW = static_cast<std::uint32_t>(std::max(0.0f, validation.snappedRect.width()));
        const std::uint32_t rectH = static_cast<std::uint32_t>(std::max(0.0f, validation.snappedRect.height()));
        validation.sizeMatchesRect = input.descriptor.allowScaling ||
            (input.descriptor.width == rectW && input.descriptor.height == rectH);

        std::ostringstream reason;
        if (!validation.visible)
        {
            reason << "viewport_invisible";
        }
        else if (!input.descriptor.valid)
        {
            reason << "descriptor_invalid";
        }
        else if (input.descriptor.width == 0 || input.descriptor.height == 0)
        {
            reason << "zero_texture_extent";
        }
        else if (!validation.resourceEpochValid)
        {
            reason << "missing_resource_epoch";
        }
        else if (!validation.sizeMatchesRect)
        {
            reason << "viewport_size_mismatch";
        }
        else
        {
            validation.valid = true;
            reason << "viewport_element_valid";
        }
        reason << ";rect=" << validation.snappedRect.left << ',' << validation.snappedRect.top << ','
            << validation.snappedRect.width() << 'x' << validation.snappedRect.height()
            << ";texture=" << input.descriptor.width << 'x' << input.descriptor.height
            << ";epoch=" << input.descriptor.resourceEpoch
            << ";alpha=" << AceSlateViewportCompositeModeName(input.compositeMode)
            << ";color=" << AceSlateViewportColorSpaceName(input.colorSpace);
        validation.reason = reason.str();
        if (validation.valid)
        {
            ++builtCount_;
        }
        else
        {
            ++invalidCount_;
        }
        lastReason_ = validation.reason;
        return validation;
    }

    AceSlateElement AceSlateViewportElementBuilder::BuildDrawElement(const AceSlateViewportElementInput& input, const AceSlateViewportElementValidation& validation) const
    {
        AceSlateElement element{};
        element.type = AceSlateElementType::Viewport;
        element.layer = static_cast<int>(input.layer);
        element.geometry.rect = validation.snappedRect;
                element.clip.rect = input.clipRect;
        element.tint = input.tint;
        element.effects = input.effects;
        if (validation.ignoreAlpha)
        {
            element.effects = element.effects | AceSlateDrawEffect::IgnoreTextureAlpha;
        }
        element.viewport = input.descriptor;
        element.debugName = input.debugName.empty() ? "slate_viewport" : input.debugName;
        return element;
    }

    AceSlateViewportBatchRecord AceSlateViewportElementBuilder::BuildBatchRecord(const AceSlateViewportElementInput& input, const AceSlateViewportElementValidation& validation) const
    {
        AceSlateViewportBatchRecord record{};
        record.key.resourceEpoch = input.descriptor.resourceEpoch;
        record.key.layer = input.layer;
        record.key.width = input.descriptor.width;
        record.key.height = input.descriptor.height;
        record.key.compositeMode = input.compositeMode;
        record.key.colorSpace = input.colorSpace;
        record.rect = validation.snappedRect;
        record.clip = input.clipRect;
        record.tint = input.tint;
        record.frameNumber = input.frameNumber;
        record.valid = validation.valid;
        record.debugName = input.debugName;
        return record;
    }

    std::string AceSlateViewportElementBuilder::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "built=" << builtCount_
            << ";invalid=" << invalidCount_
            << ";last=" << lastReason_;
        return oss.str();
    }

    std::wstring AceSlateViewportElementBuilder::WideDiagnostics() const
    {
        const std::string text = Diagnostics();
        return std::wstring(text.begin(), text.end());
    }

    UiRect AceSlateViewportElementBuilder::SnapRect(UiRect rect) const
    {
        rect.left = std::floor(rect.left + 0.5f);
        rect.top = std::floor(rect.top + 0.5f);
        rect.right = std::floor(rect.right + 0.5f);
        rect.bottom = std::floor(rect.bottom + 0.5f);
        return rect;
    }

    const char* AceSlateViewportColorSpaceName(AceSlateViewportColorSpace value)
    {
        switch (value)
        {
        case AceSlateViewportColorSpace::Linear: return "linear";
        case AceSlateViewportColorSpace::SRgb: return "srgb";
        case AceSlateViewportColorSpace::HDR10Reserved: return "hdr10_reserved";
        default: return "unknown";
        }
    }

    const char* AceSlateViewportCompositeModeName(AceSlateViewportCompositeMode value)
    {
        switch (value)
        {
        case AceSlateViewportCompositeMode::Opaque: return "opaque";
        case AceSlateViewportCompositeMode::IgnoreAlpha: return "ignore_alpha";
        case AceSlateViewportCompositeMode::PremultipliedAlpha: return "premultiplied_alpha";
        case AceSlateViewportCompositeMode::AdditiveDebugReserved: return "additive_debug_reserved";
        default: return "unknown";
        }
    }
}
