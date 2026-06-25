#pragma once

#include "ArhqenCognitionEngine/Ui/Core/AceUiInvalidationRoot.h"
#include "ArhqenCognitionEngine/Ui/D2D/D2DUiTypes.h"

#include <cstdint>
#include <string>
#include <vector>

namespace am::ui
{
    enum class AceUiLayoutAxis
    {
        None,
        Horizontal,
        Vertical
    };

    struct AceUiLayoutNode
    {
        std::wstring id;
        UiRect desired{};
        UiRect allocated{};
        UiRect content{};
        UiRect clip{};
        UiRect hit{};
        float minWidth = 0.0f;
        float minHeight = 0.0f;
        float weight = 0.0f;
        float padding = 0.0f;
        float gap = 0.0f;
        int layer = 0;
        AceUiLayoutAxis axis = AceUiLayoutAxis::None;
        AceUiDirtyReason dirty = AceUiDirtyReason::None;
        std::vector<std::uint32_t> children;
    };

    struct AceUiLayoutStats
    {
        std::uint64_t measurePasses = 0;
        std::uint64_t arrangePasses = 0;
        std::uint64_t hitTests = 0;
        std::uint64_t nodeCount = 0;
        std::uint64_t overlapWarnings = 0;
    };

    class AceUiRetainedLayoutTree
    {
    public:
        void Clear();
        std::uint32_t AddNode(std::wstring id, UiRect desired, AceUiLayoutAxis axis = AceUiLayoutAxis::None, std::uint32_t parent = kInvalidNode);
        AceUiLayoutNode* Find(const std::wstring& id);
        const AceUiLayoutNode* Find(const std::wstring& id) const;
        AceUiLayoutNode* Node(std::uint32_t index);
        const AceUiLayoutNode* Node(std::uint32_t index) const;
        void Arrange(std::uint32_t root, UiRect rect);
        std::uint32_t HitTest(float x, float y) const;
        bool HasSiblingOverlap(std::uint32_t parent) const;
        AceUiLayoutStats Stats() const;

        static constexpr std::uint32_t kInvalidNode = 0xffffffffu;

    private:
        void ArrangeChildren(AceUiLayoutNode& node);

        std::vector<AceUiLayoutNode> nodes_;
        AceUiLayoutStats stats_{};
    };
}
