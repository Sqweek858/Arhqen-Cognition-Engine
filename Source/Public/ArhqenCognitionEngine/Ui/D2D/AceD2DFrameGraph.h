#pragma once

#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"

#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace am::ui
{
    enum class AceD2DFrameGraphNodeKind : std::uint8_t
    {
        Unknown,
        SwapchainBackbuffer,
        D2DTargetBitmap,
        SceneViewportTexture,
        SharedIntermediateTexture,
        SlateElementBuffer,
        OverlayLayer,
        Present
    };

    enum class AceD2DFrameGraphAccess : std::uint8_t
    {
        None,
        Read,
        Write,
        ReadWrite,
        Present
    };

    struct AceD2DFrameGraphNode
    {
        std::uint64_t id = 0;
        AceD2DFrameGraphNodeKind kind = AceD2DFrameGraphNodeKind::Unknown;
        AceD2DFrameGraphAccess access = AceD2DFrameGraphAccess::None;
        UiRect rect{};
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        std::uint64_t resourceEpoch = 0;
        std::uint64_t producerFrame = 0;
        std::uint64_t consumerFrame = 0;
        bool valid = false;
        bool external = false;
        std::string name;
        std::string diagnostics;
    };

    struct AceD2DFrameGraphEdge
    {
        std::uint64_t from = 0;
        std::uint64_t to = 0;
        AceD2DFrameGraphAccess requiredAccess = AceD2DFrameGraphAccess::None;
        bool barrierRequired = false;
        bool fullFrameDependency = true;
        std::string reason;
    };

    struct AceD2DFrameGraphStats
    {
        std::uint64_t frames = 0;
        std::uint64_t nodes = 0;
        std::uint64_t edges = 0;
        std::uint64_t invalidNodes = 0;
        std::uint64_t fullFrameTransactions = 0;
        std::uint64_t partialTransactionsRejected = 0;
        std::uint64_t lastGoodViewportReuses = 0;
        std::uint64_t resizeEpochs = 0;
        std::uint64_t presentDependencies = 0;
        std::string diagnostics;
        std::string Summary() const;
    };

    class AceD2DFrameGraph
    {
    public:
        void BeginFrame(std::uint64_t frameNumber, std::uint32_t width, std::uint32_t height, bool fullFrame);
        std::uint64_t AddNode(AceD2DFrameGraphNode node);
        void AddEdge(std::uint64_t from, std::uint64_t to, AceD2DFrameGraphAccess access, std::string reason);
        std::optional<AceD2DFrameGraphNode> FindNode(AceD2DFrameGraphNodeKind kind) const;
        bool Validate(std::string* error = nullptr);
        void MarkResize(std::uint32_t width, std::uint32_t height);
        void MarkLastGoodViewportReuse();
        const AceD2DFrameGraphStats& Stats() const { return stats_; }
        const std::vector<AceD2DFrameGraphNode>& Nodes() const { return nodes_; }
        const std::vector<AceD2DFrameGraphEdge>& Edges() const { return edges_; }
        std::string Diagnostics() const;
        void Reset();

    private:
        std::uint64_t nextNodeId_ = 1;
        std::uint64_t frameNumber_ = 0;
        std::uint32_t width_ = 0;
        std::uint32_t height_ = 0;
        bool fullFrame_ = true;
        std::vector<AceD2DFrameGraphNode> nodes_;
        std::vector<AceD2DFrameGraphEdge> edges_;
        AceD2DFrameGraphStats stats_{};
    };

    const char* AceD2DFrameGraphNodeKindName(AceD2DFrameGraphNodeKind kind);
    const char* AceD2DFrameGraphAccessName(AceD2DFrameGraphAccess access);
}
