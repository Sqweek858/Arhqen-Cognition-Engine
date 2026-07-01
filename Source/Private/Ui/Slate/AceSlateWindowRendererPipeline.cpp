#include "ArhqenCognitionEngine/Ui/Slate/AceSlateWindowRendererPipeline.h"

#include <algorithm>
#include <sstream>

namespace am::ui::slate
{
    void AceSlateWindowRendererPipeline::Reset()
    {
        invalidationPlanner_.Reset();
        batchCache_.Reset();
        clipStack_.Reset({});
        stats_ = {};
        lastPlan_ = {};
    }

    AceSlatePipelineOutput AceSlateWindowRendererPipeline::Build(AceSlateWindowElementList& list, const AceSlatePipelineInput& input)
    {
        ++stats_.frames;
        AceSlatePipelineOutput output{};
        output.invalidationPlan = invalidationPlanner_.BuildPlan(input.invalidation);
        output.fullFrame = output.invalidationPlan.fullFrame;
        if (output.fullFrame)
        {
            ++stats_.fullFrameFrames;
        }
        output.elementCount = list.Elements().size();
        output.valid = true;

        AceSlateBatchCacheInput cacheInput{};
        cacheInput.frameNumber = input.invalidation.frameNumber;
        cacheInput.resourceEpoch = input.resourceEpoch;
        cacheInput.fullFrame = output.fullFrame;
        cacheInput.allowRetainedBatches = output.invalidationPlan.allowRetainedContents;
        cacheInput.frameRect = input.invalidation.windowRect;
        batchCache_.BeginFrame(cacheInput);

        clipStack_.Reset(input.invalidation.windowRect);
        for (const auto& element : list.Elements())
        {
            const auto clipDecision = clipStack_.ResolveForLayer(element.geometry.rect, element.layer);
            if (!clipDecision.visible)
            {
                ++output.culledCount;
                ++stats_.clipRejects;
            }
            const std::uint64_t key = AceSlateHashElementForCache(element, input.resourceEpoch);
            if (batchCache_.Find(key, input.resourceEpoch).has_value())
            {
                ++output.cachedBatchHits;
                ++stats_.cacheHits;
            }
            else
            {
                ++output.cachedBatchMisses;
                ++stats_.cacheMisses;
            }
            if (element.type == AceSlateElementType::Viewport)
            {
                ++stats_.viewportElements;
            }
        }

        std::string layerFailure;
        if (input.debugValidateLayerOrder && !ValidateLayerOrder(list, &layerFailure))
        {
            ++stats_.layerValidationFailures;
            output.valid = false;
            output.diagnostics = layerFailure;
        }

        list.CullAndBatch();
        output.batches = list.Batches();
        ++stats_.batchBuilds;
        ++stats_.elementFrames;
        BuildCacheEntries(list, input);

        std::ostringstream diagnostics;
        diagnostics << "slate_pipeline"
            << ";elements=" << output.elementCount
            << ";batches=" << output.batches.size()
            << ";culled=" << output.culledCount
            << ";full_frame=" << (output.fullFrame ? "true" : "false")
            << ";cache_hit=" << output.cachedBatchHits
            << ";cache_miss=" << output.cachedBatchMisses
            << ";invalidate=" << output.invalidationPlan.diagnostics
            << ";valid=" << (output.valid ? "true" : "false");
        if (!output.diagnostics.empty())
        {
            diagnostics << ";failure=" << output.diagnostics;
        }
        output.diagnostics = diagnostics.str();
        stats_.lastDiagnostics = output.diagnostics;
        lastPlan_ = output.invalidationPlan;
        return output;
    }

    void AceSlateWindowRendererPipeline::CommitPresent(HRESULT presentHr)
    {
        invalidationPlanner_.CommitPresentedPlan(lastPlan_, presentHr);
    }

    void AceSlateWindowRendererPipeline::BuildCacheEntries(const AceSlateWindowElementList& list, const AceSlatePipelineInput& input)
    {
        for (const auto& element : list.Elements())
        {
            AceSlateCachedBatch batch{};
            batch.cacheKey = AceSlateHashElementForCache(element, input.resourceEpoch);
            batch.layer = element.layer;
            batch.kind = element.type;
            batch.bounds = element.geometry.rect;
            batch.clip = element.clip.rect;
            batch.resourceEpoch = input.resourceEpoch;
            batch.lastUsedFrame = input.invalidation.frameNumber;
            batch.hitCount = 0;
            batch.valid = !element.geometry.rect.empty();
            batch.debugName = element.debugName;
            if (batch.valid && !input.framePolicy.forceFullFrameRedraw)
            {
                batchCache_.Store(batch);
            }
        }
    }

    bool AceSlateWindowRendererPipeline::ValidateLayerOrder(const AceSlateWindowElementList& list, std::string* failure) const
    {
        int previousLayer = 0;
        bool first = true;
        for (const auto& element : list.Elements())
        {
            if (!first && element.layer < previousLayer)
            {
                if (failure)
                {
                    std::ostringstream oss;
                    oss << "layer_order_failed;prev=" << previousLayer
                        << ";current=" << element.layer
                        << ";element=" << element.debugName;
                    *failure = oss.str();
                }
                return false;
            }
            previousLayer = element.layer;
            first = false;
        }
        return true;
    }

    std::string AceSlateWindowRendererPipeline::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "frames=" << stats_.frames
            << ";full_frame=" << stats_.fullFrameFrames
            << ";element_frames=" << stats_.elementFrames
            << ";viewport_elements=" << stats_.viewportElements
            << ";cache_hits=" << stats_.cacheHits
            << ";cache_misses=" << stats_.cacheMisses
            << ";layer_failures=" << stats_.layerValidationFailures
            << ";clip_rejects=" << stats_.clipRejects
            << ";batch_builds=" << stats_.batchBuilds
            << ";invalidation={" << invalidationPlanner_.Diagnostics() << "}"
            << ";batch_cache={" << batchCache_.Diagnostics() << "}"
            << ";clip={" << clipStack_.Diagnostics() << "}"
            << ";last={" << stats_.lastDiagnostics << "}";
        return oss.str();
    }

    std::wstring AceSlateWindowRendererPipeline::WideDiagnostics() const
    {
        const std::string text = Diagnostics();
        return std::wstring(text.begin(), text.end());
    }
}
