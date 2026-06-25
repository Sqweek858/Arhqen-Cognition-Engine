#pragma once

#include <optional>
#include <string>
#include <vector>

namespace am::ui
{
    class D2DCommandHistory
    {
    public:
        void push(std::wstring text);
        void clear();

        std::optional<std::wstring> previous();
        std::optional<std::wstring> next();

        std::size_t size() const;
        std::size_t cursor() const;
        bool empty() const;

    private:
        std::vector<std::wstring> entries_;
        std::size_t cursor_ = 0;
        std::size_t maxEntries_ = 64;
    };
}
