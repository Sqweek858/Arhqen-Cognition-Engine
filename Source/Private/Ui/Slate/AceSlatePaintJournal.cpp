#include "ArhqenCognitionEngine/Ui/Slate/AceSlatePaintJournal.h"

#include <algorithm>
#include <sstream>

namespace am::ui::slate
{
    AceSlatePaintJournal::AceSlatePaintJournal(std::size_t capacity)
        : capacity_(std::max<std::size_t>(4, capacity))
    {
    }

    void AceSlatePaintJournal::Reset()
    {
        frames_.clear();
        stats_ = {};
    }

    void AceSlatePaintJournal::RecordFrame(const AceSlateWindowElementList& list, std::uint64_t frameNumber, UiRect frameRect, bool fullFrame)
    {
        AceSlatePaintJournalFrame frame{};
        frame.frameNumber = frameNumber;
        frame.elementCount = list.Elements().size();
        frame.fullFrame = fullFrame;
        frame.frameRect = frameRect;
        for (const auto& element : list.Elements())
        {
            AceSlatePaintJournalEntry entry{};
            entry.frameNumber = frameNumber;
            entry.layer = static_cast<std::uint32_t>(std::max(0, element.layer));
            entry.kind = element.type;
            entry.bounds = element.geometry.rect;
            entry.debugName = element.debugName;
            frame.entries.push_back(entry);
            if (element.type == AceSlateElementType::Viewport)
            {
                ++frame.viewportCount;
            }
            if (element.type == AceSlateElementType::Text)
            {
                ++frame.textCount;
            }
        }
        if (frames_.size() == capacity_)
        {
            frames_.pop_front();
            ++stats_.droppedFrames;
        }
        frames_.push_back(std::move(frame));
        ++stats_.frames;
        stats_.entries += list.Elements().size();
        stats_.viewportEntries += frames_.back().viewportCount;
        if (fullFrame)
        {
            ++stats_.fullFrames;
        }
        else
        {
            ++stats_.partialFrames;
        }
    }

    std::string AceSlatePaintJournal::Diagnostics() const
    {
        std::ostringstream oss;
        oss << "frames=" << stats_.frames
            << ";stored=" << frames_.size()
            << ";entries=" << stats_.entries
            << ";viewport_entries=" << stats_.viewportEntries
            << ";full=" << stats_.fullFrames
            << ";partial=" << stats_.partialFrames
            << ";dropped=" << stats_.droppedFrames;
        if (!frames_.empty())
        {
            const auto& last = frames_.back();
            oss << ";last_frame=" << last.frameNumber
                << ";last_elements=" << last.elementCount
                << ";last_viewports=" << last.viewportCount;
        }
        return oss.str();
    }

    std::wstring AceSlatePaintJournal::WideDiagnostics() const
    {
        const std::string text = Diagnostics();
        return std::wstring(text.begin(), text.end());
    }
}
