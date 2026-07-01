#include "ArhqenCognitionEngine/Ui/D2D/AceD2DFrameGraph.h"

#include <algorithm>
#include <sstream>

namespace am::ui
{
    std::string AceD2DFrameGraphStats::Summary() const
    {
        std::ostringstream oss;
        oss << "frames=" << frames
            << ";nodes=" << nodes
            << ";edges=" << edges
            << ";invalid=" << invalidNodes
            << ";full=" << fullFrameTransactions
            << ";partial_rejected=" << partialTransactionsRejected
            << ";last_good=" << lastGoodViewportReuses
            << ";resize_epochs=" << resizeEpochs
            << ";present_deps=" << presentDependencies;
        if (!diagnostics.empty())
        {
            oss << ";diag=" << diagnostics;
        }
        return oss.str();
    }

    void AceD2DFrameGraph::BeginFrame(std::uint64_t frameNumber, std::uint32_t width, std::uint32_t height, bool fullFrame)
    {
        frameNumber_ = frameNumber;
        width_ = width;
        height_ = height;
        fullFrame_ = fullFrame;
        nodes_.clear();
        edges_.clear();
        ++stats_.frames;
        if (fullFrame_)
        {
            ++stats_.fullFrameTransactions;
        }
        else
        {
            ++stats_.partialTransactionsRejected;
        }
        AceD2DFrameGraphNode backbuffer{};
        backbuffer.kind = AceD2DFrameGraphNodeKind::SwapchainBackbuffer;
        backbuffer.access = AceD2DFrameGraphAccess::Write;
        backbuffer.width = width_;
        backbuffer.height = height_;
        backbuffer.rect = makeUiRect(0.0f, 0.0f, static_cast<float>(width_), static_cast<float>(height_));
        backbuffer.valid = width_ > 0 && height_ > 0;
        backbuffer.external = true;
        backbuffer.name = "DXGI swapchain backbuffer";
        AddNode(backbuffer);
    }

    std::uint64_t AceD2DFrameGraph::AddNode(AceD2DFrameGraphNode node)
    {
        node.id = nextNodeId_++;
        if (node.resourceEpoch == 0)
        {
            node.resourceEpoch = stats_.resizeEpochs + 1;
        }
        nodes_.push_back(std::move(node));
        stats_.nodes = nodes_.size();
        if (!nodes_.back().valid)
        {
            ++stats_.invalidNodes;
        }
        return nodes_.back().id;
    }

    void AceD2DFrameGraph::AddEdge(std::uint64_t from, std::uint64_t to, AceD2DFrameGraphAccess access, std::string reason)
    {
        AceD2DFrameGraphEdge edge{};
        edge.from = from;
        edge.to = to;
        edge.requiredAccess = access;
        edge.barrierRequired = access == AceD2DFrameGraphAccess::ReadWrite || access == AceD2DFrameGraphAccess::Present;
        edge.fullFrameDependency = fullFrame_;
        edge.reason = std::move(reason);
        edges_.push_back(std::move(edge));
        stats_.edges = edges_.size();
        if (access == AceD2DFrameGraphAccess::Present)
        {
            ++stats_.presentDependencies;
        }
    }

    std::optional<AceD2DFrameGraphNode> AceD2DFrameGraph::FindNode(AceD2DFrameGraphNodeKind kind) const
    {
        const auto it = std::find_if(nodes_.begin(), nodes_.end(), [&](const AceD2DFrameGraphNode& node) { return node.kind == kind; });
        if (it == nodes_.end())
        {
            return std::nullopt;
        }
        return *it;
    }

    bool AceD2DFrameGraph::Validate(std::string* error)
    {
        if (width_ == 0 || height_ == 0)
        {
            if (error) { *error = "FrameGraph output extent is zero."; }
            return false;
        }
        if (!fullFrame_)
        {
            if (error) { *error = "FrameGraph rejected partial repaint for flip-model D2D DeviceContext."; }
            return false;
        }
        for (const auto& node : nodes_)
        {
            if (!node.valid)
            {
                if (error) { *error = "FrameGraph node invalid: " + node.name; }
                return false;
            }
        }
        for (const auto& edge : edges_)
        {
            const bool fromOk = std::any_of(nodes_.begin(), nodes_.end(), [&](const auto& n) { return n.id == edge.from; });
            const bool toOk = std::any_of(nodes_.begin(), nodes_.end(), [&](const auto& n) { return n.id == edge.to; });
            if (!fromOk || !toOk)
            {
                if (error) { *error = "FrameGraph edge references a missing node: " + edge.reason; }
                return false;
            }
        }
        return true;
    }

    void AceD2DFrameGraph::MarkResize(std::uint32_t width, std::uint32_t height)
    {
        width_ = width;
        height_ = height;
        ++stats_.resizeEpochs;
    }

    void AceD2DFrameGraph::MarkLastGoodViewportReuse()
    {
        ++stats_.lastGoodViewportReuses;
    }

    std::string AceD2DFrameGraph::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "frame=" << frameNumber_
            << ";extent=" << width_ << "x" << height_
            << ";full=" << (fullFrame_ ? "true" : "false")
            << ";" << stats_.Summary();
        for (const auto& node : nodes_)
        {
            oss << ";node" << node.id << "=" << AceD2DFrameGraphNodeKindName(node.kind) << ":" << node.name;
        }
        return oss.str();
    }

    void AceD2DFrameGraph::Reset()
    {
        nextNodeId_ = 1;
        frameNumber_ = 0;
        width_ = 0;
        height_ = 0;
        fullFrame_ = true;
        nodes_.clear();
        edges_.clear();
        stats_ = {};
    }

    const char* AceD2DFrameGraphNodeKindName(AceD2DFrameGraphNodeKind kind)
    {
        switch (kind)
        {
        case AceD2DFrameGraphNodeKind::SwapchainBackbuffer: return "SwapchainBackbuffer";
        case AceD2DFrameGraphNodeKind::D2DTargetBitmap: return "D2DTargetBitmap";
        case AceD2DFrameGraphNodeKind::SceneViewportTexture: return "SceneViewportTexture";
        case AceD2DFrameGraphNodeKind::SharedIntermediateTexture: return "SharedIntermediateTexture";
        case AceD2DFrameGraphNodeKind::SlateElementBuffer: return "SlateElementBuffer";
        case AceD2DFrameGraphNodeKind::OverlayLayer: return "OverlayLayer";
        case AceD2DFrameGraphNodeKind::Present: return "Present";
        case AceD2DFrameGraphNodeKind::Unknown:
        default: return "Unknown";
        }
    }

    const char* AceD2DFrameGraphAccessName(AceD2DFrameGraphAccess access)
    {
        switch (access)
        {
        case AceD2DFrameGraphAccess::Read: return "Read";
        case AceD2DFrameGraphAccess::Write: return "Write";
        case AceD2DFrameGraphAccess::ReadWrite: return "ReadWrite";
        case AceD2DFrameGraphAccess::Present: return "Present";
        case AceD2DFrameGraphAccess::None:
        default: return "None";
        }
    }
}
