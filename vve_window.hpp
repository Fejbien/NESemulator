#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace vve
{

    class VveWindow
    {
    private:
        GLFWwindow *_window;
        int _width;
        int _height;
        std::string _windowName;

        void _initWindow();

    public:
        VveWindow(int width, int height, const std::string &windowName);
        ~VveWindow();

        bool shouldClose();
    };

} // namespace vve