#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace am::editor::commands
{
    enum class Modifier : std::uint8_t { None=0, Control=1, Shift=2, Alt=4, Super=8 };
    constexpr Modifier operator|(Modifier a, Modifier b) noexcept { return static_cast<Modifier>(static_cast<unsigned>(a)|static_cast<unsigned>(b)); }

    struct KeyChord
    {
        std::uint16_t key = 0;
        Modifier modifiers = Modifier::None;
        friend bool operator==(const KeyChord&, const KeyChord&) = default;
        [[nodiscard]] bool valid() const noexcept { return key != 0; }
    };

    enum class CommandType { Action, Toggle, Radio };

    struct CommandDescriptor
    {
        std::string id;
        std::wstring label;
        std::wstring description;
        std::wstring category;
        std::string context = "Global";
        CommandType type = CommandType::Action;
        std::optional<KeyChord> defaultChord;
        std::function<void()> execute;
        std::function<bool()> canExecute;
        std::function<bool()> isChecked;
        bool repeatable = false;
    };

    struct CommandView
    {
        std::string id;
        std::wstring label;
        std::wstring description;
        std::wstring category;
        std::string context;
        CommandType type = CommandType::Action;
        std::optional<KeyChord> chord;
        bool enabled = false;
        bool checked = false;
        bool repeatable = false;
    };

    struct ShortcutConflict { std::string context; KeyChord chord; std::vector<std::string> commandIds; };

    class CommandRegistry final
    {
    public:
        bool registerCommand(CommandDescriptor descriptor, std::string* error = nullptr);
        bool unregisterCommand(std::string_view id);
        bool setChord(std::string_view id, std::optional<KeyChord> chord, std::string* error = nullptr);
        bool execute(std::string_view id) const;
        [[nodiscard]] std::optional<CommandView> find(std::string_view id) const;
        [[nodiscard]] std::optional<CommandView> resolve(KeyChord chord,
                                                        const std::vector<std::string>& contexts) const;
        [[nodiscard]] std::vector<CommandView> search(std::wstring_view query) const;
        [[nodiscard]] std::vector<ShortcutConflict> conflicts() const;
        [[nodiscard]] std::size_t size() const noexcept { return commands_.size(); }

    private:
        struct Entry { CommandDescriptor descriptor; std::optional<KeyChord> chord; };
        static bool validId(std::string_view id) noexcept;
        static CommandView view(const Entry& entry);
        std::unordered_map<std::string, Entry> commands_;
    };

    [[nodiscard]] std::wstring chordText(KeyChord chord);
}
