#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace am::core
{
    enum class AceUiItemKind
    {
        None,
        Concept,
        Claim,
        Relation,
        Evidence,
        Hypothesis,
        Question,
        Rule,
        GraphNode,
        Message,
        Store,
        Ledger
    };

    struct AceUiSelection
    {
        AceUiItemKind kind = AceUiItemKind::None;
        std::uint64_t id = 0;
        std::wstring label;
        std::wstring source;
    };

    struct AceUiMetric
    {
        std::wstring label;
        std::wstring value;
        int accentIndex = 0;
    };

    struct AceUiListItem
    {
        std::uint64_t id = 0;
        std::wstring title;
        std::wstring subtitle;
        std::wstring detail;
        int accentIndex = 0;
        AceUiItemKind kind = AceUiItemKind::None;
    };

    struct AceUiGraphNode
    {
        std::uint64_t id = 0;
        std::wstring label;
        float x = 0.0f;
        float y = 0.0f;
        int accentIndex = 0;
        AceUiItemKind kind = AceUiItemKind::Concept;
    };

    struct AceUiGraphEdge
    {
        std::uint64_t from = 0;
        std::uint64_t to = 0;
        std::wstring label;
        int accentIndex = 0;
    };

    struct AceUiGraphViewport
    {
        float panX = 0.0f;
        float panY = 0.0f;
        float zoom = 1.0f;
        std::uint64_t hoveredNode = 0;
        std::uint64_t selectedNode = 0;
        bool dragging = false;
    };

    struct AceUiGraphStats
    {
        std::size_t nodeCount = 0;
        std::size_t edgeCount = 0;
        float zoom = 1.0f;
        float panX = 0.0f;
        float panY = 0.0f;
    };

    struct AceUiSnapshot
    {
        std::wstring title = L"Workspace";
        std::wstring subtitle = L"No backend snapshot loaded.";
        std::wstring ledgerSummary;
        std::vector<AceUiMetric> metrics;
        std::vector<AceUiListItem> concepts;
        std::vector<AceUiListItem> hypotheses;
        std::vector<AceUiListItem> questions;
        std::vector<AceUiListItem> rules;
        std::vector<AceUiGraphNode> graphNodes;
        std::vector<AceUiGraphEdge> graphEdges;
    };

    struct AceUiInspectorProperty
    {
        std::wstring name;
        std::wstring value;
        int accentIndex = 0;
    };

    struct AceUiInspectorRecord
    {
        AceUiSelection selection;
        std::wstring title;
        std::wstring subtitle;
        std::wstring body;
        std::vector<AceUiInspectorProperty> properties;
        std::vector<AceUiListItem> relatedItems;
    };

    struct AceSuggestion
    {
        std::wstring insertText;
        std::wstring label;
        std::wstring detail;
        int accentIndex = 0;
    };
}
