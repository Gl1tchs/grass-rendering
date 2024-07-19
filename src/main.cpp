#define GL_MAIN_IMPLEMENTATION
#include <core/entrypoint.h>

#include "game.h"

Application* create_application(int argc, const char** argv) {
	ApplicationCreateInfo info = {
		.name = "Grass Rendering",
		.argc = argc,
		.argv = argv,
	};
	return new Game(info);
}
