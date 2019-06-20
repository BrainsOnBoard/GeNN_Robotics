#pragma once

// BoB robotics includes
#include "common/pose.h"
#include "common/stopwatch.h"
#include "robots/ackermann.h"

// Third-party includes
#include "third_party/units.h"

namespace BoBRobotics {
namespace Robots {
using namespace units::literals;

class SimulatedAckermann
  : public Ackermann
{
    using meter_t = units::length::meter_t;
    using meters_per_second_t = units::velocity::meters_per_second_t;
    using degree_t = units::angle::degree_t;
    using second_t = units::time::second_t;

public:
    SimulatedAckermann(const meters_per_second_t maximumSpeed,
                       const meter_t axisDist,
                       const meter_t carHeight = 0_m,
                       const degree_t maxTurn = 45_deg);

    meter_t getDistanceBetweenAxis() const;

    template<typename LengthUnit = meter_t>
    Vector3<LengthUnit> getPosition()
    {
        updatePose();
        return { m_Pose.x(), m_Pose.y(), m_Pose.z() };
    }

    template<typename AngleUnit = degree_t>
    std::array<AngleUnit, 3> getAttitude()
    {
        updatePose();
        return { m_Pose.yaw(), 0_rad, 0_rad };
    }

    const Pose3<meter_t, degree_t> &getPose();
    meters_per_second_t getAbsoluteMaximumSpeed() const;
    void setPose(const Pose3<meter_t, degree_t> &pose);

    // Public virtual methods
    virtual degree_t getMaximumTurn() const override;
    virtual void moveForward(float speed) override;
    virtual void steer(float value) override;
    virtual void steer(degree_t steeringAngle) override;
    virtual void stopMoving() override;

    //! sets the robot velocity and steering to move
    virtual void move(meters_per_second_t velocity, degree_t steeringAngle) override;

private:
    Pose3<meter_t, degree_t> m_Pose;
    Stopwatch m_MoveStopwatch;
    const meters_per_second_t m_MaximumSpeed;
    const degree_t m_MaximumTurn;
    const meter_t m_distanceBetweenAxis; // distance between front and rear axis
    degree_t m_steeringWheelAngle{};     // steering wheel angle
    meters_per_second_t m_currentVelocity{};

    void updatePose();
}; // SimulatedAckermann
} // Robots
} // BoBRobotics

