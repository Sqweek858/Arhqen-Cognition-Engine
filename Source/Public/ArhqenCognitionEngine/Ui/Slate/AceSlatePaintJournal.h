#pragma once

#include "ArhqenCognitionEngine/Ui/Slate/AceSlateElementList.h"
#include "ArhqenCognitionEngine/Ui/Slate/AceSlateFramePrimitives.h"

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace am::ui::slate
{
    struct AceSlatePaintJournalEntry
    {
        std::uint64_t frameNumber = 0;
        std::uint32_t layer = 0;
        AceSlateElementType kind = AceSlateElementType::Box;
        UiRect bounds{};
        std::string debugName;
    };

    struct AceSlatePaintJournalFrame
    {
        std::uint64_t frameNumber = 0;
        std::uint64_t elementCount = 0;
        std::uint64_t viewportCount = 0;
        std::uint64_t textCount = 0;
        bool fullFrame = true;
        UiRect frameRect{};
        std::vector<AceSlatePaintJournalEntry> entries;
    };

    struct AceSlatePaintJournalStats
    {
        std::uint64_t frames = 0;
        std::uint64_t entries = 0;
        std::uint64_t viewportEntries = 0;
        std::uint64_t fullFrames = 0;
        std::uint64_t partialFrames = 0;
        std::uint64_t droppedFrames = 0;
    };

    class AceSlatePaintJournal
    {
    public:
        explicit AceSlatePaintJournal(std::size_t capacity = 32);
        void Reset();
        void RecordFrame(const AceSlateWindowElementList& list, std::uint64_t frameNumber, UiRect frameRect, bool fullFrame);
        const std::deque<AceSlatePaintJournalFrame>& Frames() const { return frames_; }
        const AceSlatePaintJournalStats& Stats() const { return stats_; }
        std::string Diagnostics() const;
        std::wstring WideDiagnostics() const;
    private:
        std::size_t capacity_ = 32;
        std::deque<AceSlatePaintJournalFrame> frames_{};
        AceSlatePaintJournalStats stats_{};
    };
}
