#include <Application.h>
#include <exception>
int main() {
    try {
        Application app(1920, 1080);
        app.Run();
    } catch (const std::exception &e) {
        log::error("\nError: {}", e.what());
        return 1;
    } catch (...) {
        log::error("Caught unknown exception!");
        return 1;
    }
    return 0;
}
