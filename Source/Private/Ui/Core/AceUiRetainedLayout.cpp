#include "ArhqenCognitionEngine/Ui/Core/AceUiRetainedLayout.h"

#include <algorithm>

namespace am::ui
{
    void AceUiRetainedLayoutTree::Clear()
    {
        nodes_.clear();
        stats_ = {};
    }

    std::uint32_t AceUiRetainedLayoutTree::AddNode(std::wstring id, UiRect desired, AceUiLayoutAxis axis, std::uint32_t parent)
    {
        AceUiLayoutNode node;
        node.id = std::move(id);
        node.desired = desired;
        node.axis = axis;
        const std::uint32_t index = static_cast<std::uint32_t>(nodes_.size());
        nodes_.push_back(std::move(node));
        if (parent != kInvalidNode && parent < nodes_.size())
        {
            nodes_[parent].children.push_back(index);
        }
        stats_.nodeCount = static_cast<std::uint64_t>(nodes_.size());
        return index;
    }

    AceUiLayoutNode* AceUiRetainedLayoutTree::Find(const std::wstring& id)
    {
        for (auto& node : nodes_)
        {
            if (node.id == id)
            {
                return &node;
            }
        }
        return nullptr;
    }

    const AceUiLayoutNode* AceUiRetainedLayoutTree::Find(const std::wstring& id) const
    {
        for (const auto& node : nodes_)
        {
            if (node.id == id)
            {
                return &node;
            }
        }
        return nullptr;
    }

    AceUiLayoutNode* AceUiRetainedLayoutTree::Node(std::uint32_t index)
    {
        return index < nodes_.size() ? &nodes_[index] : nullptr;
    }

    const AceUiLayoutNode* AceUiRetainedLayoutTree::Node(std::uint32_t index) const
    {
        return index < nodes_.size() ? &nodes_[index] : nullptr;
    }

    void AceUiRetainedLayoutTree::Arrange(std::uint32_t root, UiRect rect)
    {
        if (root >= nodes_.size())
        {
            return;
        }

        ++stats_.measurePasses;
        ++stats_.arrangePasses;
        nodes_[root].allocated = rect;
        nodes_[root].content = rect.inset(nodes_[root].padding);
        nodes_[root].clip = nodes_[root].content;
        nodes_[root].hit = rect;
        ArrangeChildren(nodes_[root]);
    }

    void AceUiRetainedLayoutTree::ArrangeChildren(AceUiLayoutNode& node)
    {
        if (node.children.empty())
        {
            return;
        }

        UiRect content = node.content.empty() ? node.allocated.inset(node.padding) : node.content;
        const float gap = node.gap;

        if (node.axis == AceUiLayoutAxis::Horizontal)
        {
            float fixed = 0.0f;
            float totalWeight = 0.0f;
            for (const auto childIndex : node.children)
            {
                const auto& child = nodes_[childIndex];
                if (child.weight > 0.0f) totalWeight += child.weight;
                else fixed += std::max(child.minWidth, child.desired.width());
            }
            fixed += gap * std::max<int>(0, static_cast<int>(node.children.size()) - 1);
            float cursor = content.left;
            const float free = std::max(0.0f, content.width() - fixed);
            for (const auto childIndex : node.children)
            {
                auto& child = nodes_[childIndex];
                const float width = child.weight > 0.0f && totalWeight > 0.0f ? free * (child.weight / totalWeight) : std::max(child.minWidth, child.desired.width());
                child.allocated = makeUiRect(cursor, content.top, std::min(content.right, cursor + width), content.bottom);
                child.content = child.allocated.inset(child.padding);
                child.clip = child.content;
                child.hit = child.allocated;
                cursor = child.allocated.right + gap;
                ArrangeChildren(child);
            }
        }
        else
        {
            float fixed = 0.0f;
            float totalWeight = 0.0f;
            for (const auto childIndex : node.children)
            {
                const auto& child = nodes_[childIndex];
                if (child.weight > 0.0f) totalWeight += child.weight;
                else fixed += std::max(child.minHeight, child.desired.height());
            }
            fixed += gap * std::max<int>(0, static_cast<int>(node.children.size()) - 1);
            float cursor = content.top;
            const float free = std::max(0.0f, content.height() - fixed);
            for (const auto childIndex : node.children)
            {
                auto& child = nodes_[childIndex];
                const float height = child.weight > 0.0f && totalWeight > 0.0f ? free * (child.weight / totalWeight) : std::max(child.minHeight, child.desired.height());
                child.allocated = makeUiRect(content.left, cursor, content.right, std::min(content.bottom, cursor + height));
                child.content = child.allocated.inset(child.padding);
                child.clip = child.content;
                child.hit = child.allocated;
                cursor = child.allocated.bottom + gap;
                ArrangeChildren(child);
            }
        }
    }

    std::uint32_t AceUiRetainedLayoutTree::HitTest(float x, float y) const
    {
        ++const_cast<AceUiRetainedLayoutTree*>(this)->stats_.hitTests;
        for (auto it = nodes_.rbegin(); it != nodes_.rend(); ++it)
        {
            if (it->hit.contains(x, y))
            {
                return static_cast<std::uint32_t>(std::distance(it, nodes_.rend()) - 1);
            }
        }
        return kInvalidNode;
    }

    bool AceUiRetainedLayoutTree::HasSiblingOverlap(std::uint32_t parent) const
    {
        if (parent >= nodes_.size())
        {
            return false;
        }

        const auto& children = nodes_[parent].children;
        for (std::size_t a = 0; a < children.size(); ++a)
        {
            for (std::size_t b = a + 1; b < children.size(); ++b)
            {
                const UiRect ra = nodes_[children[a]].allocated;
                const UiRect rb = nodes_[children[b]].allocated;
                const bool separated = ra.right <= rb.left || rb.right <= ra.left || ra.bottom <= rb.top || rb.bottom <= ra.top;
                if (!separated)
                {
                    ++const_cast<AceUiRetainedLayoutTree*>(this)->stats_.overlapWarnings;
                    return true;
                }
            }
        }
        return false;
    }

    AceUiLayoutStats AceUiRetainedLayoutTree::Stats() const
    {
        AceUiLayoutStats s = stats_;
        s.nodeCount = static_cast<std::uint64_t>(nodes_.size());
        return s;
    }
}
