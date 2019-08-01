#pragma once

// BoB robotics includes
#include "hid/joystick.h"
#include "net/connection.h"
#include "robot.h"

namespace BoBRobotics {
namespace Robots {
//----------------------------------------------------------------------------
// BoBRobotics::Robots::Omni2D
//----------------------------------------------------------------------------
// Interface for driving Omni2D-like wheeled robots
class Omni2D : public Robot
{
public:
    //------------------------------------------------------------------------
    // Declared virtuals
    //------------------------------------------------------------------------
    virtual void omni2D(float forward, float sideways, float turn);

    //------------------------------------------------------------------------
    // Robot virtuals
    //------------------------------------------------------------------------
    /**!
     * \brief Move forward at the specified relative speed
     *
     * Values must be between -1 and 1 inclusive.
     */
    virtual void moveForward(float speed);

    /**!
     * \brief Stop moving forward and start turning at the specified relative speed
     *
     * Values must be between -1 and 1 inclusive.
     */
    virtual void turnOnTheSpot(float clockwiseSpeed);

    //! Stop the robot moving
    virtual void stopMoving();

    //------------------------------------------------------------------------
    // Public API
    //------------------------------------------------------------------------
    void addJoystick(HID::Joystick &joystick, float deadZone = 0.25f);
    void drive(const HID::Joystick &joystick, float deadZone = 0.25f);
    void readFromNetwork(Net::Connection &connection);

private:
    float m_X = 0;
    float m_Y = 0;
    float m_R = 0;

    void drive(float x, float y, float rot, float deadZone);
    void onCommandReceived(Net::Connection &, const Net::Command &command);
    bool onJoystickEvent(HID::JAxis axis, float value, float deadZone);
}; // Omni2D
} // Robots
} // BoBRobotics