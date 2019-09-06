// BoB robotics includes
#include "common/logging.h"
#include "hid/joystick.h"
#include "hid/joystick_glfw_keyboard.h"
#include "antworld/common.h"
#include "antworld/renderer.h"
#include "antworld/render_target_hex_display.h"

// Third-party includes
#include "third_party/path.h"
#include "third_party/units.h"

// OpenGL includes
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// Standard C++ includes
#include <cstring>
#include <memory>

using namespace BoBRobotics;
using namespace units::angle;
using namespace units::length;
using namespace units::math;
using namespace units::time;

// Anonymous namespace
namespace
{
void handleGLFWError(int errorNumber, const char *message)
{
    LOGE << "GLFW error number:" << errorNumber << ", message:" << message;
}

void handleGLError(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar *message,
                   const void *)
{
    throw std::runtime_error(message);
}

inline second_t getCurrentTime()
{
    return units::make_unit<second_t>(glfwGetTime());
}

std::unique_ptr<HID::JoystickBase<HID::JAxis, HID::JButton>> createJoystick(GLFWwindow *window)
{
    try
    {
        return std::make_unique<HID::Joystick>(0.25f);
    }
    catch(std::runtime_error &)
    {
        return std::make_unique<HID::JoystickGLFWKeyboard>(window);
    }
}
}

int main(int argc, char **argv)
{
    const auto turnSpeed = 200_deg_per_s;
    const auto moveSpeed = 3_mps;
    const unsigned int width = 1024;
    const unsigned int height = 262;

    // Whether to use the 3D reconstructed Rothamsted model
    const bool useRothamstedModel = argc > 1 && strcmp(argv[1], "--rothamsted") == 0;

    // Set GLFW error callback
    glfwSetErrorCallback(handleGLFWError);

    // Initialize the library
    if(!glfwInit()) {
        LOGE << "Failed to initialize GLFW";
        return EXIT_FAILURE;
    }

    // Prevent window being resized
    glfwWindowHint(GLFW_RESIZABLE, false);

    // Create a windowed mode window and its OpenGL context
    GLFWwindow *window = glfwCreateWindow(width, height, "Ant world", nullptr, nullptr);
    if(!window)
    {
        glfwTerminate();
        LOGE << "Failed to create window";
        return EXIT_FAILURE;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if(glewInit() != GLEW_OK) {
        LOGE << "Failed to initialize GLEW";
        return EXIT_FAILURE;
    }

    // Enable VSync
    glfwSwapInterval(1);

    glDebugMessageCallback(handleGLError, nullptr);

    // Set clear colour to match matlab and enable depth test
    //glClearColor(0.75f, 0.75f, 0.75f, 1.0f);
    glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(4.0);
    glPointSize(4.0);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_TEXTURE_2D);

    // Create renderer - increasing cubemap size to improve quality in larger window
    // and pushing back clipping plane to reduce Z fighting
    //AntWorld::Renderer renderer(std::make_unique<AntWorld::RenderMeshHexagonal>(150_deg, 75_deg, 3.7_deg),
    //                            256, 0.1);

    AntWorld::Renderer renderer(512, 0.1);
    // Create a render target for displaying world re-mapped onto hexagonal mesh
    //AntWorld::RenderTargetHexDisplay renderTarget(*dynamic_cast<const AntWorld::RenderMeshHexagonal*>(renderer.getRenderMesh()));

    //AntWorld::Renderer renderer(512, 0.1);
    if (useRothamstedModel) {
        const char *modelPath = std::getenv("ROTHAMSTED_3D_MODEL_PATH");
        if (!modelPath) {
            throw std::runtime_error("Error: ROTHAMSTED_3D_MODEL_PATH env var is not set");
        }
        renderer.getWorld().loadObj((filesystem::path(modelPath).parent_path() / "flight_1_decimate.obj").str(),
                                    0.1f,
                                    4096,
                                    GL_COMPRESSED_RGB);
    } else {
        renderer.getWorld().load(filesystem::path(argv[0]).parent_path() / "../../resources/antworld/world5000_gray.bin",
                                 { 0.0f, 1.0f, 0.0f },
                                 { 0.898f, 0.718f, 0.353f });
    }

    // Load world, keeping texture sizes below 4096 and compressing textures on upload
    //renderer.getWorld().loadObj("object.obj",
    //                            0.1f, 4096, GL_COMPRESSED_RGB);


    // Create HID device for controlling movement
    //HID::Joystick joystick(0.25f);
    auto joystick = createJoystick(window);

    // Get world bounds and initially centre agent in world
    const auto &worldMin = renderer.getWorld().getMinBound();
    const auto &worldMax = renderer.getWorld().getMaxBound();
    meter_t x = worldMin[0] + (worldMax[0] - worldMin[0]) / 2.0;
    meter_t y = worldMin[1] + (worldMax[1] - worldMin[1]) / 2.0;
    meter_t z = worldMin[2] + (worldMax[2] - worldMin[2]) / 2.0;
    degree_t yaw = 0_deg;
    degree_t pitch = 0_deg;

    second_t lastTime = getCurrentTime();
    while (!glfwWindowShouldClose(window)) {
        // Poll joystick
        joystick->update();

        // Calculate time
        const second_t currentTime = getCurrentTime();
        const second_t deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        char buffer[100];
        sprintf(buffer, "%d FPS", (int)std::round(1.0 / deltaTime.value()));
        glfwSetWindowTitle(window, buffer);

        // Control yaw and pitch with left stick
        yaw += joystick->getState(HID::JAxis::LeftStickHorizontal) * deltaTime * turnSpeed;
        pitch += joystick->getState(HID::JAxis::LeftStickVertical) * deltaTime * turnSpeed;

        // Use right trigger to control forward movement speed
        const meter_t forwardMove = moveSpeed * deltaTime * joystick->getState(HID::JAxis::RightTrigger);

        // Calculate movement delta in 3D space
        auto cosPitch = cos(pitch);
        x += forwardMove * sin(yaw) * cosPitch;
        y += forwardMove * cos(yaw) * cosPitch;
        z -= forwardMove * sin(pitch);

        // Clear colour and depth buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer.renderPanoramicView(x, y, z, yaw, pitch, 0_deg,
                                     0, 0, width, height);
        // Render panorama to render target
        /*renderer.renderPanoramicView(x, y, z, yaw, pitch, 0_deg,
                                     renderTarget);

        // Clear colour and depth buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render render mesh
        renderTarget.render(0, 0, width, height);*/

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }
    return EXIT_SUCCESS;
}
