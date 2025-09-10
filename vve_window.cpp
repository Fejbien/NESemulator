#include "vve_window.hpp"

void vve::VveWindow::_initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _window = glfwCreateWindow(_width, _height, _windowName.c_str(), nullptr, nullptr);
}

vve::VveWindow::VveWindow(int width, int height, const std::string &windowName) : _width(width), _height(height), _windowName(windowName)
{
    _initWindow();
}

vve::VveWindow::~VveWindow()
{
    glfwDestroyWindow(_window);
    glfwTerminate();
}

bool vve::VveWindow::shouldClose()
{
    return glfwWindowShouldClose(_window);
}
