#pragma once

// BoB robotics includes
#include "input.h"
#include "os/net.h"

// OpenCV
#include <opencv2/opencv.hpp>

// Standard C includes
#include <cstdlib>

namespace BoBRobotics {
namespace Video {

class RPiCamera : public Input
{
public:
    RPiCamera();
    RPiCamera(uint16_t port = 0);

    virtual std::string getCameraName() const override;
    virtual cv::Size getOutputSize() const override;
    virtual bool readFrame(cv::Mat &outFrame) override;
    virtual bool needsUnwrapping() const override;
    virtual void setOutputSize(const cv::Size &) override;

private:
    void setupSockets();

    socket_t m_Socket;
    uint16_t m_Port;
}; // RPiCamera
} // Video
} // BoBRobotics
