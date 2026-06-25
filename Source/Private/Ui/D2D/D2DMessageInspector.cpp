#include "ArhqenCognitionEngine/Ui/D2D/D2DMessageInspector.h"

#include <utility>

namespace am::ui
{
    am::core::AceUiInspectorRecord D2DMessageInspector::makeRecord(const ChatMessage& message)
    {
        am::core::AceUiInspectorRecord record;
        record.selection = {
            am::core::AceUiItemKind::Message,
            message.id,
            message.author,
            L"message_list"
        };

        record.title = L"Message #" + std::to_wstring(message.id);
        record.subtitle = message.author + L" • " + kindName(message.kind);
        record.body = message.text;

        record.properties.push_back({L"id", std::to_wstring(message.id), 0});
        record.properties.push_back({L"author", message.author, 1});
        record.properties.push_back({L"kind", kindName(message.kind), 2});
        record.properties.push_back({L"timestamp", message.timestamp.empty() ? L"none" : message.timestamp, 0});
        record.properties.push_back({L"from user", message.fromUser ? L"true" : L"false", 1});
        record.properties.push_back({L"system", message.system ? L"true" : L"false", 2});
        record.properties.push_back({L"metadata", message.metadata.empty() ? L"none" : message.metadata, 0});
        record.properties.push_back({L"length", std::to_wstring(message.text.size()), 1});

        if (!message.metadata.empty())
        {
            am::core::AceUiListItem metadataItem;
            metadataItem.id = message.id;
            metadataItem.title = L"Metadata";
            metadataItem.subtitle = message.metadata;
            metadataItem.detail = L"local message metadata";
            metadataItem.accentIndex = 2;
            metadataItem.kind = am::core::AceUiItemKind::Message;
            record.relatedItems.push_back(std::move(metadataItem));
        }

        return record;
    }

    std::wstring D2DMessageInspector::kindName(ChatMessageKind kind)
    {
        switch (kind)
        {
        case ChatMessageKind::Assistant:
            return L"Assistant";
        case ChatMessageKind::User:
            return L"User";
        case ChatMessageKind::System:
            return L"System";
        case ChatMessageKind::Tool:
            return L"Tool";
        case ChatMessageKind::Warning:
            return L"Warning";
        default:
            return L"Unknown";
        }
    }
}
