#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"


// inspired by  https://github.com/MomentsInGraphics/vulkan_renderer

typedef struct Device_t {
    // Number of loaded extensions for the instance
	uint32_t instance_extension_count;
	// Names of loaded extensions for the instance
	const char** instance_extension_names;
	// Number of loaded extensions for the device
	uint32_t device_extension_count;
	// Names of loaded extensions for the device
	const char** device_extension_names;

	// Number of available physical devices
	uint32_t physical_device_count;
	// List of available physical devices
	VkPhysicalDevice* physical_devices;
	// The physical device that is being used
	VkPhysicalDevice physical_device;
	// Properties of the used physical device
	VkPhysicalDeviceProperties physical_device_properties;
	// Information about memory available from the used physical device
	VkPhysicalDeviceMemoryProperties memory_properties;

	// This object makes Vulkan functions available
	VkInstance instance;
	// The Vulkan device object for the physical device above
	VkDevice device;

	// Number of available queue families
	uint32_t queue_family_count;
	// Properties for each available queue
	VkQueueFamilyProperties* queue_family_properties;
	// Index of a queue family that supports graphics and compute
	uint32_t queue_family_index;
	// A queue based on queue_family_index
	VkQueue queue;
	// A command pool for queue
	VkCommandPool command_pool;
};