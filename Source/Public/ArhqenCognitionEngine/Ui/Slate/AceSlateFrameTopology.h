#pragma once

#include "ArhqenCognitionEngine/Ui/Slate/AceSlateElementList.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateLayerTree.h"

#include <cstdint>
#include <string>
#include <vector>

namespace am::ui::slate
{
    enum class AceSlateFrameTopologyNodeKind : std::uint8_t
    {
        Root,
        Layer,
        Viewport,
        Overlay,
        Primitive
    };

    struct AceSlateFrameTopologyNode
    {
        std::uint64_t id = 0;
        std::uint64_t parent = 0;
        AceSlateFrameTopologyNodeKind kind = AceSlateFrameTopologyNodeKind::Primitive;
        std::uint32_t layer = 0;
        UiRect bounds{};
        std::uint64_t elementCount = 0;
        bool opaque = false;
        bool viewport = false;
        std::string name;
    };

    struct AceSlateFrameTopologyStats
    {
        std::uint64_t builds = 0;
        std::uint64_t nodes = 0;
        std::uint64_t viewportNodes = 0;
        std::uint64_t overlayNodes = 0;
        std::uint64_t opaqueNodes = 0;
        std::uint64_t invalidTrees = 0;
        std::string lastDiagnostics;
    };

    class AceSlateFrameTopology
    {
    public:
        void Reset();
        void Build(const AceSlateWindowElementList& list, const AceSlateLayerTree& layers, UiRect rootBounds);
        const std::vector<AceSlateFrameTopologyNode>& Nodes() const { return nodes_; }
        const AceSlateFrameTopologyNode* Root() const;
        bool Validate(std::string* failure) const;
        const AceSlateFrameTopologyStats& Stats() const { return stats_; }
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
    private:
        std::uint64_t AddNode(AceSlateFrameTopologyNode node);
        UiRect Union(UiRect a, UiRect b) const;
        std::vector<AceSlateFrameTopologyNode> nodes_{};
        AceSlateFrameTopologyStats stats_{};
        std::uint64_t nextId_ = 1;
    };

    const char* AceSlateFrameTopologyNodeKindName(AceSlateFrameTopologyNodeKind kind);
}
