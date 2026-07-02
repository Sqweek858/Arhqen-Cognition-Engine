#include "ArhqenCognitionEngine/Editor/Workspace/AceEditorWorkspaceLayout.h"

#include "ArhqenCognitionEngine/Core/Identity/AceGuid.h"
#include "ArhqenCognitionEngine/Core/Serialization/AceArchive.h"
#include "ArhqenCognitionEngine/Core/Serialization/AceAtomicFile.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>
#include <utility>

namespace am::editor
{
    namespace
    {
        using am::core::Guid;
        using am::core::serialization::ArchiveReader;
        using am::core::serialization::ArchiveWriter;
        using am::core::serialization::AtomicFile;

        constexpr std::uint32_t kLayoutVersion = 1;
        constexpr std::size_t kMaximumFileBytes = 1024 * 1024;
        constexpr std::size_t kMaximumDepth = 32;
        constexpr std::size_t kMaximumNodes = 256;
        constexpr std::size_t kMaximumTabs = 512;
        constexpr double kMinimumCoefficient = 0.02;
        constexpr double kCoefficientTolerance = 0.000001;
        constexpr Guid kLayoutSchema = Guid::fromBytes({
            0x9a, 0xb7, 0xf3, 0x45, 0xd1, 0x38, 0x48, 0x85,
            0xa5, 0x81, 0x4d, 0x50, 0x62, 0x58, 0x66, 0x13});

        void fail(std::string* error, std::string message)
        {
            if (error)
            {
                *error = std::move(message);
            }
        }

        bool validIdentifier(std::string_view id)
        {
            if (id.empty() || id.size() > 128)
            {
                return false;
            }
            return std::all_of(id.begin(), id.end(), [](unsigned char value)
            {
                return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
                    (value >= '0' && value <= '9') || value == '.' || value == '_' || value == '-';
            });
        }

        bool validUtf8(std::string_view text)
        {
            std::size_t index = 0;
            while (index < text.size())
            {
                const auto first = static_cast<unsigned char>(text[index]);
                if (first < 0x80U) { ++index; continue; }
                std::size_t continuationCount = 0;
                std::uint32_t codePoint = 0;
                if ((first & 0xe0U) == 0xc0U) { continuationCount = 1; codePoint = first & 0x1fU; }
                else if ((first & 0xf0U) == 0xe0U) { continuationCount = 2; codePoint = first & 0x0fU; }
                else if ((first & 0xf8U) == 0xf0U) { continuationCount = 3; codePoint = first & 0x07U; }
                else return false;
                if (index + continuationCount >= text.size()) return false;
                for (std::size_t offset = 1; offset <= continuationCount; ++offset)
                {
                    const auto next = static_cast<unsigned char>(text[index + offset]);
                    if ((next & 0xc0U) != 0x80U) return false;
                    codePoint = (codePoint << 6) | (next & 0x3fU);
                }
                if ((continuationCount == 1 && codePoint < 0x80U) ||
                    (continuationCount == 2 && codePoint < 0x800U) ||
                    (continuationCount == 3 && codePoint < 0x10000U) ||
                    codePoint > 0x10ffffU || (codePoint >= 0xd800U && codePoint <= 0xdfffU)) return false;
                index += continuationCount + 1;
            }
            return true;
        }

        EditorDockNode makeStack(std::string id, double coefficient, EditorDockTab tab)
        {
            EditorDockNode node;
            node.id = std::move(id);
            node.kind = DockNodeKind::Stack;
            node.sizeCoefficient = coefficient;
            node.activeTab = tab.visible ? tab.id : std::string{};
            node.tabs.push_back(std::move(tab));
            return node;
        }

        EditorDockNode makeSplit(std::string id, DockOrientation orientation, double coefficient, std::vector<EditorDockNode> children)
        {
            EditorDockNode node;
            node.id = std::move(id);
            node.kind = DockNodeKind::Split;
            node.orientation = orientation;
            node.sizeCoefficient = coefficient;
            node.children = std::move(children);
            return node;
        }

        EditorDockNode* findNodeRecursive(EditorDockNode& node, std::string_view id) noexcept
        {
            if (node.id == id)
            {
                return &node;
            }
            for (auto& child : node.children)
            {
                if (auto* found = findNodeRecursive(child, id))
                {
                    return found;
                }
            }
            return nullptr;
        }

        const EditorDockNode* findNodeRecursive(const EditorDockNode& node, std::string_view id) noexcept
        {
            if (node.id == id)
            {
                return &node;
            }
            for (const auto& child : node.children)
            {
                if (const auto* found = findNodeRecursive(child, id))
                {
                    return found;
                }
            }
            return nullptr;
        }

        EditorDockTab* findTabRecursive(EditorDockNode& node, std::string_view id) noexcept
        {
            for (auto& tab : node.tabs)
            {
                if (tab.id == id)
                {
                    return &tab;
                }
            }
            for (auto& child : node.children)
            {
                if (auto* found = findTabRecursive(child, id))
                {
                    return found;
                }
            }
            return nullptr;
        }

        const EditorDockTab* findTabRecursive(const EditorDockNode& node, std::string_view id) noexcept
        {
            for (const auto& tab : node.tabs)
            {
                if (tab.id == id)
                {
                    return &tab;
                }
            }
            for (const auto& child : node.children)
            {
                if (const auto* found = findTabRecursive(child, id))
                {
                    return found;
                }
            }
            return nullptr;
        }

        EditorDockNode* findOwningStack(EditorDockNode& node, std::string_view tabId) noexcept
        {
            if (node.kind == DockNodeKind::Stack &&
                std::any_of(node.tabs.begin(), node.tabs.end(), [tabId](const EditorDockTab& tab) { return tab.id == tabId; }))
            {
                return &node;
            }
            for (auto& child : node.children)
            {
                if (auto* found = findOwningStack(child, tabId))
                {
                    return found;
                }
            }
            return nullptr;
        }

        bool nodeVisible(const EditorDockNode& node) noexcept
        {
            if (node.kind == DockNodeKind::Stack)
            {
                return std::any_of(node.tabs.begin(), node.tabs.end(), [](const EditorDockTab& tab) { return tab.visible; });
            }
            return std::any_of(node.children.begin(), node.children.end(), [](const EditorDockNode& child) { return nodeVisible(child); });
        }

        bool normalizeNode(EditorDockNode& node, std::size_t depth, std::string* error)
        {
            if (depth > kMaximumDepth)
            {
                fail(error, "Workspace layout exceeds maximum nesting depth");
                return false;
            }
            if (!std::isfinite(node.sizeCoefficient) || node.sizeCoefficient <= 0.0)
            {
                node.sizeCoefficient = 1.0;
            }
            if (node.kind == DockNodeKind::Stack)
            {
                const auto active = std::find_if(node.tabs.begin(), node.tabs.end(), [&node](const EditorDockTab& tab)
                {
                    return tab.id == node.activeTab && tab.visible;
                });
                if (active == node.tabs.end())
                {
                    const auto firstVisible = std::find_if(node.tabs.begin(), node.tabs.end(), [](const EditorDockTab& tab) { return tab.visible; });
                    node.activeTab = firstVisible == node.tabs.end() ? std::string{} : firstVisible->id;
                }
                return true;
            }

            double total = 0.0;
            for (auto& child : node.children)
            {
                if (!normalizeNode(child, depth + 1, error))
                {
                    return false;
                }
                child.sizeCoefficient = std::max(kMinimumCoefficient, child.sizeCoefficient);
                total += child.sizeCoefficient;
            }
            if (!std::isfinite(total) || total <= 0.0)
            {
                fail(error, "Workspace split has invalid size coefficients");
                return false;
            }
            for (auto& child : node.children)
            {
                child.sizeCoefficient /= total;
            }
            return true;
        }

        struct ValidationState
        {
            std::unordered_set<std::string> nodeIds;
            std::unordered_set<std::string> tabIds;
            std::size_t nodes = 0;
            std::size_t tabs = 0;
        };

        bool validateNode(const EditorDockNode& node, std::size_t depth, ValidationState& state, std::string* error)
        {
            if (depth > kMaximumDepth || ++state.nodes > kMaximumNodes)
            {
                fail(error, "Workspace layout exceeds structural limits");
                return false;
            }
            if (!validIdentifier(node.id) || !state.nodeIds.insert(node.id).second)
            {
                fail(error, "Workspace layout contains an invalid or duplicate node id");
                return false;
            }
            if (!std::isfinite(node.sizeCoefficient) || node.sizeCoefficient <= 0.0)
            {
                fail(error, "Workspace node has an invalid size coefficient");
                return false;
            }

            if (node.kind == DockNodeKind::Stack)
            {
                if (!node.children.empty() || node.tabs.empty())
                {
                    fail(error, "Workspace stack must contain tabs and no child nodes");
                    return false;
                }
                bool anyVisible = false;
                bool activeFound = false;
                for (const auto& tab : node.tabs)
                {
                    if (++state.tabs > kMaximumTabs || !validIdentifier(tab.id) || tab.label.empty() || tab.label.size() > 512 || !validUtf8(tab.label) ||
                        !state.tabIds.insert(tab.id).second)
                    {
                        fail(error, "Workspace layout contains an invalid or duplicate tab");
                        return false;
                    }
                    anyVisible = anyVisible || tab.visible;
                    activeFound = activeFound || (tab.id == node.activeTab && tab.visible);
                }
                if ((anyVisible && !activeFound) || (!anyVisible && !node.activeTab.empty()))
                {
                    fail(error, "Workspace stack active tab is missing or hidden");
                    return false;
                }
                return true;
            }

            if (node.kind != DockNodeKind::Split || !node.tabs.empty() || !node.activeTab.empty() || node.children.size() < 2)
            {
                fail(error, "Workspace split must contain at least two child nodes and no tabs");
                return false;
            }
            double total = 0.0;
            for (const auto& child : node.children)
            {
                total += child.sizeCoefficient;
                if (!validateNode(child, depth + 1, state, error))
                {
                    return false;
                }
            }
            if (std::abs(total - 1.0) > kCoefficientTolerance)
            {
                fail(error, "Workspace split coefficients are not normalized");
                return false;
            }
            return true;
        }

        bool writeNode(ArchiveWriter& writer, const EditorDockNode& node)
        {
            writer.writeU8(static_cast<std::uint8_t>(node.kind));
            if (!writer.writeString(node.id)) return false;
            writer.writeDouble(node.sizeCoefficient);
            if (node.kind == DockNodeKind::Split)
            {
                writer.writeU8(static_cast<std::uint8_t>(node.orientation));
                writer.writeU32(static_cast<std::uint32_t>(node.children.size()));
                for (const auto& child : node.children) if (!writeNode(writer, child)) return false;
                return true;
            }
            if (!writer.writeString(node.activeTab)) return false;
            writer.writeU32(static_cast<std::uint32_t>(node.tabs.size()));
            for (const auto& tab : node.tabs)
            {
                if (!writer.writeString(tab.id) || !writer.writeString(tab.label)) return false;
                writer.writeBool(tab.visible);
                writer.writeBool(tab.closeable);
            }
            return true;
        }

        bool readNode(ArchiveReader& reader, EditorDockNode& node, std::size_t depth, std::size_t& nodeCount, std::size_t& tabCount)
        {
            if (depth > kMaximumDepth || ++nodeCount > kMaximumNodes) return false;
            std::uint8_t kind = 0;
            if (!reader.readU8(kind) || kind > static_cast<std::uint8_t>(DockNodeKind::Stack) ||
                !reader.readString(node.id, 128) || !reader.readDouble(node.sizeCoefficient)) return false;
            node.kind = static_cast<DockNodeKind>(kind);
            if (node.kind == DockNodeKind::Split)
            {
                std::uint8_t orientation = 0;
                std::uint32_t count = 0;
                if (!reader.readU8(orientation) || orientation > static_cast<std::uint8_t>(DockOrientation::Vertical) ||
                    !reader.readU32(count) || count < 2 || count > kMaximumNodes - nodeCount) return false;
                node.orientation = static_cast<DockOrientation>(orientation);
                node.children.resize(count);
                for (auto& child : node.children) if (!readNode(reader, child, depth + 1, nodeCount, tabCount)) return false;
                return true;
            }
            std::uint32_t count = 0;
            if (!reader.readString(node.activeTab, 128) || !reader.readU32(count) || count == 0 || count > kMaximumTabs - tabCount) return false;
            tabCount += count;
            node.tabs.resize(count);
            for (auto& tab : node.tabs)
            {
                if (!reader.readString(tab.id, 128) || !reader.readString(tab.label, 512) ||
                    !reader.readBool(tab.visible) || !reader.readBool(tab.closeable)) return false;
            }
            return true;
        }
    }

    EditorWorkspaceLayout EditorWorkspaceLayout::createDefault()
    {
        EditorWorkspaceLayout layout;
        auto viewport = makeStack("stack.viewport", 0.72, {"tab.viewport", "Viewport", true, false});
        auto outliner = makeStack("stack.outliner", 0.52, {"tab.outliner", "Hierarchy / World Outliner", true, false});
        auto details = makeStack("stack.details", 0.48, {"tab.details", "Details", true, false});
        auto right = makeSplit("split.right", DockOrientation::Vertical, 0.28, {std::move(outliner), std::move(details)});
        auto main = makeSplit("split.main", DockOrientation::Horizontal, 0.76, {std::move(viewport), std::move(right)});
        auto content = makeStack("stack.content", 0.24, {"tab.content", "Content Browser", false, false});
        layout.root_ = makeSplit("split.root", DockOrientation::Vertical, 1.0, {std::move(main), std::move(content)});
        std::string ignored;
        layout.normalizeAndValidate(&ignored);
        return layout;
    }

    const EditorDockNode& EditorWorkspaceLayout::root() const noexcept { return root_; }
    EditorDockNode& EditorWorkspaceLayout::root() noexcept { return root_; }
    const EditorDockNode* EditorWorkspaceLayout::findNode(std::string_view nodeId) const noexcept { return findNodeRecursive(root_, nodeId); }
    EditorDockNode* EditorWorkspaceLayout::findNode(std::string_view nodeId) noexcept { return findNodeRecursive(root_, nodeId); }
    const EditorDockTab* EditorWorkspaceLayout::findTab(std::string_view tabId) const noexcept { return findTabRecursive(root_, tabId); }

    bool EditorWorkspaceLayout::isNodeVisible(std::string_view nodeId) const noexcept
    {
        const auto* node = findNode(nodeId);
        return node && nodeVisible(*node);
    }

    bool EditorWorkspaceLayout::setTabVisible(std::string_view tabId, bool visible, std::string* error)
    {
        auto* stack = findOwningStack(root_, tabId);
        auto* tab = findTabRecursive(root_, tabId);
        if (!stack || !tab)
        {
            fail(error, "Workspace tab does not exist");
            return false;
        }
        tab->visible = visible;
        if (visible)
        {
            stack->activeTab = tab->id;
        }
        else if (stack->activeTab == tab->id)
        {
            const auto next = std::find_if(stack->tabs.begin(), stack->tabs.end(), [](const EditorDockTab& item) { return item.visible; });
            stack->activeTab = next == stack->tabs.end() ? std::string{} : next->id;
        }
        return true;
    }

    bool EditorWorkspaceLayout::activateTab(std::string_view tabId, std::string* error)
    {
        auto* stack = findOwningStack(root_, tabId);
        auto* tab = findTabRecursive(root_, tabId);
        if (!stack || !tab)
        {
            fail(error, "Workspace tab does not exist");
            return false;
        }
        tab->visible = true;
        stack->activeTab = tab->id;
        return true;
    }

    bool EditorWorkspaceLayout::setChildCoefficient(std::string_view splitterId, std::size_t childIndex, double coefficient, std::string* error)
    {
        auto* splitter = findNode(splitterId);
        if (!splitter || splitter->kind != DockNodeKind::Split || childIndex >= splitter->children.size() ||
            !std::isfinite(coefficient) || coefficient <= 0.0)
        {
            fail(error, "Invalid workspace splitter resize request");
            return false;
        }
        const std::size_t count = splitter->children.size();
        const double maximum = 1.0 - (kMinimumCoefficient * static_cast<double>(count - 1));
        splitter->children[childIndex].sizeCoefficient = std::clamp(coefficient, kMinimumCoefficient, maximum);

        double remaining = 1.0 - splitter->children[childIndex].sizeCoefficient;
        std::vector<bool> fixed(count, false);
        fixed[childIndex] = true;
        std::size_t remainingChildren = count - 1;
        while (remainingChildren > 0)
        {
            double weightTotal = 0.0;
            for (std::size_t index = 0; index < count; ++index)
            {
                if (!fixed[index]) weightTotal += splitter->children[index].sizeCoefficient;
            }

            bool fixedAnother = false;
            for (std::size_t index = 0; index < count; ++index)
            {
                if (fixed[index]) continue;
                const double proposed = weightTotal > 0.0
                    ? remaining * (splitter->children[index].sizeCoefficient / weightTotal)
                    : remaining / static_cast<double>(remainingChildren);
                if (proposed < kMinimumCoefficient)
                {
                    splitter->children[index].sizeCoefficient = kMinimumCoefficient;
                    fixed[index] = true;
                    remaining -= kMinimumCoefficient;
                    --remainingChildren;
                    fixedAnother = true;
                }
            }
            if (fixedAnother) continue;

            for (std::size_t index = 0; index < count; ++index)
            {
                if (fixed[index]) continue;
                splitter->children[index].sizeCoefficient = weightTotal > 0.0
                    ? remaining * (splitter->children[index].sizeCoefficient / weightTotal)
                    : remaining / static_cast<double>(remainingChildren);
            }
            break;
        }
        return normalizeNode(*splitter, 0, error);
    }

    bool EditorWorkspaceLayout::resetToDefault()
    {
        *this = createDefault();
        return true;
    }

    bool EditorWorkspaceLayout::normalizeAndValidate(std::string* error)
    {
        if (error) error->clear();
        if (!normalizeNode(root_, 0, error)) return false;
        return validate(error);
    }

    bool EditorWorkspaceLayout::validate(std::string* error) const
    {
        if (error) error->clear();
        ValidationState state;
        return validateNode(root_, 0, state, error);
    }

    bool EditorWorkspaceLayout::save(const std::filesystem::path& path, std::string* error) const
    {
        EditorWorkspaceLayout normalized = *this;
        if (!normalized.normalizeAndValidate(error)) return false;
        ArchiveWriter writer(kLayoutSchema, kLayoutVersion);
        if (!writeNode(writer, normalized.root_))
        {
            fail(error, "Workspace layout contains invalid UTF-8 text");
            return false;
        }
        return AtomicFile::write(path, writer.finish(), error);
    }

    std::optional<EditorWorkspaceLayout> EditorWorkspaceLayout::load(const std::filesystem::path& path, std::string* error)
    {
        std::vector<std::uint8_t> bytes;
        if (!AtomicFile::read(path, bytes, kMaximumFileBytes, error)) return std::nullopt;
        ArchiveReader reader;
        if (!reader.open(bytes, kLayoutSchema, kLayoutVersion, kLayoutVersion, kMaximumFileBytes))
        {
            fail(error, std::string("Workspace layout archive rejected: ") + std::string(am::core::serialization::archiveErrorText(reader.error())));
            return std::nullopt;
        }
        EditorWorkspaceLayout result;
        std::size_t nodes = 0;
        std::size_t tabs = 0;
        if (!readNode(reader, result.root_, 0, nodes, tabs) || !reader.atEnd() || !result.normalizeAndValidate(error))
        {
            if (!error || error->empty()) fail(error, "Workspace layout payload is malformed");
            return std::nullopt;
        }
        return result;
    }
}
