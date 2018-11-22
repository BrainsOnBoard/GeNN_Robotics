#pragma once

// Third-party includes
#include "../third_party/units.h"

// Standard C++ includes
#include <array>
#include <tuple>

namespace BoBRobotics {
//! A generic template for 2D unit arrays
template<typename T>
using Vector2 = std::array<T, 2>;

//! A generic template for 3D unit arrays
template<typename T>
using Vector3 = std::array<T, 3>;

//! A two-dimensional pose
template<typename LengthUnit, typename AngleUnit>
class Pose2
  : public std::tuple<Vector2<LengthUnit>, AngleUnit>
{
    static_assert(units::traits::is_length_unit<LengthUnit>::value,
                  "LengthUnit is not a unit of length");
    static_assert(units::traits::is_angle_unit<AngleUnit>::value,
                  "AngleUnit is not a unit of angle");

public:
    Pose2() = default;

    Pose2(LengthUnit x, LengthUnit y, AngleUnit angle)
      : std::tuple<Vector2<LengthUnit>, AngleUnit>({ x, y }, angle)
    {}

    template<typename LengthUnit2, typename AngleUnit2>
    operator Pose2<LengthUnit2, AngleUnit2>() const
    {
        return Pose2<LengthUnit2, AngleUnit2>{ x(), y(), yaw() };
    }

    Vector2<LengthUnit> &position() { return std::get<0>(*this); }
    const Vector2<LengthUnit> &position() const { return std::get<0>(*this); }
    LengthUnit &x() { return std::get<0>(*this)[0]; }
    const LengthUnit &x() const { return std::get<0>(*this)[0]; }
    LengthUnit &y() { return std::get<0>(*this)[1]; }
    const LengthUnit &y() const { return std::get<0>(*this)[1]; }
    static constexpr LengthUnit z() { return LengthUnit(0); }

    Vector3<AngleUnit> attitude() const { return { yaw(), AngleUnit(0), AngleUnit(0) }; }
    AngleUnit &yaw() { return std::get<1>(*this); }
    const AngleUnit &yaw() const { return std::get<1>(*this); }
    static constexpr AngleUnit pitch() { return AngleUnit(0); }
    static constexpr AngleUnit roll() { return AngleUnit(0); }
};

//! A three-dimensional pose
template<typename LengthUnit, typename AngleUnit>
class Pose3
  : public std::tuple<Vector3<LengthUnit>, Vector3<AngleUnit>>
{
    static_assert(units::traits::is_length_unit<LengthUnit>::value,
                  "LengthUnit is not a unit of length");
    static_assert(units::traits::is_angle_unit<AngleUnit>::value,
                  "AngleUnit is not a unit of angle");

public:
    Pose3() = default;

    Pose3(const Vector3<LengthUnit> &position, const Vector3<AngleUnit> &attitude)
      : std::tuple<Vector3<LengthUnit>, Vector3<AngleUnit>>(position, attitude)
    {}

    template<typename LengthUnit2, typename AngleUnit2>
    operator Pose2<LengthUnit2, AngleUnit2>() const
    {
        return Pose2<LengthUnit2, AngleUnit2>{ x(), y(), yaw() };
    }

    Vector3<LengthUnit> &position() { return std::get<0>(*this); }
    const Vector3<LengthUnit> &position() const { return std::get<0>(*this); }
    LengthUnit &x() { return std::get<0>(*this)[0]; }
    const LengthUnit &x() const { return std::get<0>(*this)[0]; }
    LengthUnit &y() { return std::get<0>(*this)[1]; }
    const LengthUnit &y() const { return std::get<0>(*this)[1]; }
    LengthUnit &z() { return std::get<0>(*this)[0]; }
    const LengthUnit &z() const { return std::get<0>(*this)[0]; }

    Vector3<AngleUnit> &attitude() { return std::get<1>(*this); }
    const Vector3<AngleUnit> &attitude() const { return std::get<1>(*this); }
    AngleUnit &yaw() { return std::get<1>(*this); }
    const AngleUnit &yaw() const { return std::get<1>(*this)[0]; }
    AngleUnit &pitch() { return std::get<1>(*this); }
    const AngleUnit &pitch() const { return std::get<1>(*this); }
    AngleUnit &roll() { return std::get<1>(*this); }
    const AngleUnit &roll() const { return std::get<1>(*this); }
};

//! Converts the input array to a unit-type of OutputUnit
template<typename OutputUnit, typename ArrayType>
inline constexpr Vector3<OutputUnit>
convertUnitArray(const ArrayType &values)
{
    return { static_cast<OutputUnit>(values[0]),
             static_cast<OutputUnit>(values[1]),
             static_cast<OutputUnit>(values[2]) };
}
} // BoBRobotics
