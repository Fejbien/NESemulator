#include "vve_window.hpp"

namespace vve
{
    class FirstApp
    {
    private:
        VveWindow _window{WIDTH, HEIGHT, WINDOW_NAME};

    public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;
        static constexpr const char *WINDOW_NAME = "NES";

        void run();
    };
}