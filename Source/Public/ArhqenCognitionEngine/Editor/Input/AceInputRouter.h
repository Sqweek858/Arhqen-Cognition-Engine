#pragma once

#include "ArhqenCognitionEngine/Editor/Commands/AceCommandRegistry.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace am::editor::input
{
    enum class EventType { PointerMove, PointerDown, PointerUp, Wheel, KeyDown, KeyUp, TextInput, FocusLost };
    enum class PointerButton { None, Left, Right, Middle, X1, X2 };
    enum class InputLayer { Normal, Overlay, Popup, Modal };

    struct InputEvent
    {
        EventType type = EventType::PointerMove;
        double timestampSeconds = 0.0;
        float x = 0.0f, y = 0.0f;
        float deltaX = 0.0f, deltaY = 0.0f;
        float wheelDelta = 0.0f;
        PointerButton button = PointerButton::None;
        commands::KeyChord chord{};
        bool repeat = false;
        std::wstring text;
    };

    struct InputReply
    {
        bool handled = false;
        bool capturePointer = false;
        bool releasePointer = false;
        bool requestFocus = false;
        bool clearFocus = false;
        static InputReply unhandled() noexcept { return {}; }
        static InputReply handledOnly() noexcept { InputReply value; value.handled=true; return value; }
    };

    struct TargetDescriptor
    {
        std::string id;
        std::string commandContext = "Global";
        int zOrder = 0;
        InputLayer layer = InputLayer::Normal;
        bool enabled = true;
        bool visible = true;
        bool focusable = false;
        std::function<bool(float,float)> hitTest;
        std::function<InputReply(const InputEvent&)> handle;
    };

    struct RouteResult { bool handled=false; std::string targetId; std::string commandId; };

    class InputRouter final
    {
    public:
        explicit InputRouter(commands::CommandRegistry* commands = nullptr) : commands_(commands) {}
        bool registerTarget(TargetDescriptor descriptor, std::string* error = nullptr);
        bool unregisterTarget(std::string_view id);
        bool setTargetState(std::string_view id, bool visible, bool enabled);
        void setActiveContexts(std::vector<std::string> contexts);
        RouteResult route(const InputEvent& event);
        void deactivate();
        [[nodiscard]] std::string_view focusedTarget() const noexcept { return focus_; }
        [[nodiscard]] std::string_view capturedTarget() const noexcept { return capture_; }
        [[nodiscard]] std::size_t targetCount() const noexcept { return targets_.size(); }

    private:
        TargetDescriptor* find(std::string_view id);
        const TargetDescriptor* find(std::string_view id) const;
        TargetDescriptor* pointerTarget(float x,float y);
        RouteResult dispatch(TargetDescriptor& target,const InputEvent& event);
        static bool pointerEvent(EventType type) noexcept;

        commands::CommandRegistry* commands_ = nullptr;
        std::vector<TargetDescriptor> targets_;
        std::vector<std::string> activeContexts_;
        std::string focus_;
        std::string capture_;
        PointerButton capturedButton_ = PointerButton::None;
    };
}
