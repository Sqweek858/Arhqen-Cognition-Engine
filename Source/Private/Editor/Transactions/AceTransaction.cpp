#include "ArhqenCognitionEngine/Editor/Transactions/AceTransaction.h"

#include <algorithm>
#include <utility>

namespace am::editor::transactions
{
    LambdaOperation::LambdaOperation(std::function<void()> undo, std::function<void()> redo, std::size_t memoryCost)
        : undo_(std::move(undo)), redo_(std::move(redo)), memoryCost_(std::max(memoryCost, sizeof(LambdaOperation))) {}
    void LambdaOperation::undo() noexcept { try { if (undo_) undo_(); } catch (...) {} }
    void LambdaOperation::redo() noexcept { try { if (redo_) redo_(); } catch (...) {} }

    TransactionManager::TransactionManager(std::size_t maximumEntries, std::size_t maximumBytes)
        : maximumEntries_(maximumEntries), maximumBytes_(maximumBytes) {}

    bool TransactionManager::begin(std::string name)
    {
        if (active_ || name.empty()) return false;
        active_ = std::make_unique<Transaction>(); active_->name = std::move(name); return true;
    }

    bool TransactionManager::add(std::unique_ptr<Operation> operation)
    {
        if (!active_ || !operation) return false;
        active_->memoryCost += operation->memoryCost();
        active_->operations.push_back(std::move(operation));
        return true;
    }

    bool TransactionManager::commit()
    {
        if (!active_) return false;
        if (active_->operations.empty()) { active_.reset(); return false; }
        clearRedo(); historyBytes_ += active_->memoryCost; undo_.push_back(std::move(active_)); trimToBudget(); return true;
    }

    bool TransactionManager::cancel() noexcept
    {
        if (!active_) return false;
        applyUndo(*active_); active_.reset(); return true;
    }

    bool TransactionManager::undo() noexcept
    {
        if (!canUndo()) return false;
        auto transaction = std::move(undo_.back()); undo_.pop_back();
        applyUndo(*transaction); redo_.push_back(std::move(transaction)); return true;
    }

    bool TransactionManager::redo() noexcept
    {
        if (!canRedo()) return false;
        auto transaction = std::move(redo_.back()); redo_.pop_back();
        applyRedo(*transaction); undo_.push_back(std::move(transaction)); return true;
    }

    void TransactionManager::clear() noexcept
    {
        if (active_) cancel();
        undo_.clear(); redo_.clear(); historyBytes_ = 0;
    }

    void TransactionManager::setLimits(std::size_t maximumEntries, std::size_t maximumBytes) noexcept
    { maximumEntries_ = maximumEntries; maximumBytes_ = maximumBytes; trimToBudget(); }

    HistoryEntry TransactionManager::nextUndo() const { return describe(canUndo() ? undo_.back().get() : nullptr); }
    HistoryEntry TransactionManager::nextRedo() const { return describe(canRedo() ? redo_.back().get() : nullptr); }

    void TransactionManager::applyUndo(Transaction& transaction) noexcept
    { for (auto it = transaction.operations.rbegin(); it != transaction.operations.rend(); ++it) (*it)->undo(); }
    void TransactionManager::applyRedo(Transaction& transaction) noexcept
    { for (auto& operation : transaction.operations) operation->redo(); }

    void TransactionManager::clearRedo() noexcept
    {
        for (const auto& transaction : redo_) historyBytes_ -= transaction->memoryCost;
        redo_.clear();
    }

    void TransactionManager::trimToBudget() noexcept
    {
        while (!undo_.empty() && (undo_.size() + redo_.size() > maximumEntries_ || historyBytes_ > maximumBytes_))
        { historyBytes_ -= undo_.front()->memoryCost; undo_.pop_front(); }
        while (!redo_.empty() && (undo_.size() + redo_.size() > maximumEntries_ || historyBytes_ > maximumBytes_))
        { historyBytes_ -= redo_.front()->memoryCost; redo_.pop_front(); }
    }

    HistoryEntry TransactionManager::describe(const Transaction* transaction)
    { return transaction ? HistoryEntry{transaction->name, transaction->operations.size(), transaction->memoryCost} : HistoryEntry{}; }

    ScopedTransaction::ScopedTransaction(TransactionManager& manager, std::string name, bool enabled)
        : manager_(&manager), active_(enabled && manager.begin(std::move(name))) {}
    ScopedTransaction::~ScopedTransaction() { if (active_) manager_->commit(); }
    void ScopedTransaction::cancel() noexcept { if (active_) { manager_->cancel(); active_ = false; } }
    bool ScopedTransaction::commit() { if (!active_) return false; active_ = false; return manager_->commit(); }
}
