#pragma once

#include <string>
#include <utility>

namespace am::core
{
    struct Result
    {
        bool ok = false;
        std::string message;

        static Result success(std::string msg = {})
        {
            return Result{true, std::move(msg)};
        }

        static Result failure(std::string msg)
        {
            return Result{false, std::move(msg)};
        }
    };
}
