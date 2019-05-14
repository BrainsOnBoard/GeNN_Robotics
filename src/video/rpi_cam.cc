// BoB robotics includes
#include "common/macros.h"
#include "video/rpi_cam.h"

// Standard C includes
#include <cstring>

// Standard C++ includes
#include <stdexcept>

// Extra POSIX includes
#ifdef unix
#include <fcntl.h>
#endif

namespace BoBRobotics {
namespace Video {

RPiCamera::RPiCamera(uint16_t port)
  : m_Frame(72, 152, CV_8UC3)
  , m_Socket(INVALID_SOCKET)
{
    m_Port = port;

    // set up the networking code needed to receive images
    setupSockets();
}

std::string
RPiCamera::getCameraName() const
{
    return "Raspberry Pi Camera";
}

cv::Size
RPiCamera::getOutputSize() const
{
    // this is fixed
    return cv::Size(152, 72);
}

bool
RPiCamera::readFrame(cv::Mat &outFrame)
{
    if (!readGreyscaleFrame(m_Frame)) {
        return false;
    }

    // Make sure output frame is the right size and type
    outFrame.create(72, 152, CV_8UC3);

    // Convert to the correct cv::Mat type
    m_Frame.convertTo(outFrame, CV_8UC3);
    return true;
}

bool
RPiCamera::readGreyscaleFrame(cv::Mat &outFrame)
{
    unsigned char buffer[72 * 19];

    // Check we're connected
    BOB_ASSERT(m_Socket != INVALID_SOCKET);

    // Make sure output frame is the right size and type
    outFrame.create(72, 152, CV_8U);

    // get the most recent UDP frame (grayscale for now)
    while (recv(m_Socket, buffer, 72 * 19, 0) > 0) {
        /*
         * Fill in the outFrame.
         *
         * NB: We might be able to std::copy the memory from the buffer, but I
         * won't do this just yet in case it inverts the rows and columns or
         * something... -- AD
         */
        for (int i = 0; i < 72 * 19 - 1; ++i) {
            outFrame.at<uchar>(i % 72, buffer[0] + floor(i / 72)) = buffer[i];
        }
    }

    return true;
}

bool
RPiCamera::needsUnwrapping() const
{
    // we do not need to unwrap this - it is done onboard the RPi
    return false;
}

void
RPiCamera::setOutputSize(const cv::Size &)
{
    throw std::runtime_error("This camera's resolution cannot be changed");
}

void
RPiCamera::setupSockets()
{
    struct sockaddr_in addr;

    m_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_Socket == INVALID_SOCKET) {
        throw OS::Net::NetworkError("Could not create socket");
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(m_Port);

    if (bind(m_Socket, (const sockaddr *) &addr, (int) sizeof(addr))) {
        throw OS::Net::NetworkError("Could not bind to socket");
    }

    // non-blocking socket
#ifdef WIN32
    ulong nonblocking_enabled = 1;
    ioctlsocket(m_Socket, FIONBIO, &nonblocking_enabled);
#else
    fcntl(m_Socket, F_SETFL, O_NONBLOCK);
#endif
}

} // Video
} // BoBRobotics
