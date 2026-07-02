#include "ArhqenCognitionEngine/Editor/Commands/AceCommandRegistry.h"

#include <algorithm>
#include <cwctype>
#include <map>
#include <utility>

namespace am::editor::commands
{
    namespace
    {
        std::wstring lower(std::wstring_view value)
        { std::wstring out(value); std::transform(out.begin(),out.end(),out.begin(),[](wchar_t c){return static_cast<wchar_t>(std::towlower(c));}); return out; }
        bool contains(std::wstring_view text, std::wstring_view query) { return lower(text).find(lower(query)) != std::wstring::npos; }
        void fail(std::string* error, std::string value) { if(error) *error=std::move(value); }
        struct ChordLess { bool operator()(const KeyChord& a,const KeyChord& b) const noexcept { return a.key<b.key || (a.key==b.key && static_cast<unsigned>(a.modifiers)<static_cast<unsigned>(b.modifiers)); } };
    }

    bool CommandRegistry::registerCommand(CommandDescriptor descriptor, std::string* error)
    {
        if(error) error->clear();
        if(!validId(descriptor.id)){fail(error,"Invalid command id");return false;}
        if(descriptor.label.empty()||!descriptor.execute){fail(error,"Command requires a label and execute callback");return false;}
        if(descriptor.context.empty()) descriptor.context="Global";
        if(commands_.contains(descriptor.id)){fail(error,"Command id is already registered");return false;}
        const auto chord=descriptor.defaultChord;
        const std::string id=descriptor.id;
        commands_.emplace(id,Entry{std::move(descriptor),chord}); return true;
    }

    bool CommandRegistry::unregisterCommand(std::string_view id) { return commands_.erase(std::string(id))!=0; }
    bool CommandRegistry::setChord(std::string_view id,std::optional<KeyChord> chord,std::string* error)
    {
        if(error) error->clear(); auto it=commands_.find(std::string(id)); if(it==commands_.end()){fail(error,"Unknown command");return false;}
        if(chord && !chord->valid()){fail(error,"Shortcut key is invalid");return false;} it->second.chord=chord; return true;
    }
    bool CommandRegistry::execute(std::string_view id) const
    {
        auto it=commands_.find(std::string(id)); if(it==commands_.end()) return false;
        if(it->second.descriptor.canExecute && !it->second.descriptor.canExecute()) return false;
        it->second.descriptor.execute(); return true;
    }
    std::optional<CommandView> CommandRegistry::find(std::string_view id) const
    { auto it=commands_.find(std::string(id)); return it==commands_.end()?std::nullopt:std::optional<CommandView>{view(it->second)}; }
    std::optional<CommandView> CommandRegistry::resolve(KeyChord chord,const std::vector<std::string>& contexts) const
    {
        auto resolveContext=[&](std::string_view context,const Entry*& match)->int{
            int count=0;match=nullptr;for(const auto& [id,entry]:commands_)if(entry.descriptor.context==context&&entry.chord==chord){match=&entry;++count;}return count;};
        for(const auto& context:contexts){const Entry* match=nullptr;const int count=resolveContext(context,match);if(count>1)return std::nullopt;if(count==1)return view(*match);}
        if(std::find(contexts.begin(),contexts.end(),"Global")==contexts.end()){const Entry* match=nullptr;const int count=resolveContext("Global",match);if(count==1)return view(*match);}
        return std::nullopt;
    }
    std::vector<CommandView> CommandRegistry::search(std::wstring_view query) const
    {
        std::vector<CommandView> out; for(const auto& [id,entry]:commands_){const auto& d=entry.descriptor;if(query.empty()||contains(d.label,query)||contains(d.description,query)||contains(d.category,query))out.push_back(view(entry));}
        std::sort(out.begin(),out.end(),[](const auto& a,const auto& b){return a.label<b.label;}); return out;
    }
    std::vector<ShortcutConflict> CommandRegistry::conflicts() const
    {
        std::map<std::string,std::map<KeyChord,std::vector<std::string>,ChordLess>> groups;
        for(const auto& [id,entry]:commands_) if(entry.chord) groups[entry.descriptor.context][*entry.chord].push_back(id);
        std::vector<ShortcutConflict> out; for(auto& [context,chords]:groups) for(auto& [chord,ids]:chords) if(ids.size()>1){std::sort(ids.begin(),ids.end());out.push_back({context,chord,std::move(ids)});} return out;
    }
    bool CommandRegistry::validId(std::string_view id) noexcept
    { if(id.empty()||id.front()=='.'||id.back()=='.')return false;for(char c:id)if(!(c=='.'||c=='_'||(c>='a'&&c<='z')||(c>='0'&&c<='9')))return false;return true; }
    CommandView CommandRegistry::view(const Entry& e)
    { const auto& d=e.descriptor;return {d.id,d.label,d.description,d.category,d.context,d.type,e.chord,!d.canExecute||d.canExecute(),d.isChecked&&d.isChecked(),d.repeatable}; }
    std::wstring chordText(KeyChord chord)
    {
        std::wstring text; const auto mods=static_cast<unsigned>(chord.modifiers);
        if(mods&static_cast<unsigned>(Modifier::Control))text+=L"Ctrl+"; if(mods&static_cast<unsigned>(Modifier::Shift))text+=L"Shift+";
        if(mods&static_cast<unsigned>(Modifier::Alt))text+=L"Alt+"; if(mods&static_cast<unsigned>(Modifier::Super))text+=L"Win+";
        if(chord.key>='A'&&chord.key<='Z')text.push_back(static_cast<wchar_t>(chord.key)); else text+=L"Key"+std::to_wstring(chord.key); return text;
    }
}
