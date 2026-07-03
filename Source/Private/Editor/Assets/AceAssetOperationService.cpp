#include "ArhqenCognitionEngine/Editor/Assets/AceAssetOperationService.h"

#include <algorithm>
#include <cctype>
#include <system_error>

namespace am::editor::assets
{
    using am::core::Guid;
    using am::core::assets::AssetPath;
    using am::core::assets::AssetRecord;
    using am::core::assets::AssetReferenceIndex;
    using am::core::assets::AssetRegistry;
    using am::editor::transactions::Operation;
    using am::editor::transactions::TransactionManager;

    struct AssetOperationService::ErrorState
    {
        std::string value;
        void clear() { value.clear(); }
        void fail(std::string message) { if (value.empty()) value = std::move(message); }
    };

    namespace
    {
        void fail(std::string* error, std::string message)
        {
            if (error) *error = std::move(message);
        }

        std::size_t pathCost(const std::filesystem::path& lhs, const std::filesystem::path& rhs)
        {
            return sizeof(std::filesystem::path) * 2 + (lhs.native().size() + rhs.native().size()) * sizeof(wchar_t);
        }

        bool renamePhysical(const std::filesystem::path& from, const std::filesystem::path& to, std::string* error)
        {
            std::error_code ec;
            std::filesystem::rename(from, to, ec);
            if (!ec) return true;
            fail(error, "Filesystem rename failed: " + ec.message());
            return false;
        }

        void rescanNoThrow(AssetRegistry& registry, const std::shared_ptr<AssetOperationService::ErrorState>& state)
        {
            std::string error;
            if (!registry.rescan(&error)) state->fail("Asset Registry rescan failed during transaction: " + error);
        }

        class MoveOperation final : public Operation
        {
        public:
            MoveOperation(AssetRegistry& registry, AssetPath oldPath, AssetPath newPath,
                          std::filesystem::path oldPhysical, std::filesystem::path newPhysical,
                          std::shared_ptr<AssetOperationService::ErrorState> state)
                : registry_(registry), oldPath_(std::move(oldPath)), newPath_(std::move(newPath)),
                  oldPhysical_(std::move(oldPhysical)), newPhysical_(std::move(newPhysical)), state_(std::move(state)) {}
            void undo() noexcept override { apply(newPath_, oldPath_, newPhysical_, oldPhysical_); }
            void redo() noexcept override { apply(oldPath_, newPath_, oldPhysical_, newPhysical_); }
            std::size_t memoryCost() const noexcept override { return sizeof(*this) + pathCost(oldPhysical_, newPhysical_); }
        private:
            void apply(const AssetPath& from, const AssetPath& to,
                       const std::filesystem::path& fromPhysical, const std::filesystem::path& toPhysical) noexcept
            {
                try
                {
                    std::string error;
                    if (!renamePhysical(fromPhysical, toPhysical, &error)) { state_->fail(error); return; }
                    if (!registry_.remapPath(from, to, &error))
                    {
                        std::string ignored;
                        renamePhysical(toPhysical, fromPhysical, &ignored);
                        state_->fail("Registry identity remap failed during transaction: " + error);
                    }
                }
                catch (...) { state_->fail("Unexpected exception during asset move transaction"); }
            }
            AssetRegistry& registry_;
            AssetPath oldPath_;
            AssetPath newPath_;
            std::filesystem::path oldPhysical_;
            std::filesystem::path newPhysical_;
            std::shared_ptr<AssetOperationService::ErrorState> state_;
        };

        class FolderCreateOperation final : public Operation
        {
        public:
            FolderCreateOperation(AssetRegistry& registry, std::filesystem::path physical,
                                  std::shared_ptr<AssetOperationService::ErrorState> state, bool createApplied)
                : registry_(registry), physical_(std::move(physical)), state_(std::move(state)), createApplied_(createApplied) {}
            void undo() noexcept override { createApplied_ ? remove() : create(); }
            void redo() noexcept override { createApplied_ ? create() : remove(); }
            std::size_t memoryCost() const noexcept override { return sizeof(*this) + physical_.native().size() * sizeof(wchar_t); }
        private:
            void create() noexcept
            {
                std::error_code ec;
                if (!std::filesystem::create_directory(physical_, ec) && (ec || !std::filesystem::is_directory(physical_)))
                { state_->fail("Could not restore asset folder: " + ec.message()); return; }
                rescanNoThrow(registry_, state_);
            }
            void remove() noexcept
            {
                std::error_code ec;
                if (!std::filesystem::remove(physical_, ec) || ec)
                { state_->fail("Asset folder is no longer empty or removable"); return; }
                rescanNoThrow(registry_, state_);
            }
            AssetRegistry& registry_;
            std::filesystem::path physical_;
            std::shared_ptr<AssetOperationService::ErrorState> state_;
            bool createApplied_ = true;
        };

        class StashedFileOperation final : public Operation
        {
        public:
            enum class Mode { Duplicate, Delete };
            StashedFileOperation(AssetRegistry& registry, AssetReferenceIndex& references, Guid id,
                                 std::filesystem::path live, std::filesystem::path stash, Mode mode,
                                 std::vector<Guid> outgoingReferences,
                                 std::shared_ptr<AssetOperationService::ErrorState> state)
                : registry_(registry), references_(references), id_(id), live_(std::move(live)),
                  stash_(std::move(stash)), mode_(mode), outgoingReferences_(std::move(outgoingReferences)), state_(std::move(state)) {}
            ~StashedFileOperation() override { std::error_code ignored; std::filesystem::remove(stash_, ignored); }
            void undo() noexcept override { mode_ == Mode::Duplicate ? removeDuplicate() : restoreDeleted(); }
            void redo() noexcept override { mode_ == Mode::Duplicate ? restoreDuplicate() : removeDeleted(); }
            std::size_t memoryCost() const noexcept override
            {
                return sizeof(*this) + pathCost(live_, stash_) + outgoingReferences_.size() * sizeof(Guid);
            }
        private:
            void removeDuplicate() noexcept
            {
                std::error_code ec;
                if (!std::filesystem::remove(live_, ec) || ec) { state_->fail("Could not undo duplicated asset"); return; }
                rescanNoThrow(registry_, state_);
            }
            void restoreDuplicate() noexcept
            {
                std::error_code ec;
                if (!std::filesystem::copy_file(stash_, live_, std::filesystem::copy_options::none, ec) || ec)
                { state_->fail("Could not redo duplicated asset: " + ec.message()); return; }
                rescanNoThrow(registry_, state_);
            }
            void restoreDeleted() noexcept
            {
                std::string error;
                if (!renamePhysical(stash_, live_, &error)) { state_->fail(error); return; }
                rescanNoThrow(registry_, state_);
                if (!outgoingReferences_.empty() && !references_.setReferences(id_, outgoingReferences_, &error))
                    state_->fail("Could not restore asset references: " + error);
            }
            void removeDeleted() noexcept
            {
                std::string error;
                references_.removeReferencer(id_);
                if (!renamePhysical(live_, stash_, &error)) { state_->fail(error); return; }
                rescanNoThrow(registry_, state_);
            }
            AssetRegistry& registry_;
            AssetReferenceIndex& references_;
            Guid id_{};
            std::filesystem::path live_;
            std::filesystem::path stash_;
            Mode mode_ = Mode::Duplicate;
            std::vector<Guid> outgoingReferences_;
            std::shared_ptr<AssetOperationService::ErrorState> state_;
        };

        bool folderInSnapshot(const AssetRegistry& registry, const AssetPath& path)
        {
            const std::string key = path.comparisonKey();
            return std::any_of(registry.snapshot().folders.begin(), registry.snapshot().folders.end(),
                [&](const auto& folder) { return folder.path.comparisonKey() == key; });
        }

        std::string extensionOf(const AssetPath& path)
        {
            const std::string_view leaf = path.leafName();
            const auto dot = leaf.find_last_of('.');
            return dot == std::string_view::npos ? std::string{} : std::string(leaf.substr(dot));
        }
    }

    AssetOperationService::AssetOperationService() : errorState_(std::make_shared<ErrorState>()) {}
    AssetOperationService::~AssetOperationService() = default;

    bool AssetOperationService::initialize(AssetRegistry& registry, AssetReferenceIndex& references,
                                           TransactionManager& transactions, std::filesystem::path undoStorageRoot,
                                           std::string* error)
    {
        if (error) error->clear();
        if (!registry.initialized() || undoStorageRoot.empty())
        {
            fail(error, "Asset Operation Service requires an initialized registry and undo root");
            return false;
        }
        std::error_code ec;
        undoStorageRoot = std::filesystem::absolute(undoStorageRoot, ec).lexically_normal();
        const auto contentRelativeToUndo = registry.contentRoot().lexically_relative(undoStorageRoot);
        bool undoContainsContent = !contentRelativeToUndo.empty();
        for (const auto& part : contentRelativeToUndo) if (part == "..") undoContainsContent = false;
        if (ec || undoContainsContent || AssetPath::fromFilesystemPath(registry.contentRoot(), undoStorageRoot))
        {
            fail(error, "Asset undo storage must be outside Content");
            return false;
        }
        if (!std::filesystem::create_directories(undoStorageRoot, ec) && ec)
        {
            fail(error, "Could not prepare asset undo storage: " + ec.message());
            return false;
        }
        for (std::filesystem::directory_iterator it(undoStorageRoot, ec), end; !ec && it != end; it.increment(ec))
        {
            const auto extension = it->path().extension();
            if (it->is_regular_file() && (extension == L".deleted" || extension == L".duplicate"))
            {
                std::error_code ignored;
                std::filesystem::remove(it->path(), ignored);
            }
        }
        if (ec)
        {
            fail(error, "Could not inspect asset undo storage: " + ec.message());
            return false;
        }
        registry_ = &registry;
        references_ = &references;
        transactions_ = &transactions;
        undoStorageRoot_ = std::move(undoStorageRoot);
        errorState_->clear();
        return true;
    }

    bool AssetOperationService::folderExists(const AssetPath& path) const
    {
        return registry_ && folderInSnapshot(*registry_, path);
    }

    bool AssetOperationService::validateName(std::string_view name, std::string* error) const
    {
        if (name.empty() || name.size() > 128 || name.front() == ' ' || name.back() == ' ' || name.find('.') != std::string_view::npos ||
            name.find('/') != std::string_view::npos || name.find('\\') != std::string_view::npos)
        {
            fail(error, "Asset display name is empty, unsafe, dotted, or too long");
            return false;
        }
        return true;
    }

    AssetOperationResult AssetOperationService::createFolder(const AssetPath& parent, std::string_view name)
    {
        AssetOperationResult result;
        if (!initialized() || !folderExists(parent) || !validateName(name, &result.error)) return result;
        const auto destination = parent.child(name);
        if (!destination) { result.error = "Folder name is not a valid /Game segment"; return result; }
        const auto physical = destination->toFilesystemPath(registry_->contentRoot());
        if (std::filesystem::exists(physical)) { result.error = "Folder already exists"; return result; }
        if (!transactions_->begin("Create folder")) { result.error = "Another editor transaction is active"; return result; }
        std::error_code ec;
        if (!std::filesystem::create_directory(physical, ec) || ec)
        { transactions_->cancel(); result.error = "Could not create folder: " + ec.message(); return result; }
        if (!registry_->rescan(&result.error))
        { std::filesystem::remove(physical, ec); transactions_->cancel(); return result; }
        if (!transactions_->add(std::make_unique<FolderCreateOperation>(*registry_, physical, errorState_, true)) || !transactions_->commit())
        { std::filesystem::remove(physical, ec); registry_->rescan(nullptr); transactions_->cancel(); result.error = "Could not record folder transaction"; return result; }
        result.success = true;
        result.path = *destination;
        return result;
    }

    AssetOperationResult AssetOperationService::moveInternal(const AssetPath& source, const AssetPath& destination,
                                                              std::string transactionName)
    {
        AssetOperationResult result;
        if (!initialized() || source.isRoot() || destination.isRoot()) { result.error = "Asset move path is invalid"; return result; }
        const auto* asset = registry_->findByPath(source.string());
        if (!asset && !folderExists(source)) { result.error = "Asset move source does not exist"; return result; }
        const Guid movedId = asset ? asset->id : Guid{};
        const auto sourcePhysical = source.toFilesystemPath(registry_->contentRoot());
        const auto destinationPhysical = destination.toFilesystemPath(registry_->contentRoot());
        if (std::filesystem::exists(destinationPhysical)) { result.error = "Asset move destination already exists"; return result; }
        if (!transactions_->begin(std::move(transactionName))) { result.error = "Another editor transaction is active"; return result; }
        if (!renamePhysical(sourcePhysical, destinationPhysical, &result.error)) { transactions_->cancel(); return result; }
        if (!registry_->remapPath(source, destination, &result.error))
        {
            std::string ignored; renamePhysical(destinationPhysical, sourcePhysical, &ignored);
            transactions_->cancel(); return result;
        }
        auto operation = std::make_unique<MoveOperation>(*registry_, source, destination, sourcePhysical, destinationPhysical, errorState_);
        if (!transactions_->add(std::move(operation)) || !transactions_->commit())
        {
            std::string ignored; renamePhysical(destinationPhysical, sourcePhysical, &ignored);
            registry_->remapPath(destination, source, nullptr); transactions_->cancel();
            result.error = "Could not record asset move transaction"; return result;
        }
        result.success = true;
        result.path = destination;
        result.id = movedId;
        return result;
    }

    AssetOperationResult AssetOperationService::rename(const AssetPath& source, std::string_view newName)
    {
        AssetOperationResult result;
        if (!validateName(newName, &result.error)) return result;
        const auto* asset = registry_ ? registry_->findByPath(source.string()) : nullptr;
        const std::string leaf = std::string(newName) + (asset ? extensionOf(source) : std::string{});
        const auto destination = source.parent().child(leaf);
        if (!destination) { result.error = "Renamed asset path is invalid"; return result; }
        return moveInternal(source, *destination, "Rename asset");
    }

    AssetOperationResult AssetOperationService::move(const AssetPath& source, const AssetPath& destinationFolder)
    {
        AssetOperationResult result;
        if (!folderExists(destinationFolder)) { result.error = "Destination folder does not exist"; return result; }
        const auto destination = destinationFolder.child(source.leafName());
        if (!destination) { result.error = "Moved asset path is invalid"; return result; }
        return moveInternal(source, *destination, "Move asset");
    }

    AssetOperationResult AssetOperationService::duplicateAsset(const AssetPath& source, std::string_view newName)
    {
        AssetOperationResult result;
        if (!initialized() || !validateName(newName, &result.error)) return result;
        const AssetRecord* asset = registry_->findByPath(source.string());
        if (!asset) { result.error = "Only registered files can be duplicated"; return result; }
        const auto destination = source.parent().child(std::string(newName) + extensionOf(source));
        if (!destination) { result.error = "Duplicate asset path is invalid"; return result; }
        const auto sourcePhysical = source.toFilesystemPath(registry_->contentRoot());
        const auto destinationPhysical = destination->toFilesystemPath(registry_->contentRoot());
        if (std::filesystem::exists(destinationPhysical)) { result.error = "Duplicate destination already exists"; return result; }
        if (!transactions_->begin("Duplicate asset")) { result.error = "Another editor transaction is active"; return result; }
        std::error_code ec;
        if (!std::filesystem::copy_file(sourcePhysical, destinationPhysical, std::filesystem::copy_options::none, ec) || ec)
        { transactions_->cancel(); result.error = "Could not duplicate asset: " + ec.message(); return result; }
        const auto stash = undoStorageRoot_ / (Guid::create().toString() + ".duplicate");
        if (!std::filesystem::copy_file(destinationPhysical, stash, std::filesystem::copy_options::none, ec) || ec ||
            !registry_->rescan(&result.error))
        {
            std::filesystem::remove(destinationPhysical, ec); std::filesystem::remove(stash, ec);
            registry_->rescan(nullptr); transactions_->cancel();
            if (result.error.empty()) result.error = "Could not prepare duplicate undo data: " + ec.message();
            return result;
        }
        const auto* duplicate = registry_->findByPath(destination->string());
        if (!duplicate)
        {
            result.error = "Duplicated file was not indexed";
            std::filesystem::remove(destinationPhysical, ec);
            std::filesystem::remove(stash, ec);
            registry_->rescan(nullptr);
            transactions_->cancel();
            return result;
        }
        const Guid duplicateId = duplicate->id;
        auto operation = std::make_unique<StashedFileOperation>(*registry_, *references_, duplicateId,
            destinationPhysical, stash, StashedFileOperation::Mode::Duplicate, std::vector<Guid>{}, errorState_);
        if (!transactions_->add(std::move(operation)) || !transactions_->commit())
        { std::filesystem::remove(destinationPhysical, ec); std::filesystem::remove(stash, ec); registry_->rescan(nullptr); transactions_->cancel(); result.error = "Could not record duplicate transaction"; return result; }
        result.success = true; result.path = *destination; result.id = duplicateId; return result;
    }

    AssetOperationResult AssetOperationService::deleteAsset(const AssetPath& source, bool force)
    {
        AssetOperationResult result;
        if (!initialized()) { result.error = "Asset Operation Service is not initialized"; return result; }
        const AssetRecord* asset = registry_->findByPath(source.string());
        if (!asset) { result.error = "Only registered files can be deleted"; return result; }
        result.blockingReferencers = references_->referencersTo(asset->id);
        if (!force && !result.blockingReferencers.empty())
        { result.error = "Asset is referenced and cannot be deleted safely"; return result; }
        if (!transactions_->begin("Delete asset")) { result.error = "Another editor transaction is active"; return result; }
        const Guid id = asset->id;
        const auto outgoing = references_->referencesFrom(id);
        const auto live = source.toFilesystemPath(registry_->contentRoot());
        const auto stash = undoStorageRoot_ / (Guid::create().toString() + ".deleted");
        if (!renamePhysical(live, stash, &result.error)) { transactions_->cancel(); return result; }
        references_->removeReferencer(id);
        if (!registry_->rescan(&result.error))
        {
            std::string ignored; renamePhysical(stash, live, &ignored); references_->setReferences(id, outgoing, nullptr);
            transactions_->cancel(); return result;
        }
        auto operation = std::make_unique<StashedFileOperation>(*registry_, *references_, id, live, stash,
            StashedFileOperation::Mode::Delete, outgoing, errorState_);
        if (!transactions_->add(std::move(operation)) || !transactions_->commit())
        { std::string ignored; renamePhysical(stash, live, &ignored); registry_->rescan(nullptr); references_->setReferences(id, outgoing, nullptr); transactions_->cancel(); result.error = "Could not record delete transaction"; return result; }
        result.success = true; result.path = source; result.id = id; return result;
    }

    AssetOperationResult AssetOperationService::deleteEmptyFolder(const AssetPath& source)
    {
        AssetOperationResult result;
        if (!initialized() || source.isRoot() || !folderExists(source)) { result.error = "Folder does not exist or is the /Game root"; return result; }
        const auto physical = source.toFilesystemPath(registry_->contentRoot());
        std::error_code ec;
        if (!std::filesystem::is_empty(physical, ec) || ec) { result.error = "Only empty folders can be deleted"; return result; }
        if (!transactions_->begin("Delete folder")) { result.error = "Another editor transaction is active"; return result; }
        if (!std::filesystem::remove(physical, ec) || ec) { transactions_->cancel(); result.error = "Could not delete folder: " + ec.message(); return result; }
        if (!registry_->rescan(&result.error)) { std::filesystem::create_directory(physical, ec); transactions_->cancel(); return result; }
        if (!transactions_->add(std::make_unique<FolderCreateOperation>(*registry_, physical, errorState_, false)) || !transactions_->commit())
        { std::filesystem::create_directory(physical, ec); registry_->rescan(nullptr); transactions_->cancel(); result.error = "Could not record folder delete transaction"; return result; }
        result.success = true; result.path = source; return result;
    }

    bool AssetOperationService::undo(std::string* error)
    {
        if (error) error->clear();
        errorState_->clear();
        if (!transactions_ || !transactions_->undo()) { fail(error, "No asset transaction to undo"); return false; }
        if (!errorState_->value.empty()) { fail(error, errorState_->value); return false; }
        return true;
    }

    bool AssetOperationService::redo(std::string* error)
    {
        if (error) error->clear();
        errorState_->clear();
        if (!transactions_ || !transactions_->redo()) { fail(error, "No asset transaction to redo"); return false; }
        if (!errorState_->value.empty()) { fail(error, errorState_->value); return false; }
        return true;
    }
}
