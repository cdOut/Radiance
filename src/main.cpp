#include <Application.h>
#include <filesystem>

namespace fs = std::filesystem;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const char* WINDOW_TITLE = "raytracing-thesis";

int main(int argc, char** argv) {
    fs::current_path(fs::path(argv[0]).parent_path());
    try {
        Application app(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}