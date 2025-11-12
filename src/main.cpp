#include <Application.h>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const char* WINDOW_TITLE = "raytracing-thesis";

int main() {
    try {
        Application app(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}