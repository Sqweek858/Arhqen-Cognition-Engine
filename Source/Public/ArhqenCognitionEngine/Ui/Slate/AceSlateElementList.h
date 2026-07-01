#pragma once

#include "ArhqenCognitionEngine/Ui/Slate/AceSlateFramePrimitives.h"

#include <deque>
#include <map>

namespace am::ui::slate
{
    class AceSlateWindowElementList
    {
    public:
        void BeginFrame(std::uint64_t frameNumber, UiRect windowRect, UiRect cullingRect, AceSlateFramePolicy policy);
        void EndFrame();
        void Reset();

        std::uint64_t FrameNumber() const { return frameNumber_; }
        UiRect WindowRect() const { return windowRect_; }
        UiRect CullingRect() const { return cullingRect_; }
        const AceSlateFramePolicy& Policy() const { return policy_; }
        const AceSlateFrameStats& Stats() const { return stats_; }
        AceSlateFrameStats& MutableStats() { return stats_; }

        std::uint64_t MakeBox(int layer, UiRect rect, AceSlateColor color, AceSlateDrawEffect effects = AceSlateDrawEffect::PixelSnap, std::string debugName = {});
        std::uint64_t MakeRoundedBox(int layer, UiRect rect, float radius, AceSlateColor color, AceSlateDrawEffect effects = AceSlateDrawEffect::PixelSnap, std::string debugName = {});
        std::uint64_t MakeBorder(int layer, UiRect rect, float strokeWidth, AceSlateColor color, AceSlateDrawEffect effects = AceSlateDrawEffect::PixelSnap, std::string debugName = {});
        std::uint64_t MakeText(int layer, UiRect rect, std::wstring text, AceSlateColor color, AceSlateDrawEffect effects = AceSlateDrawEffect::PixelSnap, std::string debugName = {});
        std::uint64_t MakeViewport(int layer, UiRect rect, AceSlateViewportDescriptor viewport, AceSlateDrawEffect effects, AceSlateColor tint, std::string debugName = {});
        std::uint64_t MakeViewportFallbackBox(int layer, UiRect rect, std::string reason);
        void PushClip(UiRect rect);
        void PopClip();

        const std::vector<AceSlateElement>& Elements() const { return elements_; }
        std::vector<AceSlateElement>& MutableElements() { return elements_; }
        const std::vector<AceSlateRenderBatch>& Batches() const { return batches_; }
        std::vector<AceSlateRenderBatch>& MutableBatches() { return batches_; }

        void SortByLayerStable();
        void CullAndBatch();
        void BuildViewportBatches();
        void BuildBoxBatches();
        void BuildTextBatchesAsDebugBoxes();
        std::string Diagnostics() const;
        std::vector<std::string> BatchSummaries(std::size_t maxCount = 16) const;

        bool HasViewportElements() const;
        bool RequiresVSync() const;
        UiRect UnionOfDrawableElements() const;
        std::optional<AceSlateElement> LastViewportElement() const;

    private:
        AceSlateElement& AddElement(AceSlateElement element);
        AceSlateClipState CurrentClip() const;
        void AddQuadBatchForElement(const AceSlateElement& element, void* resourceOverride = nullptr);

        std::uint64_t frameNumber_ = 0;
        UiRect windowRect_{};
        UiRect cullingRect_{};
        AceSlateFramePolicy policy_{};
        AceSlateFrameStats stats_{};
        std::vector<AceSlateElement> elements_;
        std::vector<AceSlateRenderBatch> batches_;
        std::vector<AceSlateClipState> clipStack_;
        std::uint64_t nextSerial_ = 1;
    };

    class AceSlateElementBatcher
    {
    public:
        void Build(AceSlateWindowElementList& list);
        const AceSlateFrameStats& Stats() const { return stats_; }
        std::string LastBuildDiagnostics() const { return diagnostics_; }

    private:
        AceSlateFrameStats stats_{};
        std::string diagnostics_;
    };
}
