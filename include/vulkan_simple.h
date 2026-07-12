#ifndef VULKAN_SIMPLE_H
#define VULKAN_SIMPLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <stdbool.h>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "stb_image.h"


// Configuration constants
#define VULKAN_VERSION        VK_API_VERSION_1_4
#define MAX_FRAMES_IN_FLIGHT  2
#define SWAPCHAIN_FORMAT      VK_FORMAT_B8G8R8A8_SRGB
#define DEPTH_FORMAT          VK_FORMAT_D32_SFLOAT


// inspired by https://github.com/nickenchev/modern-vulkan


typedef struct {
	VkPipelineLayout layout;
	VkPipeline handle;
} Pipeline_t;

typedef struct {
	uint32_t lastFrameId;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VkSemaphore imageAcquiredSemaphore;
	VkSemaphore workCompleteSemaphore;
} FrameResources_t;

typedef struct {
	GLFWwindow *window;
	uint32_t width;
	uint32_t height;

	VkApplicationInfo app_info;
	char name[VK_MAX_EXTENSION_NAME_SIZE];

	// vulkan core
	VkInstance vulkanInstance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkSurfaceKHR surface;
	
	//queue
	uint32_t gfxQueueFamIdx;
	VkQueue gfxQueue;

	// swapchain related
	VkSwapchainKHR swapchain;
	VkImage* swapchainImages;
	VkImageView* swapchainImageViews;
	VkSemaphore* renderCompleteSemaphores;
	bool requireSwapchainRecreate;
	uint32_t swapchainWidth;
	uint32_t swapchainHeight;

	VkImage depthImage;
	VkImageView depthImageView;

	VkDeviceMemory depthImageMemory;
	Pipeline_t pipeline;

	// frame and synchronization resources
	VkSemaphore timelineSemaphore;
	FrameResources_t frameResources[MAX_FRAMES_IN_FLIGHT];


	//shader
	VkShaderModule vertShader;
	VkShaderModule fragShader;
}  Display_t;


bool display_init(Display_t* display);
void display_run(Display_t* display);
bool display_close(Display_t* display);

VkApplicationInfo createAppInfo(Display_t* display);
VkInstance createVulkanInstance(Display_t* display);
GLFWwindow* createWindow(Display_t* display);
VkSurfaceKHR createSurface(Display_t* display);

VkPhysicalDevice findPhysicalDevice(Display_t* display);
uint32_t findGraphicsQueue(Display_t* display);
VkDevice createDevice(Display_t* display);
VkQueue createGraphicsQueue(Display_t* display);

bool initializeDeviceMemory();
bool createSwapchain(uint32_t width, uint32_t height);
void destroySwapchain();


VkShaderModule createShaderModule(const char* file);
bool createShaders();
Pipeline_t createGraphicsPipeline();
bool createSyncResources();
bool createCommandBuffers();

void render();






#endif //VULKAN_SIMPLE_H