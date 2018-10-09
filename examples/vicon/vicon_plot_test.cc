// C++ includes
#include <chrono>
#include <iostream>
#include <thread>

// BoB robotics includes
#include "common/plot_agent.h"
#include "vicon/capture_control.h"
#include "vicon/udp.h"

// Third-party includes
#include "third_party/matplotlibcpp.h"

using namespace BoBRobotics;
using namespace std::literals;
using namespace units::angle;
namespace plt = matplotlibcpp;

auto now()
{
    return std::chrono::high_resolution_clock::now();
}

int
main()
{
    Vicon::UDPClient<> vicon(51001);
    vicon.waitForObject();

    Vicon::CaptureControl viconCaptureControl("192.168.1.100", 3003, "c:\\users\\ad374\\Desktop");
    if (!viconCaptureControl.startRecording("test1")) {
        return EXIT_FAILURE;
    }

    do {
        plt::figure(1);
        plotAgent(vicon.getObjectData(0), { -2500, 2500 }, { -2500, 2500 });
        plt::pause(0.025);
    } while (plt::fignum_exists(1));

    if (!viconCaptureControl.stopRecording("test1")) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
