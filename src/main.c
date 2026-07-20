#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#include "vulkan_simple.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "world.h"


int main(int argc, char* argv[]) {
	Display_t display;
	display_init(&display);

	World wrld = create_world(0xFF);
	createBuffer(&display,0xFF);


	while(!glfwWindowShouldClose(display.window)){
		glfwPollEvents();

		display_run(&display);
	}

	display_close(&display);
	return 0;
}