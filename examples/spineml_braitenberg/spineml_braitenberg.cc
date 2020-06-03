// BoB robotics includes
#include "common/circstat.h"
#include "hid/joystick.h"
#include "navigation/read_objects.h"
#include "robots/control/collision_detector.h"
#include "robots/simulated_tank.h"
#include "viz/sfml/arena_object.h"
#include "viz/sfml/sfml_world.h"

// SpineML simulator includes
#include "spineml/simulator/simulator.h"

// Third-party includes
#include "third_party/units.h"

// Standard C++ includes
#include <chrono>
#include <iostream>
#include <thread>

using namespace BoBRobotics;
using namespace std::literals;
using namespace units::angle;
using namespace units::length;
using namespace units::math;
using namespace units::literals;

int bobMain(int, char **)
{
    // The x and y dimensions of the robot
    // **THINK** if would be nice if this was part of a robot class?
    using V = Vector2<meter_t>;
    constexpr std::array<V, 4> robotDimensions = {
        V{ -75_mm, 70_mm},
        V{ 75_mm, 70_mm },
        V{ 75_mm, -120_mm },
        V{ -75_mm, -120_mm }
    };

    // Tank agent
    Robots::SimulatedTank<> robot(1.0_mps, 104_mm);

    // For displaying the agent
    Viz::SFMLWorld display({5_m, 5_m});
    auto car = display.createCarAgent();

    // Read objects from file
    const auto objects = Navigation::readObjects("objects.yaml");
    Robots::CollisionDetector collisionDetector{ robotDimensions, objects, 30_cm, 1_cm };

    // Object size + buffer around
    const auto &resizedObjects = collisionDetector.getResizedObjects();

    // Create drawable objects
    std::vector<Viz::ArenaObject> objectShapes;
    objectShapes.reserve(objects.size());
    for (size_t i = 0; i < objects.size(); i++) {
        objectShapes.emplace_back(display, objects[i], resizedObjects[i]);
    }

    // Create SpineML simulator object
    SpineMLSimulator::Simulator simulator("experiment1.xml", ".");

    // Get external loggers used for accessing steering signals generated by network
    const auto *steerLeft = simulator.getExternalLogger("steer left");
    const auto *steerRight = simulator.getExternalLogger("steer right");
    BOB_ASSERT(steerLeft != nullptr);
    BOB_ASSERT(steerRight != nullptr);
    BOB_ASSERT(steerLeft->getModelPropertySize() == 1);
    BOB_ASSERT(steerRight->getModelPropertySize() == 1);

    // Get external inputs used for providing sensor input to the network
    auto *sensorLeft = simulator.getExternalInput("sensor left");
    auto *sensorRight = simulator.getExternalInput("sensor right");
    BOB_ASSERT(sensorLeft != nullptr);
    BOB_ASSERT(sensorRight != nullptr);

    while(true) {
        // Move robot
        collisionDetector.setRobotPose(robot.getPose());

        // If robot has hit right wall
        // **TODO** do this more elegantly with err maths
        double hitLeft = 0.0;
        double hitRight = 0.0;
        Vector2<meter_t> collisionPosition;
        if(robot.getPose().x() > 2.5_m) {
            if(robot.getPose().yaw() > 0.0_deg) {
                hitRight += 1.0;
            }
            else {
                hitLeft += 1.0;
            }
        }
        // If robot has hit left wall
        if(robot.getPose().x() < -2.5_m) {
            if(robot.getPose().yaw() > 180.0_deg) {
                hitRight += 1.0;
            }
            else {
                hitLeft += 1.0;
            }
        }
        // If robot has hit bottom wall
        if(robot.getPose().y() < -2.5_m) {
            if(robot.getPose().yaw() > -90.0_deg) {
                hitRight += 1.0;
            }
            else {
                hitLeft += 1.0;
            }
        }
        // If robot has hit top wall
        if(robot.getPose().y() > 2.5_m) {
            if(robot.getPose().yaw() > 90.0_deg) {
                hitRight += 1.0;
            }
            else {
                hitLeft += 1.0;
            }
        }

        // If there's been a collision
        if (collisionDetector.collisionOccurred(collisionPosition)) {
            // Get angle of collision relative to robot
            const degree_t collisionAngle = atan2(collisionPosition.y() - robot.getPose().y(),
                                                  collisionPosition.x() - robot.getPose().x());

            // Get shortest angle between this and robot's heading
            const degree_t relativeCollisionAngle = circularDistance(collisionAngle, robot.getPose().yaw());

            // Based on this, determine which way to steer
            if(relativeCollisionAngle > 0_deg) {
                hitLeft += 1.0;
            }
            else {
                hitRight += 1.0;
            }
        }

        // Copy turn signals into sensor input buffers
        std::fill(sensorLeft->getBufferBegin(), sensorLeft->getBufferEnd(), hitLeft);
        std::fill(sensorRight->getBufferBegin(), sensorRight->getBufferEnd(), hitRight);

        // Advance SpineML simulation
        simulator.stepTime();

        // Read steering signals
        const float left = *steerLeft->getStateVarBegin();
        const float right = *steerRight->getStateVarBegin();

        // Generate tank steering signals
        const float tankLeft = std::min(1.0f, std::max(-1.0f, 1.0f + right - left));
        const float tankRight = std::min(1.0f, std::max(-1.0f, 1.0f - right + left));

        // Drive robot
        robot.tank(tankLeft, tankRight);


        // Refresh display
        car.setPose(robot.getPose());
        display.update(objectShapes, car);

    }

    return EXIT_SUCCESS;
}
