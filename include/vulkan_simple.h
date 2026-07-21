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
#include "world.h"

#include "camera.h"


// Configuration constants
#define VULKAN_VERSION        VK_API_VERSION_1_4
#define MAX_FRAMES_IN_FLIGHT  2
#define SWAPCHAIN_FORMAT      VK_FORMAT_B8G8R8A8_SRGB
#define DEPTH_FORMAT          VK_FORMAT_D32_SFLOAT
#define OUTPUT_IMAGE_FORMAT   VK_FORMAT_R8G8B8A8_UNORM
#define VOXEL_GRID_DIM         512U // placeholder
#define MAX_MATERIALS          256U // one per possible voxel byte value


// inspired by https://github.com/nickenchev/modern-vulkan


typedef struct {
	VkPipelineLayout layout;
	VkPipeline handle;
} Pipeline_t;

// HLSL cbuffer packing 
typedef struct {
	float camera_position[4];
	float camera_rotation[4];
	int32_t screen_size[2];
	int32_t pixel_count;
	float tan_fov_v;
	float tan_fov_h;
	int32_t voxel_grid_size[3];
} ShaderDataUBO;

// mirrors the HLSL StructuredBuffer<Material> element layout
typedef struct {
	float color[4];
} Material;

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
	VkCommandPool commandPool;

	// swapchain related
	VkSwapchainKHR swapchain;
	VkImage* swapchainImages;
	VkImageView* swapchainImageViews;
	VkSemaphore* renderCompleteSemaphores;
	bool requireSwapchainRecreate;
	uint32_t swapchainWidth;
	uint32_t swapchainHeight;
	uint32_t swapchainImageCount;

	VkImage depthImage;
	VkImageView depthImageView;

	VkDeviceMemory depthImageMemory;

	VkImage outputImage;
	VkImageView outputImageView;
	VkDeviceMemory outputImageMemory;
	Pipeline_t gfxPipeline; //not using currently

	Pipeline_t computePipeline;
	VkDescriptorSetLayout computeDescriptorSetLayout;
	VkDescriptorPool computeDescriptorPool;
	VkDescriptorSet computeDescriptorSet;

	// frame and synchronization resources
	VkSemaphore timelineSemaphore;
	FrameResources_t frameResources[MAX_FRAMES_IN_FLIGHT];

	VkBuffer SSBO;
	VkDeviceMemory SSBOMemory;

	VkBuffer shaderDataUBO;
	VkDeviceMemory shaderDataUBOMemory;
	void* shaderDataMapped;

	VkBuffer materialsBuffer;
	VkDeviceMemory materialsBufferMemory;
	void* materialsMapped;


	//shader
	VkShaderModule vertShader;
	VkShaderModule fragShader;

	VkShaderModule computeShader;
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

uint32_t findMemoryTypeIndex(Display_t* display, uint32_t typeBits, VkMemoryPropertyFlags required);

void createDepthImage(Display_t* display);

void createOutputimage(Display_t* display);

void createSwapchain(Display_t* display);

void destroySwapchain();

void createShaders(Display_t* display);

void createBuffer(Display_t* display, VkDeviceSize size);

void createShaderDataUBO(Display_t* display);

void createMaterialsBuffer(Display_t* display);

void uploadWorldToSSBO(Display_t* display, World* world);


Pipeline_t createGraphicsPipeline(Display_t* display);

Pipeline_t createComputePipeline(Display_t* display);

void createComputeDescriptorSet(Display_t* display);


void createSyncResources(Display_t* display);
void createCommandBuffers(Display_t* display);

void render(Display_t* display);



void update_camera(Display_t* display, Camera* c);






#endif //VULKAN_SIMPLE_H