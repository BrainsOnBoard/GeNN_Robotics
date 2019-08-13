// BoB robotics includes
#include "common/bob_robotics_path.h"

// Standard C includes
#include <cstdlib>

// Standard C++ includes
#include <stdexcept>

#define BOB_TO_STRING_LITERAL(s) #s

namespace BoBRobotics {
filesystem::path
getBoBRoboticsPath()
{
#ifdef BOB_ROBOTICS_SUBMODULE_PATH
    return filesystem::path{ BOB_TO_STRING_LITERAL(BOB_ROBOTICS_SUBMODULE_PATH) };
#else
    // Get from environment variable
    const char *path = std::getenv("BOB_ROBOTICS_PATH");
    if (!path) {
        throw std::runtime_error("BOB_ROBOTICS_PATH environment variable not set");
    }
    return path;
#endif
}

filesystem::path
getResourcesPath()
{
    return getBoBRoboticsPath() / "resources";
}
} // BoBRobotics