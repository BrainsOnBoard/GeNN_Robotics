#pragma once

// BoB robotics includes
#include "../net/connection.h"
#include "tank.h"

// Third-party includes
#include "../third_party/units.h"

// Standard C includes
#include <cmath>

// Standard C++ includes
#include <limits>
#include <string>

namespace BoBRobotics {
namespace Robots {
//----------------------------------------------------------------------------
// BoBRobotics::Robots::TankNetSink
//----------------------------------------------------------------------------
//! An interface for transmitting tank steering commands over the network
class TankNetSink : public Tank
{
private:
    using degrees_per_second_t = units::angular_velocity::degrees_per_second_t;

    Net::Connection &m_Connection;
    degrees_per_second_t m_TurnSpeed{ std::numeric_limits<double>::quiet_NaN() };
    float m_OldLeft = 0, m_OldRight = 0;

public:
    TankNetSink(Net::Connection &connection)
      : m_Connection(connection)
    {
        connection.addCommandHandler("TRN", [this](Net::Connection &, const Net::Command &command) {
            m_TurnSpeed = degrees_per_second_t(stod(command[1]));
        });
    }

    virtual ~TankNetSink()
    {
        try {
            stopMoving();
        } catch (...) {
        }

        stopReadingFromNetwork();
    }

    //! Motor command: send TNK command over TCP
    virtual void tank(float left, float right) override
    {
        BOB_ASSERT(left >= -1.f && left <= 1.f);
        BOB_ASSERT(right >= -1.f && right <= 1.f);

        // don't send a command if it's the same as the last one
        if (left == m_OldLeft && right == m_OldRight) {
            return;
        }

        // send steering command
        m_Connection.getSocketWriter().send("TNK " + std::to_string(left) + " " +
                                            std::to_string(right) + "\n");

        // store current left/right values to compare next time
        m_OldLeft = left;
        m_OldRight = right;
    }

    virtual degrees_per_second_t getMaximumTurnSpeed() override
    {
        if (std::isnan(m_TurnSpeed.value())) {
            return Tank::getMaximumTurnSpeed();
        } else {
            return m_TurnSpeed;
        }
    }
}; // TankNetSink
} // Robots
} // BoBRobotics
