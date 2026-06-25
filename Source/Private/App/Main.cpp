#include "ArhqenCognitionEngine/Core/Application.h"
#include "ArhqenCognitionEngine/Core/ExitCode.h"

int main()
{
    am::core::Application app;
    return am::core::toProcessExitCode(app.run());
}
