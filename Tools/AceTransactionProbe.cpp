#include "ArhqenCognitionEngine/Editor/Transactions/AceTransaction.h"
#include <iostream>
#include <memory>
#include <string_view>

namespace { int failures=0; void check(bool v,std::string_view n){std::cout<<(v?"PASS|":"FAIL|")<<n<<'\n';if(!v)++failures;} }

int main()
{
    using namespace am::editor::transactions;
    TransactionManager manager(4, 1024);
    int value = 0;
    check(manager.begin("Move object"), "begin_transaction");
    value = 10;
    check(manager.add(std::make_unique<LambdaOperation>([&]{value=0;},[&]{value=10;})), "add_operation");
    check(manager.commit() && manager.canUndo() && value==10, "commit_preserves_applied_state");
    check(manager.nextUndo().name=="Move object" && manager.nextUndo().operationCount==1, "undo_metadata");
    check(manager.undo() && value==0 && manager.canRedo(), "undo_applies_reverse");
    check(manager.redo() && value==10, "redo_applies_forward");

    check(manager.begin("Grouped transform"), "begin_group");
    value=20; manager.add(std::make_unique<LambdaOperation>([&]{value=10;},[&]{value=20;}));
    value=30; manager.add(std::make_unique<LambdaOperation>([&]{value=20;},[&]{value=30;}));
    manager.commit(); manager.undo();
    check(value==10, "group_undo_runs_reverse_order");
    manager.redo(); check(value==30, "group_redo_runs_forward_order");

    check(manager.begin("Cancel drag"), "begin_cancelable");
    value=99; manager.add(std::make_unique<LambdaOperation>([&]{value=30;},[&]{value=99;}));
    check(manager.cancel() && value==30, "cancel_reverts_active_change");
    check(!manager.undoCount() || manager.nextUndo().name!="Cancel drag", "cancel_does_not_enter_history");

    check(manager.begin("Active"), "begin_active");
    check(!manager.begin("Nested"), "nested_transaction_rejected");
    check(!manager.undo() && !manager.redo(), "history_blocked_while_active");
    manager.cancel();

    {
        ScopedTransaction scoped(manager,"Scoped edit");
        value=7; manager.add(std::make_unique<LambdaOperation>([&]{value=30;},[&]{value=7;}));
    }
    check(manager.nextUndo().name=="Scoped edit", "scoped_transaction_auto_commits");
    manager.undo(); check(value==30, "scoped_transaction_undo");

    manager.redo();
    manager.begin("Branch edit"); value=8; manager.add(std::make_unique<LambdaOperation>([&]{value=7;},[&]{value=8;})); manager.commit();
    check(!manager.canRedo(), "new_commit_clears_redo_branch");

    TransactionManager budgeted(2, 1000);
    for(int i=1;i<=3;++i){budgeted.begin("Edit");budgeted.add(std::make_unique<LambdaOperation>([]{},[]{},100));budgeted.commit();}
    check(budgeted.undoCount()==2, "entry_budget_evicts_oldest");
    budgeted.setLimits(10,150); check(budgeted.undoCount()==1 && budgeted.historyBytes()>=100, "memory_budget_evicts_oldest");
    budgeted.clear(); check(!budgeted.canUndo()&&!budgeted.canRedo()&&budgeted.historyBytes()==0, "clear_resets_history");

    if(failures){std::cout<<"FAIL|ace_transaction_probe|count="<<failures<<'\n';return 1;}
    std::cout<<"PASS|ace_transaction_probe\n";return 0;
}
