#include "ArhqenCognitionEngine/Editor/Input/AceInputRouter.h"
#include <iostream>
#include <string_view>
namespace{int f=0;void ck(bool v,std::string_view n){std::cout<<(v?"PASS|":"FAIL|")<<n<<'\n';if(!v)++f;}}
int main(){using namespace am::editor;commands::CommandRegistry commands;int saves=0;commands.registerCommand({"file.save",L"Save",L"",L"File","Global",commands::CommandType::Action,commands::KeyChord{'S',commands::Modifier::Control},[&]{++saves;}});input::InputRouter r(&commands);int base=0,top=0,lost=0;
 auto box=[](float x,float y){return x>=0&&x<100&&y>=0&&y<100;};
 r.registerTarget({"base","Viewport",0,input::InputLayer::Normal,true,true,true,box,[&](const input::InputEvent&e){++base;input::InputReply q=input::InputReply::handledOnly();if(e.type==input::EventType::PointerDown){q.capturePointer=true;q.requestFocus=true;}return q;}});
 r.registerTarget({"top","Overlay",10,input::InputLayer::Overlay,true,true,true,box,[&](const input::InputEvent&e){++top;if(e.type==input::EventType::FocusLost)++lost;input::InputReply q=input::InputReply::handledOnly();if(e.type==input::EventType::PointerDown){q.capturePointer=true;q.requestFocus=true;}return q;}});
 input::InputEvent e;e.type=input::EventType::PointerDown;e.x=5;e.y=5;e.button=input::PointerButton::Left;auto rr=r.route(e);ck(rr.targetId=="top"&&top==1&&base==0,"topmost_hit_target_only");ck(r.capturedTarget()=="top"&&r.focusedTarget()=="top","handled_down_sets_capture_and_focus");
 e.type=input::EventType::PointerMove;e.x=500;e.y=500;r.route(e);ck(top==2&&base==0,"capture_routes_outside_bounds");e.type=input::EventType::PointerUp;e.button=input::PointerButton::Left;r.route(e);ck(r.capturedTarget().empty(),"matching_pointer_up_releases_capture");
 e={};e.type=input::EventType::KeyDown;e.chord={'S',commands::Modifier::Control};r.route(e);ck(saves==0,"focused_target_can_consume_key");
 r.setTargetState("top",false,false);auto cmd=r.route(e);ck(cmd.commandId=="file.save"&&saves==1,"unhandled_key_falls_back_to_command_registry");
 int modal=0;r.registerTarget({"modal","Modal",0,input::InputLayer::Modal,true,true,true,[](float x,float y){return x<50&&y<50;},[&](const input::InputEvent&){++modal;return input::InputReply::handledOnly();}});ck(r.focusedTarget().empty(),"opening_modal_clears_underlying_focus");
 e={};e.type=input::EventType::PointerDown;e.x=75;e.y=75;e.button=input::PointerButton::Left;rr=r.route(e);ck(!rr.handled&&base==0,"visible_modal_blocks_lower_hit_targets");e.x=10;e.y=10;rr=r.route(e);ck(rr.targetId=="modal"&&modal==1,"modal_receives_own_hit");
 r.setTargetState("modal",false,false);r.setTargetState("top",true,true);e.x=5;e.y=5;r.route(e);ck(r.capturedTarget()=="top","capture_reestablished");r.deactivate();ck(r.capturedTarget().empty()&&r.focusedTarget().empty()&&lost>=2,"deactivate_clears_capture_focus_and_notifies");
 e.type=input::EventType::PointerDown;r.route(e);ck(r.unregisterTarget("top")&&r.capturedTarget().empty()&&r.focusedTarget().empty(),"unregister_repairs_owned_state");ck(!r.unregisterTarget("missing"),"unknown_unregister_rejected");
 if(f){std::cout<<"FAIL|ace_input_router_probe|count="<<f<<'\n';return 1;}std::cout<<"PASS|ace_input_router_probe\n";return 0;}
