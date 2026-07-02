#include "ArhqenCognitionEngine/Editor/Commands/AceCommandRegistry.h"
#include <iostream>
#include <string_view>
namespace{int f=0;void check(bool v,std::string_view n){std::cout<<(v?"PASS|":"FAIL|")<<n<<'\n';if(!v)++f;}}
int main(){using namespace am::editor::commands;CommandRegistry r;int saves=0;bool selected=false;std::string e;
 CommandDescriptor save{"file.save",L"Save",L"Save the current scene",L"File","Global",CommandType::Action,KeyChord{'S',Modifier::Control},[&]{++saves;}};
 check(r.registerCommand(std::move(save),&e),"register_command");check(!r.registerCommand({"file.save",L"Other",L"",L"", "Global",CommandType::Action,std::nullopt,[]{}}),"duplicate_id_rejected");
 check(!r.registerCommand({"Bad ID",L"Bad",L"",L"","Global",CommandType::Action,std::nullopt,[]{}}),"invalid_id_rejected");
 check(r.execute("file.save")&&saves==1,"execute_command");
 CommandDescriptor del{"edit.delete",L"Delete",L"Delete selected objects",L"Edit","Viewport",CommandType::Action,KeyChord{46,Modifier::None},[]{},[&]{return selected;}};
 check(r.registerCommand(std::move(del)),"register_context_command");check(!r.execute("edit.delete"),"disabled_command_not_executed");selected=true;check(r.find("edit.delete")->enabled,"dynamic_enabled_state");
 auto resolved=r.resolve({'S',Modifier::Control},{"Viewport"});check(resolved&&resolved->id=="file.save","global_shortcut_fallback");
 check(r.resolve({46,Modifier::None},{"Viewport"})->id=="edit.delete","active_context_precedence");
 check(r.search(L"current scene").size()==1&&r.search(L"file").size()==1,"search_label_description_category");
 check(r.setChord("edit.delete",KeyChord{'S',Modifier::Control}),"rebind_shortcut");check(r.conflicts().empty(),"same_chord_different_context_not_conflict");
 check(r.registerCommand({"file.save_as",L"Save As",L"",L"File","Global",CommandType::Action,KeyChord{'S',Modifier::Control},[]{}}),"register_conflicting_command");
 auto conflicts=r.conflicts();check(conflicts.size()==1&&conflicts[0].commandIds.size()==2,"same_context_conflict_reported");
 check(!r.resolve({'S',Modifier::Control},{"Global"}),"ambiguous_shortcut_never_executes_arbitrarily");
 check(chordText({'S',Modifier::Control|Modifier::Shift})==L"Ctrl+Shift+S","chord_text_stable");
 check(r.setChord("file.save_as",std::nullopt)&&r.conflicts().empty(),"clear_binding_resolves_conflict");
 check(r.unregisterCommand("file.save_as")&&!r.find("file.save_as"),"unregister_command");
 if(f){std::cout<<"FAIL|ace_command_registry_probe|count="<<f<<'\n';return 1;}std::cout<<"PASS|ace_command_registry_probe\n";return 0;}
