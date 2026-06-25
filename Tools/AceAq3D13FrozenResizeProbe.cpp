#include <algorithm>
#include <cstdint>
#include <iostream>

struct Rect
{
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
};

struct FrozenResizeModel
{
    bool active = false;
    bool frozenValid = false;
    bool pendingValid = false;
    Rect frozen{};
    Rect pending{};
    std::uint64_t frozenSizingMessages = 0;
    std::uint64_t preventedClientResize = 0;
    std::uint64_t finalCommits = 0;

    void begin(Rect current)
    {
        active = true;
        frozen = current;
        frozenValid = true;
        pendingValid = false;
    }

    bool sizing(Rect& proposed)
    {
        if (!active || !frozenValid)
        {
            return false;
        }

        pending = proposed;
        pendingValid = true;
        ++frozenSizingMessages;

        proposed = frozen;
        ++preventedClientResize;
        return true;
    }

    Rect exit()
    {
        active = false;
        if (pendingValid)
        {
            ++finalCommits;
            pendingValid = false;
            return pending;
        }

        return frozen;
    }
};

int main()
{
    FrozenResizeModel model;
    const Rect initial{100, 100, 900, 700};
    Rect proposed{100, 100, 1300, 900};

    model.begin(initial);

    if (!model.sizing(proposed))
    {
        std::cerr << "FAIL|sizing_not_handled\n";
        return 1;
    }

    if (proposed.right != initial.right || proposed.bottom != initial.bottom)
    {
        std::cerr << "FAIL|native_rect_not_frozen\n";
        return 1;
    }

    const Rect finalRect = model.exit();

    if (finalRect.right != 1300 || finalRect.bottom != 900)
    {
        std::cerr << "FAIL|final_rect_not_committed\n";
        return 1;
    }

    if (model.frozenSizingMessages != 1 || model.preventedClientResize != 1 || model.finalCommits != 1)
    {
        std::cerr << "FAIL|counters_wrong\n";
        return 1;
    }

    std::cout << "PASS|ace_aq3d13_frozen_native_resize_probe\n";
    return 0;
}
