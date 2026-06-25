#include "ArhqenCognitionEngine/Ui/D2D/D2DCommandHistory.h"

#include <algorithm>
#include <utility>

namespace am::ui
{
    void D2DCommandHistory::push(std::wstring text)
    {
        if (text.empty())
        {
            cursor_ = entries_.size();
            return;
        }

        if (!entries_.empty() && entries_.back() == text)
        {
            cursor_ = entries_.size();
            return;
        }

        entries_.push_back(std::move(text));

        if (entries_.size() > maxEntries_)
        {
            entries_.erase(entries_.begin());
        }

        cursor_ = entries_.size();
    }

    void D2DCommandHistory::clear()
    {
        entries_.clear();
        cursor_ = 0;
    }

    std::optional<std::wstring> D2DCommandHistory::previous()
    {
        if (entries_.empty())
        {
            return std::nullopt;
        }

        if (cursor_ == 0)
        {
            return entries_.front();
        }

        --cursor_;
        return entries_[cursor_];
    }

    std::optional<std::wstring> D2DCommandHistory::next()
    {
        if (entries_.empty())
        {
            return std::nullopt;
        }

        if (cursor_ + 1 >= entries_.size())
        {
            cursor_ = entries_.size();
            return std::wstring{};
        }

        ++cursor_;
        return entries_[cursor_];
    }

    std::size_t D2DCommandHistory::size() const
    {
        return entries_.size();
    }

    std::size_t D2DCommandHistory::cursor() const
    {
        return cursor_;
    }

    bool D2DCommandHistory::empty() const
    {
        return entries_.empty();
    }
}
