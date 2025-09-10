#include "firstApp.hpp"

void vve::FirstApp::run()
{
    while (!_window.shouldClose())
    {
        glfwPollEvents();
    }
}