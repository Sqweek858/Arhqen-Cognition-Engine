#pragma once

#include "ArhqenCognitionEngine/Ui/Slate/AceSlateBatchCache.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateClipStack.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateElementList.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateInvalidationPlanner.h"

#include <cstdint>
#include <string>
#include <vector>

namespace am::ui::slate
{
    struct AceSlatePipelineInput
    {
        AceSlateInvalidationInput invalidation{};
        AceSlateFramePolicy framePolicy{};
        std::uint64_t resourceEpoch = 0;
        bool viewportAsElement = true;
        bool debugValidateLayerOrder = true;
    };

    struct AceSlatePipelineOutput
    {
        AceSlateInvalidationPlan invalidationPlan{};
        std::vector<AceSlateRenderBatch> batches{};
        std::uint64_t elementCount = 0;
        std::uint64_t culledCount = 0;
        std::uint64_t cachedBatchHits = 0;
        std::uint64_t cachedBatchMisses = 0;
        bool fullFrame = true;
        bool valid = true;
        std::string diagnostics;
    };

    struct AceSlatePipelineStats
    {
        std::uint64_t frames = 0;
        std::uint64_t fullFrameFrames = 0;
        std::uint64_t elementFrames = 0;
        std::uint64_t viewportElements = 0;
        std::uint64_t cacheHits = 0;
        std::uint64_t cacheMisses = 0;
        std::uint64_t layerValidationFailures = 0;
        std::uint64_t clipRejects = 0;
        std::uint64_t batchBuilds = 0;
        std::string lastDiagnostics;
    };

    class AceSlateWindowRendererPipeline
    {
    public:
        void Reset();
        AceSlatePipelineOutput Build(AceSlateWindowElementList& list, const AceSlatePipelineInput& input);
        void CommitPresent(HRESULT presentHr);
        const AceSlatePipelineStats& Stats() const { return stats_; }
        const AceSlateInvalidationPlanner& InvalidationPlanner() const { return invalidationPlanner_; }
        const AceSlateBatchCache& BatchCache() const { return batchCache_; }
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
    private:
        void BuildCacheEntries(const AceSlateWindowElementList& list, const AceSlatePipelineInput& input);
        bool ValidateLayerOrder(const AceSlateWindowElementList& list, std::string* failure) const;
        AceSlateInvalidationPlanner invalidationPlanner_{};
        AceSlateBatchCache batchCache_{512};
        AceSlateClipStack clipStack_{};
        AceSlatePipelineStats stats_{};
        AceSlateInvalidationPlan lastPlan_{};
    };
}
