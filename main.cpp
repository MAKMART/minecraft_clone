#if defined (TRACY_ENABLE)
#include "tracy/Tracy.hpp"
#endif
#include <Application.h>
#include <cstdlib>
#include <new>
void* operator new(std::size_t size) {
	void* ptr = malloc(size);
	TracyAlloc(ptr, size);
	return ptr;
}
void operator delete(void* ptr) noexcept {
	TracyFree(ptr);
	free(ptr);
}
int main() {
	Application app(1920, 1080);
	app.Run();
}
