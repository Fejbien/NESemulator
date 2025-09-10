#include <iostream>
#include <cstdlib>
#include <stdexcept>

#include "firstApp.hpp"

int main()
{
    vve::FirstApp app;

    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}