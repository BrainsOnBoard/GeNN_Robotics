#pragma once

// BoB robotics includes
#include "../common/pose.h"

// Third-party includes
#include "../third_party/matplotlibcpp.h"
#include "../third_party/units.h"

// Standard C++ includes
#include <string>
#include <tuple>
#include <vector>

namespace BoBRobotics {
namespace Viz {
template<typename LengthUnit, typename AgentType>
void
plotAgent(AgentType &agent,
               const LengthUnit xLower,
               const LengthUnit xUpper,
               const LengthUnit yLower,
               const LengthUnit yUpper)
{
    // Get agent's pose
    const auto pose = agent.template getPose<LengthUnit>();

    // Get vector components
    const std::vector<double> vx{ pose.x().value() };
    const std::vector<double> vy{ pose.y().value() };
    const std::vector<double> vu{ units::math::cos(pose.yaw()) };
    const std::vector<double> vv{ units::math::sin(pose.yaw()) };

    // Origin
    const std::vector<int> x0{ 0 }, y0{ 0 };

    // Update plot
    namespace plt = matplotlibcpp;
    plt::plot(x0, y0, "r+");
    plt::quiver(vx, vy, vu, vv);
    plt::xlim(xLower.value(), xUpper.value());
    plt::ylim(yLower.value(), yUpper.value());

    // Label axes with the length unit
    const std::string abbrev = units::abbreviation(xLower);
    plt::xlabel("x (" + abbrev + ")");
    plt::ylabel("y (" + abbrev + ")");
}
} // Viz
} // BoBRobotics
