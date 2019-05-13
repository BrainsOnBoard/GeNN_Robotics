#pragma once

// BoB robotics includes
#include "common/circstat.h"
#include "common/logging.h"
#include "common/macros.h"
#include "common/pose.h"
#include "common/stopwatch.h"
#include "os/net.h"

// Standard C includes
#include <cstring>

// Standard C++ includes
#include <algorithm>
#include <array>
#include <atomic>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace BoBRobotics
{
namespace Vicon
{
using namespace std::literals;
using namespace units::literals;

//----------------------------------------------------------------------------
// Vicon::ObjectData
//----------------------------------------------------------------------------
//! Simplest object data class - just tracks position and attitude
class ObjectData
{
    using radian_t = units::angle::radian_t;
    using millimeter_t = units::length::millimeter_t;

public:
    ObjectData(const std::string &objectName)
      : m_Name(objectName)
    {}

    //----------------------------------------------------------------------------
    // Public API
    //----------------------------------------------------------------------------
    void update(uint32_t frameNumber,
                Pose3<millimeter_t, radian_t> pose);

    uint32_t getFrameNumber() const;

    template<typename LengthUnit = millimeter_t>
    Vector3<LengthUnit> getPosition() const
    {
        return m_Pose.position();
    }

    template<typename AngleUnit = radian_t>
    std::array<AngleUnit, 3> getAttitude() const
    {
        return convertUnitArray<AngleUnit>(m_Pose.attitude());
    }

    template<typename LengthUnit = millimeter_t, typename AngleUnit = radian_t>
    Pose3<LengthUnit, AngleUnit> getPose() const
    {
        return m_Pose;
    }

    const std::string &getName() const;
    Stopwatch::Duration timeSinceReceived() const;

private:
    //----------------------------------------------------------------------------
    // Members
    //----------------------------------------------------------------------------
    uint32_t m_FrameNumber = 0;
    const std::string &m_Name;
    Pose3<millimeter_t, radian_t> m_Pose;
    Stopwatch m_ReceivedTimer;
};

// Forward declaration
template<typename ObjectDataType>
class UDPClient;

//----------------------------------------------------------------------------
// Vicon::ObjectDataVelocity
//----------------------------------------------------------------------------
//! Object data class which also calculate (un-filtered) velocity
class ObjectDataVelocity : public ObjectData
{
    using radian_t = units::angle::radian_t;
    using radians_per_second_t = units::angular_velocity::radians_per_second_t;
    using meters_per_second_t = units::velocity::meters_per_second_t;
    using millimeter_t = units::length::millimeter_t;
    using second_t = units::time::millisecond_t;

public:
    ObjectDataVelocity(const std::string &name)
      : ObjectData(name)
    {}

    //----------------------------------------------------------------------------
    // Public API
    //----------------------------------------------------------------------------
    void update(uint32_t frameNumber,
                Pose3<millimeter_t, radian_t> pose);

    template<typename VelocityUnit = meters_per_second_t>
    std::array<VelocityUnit, 3> getVelocity() const
    {
        return convertUnitArray<VelocityUnit>(m_Velocity);
    }

    template<typename AngularVelocityUnit = radians_per_second_t>
    std::array<AngularVelocityUnit, 3> getAngularVelocity() const
    {
        return convertUnitArray<AngularVelocityUnit>(m_AngularVelocity);
    }

private:
    //----------------------------------------------------------------------------
    // Members
    //----------------------------------------------------------------------------
    std::array<meters_per_second_t, 3> m_Velocity{};
    std::array<radians_per_second_t, 3> m_AngularVelocity{};

    template<typename VectorType, typename VelocityType, typename DiffFunc>
    static void calculateVelocities(const VectorType &current,
                                    const VectorType &old,
                                    std::array<VelocityType, 3> &velocityOut,
                                    const second_t &deltaS,
                                    const double alpha,
                                    DiffFunc diff)
    {
        // Temporary array
        std::array<VelocityType, 3> instVelocity;

        // Calculate instantaneous velocity
        const auto calcVelocity = [deltaS, diff](auto curr, auto prev) {
            return diff(curr, prev) / deltaS;
        };
        std::transform(std::cbegin(current), std::cend(current),
                       std::cbegin(old), std::begin(instVelocity),
                       calcVelocity);

        // Exponentially smooth velocity
        const auto smoothVelocity = [alpha](auto inst, auto prev) {
            return (alpha * inst) + ((1.0 - alpha) * prev);
        };
        std::transform(std::cbegin(instVelocity), std::cend(instVelocity),
                       std::cbegin(velocityOut), std::begin(velocityOut),
                       smoothVelocity);
    }
};

class TimedOutError
  : public std::runtime_error
{
public:
    TimedOutError();
};

//----------------------------------------------------------------------------
// BoBRobotics::Vicon::ObjectReference
//----------------------------------------------------------------------------
/**!
 *  \brief Holds a reference to a Vicon object
 *
 * The pose data is updated every time it is read, in contrast to ObjectData,
 * which only contains static data.
 */
template<typename ObjectDataType = ObjectData>
class ObjectReference
{
    using millimeter_t = units::length::millimeter_t;
    using radian_t = units::angle::radian_t;

public:
    ObjectReference(UDPClient<ObjectDataType> &client,
                    const std::string &objectName,
                    const Stopwatch::Duration timeoutDuration)
      : m_Client(client)
      , m_Name(objectName)
      , m_TimeoutDuration(timeoutDuration)
    {}

    template<typename LengthUnit = millimeter_t>
    Vector3<LengthUnit> getPosition() const
    {
        return getData().template getPosition<LengthUnit>();
    }

    template<typename AngleUnit = radian_t>
    std::array<AngleUnit, 3> getAttitude() const
    {
        return getData().template getAttitude<AngleUnit>();
    }

    template<typename LengthUnit = millimeter_t, typename AngleUnit = radian_t>
    auto getPose() const
    {
        const auto data = getData();
        return Pose3<LengthUnit, AngleUnit>(data.template getPosition<LengthUnit>(),
                                            data.template getAttitude<AngleUnit>());
    }

    const std::string &getName() const { return m_Name; }

    auto timeSinceReceived() const
    {
        return getData().timeSinceReceived();
    }

    ObjectDataType getData() const
    {
        const auto objectData = m_Client.getObjectData(m_Name);
        if (objectData.timeSinceReceived() > m_TimeoutDuration) {
            throw TimedOutError();
        }
        return objectData;
    }

private:
    UDPClient<ObjectDataType> &m_Client;
    const std::string m_Name;
    const Stopwatch::Duration m_TimeoutDuration;
};

//----------------------------------------------------------------------------
// BoBRobotics::Vicon::UDPClient
//----------------------------------------------------------------------------
//! Receiver for Vicon UDP streams
template<typename ObjectDataType = ObjectData>
class UDPClient
{
private:
    /*
     * NB: The first member of this struct *should* be object ID, but the
     * Vicon system seems to just give a value of zero for every item, which
     * isn't terribly helpful. So instead we distinguish objects based on their
     * names.
     */
    BOB_PACKED(struct RawObjectData {
        uint8_t unused;
        uint16_t itemDataSize;
        char objectName[24];
        double position[3];
        double attitude[3];
    });

public:
    UDPClient() = default;
    UDPClient(uint16_t port)
    {
        connect(port);
    }

    virtual ~UDPClient()
    {
        if (m_ReadThread.joinable()) {
            // Set quit flag
            m_ShouldQuit = true;

            // Wait for thread to finish
            m_ReadThread.join();
        }
    }

    //----------------------------------------------------------------------------
    // Public API
    //----------------------------------------------------------------------------
    void connect(uint16_t port)
    {
        // Lock until we have at least one packet from Vicon system
        m_ConnectionMutex.lock();

        // Create socket
        int socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if(socket < 0) {
            throw OS::Net::NetworkError("Cannot open socket");
        }

        // Set socket to have 1s read timeout
        // **NOTE** this is largely to allow read thread to be stopped
#ifdef _WIN32
        DWORD timeout = 1000;
#else
        timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
#endif
        if(setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0) {
            throw OS::Net::NetworkError("Cannot set socket timeout");
        }

        // Create socket address structure
        sockaddr_in localAddress;
        memset(&localAddress, 0, sizeof(sockaddr_in));
        localAddress.sin_family = AF_INET;
        localAddress.sin_port = htons(port);
        localAddress.sin_addr.s_addr = htonl(INADDR_ANY);

        // Bind socket to local port
        if(bind(socket, reinterpret_cast<sockaddr*>(&localAddress), sizeof(localAddress)) < 0) {
            throw OS::Net::NetworkError("Cannot bind socket");
        }

        // Clear atomic stop flag and start thread
        m_ShouldQuit = false;
        m_ReadThread = std::thread(&UDPClient::readThread, this, socket);
    }

    size_t getNumObjects()
    {
        waitUntilConnected();
        std::lock_guard<std::mutex> guard(m_ObjectMutex);
        return m_ObjectData.size();
    }

    //! Get current pose information for specified object
    ObjectDataType getObjectData(const std::string &name)
    {
        waitUntilConnected();
        std::lock_guard<std::mutex> guard(m_ObjectMutex);
        return m_ObjectData.at(name);
    }

     //! Get current pose information for first object
     ObjectDataType getObjectData()
     {
         waitUntilConnected();
         std::lock_guard<std::mutex> guard(m_ObjectMutex);
         return m_ObjectData.begin()->second;
     }

     auto getObjectReference(Stopwatch::Duration timeoutDuration = 10s) const
     {
         waitUntilConnected();
         std::lock_guard<std::mutex> guard(m_ObjectMutex);
         return ObjectReference<ObjectDataType>(*this,
                                                m_ObjectData.begin()->first,
                                                timeoutDuration);
     }

    //! Returns an object whose pose is updated by the Vicon system over time
    auto getObjectReference(const std::string& name,
                            Stopwatch::Duration timeoutDuration = 10s) const
    {
        return ObjectReference<ObjectDataType>(*this,
                                               name,
                                               timeoutDuration);
    }

    bool connected() const { return m_IsConnected; }

private:
    //----------------------------------------------------------------------------
    // Private API
    //----------------------------------------------------------------------------
    void updateObjectData(uint32_t frameNumber, const RawObjectData &data)
    {
        // Check if we already have a data structure for this object...
        const std::string objectName = data.objectName;
        const auto pos = m_ObjectData.find(objectName);

        // ...and, if not, create one
        if (pos == m_ObjectData.cend()) {
            LOGI << "Vicon: Found new object: " << objectName;
            m_ObjectData.emplace(objectName, ObjectDataType{ objectName });
        }

        /*
         * Update object data with position and attitude.
         *
         * Note that we reorder the rotation angles we get from the Vicon system
         * so that they are in the order of yaw, pitch and roll (which seems to
         * be standard).
         */
        using namespace units::length;
        using namespace units::angle;
        m_ObjectData.at(objectName).update(frameNumber,
                                           { { millimeter_t(data.position[0]),
                                               millimeter_t(data.position[1]),
                                               millimeter_t(data.position[2]) },
                                           { radian_t(data.attitude[2]),
                                               radian_t(data.attitude[0]),
                                               radian_t(data.attitude[1]) } });
    }

    void readThread(int socket)
    {
        // Create buffer for reading data
        // **NOTE** this is the maximum size supported by Vicon so will support all payload sizes
        uint8_t buffer[1024];

        // Loop until quit flag is set
        while (!m_ShouldQuit) {
            // Read datagram
            const ssize_t bytesReceived = recvfrom(socket, &buffer[0], 1024,
                                                   0, nullptr, nullptr);

            // If there was an error
            if (bytesReceived == -1) {
                // If this was a timeout, continue
                if(errno == EAGAIN || errno == EINTR) {
                    continue;
                }
                // Otherwise, display error and stop
                else {
                    throw OS::Net::NetworkError("Cannot read datagram");
                }
            }
            // Otherwise, if data was received
            else {
                // Read frame number
                uint32_t frameNumber;
                memcpy(&frameNumber, &buffer[0], sizeof(uint32_t));

                // Read items in block
                const size_t itemsInBlock = (size_t) buffer[4];

                // Lock mutex
                std::lock_guard<std::mutex> guard(m_ObjectMutex);

                const auto objectData = reinterpret_cast<RawObjectData *>(&buffer[5]);
                std::for_each(objectData, &objectData[itemsInBlock], [&frameNumber, this](auto &data) {
                    BOB_ASSERT(data.itemDataSize == 72);
                    updateObjectData(frameNumber, data);
                });

                // If this is the first packet we've received, signal that we're connected
                if (!m_IsConnected) {
                    m_IsConnected = true;
                    m_ConnectionMutex.unlock();
                }
            }
        }

        // Close socket
        close(socket);
    }

    void waitUntilConnected()
    {
        /*
         * The Vicon system transmits packets at a high frequency, so if we don't
         * receive data almost immediately then we're not connected.
         */
        if (!m_IsConnected && !m_ConnectionMutex.try_lock_for(1s)) {
            throw std::runtime_error("Could not connect to Vicon system");
        }
    }

    //----------------------------------------------------------------------------
    // Members
    //----------------------------------------------------------------------------
    std::mutex m_ObjectMutex;
    std::unordered_map<std::string, ObjectDataType> m_ObjectData;
    std::atomic<bool> m_ShouldQuit;
    std::timed_mutex m_ConnectionMutex;
    bool m_IsConnected = false;
    std::thread m_ReadThread;
};
} // Vicon
} // BoBRobotics
