#include "ArhqenCognitionEngine/Ui/Slate/AceSlateFrameTopology.h"

#include <algorithm>
#include <sstream>

namespace am::ui::slate
{
    void AceSlateFrameTopology::Reset()
    {
        nodes_.clear();
        stats_ = {};
        nextId_ = 1;
    }

    void AceSlateFrameTopology::Build(const AceSlateWindowElementList& list, const AceSlateLayerTree& layers, UiRect rootBounds)
    {
        nodes_.clear();
        nextId_ = 1;
        AceSlateFrameTopologyNode root{};
        root.id = nextId_++;
        root.parent = 0;
        root.kind = AceSlateFrameTopologyNodeKind::Root;
        root.layer = 0;
        root.bounds = rootBounds;
        root.elementCount = list.Elements().size();
        root.name = "root_window";
        nodes_.push_back(root);

        for (const auto& layer : layers.Layers())
        {
            AceSlateFrameTopologyNode layerNode{};
            layerNode.parent = root.id;
            layerNode.kind = AceSlateFrameTopologyNodeKind::Layer;
            layerNode.layer = layer.layer;
            layerNode.bounds = layer.bounds;
            layerNode.elementCount = layer.elementCount;
            layerNode.opaque = layer.hasOpaqueCoverage;
            layerNode.viewport = layer.hasViewport;
            layerNode.name = layer.debugName;
            const std::uint64_t layerId = AddNode(std::move(layerNode));

            if (layer.hasViewport)
            {
                AceSlateFrameTopologyNode viewportNode{};
                viewportNode.parent = layerId;
                viewportNode.kind = AceSlateFrameTopologyNodeKind::Viewport;
                viewportNode.layer = layer.layer;
                viewportNode.bounds = layer.bounds;
                viewportNode.elementCount = layer.viewportCount;
                viewportNode.opaque = true;
                viewportNode.viewport = true;
                viewportNode.name = "viewport_layer_" + std::to_string(layer.layer);
                AddNode(std::move(viewportNode));
            }
            if (layer.textCount > 0 || layer.boxCount > 0)
            {
                AceSlateFrameTopologyNode overlayNode{};
                overlayNode.parent = layerId;
                overlayNode.kind = layer.hasViewport ? AceSlateFrameTopologyNodeKind::Overlay : AceSlateFrameTopologyNodeKind::Primitive;
                overlayNode.layer = layer.layer;
                overlayNode.bounds = layer.bounds;
                overlayNode.elementCount = layer.textCount + layer.boxCount;
                overlayNode.opaque = layer.hasOpaqueCoverage;
                overlayNode.viewport = false;
                overlayNode.name = "overlay_or_primitives_" + std::to_string(layer.layer);
                AddNode(std::move(overlayNode));
            }
        }

        ++stats_.builds;
        stats_.nodes = nodes_.size();
        stats_.viewportNodes = 0;
        stats_.overlayNodes = 0;
        stats_.opaqueNodes = 0;
        for (const auto& node : nodes_)
        {
            if (node.viewport)
            {
                ++stats_.viewportNodes;
            }
            if (node.kind == AceSlateFrameTopologyNodeKind::Overlay)
            {
                ++stats_.overlayNodes;
            }
            if (node.opaque)
            {
                ++stats_.opaqueNodes;
            }
        }
        std::string failure;
        if (!Validate(&failure))
        {
            ++stats_.invalidTrees;
            stats_.lastDiagnostics = failure;
        }
        else
        {
            stats_.lastDiagnostics = "topology_ok";
        }
    }

    const AceSlateFrameTopologyNode* AceSlateFrameTopology::Root() const
    {
        return nodes_.empty() ? nullptr : &nodes_.front();
    }

    bool AceSlateFrameTopology::Validate(std::string* failure) const
    {
        if (nodes_.empty())
        {
            if (failure)
            {
                *failure = "empty_topology";
            }
            return false;
        }
        if (nodes_.front().kind != AceSlateFrameTopologyNodeKind::Root || nodes_.front().parent != 0)
        {
            if (failure)
            {
                *failure = "invalid_root";
            }
            return false;
        }
        for (std::size_t i = 1; i < nodes_.size(); ++i)
        {
            const auto parent = std::find_if(nodes_.begin(), nodes_.end(), [&](const AceSlateFrameTopologyNode& node)
            {
                return node.id == nodes_[i].parent;
            });
            if (parent == nodes_.end())
            {
                if (failure)
                {
                    std::ostringstream oss;
                    oss << "missing_parent;node=" << nodes_[i].id << ";parent=" << nodes_[i].parent;
                    *failure = oss.str();
                }
                return false;
            }
        }
        return true;
    }

    std::uint64_t AceSlateFrameTopology::AddNode(AceSlateFrameTopologyNode node)
    {
        node.id = nextId_++;
        const std::uint64_t id = node.id;
        nodes_.push_back(std::move(node));
        return id;
    }

    UiRect AceSlateFrameTopology::Union(UiRect a, UiRect b) const
    {
        if (a.empty())
        {
            return b;
        }
        if (b.empty())
        {
            return a;
        }
        return makeUiRect(std::min(a.left, b.left), std::min(a.top, b.top), std::max(a.right, b.right), std::max(a.bottom, b.bottom));
    }

    std::string AceSlateFrameTopology::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "builds=" << stats_.builds
            << ";nodes=" << stats_.nodes
            << ";viewport_nodes=" << stats_.viewportNodes
            << ";overlay_nodes=" << stats_.overlayNodes
            << ";opaque_nodes=" << stats_.opaqueNodes
            << ";invalid=" << stats_.invalidTrees
            << ";last=" << stats_.lastDiagnostics;
        return oss.str();
    }

    std::wstring AceSlateFrameTopology::WideDiagnostics() const
    {
        const std::string text = Diagnostics();
        return std::wstring(text.begin(), text.end());
    }

    const char* AceSlateFrameTopologyNodeKindName(AceSlateFrameTopologyNodeKind kind)
    {
        switch (kind)
        {
        case AceSlateFrameTopologyNodeKind::Root: return "root";
        case AceSlateFrameTopologyNodeKind::Layer: return "layer";
        case AceSlateFrameTopologyNodeKind::Viewport: return "viewport";
        case AceSlateFrameTopologyNodeKind::Overlay: return "overlay";
        case AceSlateFrameTopologyNodeKind::Primitive: return "primitive";
        default: return "unknown";
        }
    }
}
