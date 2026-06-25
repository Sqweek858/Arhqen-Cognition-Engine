#pragma once

namespace am::core
{
    enum class ExitCode : int
    {
        Success = 0,
        MissingConfig = 2,
        LoggingFailed = 3,
        RuntimeFailure = 4
    };

    inline int toProcessExitCode(ExitCode code)
    {
        return static_cast<int>(code);
    }
}
