#include <Application.h>
#include <cstdlib>
#include <exception>
#include <iostream>
int main(void) {
    try {
        Application app(1920, 1080);
        app.Run();
    } catch (const std::exception &e) {
        log::error("\nError: {}", e.what());
        return EXIT_FAILURE;
    } catch (...) {
        log::error("Caught unknown exception!");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
