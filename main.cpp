#include <Application.h>
#include <cstdlib>
#include <exception>
#include <iostream>
int main(void) {
    try {
	Application app(1920, 1080);
	app.Run();
    } catch (const std::exception &e) {
	std::cerr << "\nError: " << e.what() << std::endl;
	return EXIT_FAILURE;
    } catch(...) {
	std::cerr << "Caught unknown exception!" << std::endl;
	return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
