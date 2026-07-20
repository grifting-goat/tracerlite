#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <math.h>

#include "vulkan_simple.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "world.h"


int main(int argc, char* argv[]) {
	Display_t display = {0};
	display_init(&display);

	glfwSetInputMode(display.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	double lastMouseX = 0.0, lastMouseY = 0.0;
	glfwGetCursorPos(display.window, &lastMouseX, &lastMouseY);

	float roll = 0.0f; 
	float pitch = 0.0f;
	float yaw = 0.0f;
	const float mouseSensitivity = 0.0025f;
	const float pitchLimit = 1.55334f;

	ShaderDataUBO* shaderData = (ShaderDataUBO*)display.shaderDataMapped;

	double lastFrameTime = glfwGetTime();
	double fpsTimer = 0.0;
	int fpsFrameCount = 0;
	const double fpsUpdateInterval = 0.2; // 5 Hz


	World wrld = create_world(VOXEL_GRID_DIM * VOXEL_GRID_DIM * VOXEL_GRID_DIM);

	uploadWorldToSSBO(&display, &wrld);


	while(!glfwWindowShouldClose(display.window)){
		glfwPollEvents();

		double currentTime = glfwGetTime();
		double frameTime = currentTime - lastFrameTime;
		lastFrameTime = currentTime;

		fpsTimer += frameTime;
		fpsFrameCount++;
		if (fpsTimer >= fpsUpdateInterval) {
			double fps = (double)fpsFrameCount / fpsTimer;
			double avgFrameTimeMs = (fpsTimer / fpsFrameCount) * 1000.0;
			printf("\rFPS: %6.1f | frame time: %6.3f ms", fps, avgFrameTimeMs);
			fflush(stdout);
			fpsTimer = 0.0;
			fpsFrameCount = 0;
		}

		double mouseX, mouseY;
		glfwGetCursorPos(display.window, &mouseX, &mouseY);
		double deltaX = mouseX - lastMouseX;
		double deltaY = mouseY - lastMouseY;
		lastMouseX = mouseX;
		lastMouseY = mouseY;

		yaw += (float)deltaX * mouseSensitivity;
		pitch -= (float)deltaY * mouseSensitivity;
		if (pitch > pitchLimit) { pitch = pitchLimit; }
		if (pitch < -pitchLimit) { pitch = -pitchLimit; }

		shaderData->camera_rotation[0] = pitch;
		shaderData->camera_rotation[1] = yaw;
		shaderData->camera_rotation[2] = roll;

		
		float forwardX = sinf(yaw);
		float forwardZ = cosf(yaw);
		float rightX = cosf(yaw);
		float rightZ = -sinf(yaw);

		float moveSpeed = 10.0f;

		float moveX = 0.0f;
		float moveY = 0.0f;
		float moveZ = 0.0f;

		if (glfwGetKey(display.window, GLFW_KEY_W) == GLFW_PRESS) { moveX += forwardX; moveZ += forwardZ; }
		if (glfwGetKey(display.window, GLFW_KEY_S) == GLFW_PRESS) { moveX -= forwardX; moveZ -= forwardZ; }
		if (glfwGetKey(display.window, GLFW_KEY_D) == GLFW_PRESS) { moveX += rightX; moveZ += rightZ; }
		if (glfwGetKey(display.window, GLFW_KEY_A) == GLFW_PRESS) { moveX -= rightX; moveZ -= rightZ; }

		if (glfwGetKey(display.window, GLFW_KEY_E) == GLFW_PRESS) { shaderData->camera_position[1] += 5 * (float)frameTime;}
		if (glfwGetKey(display.window, GLFW_KEY_Q) == GLFW_PRESS) { shaderData->camera_position[1] -= 5 * (float)frameTime;}

		if (glfwGetKey(display.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) { moveSpeed = 25.0f;}


		
		

		float moveLenSq = moveX * moveX + moveZ * moveZ;
		if (moveLenSq > 0.0f) {
			float invLen = 1.0f / sqrtf(moveLenSq);
			moveX *= invLen;
			moveZ *= invLen;

			shaderData->camera_position[0] += moveX * moveSpeed * (float)frameTime;
			shaderData->camera_position[2] += moveZ * moveSpeed * (float)frameTime;
		}

		display_run(&display);
	}

	display_close(&display);
	return 0;
}