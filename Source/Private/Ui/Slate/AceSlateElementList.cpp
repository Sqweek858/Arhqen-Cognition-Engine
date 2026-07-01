#include "ArhqenCognitionEngine/Ui/Slate/AceSlateElementList.h"

#include <numeric>

namespace am::ui::slate
{
    void AceSlateWindowElementList::BeginFrame(std::uint64_t frameNumber, UiRect windowRect, UiRect cullingRect, AceSlateFramePolicy policy)
    {
        frameNumber_ = frameNumber;
        windowRect_ = windowRect;
        cullingRect_ = cullingRect.empty() ? windowRect : cullingRect;
        policy_ = policy;
        elements_.clear();
        batches_.clear();
        clipStack_.clear();
        nextSerial_ = 1;
        stats_.resetForFrame(frameNumber_);
        if (policy_.IsFullFrameRequired())
        {
            ++stats_.fullFramePasses;
        }
    }

    void AceSlateWindowElementList::EndFrame()
    {
        stats_.elementCount = static_cast<std::uint64_t>(elements_.size());
        stats_.batchCount = static_cast<std::uint64_t>(batches_.size());
        stats_.vertexCount = 0;
        stats_.indexCount = 0;
        for (const auto& batch : batches_)
        {
            stats_.vertexCount += static_cast<std::uint64_t>(batch.vertices.size());
            stats_.indexCount += static_cast<std::uint64_t>(batch.indices.size());
        }
    }

    void AceSlateWindowElementList::Reset()
    {
        frameNumber_ = 0;
        windowRect_ = {};
        cullingRect_ = {};
        policy_ = {};
        stats_ = {};
        elements_.clear();
        batches_.clear();
        clipStack_.clear();
        nextSerial_ = 1;
    }

    std::uint64_t AceSlateWindowElementList::MakeBox(int layer, UiRect rect, AceSlateColor color, AceSlateDrawEffect effects, std::string debugName)
    {
        AceSlateElement e{};
        e.type = AceSlateElementType::Box;
        e.layer = layer;
        e.geometry.rect = rect;
        e.effects = effects;
        e.tint = color;
        e.clip = CurrentClip();
        e.debugName = std::move(debugName);
        return AddElement(std::move(e)).serial;
    }

    std::uint64_t AceSlateWindowElementList::MakeRoundedBox(int layer, UiRect rect, float radius, AceSlateColor color, AceSlateDrawEffect effects, std::string debugName)
    {
        AceSlateElement e{};
        e.type = AceSlateElementType::RoundedBox;
        e.layer = layer;
        e.geometry.rect = rect;
        e.radius = radius;
        e.effects = effects;
        e.tint = color;
        e.clip = CurrentClip();
        e.debugName = std::move(debugName);
        return AddElement(std::move(e)).serial;
    }

    std::uint64_t AceSlateWindowElementList::MakeBorder(int layer, UiRect rect, float strokeWidth, AceSlateColor color, AceSlateDrawEffect effects, std::string debugName)
    {
        AceSlateElement e{};
        e.type = AceSlateElementType::Border;
        e.layer = layer;
        e.geometry.rect = rect;
        e.strokeWidth = strokeWidth;
        e.effects = effects;
        e.tint = color;
        e.clip = CurrentClip();
        e.debugName = std::move(debugName);
        return AddElement(std::move(e)).serial;
    }

    std::uint64_t AceSlateWindowElementList::MakeText(int layer, UiRect rect, std::wstring text, AceSlateColor color, AceSlateDrawEffect effects, std::string debugName)
    {
        AceSlateElement e{};
        e.type = AceSlateElementType::Text;
        e.layer = layer;
        e.geometry.rect = rect;
        e.effects = effects;
        e.tint = color;
        e.text = std::move(text);
        e.clip = CurrentClip();
        e.debugName = std::move(debugName);
        return AddElement(std::move(e)).serial;
    }

    std::uint64_t AceSlateWindowElementList::MakeViewport(int layer, UiRect rect, AceSlateViewportDescriptor viewport, AceSlateDrawEffect effects, AceSlateColor tint, std::string debugName)
    {
        if (viewport.ignoreAlpha) { effects |= AceSlateDrawEffect::IgnoreTextureAlpha; }
        if (viewport.noGamma) { effects |= AceSlateDrawEffect::NoGamma; }
        if (viewport.premultipliedAlpha) { effects |= AceSlateDrawEffect::PreMultipliedAlpha; }
        if (viewport.allowScaling) { effects |= AceSlateDrawEffect::AllowScaling; }
        if (viewport.requiresVSync) { effects |= AceSlateDrawEffect::RequiresVSync; }
        AceSlateElement e{};
        e.type = AceSlateElementType::Viewport;
        e.layer = layer;
        e.geometry.rect = rect;
        e.effects = effects;
        e.tint = tint;
        e.viewport = std::move(viewport);
        e.clip = CurrentClip();
        e.debugName = std::move(debugName);
        return AddElement(std::move(e)).serial;
    }

    std::uint64_t AceSlateWindowElementList::MakeViewportFallbackBox(int layer, UiRect rect, std::string reason)
    {
        ++stats_.viewportFallbackBoxes;
        return MakeBox(layer, rect, AceSlateColor::Black(1.0f), AceSlateDrawEffect::PixelSnap | AceSlateDrawEffect::ForceOpaque, std::move(reason));
    }

    void AceSlateWindowElementList::PushClip(UiRect rect)
    {
        AceSlateClipState state{};
        state.rect = rect;
        state.enabled = !rect.empty();
        state.depth = static_cast<std::uint32_t>(clipStack_.size() + 1);
        clipStack_.push_back(state);
        ++stats_.clipPushCount;
        AceSlateElement e{};
        e.type = AceSlateElementType::ClipPush;
        e.geometry.rect = rect;
        e.clip = state;
        e.serial = nextSerial_++;
        elements_.push_back(std::move(e));
    }

    void AceSlateWindowElementList::PopClip()
    {
        if (!clipStack_.empty())
        {
            clipStack_.pop_back();
        }
        ++stats_.clipPopCount;
        AceSlateElement e{};
        e.type = AceSlateElementType::ClipPop;
        e.serial = nextSerial_++;
        elements_.push_back(std::move(e));
    }

    void AceSlateWindowElementList::SortByLayerStable()
    {
        std::stable_sort(elements_.begin(), elements_.end(), [](const AceSlateElement& a, const AceSlateElement& b)
        {
            if (a.layer != b.layer) { return a.layer < b.layer; }
            return a.serial < b.serial;
        });
    }

    void AceSlateWindowElementList::CullAndBatch()
    {
        SortByLayerStable();
        batches_.clear();
        for (const auto& element : elements_)
        {
            if (element.type == AceSlateElementType::Viewport)
            {
                ++stats_.viewportElementCount;
            }
            if (!element.isDrawable())
            {
                continue;
            }
            if (element.shouldCull(cullingRect_) || !element.clip.contains(element.geometry.transformedRect()))
            {
                ++stats_.culledElementCount;
                continue;
            }
            AddQuadBatchForElement(element, element.type == AceSlateElementType::Viewport ? element.viewport.d2dBitmap : nullptr);
        }
        EndFrame();
    }

    void AceSlateWindowElementList::BuildViewportBatches()
    {
        for (const auto& element : elements_)
        {
            if (element.type == AceSlateElementType::Viewport && !element.shouldCull(cullingRect_))
            {
                AddQuadBatchForElement(element, element.viewport.d2dBitmap);
            }
        }
        EndFrame();
    }

    void AceSlateWindowElementList::BuildBoxBatches()
    {
        for (const auto& element : elements_)
        {
            if ((element.type == AceSlateElementType::Box || element.type == AceSlateElementType::RoundedBox || element.type == AceSlateElementType::Border) && !element.shouldCull(cullingRect_))
            {
                AddQuadBatchForElement(element, nullptr);
            }
        }
        EndFrame();
    }

    void AceSlateWindowElementList::BuildTextBatchesAsDebugBoxes()
    {
        for (const auto& element : elements_)
        {
            if (element.type == AceSlateElementType::Text && !element.shouldCull(cullingRect_))
            {
                AddQuadBatchForElement(element, nullptr);
            }
        }
        EndFrame();
    }

    std::string AceSlateWindowElementList::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "frame=" << frameNumber_
            << ";policy=" << policy_.Explain()
            << ";stats=" << stats_.compact();
        if (!batches_.empty())
        {
            oss << ";firstBatch=" << batches_.front().summary();
        }
        return oss.str();
    }

    std::vector<std::string> AceSlateWindowElementList::BatchSummaries(std::size_t maxCount) const
    {
        std::vector<std::string> out;
        const std::size_t count = std::min(maxCount, batches_.size());
        out.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
            out.push_back(batches_[i].summary());
        }
        return out;
    }

    bool AceSlateWindowElementList::HasViewportElements() const
    {
        return std::any_of(elements_.begin(), elements_.end(), [](const AceSlateElement& e) { return e.type == AceSlateElementType::Viewport; });
    }

    bool AceSlateWindowElementList::RequiresVSync() const
    {
        return std::any_of(elements_.begin(), elements_.end(), [](const AceSlateElement& e)
        {
            return e.type == AceSlateElementType::Viewport && (e.viewport.requiresVSync || HasEffect(e.effects, AceSlateDrawEffect::RequiresVSync));
        });
    }

    UiRect AceSlateWindowElementList::UnionOfDrawableElements() const
    {
        UiRect result{};
        bool has = false;
        for (const auto& e : elements_)
        {
            if (e.isDrawable())
            {
                result = has ? UnionRect(result, e.geometry.transformedRect()) : e.geometry.transformedRect();
                has = true;
            }
        }
        return result;
    }

    std::optional<AceSlateElement> AceSlateWindowElementList::LastViewportElement() const
    {
        for (auto it = elements_.rbegin(); it != elements_.rend(); ++it)
        {
            if (it->type == AceSlateElementType::Viewport)
            {
                return *it;
            }
        }
        return std::nullopt;
    }

    AceSlateElement& AceSlateWindowElementList::AddElement(AceSlateElement element)
    {
        element.serial = nextSerial_++;
        if (element.type == AceSlateElementType::Viewport)
        {
            ++stats_.viewportElementCount;
            if (element.viewport.requiresVSync)
            {
                stats_.requiresVSync = true;
            }
        }
        elements_.push_back(std::move(element));
        return elements_.back();
    }

    AceSlateClipState AceSlateWindowElementList::CurrentClip() const
    {
        if (clipStack_.empty())
        {
            return {};
        }
        return clipStack_.back();
    }

    void AceSlateWindowElementList::AddQuadBatchForElement(const AceSlateElement& element, void* resourceOverride)
    {
        AceSlateBatchKey key{};
        key.type = element.type;
        key.layer = element.layer;
        key.resource = resourceOverride;
        key.effects = element.effects;
        key.clipEnabled = element.clip.enabled;
        key.clipRect = element.clip.rect;
        if (batches_.empty() || !batches_.back().key.compatibleWith(key))
        {
            AceSlateRenderBatch batch{};
            batch.key = key;
            batch.sourceElementFirst = element.serial;
            batch.sourceElementLast = element.serial;
            batches_.push_back(std::move(batch));
        }
        auto& batch = batches_.back();
        batch.sourceElementLast = element.serial;
        batch.reserveQuad();
        const bool snap = !HasEffect(element.effects, AceSlateDrawEffect::NoPixelSnap);
        batch.addQuad(element.geometry.transformedRect(), element.tint, snap);
    }

    void AceSlateElementBatcher::Build(AceSlateWindowElementList& list)
    {
        list.CullAndBatch();
        stats_ = list.Stats();
        diagnostics_ = list.Diagnostics();
    }
}
