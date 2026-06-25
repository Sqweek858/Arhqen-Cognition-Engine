#include "ArhqenCognitionEngine/Ui/D2D/D2DCyberText.h"

namespace am::ui
{
    namespace
    {
        D2DCyberTextState g_state {};
    }

    void D2DCyberText::setState(D2DCyberTextState state)
    {
        g_state = state;
    }

    void D2DCyberText::reset()
    {
        g_state = {};
    }

    D2DCyberTextState D2DCyberText::state()
    {
        return g_state;
    }
}
