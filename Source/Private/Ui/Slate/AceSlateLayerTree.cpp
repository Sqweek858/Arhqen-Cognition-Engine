#include "ArhqenCognitionEngine/Ui/Slate/AceSlateLayerTree.h"

#include <algorithm>
#include <sstream>

namespace am::ui::slate
{
    void AceSlateLayerTree::Reset()
    {
        layers_.clear();
        stats_ = {};
    }

    void AceSlateLayerTree::Build(const AceSlateWindowElementList& list)
    {
        layers_.clear();
        std::map<std::uint32_t, AceSlateLayerNode> layerMap;
        for (const auto& element : list.Elements())
        {
            auto& node = layerMap[static_cast<std::uint32_t>(std::max(0, element.layer))];
            if (node.elementCount == 0)
            {
                node.layer = static_cast<std::uint32_t>(std::max(0, element.layer));
                node.bounds = element.geometry.rect;
                node.clip = element.clip.rect;
                node.debugName = "layer_" + std::to_string(element.layer);
            }
            AccumulateElement(node, element);
        }
        for (auto& pair : layerMap)
        {
            layers_.push_back(std::move(pair.second));
        }
        ++stats_.builds;
        stats_.layers = layers_.size();
        stats_.elements = list.Elements().size();
        stats_.viewportLayers = 0;
        stats_.opaqueLayers = 0;
        for (const auto& node : layers_)
        {
            if (node.hasViewport)
            {
                ++stats_.viewportLayers;
            }
            if (node.hasOpaqueCoverage)
            {
                ++stats_.opaqueLayers;
            }
        }
        std::string failure;
        if (!ValidateStrictlyAscending(&failure))
        {
            ++stats_.orderViolations;
            stats_.lastDiagnostics = failure;
        }
        else
        {
            stats_.lastDiagnostics = "layer_tree_ok";
        }
    }

    const AceSlateLayerNode* AceSlateLayerTree::FindLayer(std::uint32_t layer) const
    {
        const auto it = std::find_if(layers_.begin(), layers_.end(), [&](const AceSlateLayerNode& node)
        {
            return node.layer == layer;
        });
        return it == layers_.end() ? nullptr : &(*it);
    }

    bool AceSlateLayerTree::ValidateStrictlyAscending(std::string* failure) const
    {
        if (layers_.empty())
        {
            return true;
        }
        std::uint32_t previous = layers_.front().layer;
        for (std::size_t i = 1; i < layers_.size(); ++i)
        {
            if (layers_[i].layer <= previous)
            {
                if (failure)
                {
                    std::ostringstream oss;
                    oss << "layer_tree_order_failed;prev=" << previous << ";current=" << layers_[i].layer;
                    *failure = oss.str();
                }
                return false;
            }
            previous = layers_[i].layer;
        }
        return true;
    }

    UiRect AceSlateLayerTree::CombinedBounds() const
    {
        UiRect out{};
        bool first = true;
        for (const auto& node : layers_)
        {
            if (first)
            {
                out = node.bounds;
                first = false;
            }
            else
            {
                out = Union(out, node.bounds);
            }
        }
        return out;
    }

    void AceSlateLayerTree::AccumulateElement(AceSlateLayerNode& node, const AceSlateElement& element)
    {
        node.bounds = Union(node.bounds, element.geometry.rect);
        node.clip = Union(node.clip, element.clip.rect);
        ++node.elementCount;
        switch (element.type)
        {
        case AceSlateElementType::Viewport:
            ++node.viewportCount;
            node.hasViewport = true;
            break;
        case AceSlateElementType::Text:
            ++node.textCount;
            break;
        case AceSlateElementType::Box:
        case AceSlateElementType::RoundedBox:
            ++node.boxCount;
            break;
        default:
            break;
        }
        if (HasEffect(element.effects, AceSlateDrawEffect::ForceOpaque) ||
            HasEffect(element.effects, AceSlateDrawEffect::IgnoreTextureAlpha))
        {
            node.hasOpaqueCoverage = true;
        }
    }

    UiRect AceSlateLayerTree::Union(UiRect a, UiRect b) const
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

    std::string AceSlateLayerTree::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "builds=" << stats_.builds
            << ";layers=" << stats_.layers
            << ";elements=" << stats_.elements
            << ";viewport_layers=" << stats_.viewportLayers
            << ";opaque_layers=" << stats_.opaqueLayers
            << ";order_violations=" << stats_.orderViolations
            << ";last=" << stats_.lastDiagnostics;
        for (const auto& node : layers_)
        {
            oss << ";L" << node.layer << '=' << node.elementCount
                << "/vp" << node.viewportCount
                << "/text" << node.textCount
                << "/box" << node.boxCount;
        }
        return oss.str();
    }

    std::wstring AceSlateLayerTree::WideDiagnostics() const
    {
        const std::string text = Diagnostics();
        return std::wstring(text.begin(), text.end());
    }
}
