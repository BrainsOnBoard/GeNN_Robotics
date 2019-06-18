// C++ includes
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

// BoB robotics includes
#include "vicon/capture_control.h"
#include "vicon/udp.h"

using namespace BoBRobotics::Vicon;
using namespace std::literals;
using namespace units::angular_velocity;

int main()
{
    UDPClient<ObjectDataVelocity> vicon(51001);
    CaptureControl viconCaptureControl("192.168.1.100", 3003, "c:\\users\\ad374\\Desktop");
    while (vicon.getNumObjects() == 0) {
        std::this_thread::sleep_for(1s);
        std::cout << "Waiting for object" << std::endl;
    }

    viconCaptureControl.startRecording("test1");

    for (int i = 0; i < 120; i++) {
        auto objectData = vicon.getObjectData(0);
        const auto velocity = objectData.getVelocity();
        const auto angularVelocity = objectData.getAngularVelocity<degrees_per_second_t>();

        std::cout << "("
                  << velocity[0] << ", " << velocity[1] << ", " << velocity[2]
                  << ") | ("
                  << angularVelocity[0] << ", " << angularVelocity[1] << ", " << angularVelocity[2]
                  << ")" << std::endl;

        std::this_thread::sleep_for(1s);
    }

    viconCaptureControl.stopRecording("test1");

    return EXIT_SUCCESS;
}
