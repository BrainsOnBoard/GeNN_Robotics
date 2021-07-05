// BoB robotics includes
#include "antworld/camera.h"

// Third-party includes
#include "plog/Log.h"

void
handleGLError(GLenum, GLenum, GLuint, GLenum severity, GLsizei, const GLchar *message, const void *)
{
    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        LOGE << message;
    }
    else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
        LOGW << message;
    }
    else if (severity == GL_DEBUG_SEVERITY_LOW) {
        LOGI << message;
    }
    else {
        LOGD << message;
    }
}

namespace BoBRobotics {
namespace AntWorld {

Camera::Camera(sf::Window &window, Renderer &renderer, const cv::Size &renderSize)
  : Video::OpenGL(renderSize)
  , m_Window(window)
  , m_Renderer(renderer)
{}

Pose3<units::length::meter_t, units::angle::degree_t>
Camera::getPose() const
{
    return m_Pose;
}

sf::Window &
Camera::getWindow() const
{
    return m_Window;
}

void
Camera::setPose(const Pose3<meter_t, degree_t> &pose)
{
    m_Pose = pose;
}

void
Camera::setPosition(meter_t x, meter_t y, meter_t z)
{
    m_Pose.position() = { x, y, z };
}

void
Camera::setAttitude(degree_t yaw, degree_t pitch, degree_t roll)
{
    m_Pose.attitude() = { yaw, pitch, roll };
}

bool
Camera::readFrame(cv::Mat &frame)
{
    // Render
    update();

    // Read frame (they're always synchronous for Video::OpenGL anyway)
    Video::OpenGL::readFrame(frame);

    // Swap buffers
    m_Window.display();

    return true;
}

void
Camera::display()
{
    // Render
    update();

    // Swap buffers
    m_Window.display();
}

void
Camera::update()
{
    // Render to m_Window
    m_Window.setActive(true);

    // Clear colour and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render first person
    const auto size = getOutputSize();
    m_Renderer.renderPanoramicView(m_Pose.x(), m_Pose.y(), m_Pose.z(), m_Pose.yaw(), m_Pose.pitch(), m_Pose.roll(), 0, 0, size.width, size.height);
}

bool
Camera::isOpen() const
{
    return m_Window.isOpen();
}

std::unique_ptr<sf::Window>
Camera::initialiseWindow(const cv::Size &size)
{
    // Create SFML window
    auto window = std::make_unique<sf::Window>(sf::VideoMode(size.width, size.height),
                                               "Ant world",
                                               sf::Style::Titlebar | sf::Style::Close);

    // Enable VSync
    window->setVerticalSyncEnabled(true);
    window->setActive(true);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    glDebugMessageCallback(handleGLError, nullptr);

    // Set clear colour to match matlab and enable depth test
    glClearColor(0.75f, 0.75f, 0.75f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(4.0);
    glPointSize(4.0);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_TEXTURE_2D);

    return window;
}

} // AntWorld
} // BoBRobotics
