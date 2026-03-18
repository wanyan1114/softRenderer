#include "platform/Platform.h"

#include <windows.h>

namespace sr::platform {

void Initialize()
{
    SetProcessDPIAware();
}

void Shutdown()
{
}

const char* Name()
{
    return "Windows";
}

} // namespace sr::platform
