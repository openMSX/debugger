#include "Version.h"
#include "Version.ii"

std::string Version::full()
{
        return std::string("openMSX Debugger ") + VERSION +
               (RELEASE ? "" : (std::string("-") + REVISION));
}
