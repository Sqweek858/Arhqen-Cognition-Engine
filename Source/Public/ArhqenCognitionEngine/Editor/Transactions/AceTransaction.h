#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace am::editor::transactions
{
    class Operation
    {
    public:
        virtual ~Operation() = default;
        virtual void undo() noexcept = 0;
        virtual void redo() noexcept = 0;
        [[nodiscard]] virtual std::size_t memoryCost() const noexcept = 0;
    };

    class LambdaOperation final : public Operation
    {
    public:
        LambdaOperation(std::function<void()> undo, std::function<void()> redo,
                        std::size_t memoryCost = sizeof(LambdaOperation));
        void undo() noexcept override;
        void redo() noexcept override;
        [[nodiscard]] std::size_t memoryCost() const noexcept override { return memoryCost_; }

    private:
        std::function<void()> undo_;
        std::function<void()> redo_;
        std::size_t memoryCost_ = 0;
    };

    struct HistoryEntry
    {
        std::string name;
        std::size_t operationCount = 0;
        std::size_t memoryCost = 0;
    };

    class TransactionManager final
    {
    public:
        explicit TransactionManager(std::size_t maximumEntries = 256,
                                    std::size_t maximumBytes = 64ull * 1024ull * 1024ull);

        bool begin(std::string name);
        bool add(std::unique_ptr<Operation> operation);
        bool commit();
        bool cancel() noexcept;
        bool undo() noexcept;
        bool redo() noexcept;
        void clear() noexcept;

        void setLimits(std::size_t maximumEntries, std::size_t maximumBytes) noexcept;
        [[nodiscard]] bool isActive() const noexcept { return active_ != nullptr; }
        [[nodiscard]] bool canUndo() const noexcept { return !undo_.empty() && !isActive(); }
        [[nodiscard]] bool canRedo() const noexcept { return !redo_.empty() && !isActive(); }
        [[nodiscard]] std::size_t undoCount() const noexcept { return undo_.size(); }
        [[nodiscard]] std::size_t redoCount() const noexcept { return redo_.size(); }
        [[nodiscard]] std::size_t historyBytes() const noexcept { return historyBytes_; }
        [[nodiscard]] HistoryEntry nextUndo() const;
        [[nodiscard]] HistoryEntry nextRedo() const;

    private:
        struct Transaction
        {
            std::string name;
            std::vector<std::unique_ptr<Operation>> operations;
            std::size_t memoryCost = 0;
        };

        static void applyUndo(Transaction& transaction) noexcept;
        static void applyRedo(Transaction& transaction) noexcept;
        void clearRedo() noexcept;
        void trimToBudget() noexcept;
        static HistoryEntry describe(const Transaction* transaction);

        std::unique_ptr<Transaction> active_;
        std::deque<std::unique_ptr<Transaction>> undo_;
        std::deque<std::unique_ptr<Transaction>> redo_;
        std::size_t maximumEntries_ = 0;
        std::size_t maximumBytes_ = 0;
        std::size_t historyBytes_ = 0;
    };

    class ScopedTransaction final
    {
    public:
        ScopedTransaction(TransactionManager& manager, std::string name, bool enabled = true);
        ~ScopedTransaction();
        ScopedTransaction(const ScopedTransaction&) = delete;
        ScopedTransaction& operator=(const ScopedTransaction&) = delete;
        void cancel() noexcept;
        bool commit();
        [[nodiscard]] bool active() const noexcept { return active_; }

    private:
        TransactionManager* manager_ = nullptr;
        bool active_ = false;
    };
}
