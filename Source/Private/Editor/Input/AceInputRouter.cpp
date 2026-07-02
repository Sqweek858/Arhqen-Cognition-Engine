#include "ArhqenCognitionEngine/Editor/Input/AceInputRouter.h"

#include <algorithm>
#include <utility>

namespace am::editor::input
{
    namespace { void fail(std::string* e,std::string v){if(e)*e=std::move(v);} int layerRank(InputLayer l){return static_cast<int>(l);} }

    bool InputRouter::registerTarget(TargetDescriptor descriptor,std::string* error)
    {
        if(error)error->clear();if(descriptor.id.empty()||!descriptor.handle){fail(error,"Target requires id and handler");return false;}
        if(find(descriptor.id)){fail(error,"Target id already registered");return false;}if(descriptor.commandContext.empty())descriptor.commandContext="Global";
        const bool opensModal=descriptor.visible&&descriptor.enabled&&descriptor.layer==InputLayer::Modal;
        if(opensModal&&( !capture_.empty()||!focus_.empty()))
        {
            InputEvent lost;lost.type=EventType::FocusLost;if(auto* target=find(capture_))target->handle(lost);
            if(focus_!=capture_)if(auto* target=find(focus_))target->handle(lost);capture_.clear();focus_.clear();capturedButton_=PointerButton::None;
        }
        targets_.push_back(std::move(descriptor));return true;
    }
    bool InputRouter::unregisterTarget(std::string_view id)
    {
        auto it=std::find_if(targets_.begin(),targets_.end(),[&](const auto& t){return t.id==id;});if(it==targets_.end())return false;
        if(capture_==id){InputEvent lost;lost.type=EventType::FocusLost;it->handle(lost);capture_.clear();capturedButton_=PointerButton::None;}
        if(focus_==id){InputEvent lost;lost.type=EventType::FocusLost;it->handle(lost);focus_.clear();}
        targets_.erase(it);return true;
    }
    bool InputRouter::setTargetState(std::string_view id,bool visible,bool enabled)
    {
        auto* target=find(id);if(!target)return false;target->visible=visible;target->enabled=enabled;
        if((!visible||!enabled)&&(capture_==id||focus_==id)){InputEvent lost;lost.type=EventType::FocusLost;target->handle(lost);}
        if((!visible||!enabled)&&capture_==id){capture_.clear();capturedButton_=PointerButton::None;}
        if((!visible||!enabled)&&focus_==id)focus_.clear();return true;
    }
    void InputRouter::setActiveContexts(std::vector<std::string> contexts){activeContexts_=std::move(contexts);}
    RouteResult InputRouter::route(const InputEvent& event)
    {
        if(pointerEvent(event.type))
        {
            if(!capture_.empty())if(auto* target=find(capture_))return dispatch(*target,event);
            if(auto* target=pointerTarget(event.x,event.y))return dispatch(*target,event);
            return {};
        }
        const bool hasModal=std::any_of(targets_.begin(),targets_.end(),[](const auto& target){return target.visible&&target.enabled&&target.layer==InputLayer::Modal;});
        if(!focus_.empty())if(auto* target=find(focus_);target&&(!hasModal||target->layer==InputLayer::Modal)){auto result=dispatch(*target,event);if(result.handled)return result;}
        if(event.type==EventType::KeyDown&&commands_)
        {
            std::vector<std::string> contexts;
            if(const auto* target=find(focus_);target&&target->commandContext!="Global")contexts.push_back(target->commandContext);
            contexts.insert(contexts.end(),activeContexts_.begin(),activeContexts_.end());
            if(auto command=commands_->resolve(event.chord,contexts);command&&(!event.repeat||command->repeatable)&&commands_->execute(command->id))return {true,{},command->id};
        }
        return {};
    }
    void InputRouter::deactivate()
    {
        InputEvent lost;lost.type=EventType::FocusLost;
        if(auto* target=find(capture_))target->handle(lost);
        if(focus_!=capture_)if(auto* target=find(focus_))target->handle(lost);
        capture_.clear();focus_.clear();capturedButton_=PointerButton::None;
    }
    TargetDescriptor* InputRouter::find(std::string_view id){auto it=std::find_if(targets_.begin(),targets_.end(),[&](auto& t){return t.id==id;});return it==targets_.end()?nullptr:&*it;}
    const TargetDescriptor* InputRouter::find(std::string_view id)const{auto it=std::find_if(targets_.begin(),targets_.end(),[&](const auto& t){return t.id==id;});return it==targets_.end()?nullptr:&*it;}
    TargetDescriptor* InputRouter::pointerTarget(float x,float y)
    {
        const bool hasModal=std::any_of(targets_.begin(),targets_.end(),[](const auto& target){return target.visible&&target.enabled&&target.layer==InputLayer::Modal;});
        TargetDescriptor* best=nullptr;for(auto& target:targets_)if(target.visible&&target.enabled&&(!hasModal||target.layer==InputLayer::Modal)&&target.hitTest&&target.hitTest(x,y)&&(!best||layerRank(target.layer)>layerRank(best->layer)||(target.layer==best->layer&&target.zOrder>=best->zOrder)))best=&target;
        return best;
    }
    RouteResult InputRouter::dispatch(TargetDescriptor& target,const InputEvent& event)
    {
        const InputReply reply=target.handle(event);if(reply.handled&&reply.requestFocus&&target.focusable)focus_=target.id;if(reply.handled&&reply.clearFocus&&focus_==target.id)focus_.clear();
        if(reply.handled&&reply.capturePointer&&event.type==EventType::PointerDown){capture_=target.id;capturedButton_=event.button;}
        if(reply.releasePointer||(event.type==EventType::PointerUp&&capture_==target.id&&event.button==capturedButton_)){capture_.clear();capturedButton_=PointerButton::None;}
        return {reply.handled,target.id,{}};
    }
    bool InputRouter::pointerEvent(EventType type)noexcept{return type==EventType::PointerMove||type==EventType::PointerDown||type==EventType::PointerUp||type==EventType::Wheel;}
}
