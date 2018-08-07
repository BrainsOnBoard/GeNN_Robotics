// Standard C++ includes
#include <chrono>
#include <numeric>

// Common includes
#include "hid/joystick.h"
#include "genn_utils/analogue_csv_recorder.h"
#include "robots/norbot.h"
#include "vicon/capture_control.h"
#include "vicon/udp.h"

// GeNN generated code includes
#include "stone_cx_CODE/definitions.h"

// Model includes
#include "parameters.h"
#include "robotCommon.h"
#include "robotParameters.h"
#include "simulatorCommon.h"

using namespace BoBRobotics;
using namespace BoBRobotics::StoneCX;
using namespace BoBRobotics::HID;
using namespace std::literals;

int main(int argc, char *argv[])
{
    const float speedScale = 5.0f;
    const double preferredAngleTN2[] = { Parameters::pi / 4.0, -Parameters::pi / 4.0 };

    // Create joystick interface
    Joystick joystick;

    // Create motor interface
    Robots::Norbot motor;

    // Create VICON UDP interface
    Vicon::UDPClient<Vicon::ObjectDataVelocity> vicon(51001);

    // Create VICON capture control interface
    Vicon::CaptureControl viconCaptureControl("192.168.1.100", 3003,
                                              "c:\\users\\ad374\\Desktop");
    // Initialise GeNN
    allocateMem();
    initialize();

    //---------------------------------------------------------------------------
    // Initialize neuron parameters
    //---------------------------------------------------------------------------
    // TL
    for(unsigned int i = 0; i < 8; i++) {
        preferredAngleTL[i] = preferredAngleTL[8 + i] = (Parameters::pi / 4.0) * (double)i;
    }

    //---------------------------------------------------------------------------
    // Build connectivity
    //---------------------------------------------------------------------------
    buildConnectivity();
    
    initstone_cx();

#ifdef RECORD_ELECTROPHYS
    GeNNUtils::AnalogueCSVRecorder<scalar> tn2Recorder("tn2.csv", rTN2, Parameters::numTN2, "TN2");
    GeNNUtils::AnalogueCSVRecorder<scalar> cl1Recorder("cl1.csv", rCL1, Parameters::numCL1, "CL1");
    GeNNUtils::AnalogueCSVRecorder<scalar> tb1Recorder("tb1.csv", rTB1, Parameters::numTB1, "TB1");
    GeNNUtils::AnalogueCSVRecorder<scalar> cpu4Recorder("cpu4.csv", rCPU4, Parameters::numCPU4, "CPU4");
    GeNNUtils::AnalogueCSVRecorder<scalar> cpu1Recorder("cpu1.csv", rCPU1, Parameters::numCPU1, "CPU1");
#endif  // RECORD_ELECTROPHYS
    
    // Wait for VICON system to track some objects
    while(vicon.getNumObjects() == 0) {
        std::this_thread::sleep_for(1s);
        std::cout << "Waiting for object..." << std::endl;
    }

    // Start capture
    if(!viconCaptureControl.startRecording("test")) {
        return EXIT_FAILURE;
    }

    std::cout << "Start VICON frame:" << vicon.getObjectData(0).getFrameNumber() << std::endl;

    // Loop until second joystick button is pressed
    bool outbound = true;
    unsigned int numTicks = 0;
    unsigned int numOverflowTicks = 0;
    int64_t totalMicroseconds = 0;
    for(;; numTicks++) {
        // Record time at start of tick
        const auto tickStartTime = std::chrono::high_resolution_clock::now();
        
        // Read from joystick
        joystick.update();
        
        // Stop if 2nd button is pressed
        if(joystick.isDown(JButton::B)) {
            break;
        }
        
        // Read data from VICON system
        auto objectData = vicon.getObjectData(0);
        const auto &velocity = objectData.getVelocity();
        const auto &attitude = objectData.getAttitude();
        
        /*
         * Update TL input
         * **TODO**: We could update definitions.h etc. to use physical unit types,
         * and then this would all gel together nicely.
         */
        headingAngleTL = -attitude[2].value();
        if(headingAngleTL < 0.0) {
            headingAngleTL = (2.0 * Parameters::pi) + headingAngleTL;
        }

        // Project velocity onto each TN2 cell's preferred angle and use as speed input
        for(unsigned int j = 0; j < Parameters::numTN2; j++) {
            speedTN2[j] = (sin(headingAngleTL + preferredAngleTN2[j]) * speedScale * velocity[0].value()) +
                (cos(headingAngleTL + preferredAngleTN2[j]) * speedScale * velocity[1].value());
        }

        if(numTicks % 100 == 0) {
            std::cout <<  "Ticks:" << numTicks << ", Heading: " << headingAngleTL << ", Speed: (" << speedTN2[0] << ", " << speedTN2[1] << ")" << std::endl;
        }

        // Step network
        stepTimeCPU();

#ifdef RECORD_ELECTROPHYS
        tn2Recorder.record(numTicks);
        cl1Recorder.record(numTicks);
        tb1Recorder.record(numTicks);
        cpu4Recorder.record(numTicks);
        cpu1Recorder.record(numTicks);
#endif  // RECORD_ELECTROPHYS

        // If we are going outbound
        if(outbound) {
            // Use joystick to drive motor
            motor.drive(joystick, RobotParameters::joystickDeadzone);
            
            // If first button is pressed switch to returning home
            if(joystick.isDown(JButton::A)) {
                std::cout << "Max CPU4 level r=" << *std::max_element(&rCPU4[0], &rCPU4[Parameters::numCPU4]) << ", i=" << *std::max_element(&iCPU4[0], &iCPU4[Parameters::numCPU4]) << std::endl;
                std::cout << "Returning home!" << std::endl;
                std::cout << "Turn around VICON frame:" << objectData.getFrameNumber() << std::endl;
                outbound = false;
            }
        }
        // Otherwise we're returning home so use CPU1 neurons to drive motor
        else {
            driveMotorFromCPU1(motor, (numTicks % 100) == 0);
        }
        
        // Record time at end of tick
        const auto tickEndTime = std::chrono::high_resolution_clock::now();
        
        // Calculate tick duration (in microseconds)
        const int64_t tickMicroseconds = std::chrono::duration_cast<chrono::microseconds>(tickEndTime - tickStartTime).count();
        
        // Add to total
        totalMicroseconds += tickMicroseconds;
        
        // If there is time left in tick, sleep for remainder
        if(tickMicroseconds < RobotParameters::targetTickMicroseconds) {
            std::this_thread::sleep_for(std::chrono::microseconds(RobotParameters::targetTickMicroseconds - tickMicroseconds));
        }
        // Otherwise, increment overflow counter
        else {
            numOverflowTicks++;
        }
    }
    
    // Show overflow stats
    std::cout << numOverflowTicks << "/" << numTicks << " ticks overflowed, mean tick time: " << (double)totalMicroseconds / (double)numTicks << "uS" << std::endl;

    // Stop motor
    motor.tank(0.0f, 0.0f);
    
    // Stop capture
    if(!viconCaptureControl.stopRecording("test")) {
        return EXIT_FAILURE;
    }

    // Exit
    return EXIT_SUCCESS;
}
