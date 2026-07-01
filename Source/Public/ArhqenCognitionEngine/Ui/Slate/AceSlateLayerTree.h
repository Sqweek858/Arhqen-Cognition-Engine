#pragma once

#include "ArhqenCognitionEngine/Ui/Slate/AceSlateElementList.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateFramePrimitives.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace am::ui::slate
{
    struct AceSlateLayerNode
    {
        std::uint32_t layer = 0;
        UiRect bounds{};
        UiRect clip{};
        std::uint64_t elementCount = 0;
        std::uint64_t viewportCount = 0;
        std::uint64_t textCount = 0;
        std::uint64_t boxCount = 0;
        bool hasOpaqueCoverage = false;
        bool hasViewport = false;
        std::string debugName;
    };

    struct AceSlateLayerTreeStats
    {
        std::uint64_t builds = 0;
        std::uint64_t layers = 0;
        std::uint64_t elements = 0;
        std::uint64_t viewportLayers = 0;
        std::uint64_t opaqueLayers = 0;
        std::uint64_t orderViolations = 0;
        std::string lastDiagnostics;
    };

    class AceSlateLayerTree
    {
    public:
        void Reset();
        void Build(const AceSlateWindowElementList& list);
        const std::vector<AceSlateLayerNode>& Layers() const { return layers_; }
        const AceSlateLayerNode* FindLayer(std::uint32_t layer) const;
        bool ValidateStrictlyAscending(std::string* failure) const;
        UiRect CombinedBounds() const;
        const AceSlateLayerTreeStats& Stats() const { return stats_; }
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
    private:
        void AccumulateElement(AceSlateLayerNode& node, const AceSlateElement& element);
        UiRect Union(UiRect a, UiRect b) const;
        std::vector<AceSlateLayerNode> layers_{};
        AceSlateLayerTreeStats stats_{};
    };
}
