#include "vulkan_simple.h"
#include <math.h>


//private
void set_properties(Display_t* display, const char* name, uint32_t width, uint32_t height);


bool display_init(Display_t* display) {
    glfwInit();
    
    set_properties(display, "Tracerlite", 1920U, 1080U);

    //order matters

    display->app_info = createAppInfo(display);
    display->vulkanInstance = createVulkanInstance(display);
    display->physicalDevice = findPhysicalDevice(display);
    display->window = createWindow(display);

    display->surface = createSurface(display);
    display->gfxQueueFamIdx = findGraphicsQueue(display);

    display->device = createDevice(display);
    display->gfxQueue = createGraphicsQueue(display);

    createDepthImage(display);
    createOutputimage(display);
    createShaderDataUBO(display);
    createBuffer(display, (VkDeviceSize)VOXEL_GRID_DIM * VOXEL_GRID_DIM * VOXEL_GRID_DIM * sizeof(uint8_t));
    createMaterialsBuffer(display);
    createSwapchain(display);

	createShaders(display);

    //display->gfxPipeline = createGraphicsPipeline(display);
    display->computePipeline = createComputePipeline(display);
    createComputeDescriptorSet(display);

    // placeholder camera/screen data 
    ShaderDataUBO initial_shader_data = {0};
	initial_shader_data.camera_position[0] = (float)VOXEL_GRID_DIM / 2.0f;
	initial_shader_data.camera_position[1] = (float)VOXEL_GRID_DIM / 2.0f;
    initial_shader_data.camera_position[2] = (float)VOXEL_GRID_DIM / 2.0f;
    initial_shader_data.camera_position[3] = 1.0f;
    initial_shader_data.screen_size[0] = (int32_t)display->width;
    initial_shader_data.screen_size[1] = (int32_t)display->height;
    initial_shader_data.pixel_count = (int32_t)(display->width * display->height);
    float fov = 103.0f * (3.14159265f / 180.0f);
    float fov_v = fov / 2.0f;
    float ratio = (float)display->width / (float)display->height;
    float fov_h = atanf(tanf(fov_v) * ratio);
    initial_shader_data.tan_fov_v = tanf(fov_v);
    initial_shader_data.tan_fov_h = tanf(fov_h);
    initial_shader_data.voxel_grid_size[0] = VOXEL_GRID_DIM;
    initial_shader_data.voxel_grid_size[1] = VOXEL_GRID_DIM;
    initial_shader_data.voxel_grid_size[2] = VOXEL_GRID_DIM;
    memcpy(display->shaderDataMapped, &initial_shader_data, sizeof(ShaderDataUBO));

	createSyncResources(display);

	createCommandBuffers(display);




    return true;
}

void display_run(Display_t* display) {
	render(display);
}

bool display_close(Display_t* display) {

    vkDeviceWaitIdle(display->device);

    //command pools
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        FrameResources_t *res = &display->frameResources[i];
        vkDestroyCommandPool(display->device, res->commandPool, NULL);
        vkDestroySemaphore(display->device, res->imageAcquiredSemaphore, NULL);
    }
    vkDestroySemaphore(display->device, display->timelineSemaphore, NULL);
    printf("sync resources destroyed.\n");

    vkDestroyPipeline(display->device, display->gfxPipeline.handle, NULL);
    vkDestroyPipelineLayout(display->device, display->gfxPipeline.layout, NULL);
    printf(" graphics pipeline destroyed.\n");

	vkDestroyPipeline(display->device, display->computePipeline.handle, NULL);
    vkDestroyPipelineLayout(display->device, display->computePipeline.layout, NULL);
    vkDestroyDescriptorPool(display->device, display->computeDescriptorPool, NULL);
    vkDestroyDescriptorSetLayout(display->device, display->computeDescriptorSetLayout, NULL);
    printf("compute pipeline destroyed.\n");

    for (uint32_t i = 0; i < display->swapchainImageCount; i++) {
        vkDestroySemaphore(display->device, display->renderCompleteSemaphores[i], NULL);
        vkDestroyImageView(display->device, display->swapchainImageViews[i], NULL);
    }
    free(display->renderCompleteSemaphores);
    free(display->swapchainImageViews);
    free(display->swapchainImages);
    vkDestroySwapchainKHR(display->device, display->swapchain, NULL);
    printf("swapchain destroyed.\n");

    vkDestroyImageView(display->device, display->depthImageView, NULL);
    vkDestroyImage(display->device, display->depthImage, NULL);
    vkFreeMemory(display->device, display->depthImageMemory, NULL);
    printf("depth image destroyed.\n");

    vkDestroyImageView(display->device, display->outputImageView, NULL);
    vkDestroyImage(display->device, display->outputImage, NULL);
    vkFreeMemory(display->device, display->outputImageMemory, NULL);
    printf("output image destroyed.\n");

    vkDestroyBuffer(display->device, display->SSBO, NULL);
    vkFreeMemory(display->device, display->SSBOMemory, NULL);
    printf("SSBO destroyed.\n");

    vkDestroyBuffer(display->device, display->shaderDataUBO, NULL);
    vkFreeMemory(display->device, display->shaderDataUBOMemory, NULL);
    printf("shader data UBO destroyed.\n");

    vkDestroyBuffer(display->device, display->materialsBuffer, NULL);
    vkFreeMemory(display->device, display->materialsBufferMemory, NULL);
    printf("materials buffer destroyed.\n");

    vkDestroyDevice(display->device, NULL);
    printf("device destroyed.\n");

    vkDestroySurfaceKHR(display->vulkanInstance, display->surface, NULL);
    printf("surface destroyed.\n");

    vkDestroyInstance(display->vulkanInstance, NULL);
    printf("instance destroyed.\n");

    glfwDestroyWindow(display->window);
	printf("window destroyed.\n");

    glfwTerminate();

    return true;
}



VkApplicationInfo createAppInfo(Display_t* display) {
    VkApplicationInfo app_info;
    app_info.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext=NULL;
	app_info.pApplicationName=display->name;
	app_info.applicationVersion=VK_MAKE_VERSION(0,0,1);
	char app_engine_name[VK_MAX_EXTENSION_NAME_SIZE];
	strcpy(app_engine_name,"vulkan_engine");
	app_info.pEngineName=app_engine_name;
	app_info.engineVersion=VK_MAKE_VERSION(0,0,1);
	app_info.apiVersion=VULKAN_VERSION;

    return app_info;
}

VkInstance createVulkanInstance(Display_t* display) {
    // create instance info
	VkInstanceCreateInfo instance_create_info;
	instance_create_info.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pNext=NULL;
	instance_create_info.flags=0;
	instance_create_info.pApplicationInfo=&display->app_info;
	uint32_t instance_layer_count = 1U;
	instance_create_info.enabledLayerCount=instance_layer_count;
	char pp_instance_layers[instance_layer_count][VK_MAX_EXTENSION_NAME_SIZE];
	strcpy(pp_instance_layers[0],"VK_LAYER_KHRONOS_validation");
	char *pp_instance_layer_names[instance_layer_count];
	for(uint32_t i=0;i<instance_layer_count;i++){
		pp_instance_layer_names[i]=
			pp_instance_layers[i];
	}
	instance_create_info.ppEnabledLayerNames=(const char * const *)pp_instance_layer_names;
	uint32_t instance_extension_count=0;
	const char * const *pp_instance_extension_names=glfwGetRequiredInstanceExtensions(&instance_extension_count);
	instance_create_info.enabledExtensionCount=instance_extension_count;
	instance_create_info.ppEnabledExtensionNames=pp_instance_extension_names;

    VkInstance instance;
	vkCreateInstance(&instance_create_info, NULL, &instance);

    return instance;
}


VkSurfaceKHR createSurface(Display_t* display) {
    VkSurfaceKHR surface;
	glfwCreateWindowSurface(display->vulkanInstance,display->window,NULL,&surface);
	printf("surface created.\n");

    return surface;
}

GLFWwindow* createWindow(Display_t* display) {
    glfwWindowHint(GLFW_CLIENT_API,GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE,GLFW_FALSE);
	glfwWindowHint(GLFW_DECORATED,GLFW_FALSE);

	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	display->width = (uint32_t)mode->width;
	display->height = (uint32_t)mode->height;
	display->swapchainWidth = display->width;
	display->swapchainHeight = display->height;

	GLFWwindow *window = glfwCreateWindow(display->width,display->height,display->name,NULL,NULL);

	int monitorX, monitorY;
	glfwGetMonitorPos(glfwGetPrimaryMonitor(), &monitorX, &monitorY);
	glfwSetWindowPos(window, monitorX, monitorY);

	printf("window created.\n");

	GLFWimage icons[3];
	icons[0].pixels = stbi_load("icon_32.png", &icons[0].width, &icons[0].height, 0, 4);
	icons[1].pixels = stbi_load("icon_64.png", &icons[1].width, &icons[1].height, 0, 4);
	icons[2].pixels = stbi_load("icon_128.png", &icons[2].width, &icons[2].height, 0, 4);

	if (icons[0].pixels && icons[1].pixels && icons[2].pixels) {glfwSetWindowIcon(window, 3, icons);} 
    else {printf("Icon failed.\n");}
	
	if (icons[0].pixels) stbi_image_free(icons[0].pixels);
	if (icons[1].pixels) stbi_image_free(icons[1].pixels);
	if (icons[2].pixels) stbi_image_free(icons[2].pixels);

    return window;
}


VkPhysicalDevice findPhysicalDevice(Display_t* display) {
    uint32_t physical_device_count=0;
	vkEnumeratePhysicalDevices(display->vulkanInstance,&physical_device_count,NULL);

	VkPhysicalDevice physical_device[physical_device_count];
	vkEnumeratePhysicalDevices(display->vulkanInstance,&physical_device_count,physical_device);

    VkPhysicalDeviceProperties physical_device_properties[physical_device_count];
	uint32_t discrete_gpu_list[physical_device_count];
	uint32_t discrete_gpu_count=0;
	uint32_t intergrated_gpu_list[physical_device_count];
	uint32_t intergrated_gpu_count=0;

	VkPhysicalDeviceMemoryProperties physical_device_memory_properties[physical_device_count];
	uint32_t physical_device_memory_count[physical_device_count];
	VkDeviceSize physical_device_memory_total[physical_device_count];
	VkDeviceSize physical_device_memory_vram[physical_device_count];

	for(uint32_t i=0;i<physical_device_count;i++){
		vkGetPhysicalDeviceProperties(physical_device[i],&physical_device_properties[i]);
		if(physical_device_properties[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
			discrete_gpu_list[discrete_gpu_count]=i;
			discrete_gpu_count++;
		} else if(physical_device_properties[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU){
			intergrated_gpu_list[intergrated_gpu_count]=i;
			intergrated_gpu_count++;
		}

		vkGetPhysicalDeviceMemoryProperties(physical_device[i],&physical_device_memory_properties[i]);
		physical_device_memory_count[i] = physical_device_memory_properties[i].memoryHeapCount;
		physical_device_memory_total[i]=0;
		physical_device_memory_vram[i]=0;
		for(uint32_t j=0;j<physical_device_memory_count[i];j++){
			physical_device_memory_total[i] += physical_device_memory_properties[i].memoryHeaps[j].size;
			if(physical_device_memory_properties[i].memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT){
				physical_device_memory_vram[i] += physical_device_memory_properties[i].memoryHeaps[j].size;
			}
		}
	}

	VkDeviceSize max_memory_size=0;
	uint32_t physical_device_best_index=0;

	if(discrete_gpu_count!=0){
		for(uint32_t i=0;i<discrete_gpu_count;i++){
			if(physical_device_memory_total[i]>max_memory_size){
				physical_device_best_index=discrete_gpu_list[i];
				max_memory_size=physical_device_memory_total[i];
			}
		}
	} else if(intergrated_gpu_count!=0){
		for(uint32_t i=0;i<intergrated_gpu_count;i++){
			if(physical_device_memory_total[i]>max_memory_size){
				physical_device_best_index=intergrated_gpu_list[i];
				max_memory_size=physical_device_memory_total[i];
			}
		}
	}

	// Print Best Device
	printf("best device index:%u\n",physical_device_best_index);
	printf("device name:%s\n",physical_device_properties[physical_device_best_index].deviceName);
	printf("device type:");

	if(discrete_gpu_count!=0){printf("discrete gpu\n");}
	else if(intergrated_gpu_count!=0){printf("intergrated gpu\n");}
	else{printf("unknown\n");}

	printf("memory total:%llu\n",physical_device_memory_total[physical_device_best_index]);
	double mem_gb = (double)physical_device_memory_vram[physical_device_best_index] / (1073741824.0);
	printf("VRAM total:%.3f GB\n", mem_gb);

	return physical_device[physical_device_best_index];

}

VkDevice createDevice(Display_t* display) {

	VkPhysicalDeviceVulkan14Features supported_features1_4 = {0};
	supported_features1_4.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
	supported_features1_4.pNext = NULL;

	VkPhysicalDeviceVulkan13Features supported_features1_3 = {0};
	supported_features1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	supported_features1_3.pNext = &supported_features1_4;

	VkPhysicalDeviceVulkan12Features supported_features1_2 = {0};
	supported_features1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	supported_features1_2.pNext = &supported_features1_3;

	VkPhysicalDeviceFeatures2 supported_features = {0};
	supported_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	supported_features.pNext = &supported_features1_2;

	vkGetPhysicalDeviceFeatures2(display->physicalDevice, &supported_features);

	if (!supported_features1_3.dynamicRendering || !supported_features1_3.synchronization2 ||
		!supported_features1_2.timelineSemaphore) {
		printf("physical device doesn't meet the feature requirements.\n");
		return VK_NULL_HANDLE;
	}

	VkPhysicalDeviceVulkan14Features features1_4 = {0};
	features1_4.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
	features1_4.pNext = NULL;

	VkPhysicalDeviceVulkan13Features features1_3 = {0};
	features1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features1_3.pNext = &features1_4;
	features1_3.synchronization2 = VK_TRUE;
	features1_3.dynamicRendering = VK_TRUE;

	VkPhysicalDeviceVulkan12Features features1_2 = {0};
	features1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	features1_2.pNext = &features1_3;
	features1_2.timelineSemaphore = VK_TRUE;

	VkPhysicalDeviceFeatures2 features = {0};
	features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features.pNext = &features1_2;

	float queue_priorities[1] = {1.0f};
	VkDeviceQueueCreateInfo gfx_queue_info = {0};
	gfx_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	gfx_queue_info.pNext = NULL;
	gfx_queue_info.flags = 0;
	gfx_queue_info.queueFamilyIndex = display->gfxQueueFamIdx;
	gfx_queue_info.queueCount = 1;
	gfx_queue_info.pQueuePriorities = queue_priorities;

	// device specific extensions
	const char* device_extensions[1] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkDeviceCreateInfo device_create_info = {0};
	device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.pNext = &features;
	device_create_info.flags = 0;
	device_create_info.queueCreateInfoCount = 1;
	device_create_info.pQueueCreateInfos = &gfx_queue_info;
	device_create_info.enabledLayerCount = 0;
	device_create_info.ppEnabledLayerNames = NULL;
	device_create_info.enabledExtensionCount = 1;
	device_create_info.ppEnabledExtensionNames = device_extensions;
	device_create_info.pEnabledFeatures = NULL;

	VkDevice device;
	if (vkCreateDevice(display->physicalDevice, &device_create_info, NULL, &device) != VK_SUCCESS) {
		return VK_NULL_HANDLE;
	}

    printf("Device created!\n");
	return device;
}


uint32_t findGraphicsQueue(Display_t* display) {
    //query que families
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties2(display->physicalDevice,&queue_family_count ,NULL);
	VkQueueFamilyProperties2 queue_family_properties[queue_family_count];
	for (uint32_t i = 0; i < queue_family_count; i++) {
		queue_family_properties[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
		queue_family_properties[i].pNext = NULL;
	}
	vkGetPhysicalDeviceQueueFamilyProperties2(display->physicalDevice,&queue_family_count ,queue_family_properties);

    for (uint32_t fam_idx = 0; fam_idx<queue_family_count; fam_idx++) {

        VkBool32 hasPresentSupport = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(display->physicalDevice, fam_idx, display->surface, &hasPresentSupport);

        VkQueueFamilyProperties2 props = queue_family_properties[fam_idx];
        if (props.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT && hasPresentSupport) {
            printf("Family Queue Index: %u\n", fam_idx);
			return fam_idx;
		}
    }

    printf("failed to get valid queue");
    return UINT32_MAX;
}

VkQueue createGraphicsQueue(Display_t* display) {
	// grab the VkQueue object
    VkQueue q;
	vkGetDeviceQueue(display->device, display->gfxQueueFamIdx, 0, &q);
	if (!q){
		printf("Couldn't get the graphics queue\n");
		return NULL;
	}
    printf("Graphics Queue grabbed\n");
	return q;

}

uint32_t findMemoryTypeIndex(Display_t* display, uint32_t typeBits, VkMemoryPropertyFlags required) {
	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(display->physicalDevice, &mem_props);

	for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
		bool type_supported = typeBits & (1 << i);
		bool has_properties = (mem_props.memoryTypes[i].propertyFlags & required) == required;
		if (type_supported && has_properties) {
			return i;
		}
	}

	printf("failed to find suitable memory type.\n");
	return UINT32_MAX;
}

//need to understand this one better
void createDepthImage(Display_t* display) {
	VkImageCreateInfo image_create_info = {0};
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.format = DEPTH_FORMAT;
	image_create_info.extent.width = display->width;
	image_create_info.extent.height = display->height;
	image_create_info.extent.depth = 1;
	image_create_info.mipLevels = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (vkCreateImage(display->device, &image_create_info, NULL, &display->depthImage) != VK_SUCCESS) {
		printf("failed to create depth image.\n");
		return;
	}

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(display->device, display->depthImage, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_reqs.size;
	alloc_info.memoryTypeIndex = findMemoryTypeIndex(display, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(display->device, &alloc_info, NULL, &display->depthImageMemory) != VK_SUCCESS) {
		printf("failed to allocate depth image memory.\n");
		return;
	}
	vkBindImageMemory(display->device, display->depthImage, display->depthImageMemory, 0);

	VkImageViewCreateInfo view_create_info = {0};
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = display->depthImage;
	view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_create_info.format = DEPTH_FORMAT;
	view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	view_create_info.subresourceRange.baseMipLevel = 0;
	view_create_info.subresourceRange.levelCount = 1;
	view_create_info.subresourceRange.baseArrayLayer = 0;
	view_create_info.subresourceRange.layerCount = 1;

	if (vkCreateImageView(display->device, &view_create_info, NULL, &display->depthImageView) != VK_SUCCESS) {
		printf("failed to create depth image view.\n");
		return;
	}

	printf("depth image created.\n");
}


void createOutputimage(Display_t* display) {
	VkImageCreateInfo image_create_info = {0};
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.format = OUTPUT_IMAGE_FORMAT;
	image_create_info.extent.width = display->width;
	image_create_info.extent.height = display->height;
	image_create_info.extent.depth = 1;
	image_create_info.mipLevels = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_create_info.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	if (vkCreateImage(display->device, &image_create_info, NULL, &display->outputImage) != VK_SUCCESS) {
		printf("failed to create output image.\n");
		return;
	}

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(display->device, display->outputImage, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_reqs.size;
	alloc_info.memoryTypeIndex = findMemoryTypeIndex(display, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(display->device, &alloc_info, NULL, &display->outputImageMemory) != VK_SUCCESS) {
		printf("failed to allocate output image memory.\n");
		return;
	}
	vkBindImageMemory(display->device, display->outputImage, display->outputImageMemory, 0);

	VkImageViewCreateInfo view_create_info = {0};
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = display->outputImage;
	view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_create_info.format = OUTPUT_IMAGE_FORMAT;
	view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	view_create_info.subresourceRange.baseMipLevel = 0;
	view_create_info.subresourceRange.levelCount = 1;
	view_create_info.subresourceRange.baseArrayLayer = 0;
	view_create_info.subresourceRange.layerCount = 1;

	if (vkCreateImageView(display->device, &view_create_info, NULL, &display->outputImageView) != VK_SUCCESS) {
		printf("failed to create output image view.\n");
		return;
	}

	printf("output image created.\n");
}

void createSwapchain(Display_t* display) {

    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(display->physicalDevice,display->surface,&surface_capabilities);
    printf("fetched capabilities from surface.\n");

    uint32_t requestedImageCount = surface_capabilities.minImageCount < 3U ? 3U : surface_capabilities.minImageCount;
    if (surface_capabilities.maxImageCount > 0) {
        requestedImageCount = requestedImageCount > surface_capabilities.maxImageCount ? surface_capabilities.maxImageCount :  requestedImageCount;
    }

    //fetch surface formats
	uint32_t surface_form_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(display->physicalDevice,display->surface,&surface_form_count,NULL);
	VkSurfaceFormatKHR surface_formats[surface_form_count];
	vkGetPhysicalDeviceSurfaceFormatsKHR(display->physicalDevice,display->surface,&surface_form_count,surface_formats);
	printf("fetched %d surface formats.\n",surface_form_count);
	for(uint32_t i=0;i<surface_form_count;i++){
		printf("format:%d\tcolorspace:%d\n",surface_formats[i].format,surface_formats[i].colorSpace);
	}

    //fetch surface present mode
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(display->physicalDevice,display->surface,&present_mode_count,NULL);
	VkPresentModeKHR present_modes[present_mode_count];
	vkGetPhysicalDeviceSurfacePresentModesKHR(display->physicalDevice,display->surface,&present_mode_count,present_modes);
	printf("fetched %d present modes.\n",present_mode_count);
	char mailbox_mode_supported=0;
	for(uint32_t i=0;i<present_mode_count;i++){
		printf("present mode:%d\n",present_modes[i]);
		if(present_modes[i]==VK_PRESENT_MODE_MAILBOX_KHR){
			printf("mailbox present mode supported.\n");
			mailbox_mode_supported=1;
		}
	}

    //add checks here later
    VkExtent2D actual_extent;
	actual_extent.width=display->width;
	actual_extent.height=display->height;

    //pick the format the pipeline was built for, falling back to whatever the surface offers first
	VkSurfaceFormatKHR chosen_format = surface_formats[0];
	for(uint32_t i=0;i<surface_form_count;i++){
		if(surface_formats[i].format==SWAPCHAIN_FORMAT && surface_formats[i].colorSpace==VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
			chosen_format = surface_formats[i];
			break;
		}
	}

    //create swapchain

	VkSwapchainCreateInfoKHR swap_create_info;

	swap_create_info.sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swap_create_info.pNext=NULL;
	swap_create_info.flags=0;
	swap_create_info.surface=display->surface;
	swap_create_info.minImageCount=requestedImageCount;
	swap_create_info.imageFormat=chosen_format.format;
	swap_create_info.imageColorSpace=chosen_format.colorSpace;
	swap_create_info.imageExtent = actual_extent;
	swap_create_info.imageArrayLayers=1;
	swap_create_info.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	swap_create_info.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;
	swap_create_info.queueFamilyIndexCount=0;
	swap_create_info.pQueueFamilyIndices=NULL;
	swap_create_info.preTransform=surface_capabilities.currentTransform;
	swap_create_info.compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swap_create_info.presentMode = mailbox_mode_supported ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
	swap_create_info.clipped=VK_TRUE;
	swap_create_info.oldSwapchain=VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(display->device, &swap_create_info, NULL, &display->swapchain) != VK_SUCCESS){
		printf("Error creating swapchain\n");
		return;
	}
    printf("swapchain created\n");

    // ask for the swapchain images
	uint32_t swap_image_count = 0;
	vkGetSwapchainImagesKHR(display->device, display->swapchain, &swap_image_count, NULL);
	display->swapchainImageCount = swap_image_count;
	display->swapchainImages = calloc(swap_image_count, sizeof(VkImage));
	vkGetSwapchainImagesKHR(display->device, display->swapchain, &swap_image_count, display->swapchainImages);
	display->swapchainImageViews = calloc(swap_image_count, sizeof(VkImageView));
	display->renderCompleteSemaphores = calloc(swap_image_count, sizeof(VkSemaphore));

    //create image view
	VkImageViewCreateInfo image_view_creation_infos[swap_image_count];

    VkComponentMapping image_view_rgba_component;
	image_view_rgba_component.r=VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_rgba_component.g=VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_rgba_component.b=VK_COMPONENT_SWIZZLE_IDENTITY;
	image_view_rgba_component.a=VK_COMPONENT_SWIZZLE_IDENTITY;

	VkImageSubresourceRange image_view_subresource;
	image_view_subresource.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
	image_view_subresource.baseMipLevel=0;
	image_view_subresource.levelCount=1;
	image_view_subresource.baseArrayLayer=0;
    image_view_subresource.layerCount = swap_create_info.imageArrayLayers;

    for(uint32_t i=0;i<swap_image_count;i++) {
        image_view_creation_infos[i].sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_creation_infos[i].pNext=NULL;
		image_view_creation_infos[i].flags=0;
		image_view_creation_infos[i].image=display->swapchainImages[i];
		image_view_creation_infos[i].viewType=VK_IMAGE_VIEW_TYPE_2D;
		image_view_creation_infos[i].format=chosen_format.format;
		image_view_creation_infos[i].components=image_view_rgba_component;
		image_view_creation_infos[i].subresourceRange=image_view_subresource;

		vkCreateImageView(display->device,&image_view_creation_infos[i],NULL,&display->swapchainImageViews[i]);

		printf("image view %d created.\n",i);
    }

    //one render-complete semaphore per swapchain image, since imageCount may differ from MAX_FRAMES_IN_FLIGHT
    VkSemaphoreCreateInfo render_complete_semaphore_info;
    render_complete_semaphore_info.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    render_complete_semaphore_info.pNext=NULL;
    render_complete_semaphore_info.flags=0;
    for(uint32_t i=0;i<swap_image_count;i++) {
        vkCreateSemaphore(display->device,&render_complete_semaphore_info,NULL,&display->renderCompleteSemaphores[i]);
    }
    printf("render-complete semaphores created.\n");
}


void createShaders(Display_t* display) {
	FILE *fp_compute=NULL;

	fp_compute=fopen("shaders/compute.spv","rb+");

	if(fp_compute==NULL){
		printf("can't find compute SPIR-V binary.\n");
		return;
	}

	fseek(fp_compute,0,SEEK_END);
	uint32_t compute_size=ftell(fp_compute);

	char *p_compute_code=(char *)malloc(compute_size*sizeof(char));

	rewind(fp_compute);
	fread(p_compute_code,1,compute_size,fp_compute);
	printf("compute shader binary loaded.\n");

	fclose(fp_compute);

	//create shader module
	VkShaderModuleCreateInfo compute_shader_module_create_info;
	compute_shader_module_create_info.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	compute_shader_module_create_info.pNext=NULL;
	compute_shader_module_create_info.flags=0;
	compute_shader_module_create_info.codeSize=compute_size;
	compute_shader_module_create_info.pCode=(const uint32_t *)p_compute_code;

	vkCreateShaderModule(display->device,&compute_shader_module_create_info,NULL,&display->computeShader);
	printf("compute shader module created.\n");

	free(p_compute_code);
	printf("compute shader binary released.\n");
}

Pipeline_t createGraphicsPipeline(Display_t* display) {
	Pipeline_t pipeline = {0};

	const char *entry_point = "main";
	VkPipelineShaderStageCreateInfo shader_stages[2];
	shader_stages[0].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[0].pNext=NULL;
	shader_stages[0].flags=0;
	shader_stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;
	shader_stages[0].module=display->vertShader;
	shader_stages[0].pName=entry_point;
	shader_stages[0].pSpecializationInfo=NULL;

	shader_stages[1].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[1].pNext=NULL;
	shader_stages[1].flags=0;
	shader_stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stages[1].module=display->fragShader;
	shader_stages[1].pName=entry_point;
	shader_stages[1].pSpecializationInfo=NULL;


	VkPipelineVertexInputStateCreateInfo vertex_input_info={0};
	vertex_input_info.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_info={0};
	input_assembly_info.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_info.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;


	VkPipelineDepthStencilStateCreateInfo depth_stencil_info={0};
	depth_stencil_info.sType=VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_info.depthTestEnable=VK_TRUE;
	depth_stencil_info.depthWriteEnable=VK_TRUE;
	depth_stencil_info.depthCompareOp=VK_COMPARE_OP_LESS;
	depth_stencil_info.stencilTestEnable=VK_FALSE;

	VkPipelineViewportStateCreateInfo viewport_info={0};
	viewport_info.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_info.viewportCount=1;
	viewport_info.pViewports=NULL;
	viewport_info.scissorCount=1;
	viewport_info.pScissors=NULL;


	VkPipelineRasterizationStateCreateInfo raster_info={0};
	raster_info.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster_info.polygonMode=VK_POLYGON_MODE_FILL;
	raster_info.cullMode=VK_CULL_MODE_NONE;
	raster_info.frontFace=VK_FRONT_FACE_CLOCKWISE;
	raster_info.lineWidth=1.0f;

	VkPipelineMultisampleStateCreateInfo multisample_info={0};
	multisample_info.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_info.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;


	VkPipelineColorBlendAttachmentState attach_state={0};
	attach_state.blendEnable=VK_FALSE;
	attach_state.colorWriteMask=VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|
		VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo blend_info={0};
	blend_info.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend_info.attachmentCount=1;
	blend_info.pAttachments=&attach_state;

	VkDynamicState dynamic_states[2]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_state_info={0};
	dynamic_state_info.sType=VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_info.dynamicStateCount=2;
	dynamic_state_info.pDynamicStates=dynamic_states;


	VkFormat swapchain_format=SWAPCHAIN_FORMAT;
	VkPipelineRenderingCreateInfo render_info={0};
	render_info.sType=VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	render_info.colorAttachmentCount=1;
	render_info.pColorAttachmentFormats=&swapchain_format;
	render_info.depthAttachmentFormat=DEPTH_FORMAT;

	
	VkPipelineLayoutCreateInfo pipeline_layout_info={0};
	pipeline_layout_info.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount=0;
	pipeline_layout_info.pushConstantRangeCount=0;
	pipeline_layout_info.pPushConstantRanges=NULL;

	if (vkCreatePipelineLayout(display->device,&pipeline_layout_info,NULL,&pipeline.layout) != VK_SUCCESS) {
		printf("Unable to create the pipeline layout\n");
		return (Pipeline_t){0};
	}

	VkGraphicsPipelineCreateInfo pipeline_info={0};
	pipeline_info.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.pNext=&render_info;
	pipeline_info.stageCount=2;
	pipeline_info.pStages=shader_stages;
	pipeline_info.pVertexInputState=&vertex_input_info;
	pipeline_info.pInputAssemblyState=&input_assembly_info;
	pipeline_info.pViewportState=&viewport_info;
	pipeline_info.pRasterizationState=&raster_info;
	pipeline_info.pMultisampleState=&multisample_info;
	pipeline_info.pDepthStencilState=&depth_stencil_info;
	pipeline_info.pColorBlendState=&blend_info;
	pipeline_info.pDynamicState=&dynamic_state_info;
	pipeline_info.layout=pipeline.layout;
	pipeline_info.renderPass=VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(display->device,VK_NULL_HANDLE,1,&pipeline_info,NULL,&pipeline.handle) != VK_SUCCESS) {
		printf("Error creating the pipeline\n");
		return (Pipeline_t){0};
	}

	//destroy shader module

	vkDestroyShaderModule(display->device,display->fragShader,NULL);
	printf("fragment shader module destroyed.\n");
	vkDestroyShaderModule(display->device,display->vertShader,NULL);
	printf("vertex shader module destroyed.\n");

	printf("graphics pipeline created.\n");
	return pipeline;
}


Pipeline_t createComputePipeline(Display_t* display) {
	Pipeline_t pipeline = {0};

	VkDescriptorSetLayoutBinding bindings[4] = {0};
	bindings[0].binding = 1;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	bindings[1].binding = 3;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	bindings[2].binding = 4;
	bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	bindings[2].descriptorCount = 1;
	bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	bindings[3].binding = 2;
	bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[3].descriptorCount = 1;
	bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo set_layout_info = {0};
	set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set_layout_info.bindingCount = 4;
	set_layout_info.pBindings = bindings;

	if (vkCreateDescriptorSetLayout(display->device, &set_layout_info, NULL, &display->computeDescriptorSetLayout) != VK_SUCCESS) {
		printf("Unable to create the compute descriptor set layout\n");
		return (Pipeline_t){0};
	}

	VkPushConstantRange push_constant_range = {0};
	push_constant_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	push_constant_range.offset = 0;
	push_constant_range.size = sizeof(uint32_t);

	VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &display->computeDescriptorSetLayout;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges = &push_constant_range;

	if (vkCreatePipelineLayout(display->device, &pipeline_layout_info, NULL, &pipeline.layout) != VK_SUCCESS) {
		printf("Unable to create the compute pipeline layout\n");
		return (Pipeline_t){0};
	}

	VkPipelineShaderStageCreateInfo stage_info = {0};
	stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stage_info.module = display->computeShader;
	stage_info.pName = "main";

	VkComputePipelineCreateInfo compute_pipeline_info = {0};
	compute_pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	compute_pipeline_info.stage = stage_info;
	compute_pipeline_info.layout = pipeline.layout;

	if (vkCreateComputePipelines(display->device, VK_NULL_HANDLE, 1, &compute_pipeline_info, NULL, &pipeline.handle) != VK_SUCCESS) {
		printf("Error creating the compute pipeline\n");
		return (Pipeline_t){0};
	}

	vkDestroyShaderModule(display->device, display->computeShader, NULL);
	printf("compute shader module destroyed.\n");

	printf("compute pipeline created.\n");
	return pipeline;
}

void createComputeDescriptorSet(Display_t* display) {

	VkDescriptorPoolSize pool_sizes[3] = {0};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_sizes[0].descriptorCount = 2;
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[1].descriptorCount = 1;
	pool_sizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	pool_sizes[2].descriptorCount = 1;

	VkDescriptorPoolCreateInfo pool_info = {0};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.maxSets = 1;
	pool_info.poolSizeCount = 3;
	pool_info.pPoolSizes = pool_sizes;

	if (vkCreateDescriptorPool(display->device, &pool_info, NULL, &display->computeDescriptorPool) != VK_SUCCESS) {
		printf("Unable to create the compute descriptor pool\n");
		return;
	}

	VkDescriptorSetAllocateInfo set_alloc_info = {0};
	set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	set_alloc_info.descriptorPool = display->computeDescriptorPool;
	set_alloc_info.descriptorSetCount = 1;
	set_alloc_info.pSetLayouts = &display->computeDescriptorSetLayout;

	if (vkAllocateDescriptorSets(display->device, &set_alloc_info, &display->computeDescriptorSet) != VK_SUCCESS) {
		printf("Unable to allocate the compute descriptor set\n");
		return;
	}

	VkDescriptorBufferInfo ssbo_info = {0};
	ssbo_info.buffer = display->SSBO;
	ssbo_info.offset = 0;
	ssbo_info.range = VK_WHOLE_SIZE;

	VkDescriptorBufferInfo ubo_info = {0};
	ubo_info.buffer = display->shaderDataUBO;
	ubo_info.offset = 0;
	ubo_info.range = VK_WHOLE_SIZE;

	VkDescriptorImageInfo image_info = {0};
	image_info.imageView = display->outputImageView;
	image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkDescriptorBufferInfo materials_info = {0};
	materials_info.buffer = display->materialsBuffer;
	materials_info.offset = 0;
	materials_info.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet writes[4] = {0};
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].dstSet = display->computeDescriptorSet;
	writes[0].dstBinding = 1;
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	writes[0].pBufferInfo = &ssbo_info;

	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].dstSet = display->computeDescriptorSet;
	writes[1].dstBinding = 3;
	writes[1].descriptorCount = 1;
	writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[1].pBufferInfo = &ubo_info;

	writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[2].dstSet = display->computeDescriptorSet;
	writes[2].dstBinding = 4;
	writes[2].descriptorCount = 1;
	writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	writes[2].pImageInfo = &image_info;

	writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[3].dstSet = display->computeDescriptorSet;
	writes[3].dstBinding = 2;
	writes[3].descriptorCount = 1;
	writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	writes[3].pBufferInfo = &materials_info;

	vkUpdateDescriptorSets(display->device, 4, writes, 0, NULL);

	printf("compute descriptor set created.\n");
}


void createSyncResources(Display_t* display) {

	VkSemaphoreTypeCreateInfo semaphore_type_info;
	semaphore_type_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	semaphore_type_info.pNext = NULL;
	semaphore_type_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	semaphore_type_info.initialValue = 0;

	VkSemaphoreCreateInfo semaphore_create_info;
	semaphore_create_info.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_create_info.pNext=&semaphore_type_info;
	semaphore_create_info.flags=0;

	if (vkCreateSemaphore(display->device, &semaphore_create_info, NULL, &display->timelineSemaphore) != VK_SUCCESS) {
		printf("Unable to create the timeline semaphore\n");
		return;
	}

	//per-frame image-acquire semaphores
	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

		VkSemaphoreCreateInfo semaphoreInfo;
		semaphoreInfo.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreInfo.pNext=NULL;
		semaphoreInfo.flags=0;

		FrameResources_t *res = &display->frameResources[i];

		if (vkCreateSemaphore(display->device, &semaphoreInfo, NULL, &res->imageAcquiredSemaphore) != VK_SUCCESS) {
			printf("Error creating the per-frame image-acquire semaphore\n");
			return;
		}
	}

}


void createCommandBuffers(Display_t* display) {

	for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		FrameResources_t *res = &display->frameResources[i];

		VkCommandPoolCreateInfo cmd_pool_create_info;
		cmd_pool_create_info.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmd_pool_create_info.pNext=NULL;
		cmd_pool_create_info.flags=0;
		cmd_pool_create_info.queueFamilyIndex=display->gfxQueueFamIdx;

		if (vkCreateCommandPool(display->device,&cmd_pool_create_info,NULL,&res->commandPool) != VK_SUCCESS) {
			printf("Unable to create command buffer pool\n");
			return;
		}

		VkCommandBufferAllocateInfo cmd_buffer_alloc_info;
		cmd_buffer_alloc_info.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmd_buffer_alloc_info.pNext=NULL;
		cmd_buffer_alloc_info.commandPool=res->commandPool;
		cmd_buffer_alloc_info.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmd_buffer_alloc_info.commandBufferCount=1;

		if (vkAllocateCommandBuffers(display->device, &cmd_buffer_alloc_info, &res->commandBuffer) != VK_SUCCESS) {
			printf("Unable to allocate command buffer\n");
			return;
		}

		printf("command pool and buffer %u created.\n", i);
	}
}


void render(Display_t* display) {
	static uint32_t frame_counter = 0;
	static uint64_t timeline_value = 0;


	uint32_t frame_res_index = frame_counter++ % MAX_FRAMES_IN_FLIGHT;
	FrameResources_t *res = &display->frameResources[frame_res_index];

	//frame N and frame N - MAX_FRAMES_IN_FLIGHT share resources
	uint64_t frame_id = ++timeline_value;
	uint64_t wait_for_id = frame_id > MAX_FRAMES_IN_FLIGHT ? frame_id - MAX_FRAMES_IN_FLIGHT : 0;

	VkSemaphoreWaitInfo wait_info={0};
	wait_info.sType=VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	wait_info.semaphoreCount=1;
	wait_info.pSemaphores=&display->timelineSemaphore;
	wait_info.pValues=&wait_for_id;
	vkWaitSemaphores(display->device, &wait_info, UINT64_MAX);

	vkResetCommandPool(display->device, res->commandPool, 0);

	uint32_t image_index=0;
	VkResult acquire_result = vkAcquireNextImageKHR(display->device, display->swapchain, UINT64_MAX, res->imageAcquiredSemaphore, VK_NULL_HANDLE, &image_index);
	if (acquire_result == VK_ERROR_OUT_OF_DATE_KHR) {
		printf("swapchain out of date\n");
		return;
	}

	VkCommandBufferBeginInfo cmd_begin_info={0};
	cmd_begin_info.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_begin_info.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(res->commandBuffer, &cmd_begin_info);

	//transition output image so the compute shader can imageStore into it
	VkImageMemoryBarrier2 to_general_barrier={0};
	to_general_barrier.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	to_general_barrier.srcStageMask=VK_PIPELINE_STAGE_2_NONE;
	to_general_barrier.srcAccessMask=0;
	to_general_barrier.dstStageMask=VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
	to_general_barrier.dstAccessMask=VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
	to_general_barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;
	to_general_barrier.newLayout=VK_IMAGE_LAYOUT_GENERAL;
	to_general_barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
	to_general_barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
	to_general_barrier.image=display->outputImage;
	to_general_barrier.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
	to_general_barrier.subresourceRange.baseMipLevel=0;
	to_general_barrier.subresourceRange.levelCount=1;
	to_general_barrier.subresourceRange.baseArrayLayer=0;
	to_general_barrier.subresourceRange.layerCount=1;

	VkDependencyInfo pre_dispatch_dep_info={0};
	pre_dispatch_dep_info.sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	pre_dispatch_dep_info.imageMemoryBarrierCount=1;
	pre_dispatch_dep_info.pImageMemoryBarriers=&to_general_barrier;
	vkCmdPipelineBarrier2(res->commandBuffer, &pre_dispatch_dep_info);

	vkCmdBindPipeline(res->commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, display->computePipeline.handle);
	vkCmdBindDescriptorSets(res->commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, display->computePipeline.layout, 0, 1, &display->computeDescriptorSet, 0, NULL);

	uint32_t voxel_count = VOXEL_GRID_DIM * VOXEL_GRID_DIM * VOXEL_GRID_DIM;
	vkCmdPushConstants(res->commandBuffer, display->computePipeline.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &voxel_count);

	uint32_t group_count_x = (display->swapchainWidth + 7) / 8;
	uint32_t group_count_y = (display->swapchainHeight + 7) / 8;
	vkCmdDispatch(res->commandBuffer, group_count_x, group_count_y, 1);

	//transition output image (blit source) and swapchain image (blit destination)
	VkImageMemoryBarrier2 pre_blit_barriers[2];
	pre_blit_barriers[0].sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	pre_blit_barriers[0].pNext=NULL;
	pre_blit_barriers[0].srcStageMask=VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
	pre_blit_barriers[0].srcAccessMask=VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
	pre_blit_barriers[0].dstStageMask=VK_PIPELINE_STAGE_2_BLIT_BIT;
	pre_blit_barriers[0].dstAccessMask=VK_ACCESS_2_TRANSFER_READ_BIT;
	pre_blit_barriers[0].oldLayout=VK_IMAGE_LAYOUT_GENERAL;
	pre_blit_barriers[0].newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	pre_blit_barriers[0].srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
	pre_blit_barriers[0].dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
	pre_blit_barriers[0].image=display->outputImage;
	pre_blit_barriers[0].subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
	pre_blit_barriers[0].subresourceRange.baseMipLevel=0;
	pre_blit_barriers[0].subresourceRange.levelCount=1;
	pre_blit_barriers[0].subresourceRange.baseArrayLayer=0;
	pre_blit_barriers[0].subresourceRange.layerCount=1;

	pre_blit_barriers[1].sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	pre_blit_barriers[1].pNext=NULL;
	pre_blit_barriers[1].srcStageMask=VK_PIPELINE_STAGE_2_NONE;
	pre_blit_barriers[1].srcAccessMask=0;
	pre_blit_barriers[1].dstStageMask=VK_PIPELINE_STAGE_2_BLIT_BIT;
	pre_blit_barriers[1].dstAccessMask=VK_ACCESS_2_TRANSFER_WRITE_BIT;
	pre_blit_barriers[1].oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;
	pre_blit_barriers[1].newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	pre_blit_barriers[1].srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
	pre_blit_barriers[1].dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
	pre_blit_barriers[1].image=display->swapchainImages[image_index];
	pre_blit_barriers[1].subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
	pre_blit_barriers[1].subresourceRange.baseMipLevel=0;
	pre_blit_barriers[1].subresourceRange.levelCount=1;
	pre_blit_barriers[1].subresourceRange.baseArrayLayer=0;
	pre_blit_barriers[1].subresourceRange.layerCount=1;

	VkDependencyInfo pre_blit_dep_info={0};
	pre_blit_dep_info.sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	pre_blit_dep_info.imageMemoryBarrierCount=2;
	pre_blit_dep_info.pImageMemoryBarriers=pre_blit_barriers;
	vkCmdPipelineBarrier2(res->commandBuffer, &pre_blit_dep_info);

	VkImageBlit blit_region={0};
	blit_region.srcSubresource.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
	blit_region.srcSubresource.mipLevel=0;
	blit_region.srcSubresource.baseArrayLayer=0;
	blit_region.srcSubresource.layerCount=1;
	blit_region.srcOffsets[1].x=(int32_t)display->swapchainWidth;
	blit_region.srcOffsets[1].y=(int32_t)display->swapchainHeight;
	blit_region.srcOffsets[1].z=1;

	blit_region.dstSubresource.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
	blit_region.dstSubresource.mipLevel=0;
	blit_region.dstSubresource.baseArrayLayer=0;
	blit_region.dstSubresource.layerCount=1;
	blit_region.dstOffsets[1].x=(int32_t)display->swapchainWidth;
	blit_region.dstOffsets[1].y=(int32_t)display->swapchainHeight;
	blit_region.dstOffsets[1].z=1;

	vkCmdBlitImage(res->commandBuffer,
		display->outputImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		display->swapchainImages[image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &blit_region, VK_FILTER_NEAREST);

	//transition swapchain image to presentable layout
	VkImageMemoryBarrier2 present_barrier={0};
	present_barrier.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	present_barrier.srcStageMask=VK_PIPELINE_STAGE_2_BLIT_BIT;
	present_barrier.srcAccessMask=VK_ACCESS_2_TRANSFER_WRITE_BIT;
	present_barrier.dstStageMask=VK_PIPELINE_STAGE_2_NONE;
	present_barrier.dstAccessMask=0;
	present_barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	present_barrier.newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	present_barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
	present_barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
	present_barrier.image=display->swapchainImages[image_index];
	present_barrier.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
	present_barrier.subresourceRange.baseMipLevel=0;
	present_barrier.subresourceRange.levelCount=1;
	present_barrier.subresourceRange.baseArrayLayer=0;
	present_barrier.subresourceRange.layerCount=1;

	VkDependencyInfo present_dep_info={0};
	present_dep_info.sType=VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	present_dep_info.imageMemoryBarrierCount=1;
	present_dep_info.pImageMemoryBarriers=&present_barrier;
	vkCmdPipelineBarrier2(res->commandBuffer, &present_dep_info);

	vkEndCommandBuffer(res->commandBuffer);

	VkSemaphoreSubmitInfo wait_semaphore_info={0};
	wait_semaphore_info.sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	wait_semaphore_info.semaphore=res->imageAcquiredSemaphore;
	wait_semaphore_info.stageMask=VK_PIPELINE_STAGE_2_BLIT_BIT;

	VkSemaphoreSubmitInfo signal_semaphore_infos[2];
	signal_semaphore_infos[0].sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signal_semaphore_infos[0].pNext=NULL;
	signal_semaphore_infos[0].semaphore=display->renderCompleteSemaphores[image_index];
	signal_semaphore_infos[0].value=0;
	signal_semaphore_infos[0].stageMask=VK_PIPELINE_STAGE_2_BLIT_BIT;
	signal_semaphore_infos[0].deviceIndex=0;

	signal_semaphore_infos[1].sType=VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signal_semaphore_infos[1].pNext=NULL;
	signal_semaphore_infos[1].semaphore=display->timelineSemaphore;
	signal_semaphore_infos[1].value=frame_id;
	signal_semaphore_infos[1].stageMask=VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	signal_semaphore_infos[1].deviceIndex=0;

	VkCommandBufferSubmitInfo cmd_submit_info={0};
	cmd_submit_info.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	cmd_submit_info.commandBuffer=res->commandBuffer;

	VkSubmitInfo2 submit_info={0};
	submit_info.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit_info.waitSemaphoreInfoCount=1;
	submit_info.pWaitSemaphoreInfos=&wait_semaphore_info;
	submit_info.commandBufferInfoCount=1;
	submit_info.pCommandBufferInfos=&cmd_submit_info;
	submit_info.signalSemaphoreInfoCount=2;
	submit_info.pSignalSemaphoreInfos=signal_semaphore_infos;

	vkQueueSubmit2(display->gfxQueue, 1, &submit_info, VK_NULL_HANDLE);

	VkPresentInfoKHR present_info={0};
	present_info.sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount=1;
	present_info.pWaitSemaphores=&display->renderCompleteSemaphores[image_index];
	present_info.swapchainCount=1;
	present_info.pSwapchains=&display->swapchain;
	present_info.pImageIndices=&image_index;
	present_info.pResults=NULL;

	vkQueuePresentKHR(display->gfxQueue, &present_info);
}


void createBuffer(Display_t* display, VkDeviceSize size) {

	VkBufferCreateInfo buffer_info = {0};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(display->device, &buffer_info, NULL, &display->SSBO) != VK_SUCCESS) {
		printf("failed to create SSBO.\n");
		return;
	}

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(display->device, display->SSBO, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_reqs.size;
	alloc_info.memoryTypeIndex = findMemoryTypeIndex(display, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	if (vkAllocateMemory(display->device, &alloc_info, NULL, &display->SSBOMemory) != VK_SUCCESS) {
		printf("failed to allocate SSBO memory.\n");
		return;
	}
	vkBindBufferMemory(display->device, display->SSBO, display->SSBOMemory, 0);

	printf("SSBO created.\n");
}

void createMaterialsBuffer(Display_t* display) {

	VkBufferCreateInfo buffer_info = {0};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = (VkDeviceSize)MAX_MATERIALS * sizeof(Material);
	buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(display->device, &buffer_info, NULL, &display->materialsBuffer) != VK_SUCCESS) {
		printf("failed to create materials buffer.\n");
		return;
	}

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(display->device, display->materialsBuffer, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_reqs.size;
	alloc_info.memoryTypeIndex = findMemoryTypeIndex(display, mem_reqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(display->device, &alloc_info, NULL, &display->materialsBufferMemory) != VK_SUCCESS) {
		printf("failed to allocate materials buffer memory.\n");
		return;
	}
	vkBindBufferMemory(display->device, display->materialsBuffer, display->materialsBufferMemory, 0);

	vkMapMemory(display->device, display->materialsBufferMemory, 0, buffer_info.size, 0, &display->materialsMapped);

	Material* materials = (Material*)display->materialsMapped;
	memset(materials, 0, (size_t)buffer_info.size);

	materials[1].color[0] = 1.0f; materials[1].color[1] = 0.0f; materials[1].color[2] = 0.0f; materials[1].color[3] = 1.0f;
	materials[2].color[0] = 0.0f; materials[2].color[1] = 1.0f; materials[2].color[2] = 0.0f; materials[2].color[3] = 1.0f;
	materials[3].color[0] = 0.0f; materials[3].color[1] = 0.0f; materials[3].color[2] = 1.0f; materials[3].color[3] = 1.0f;

	printf("materials buffer created.\n");
}

void createShaderDataUBO(Display_t* display) {

	VkBufferCreateInfo buffer_info = {0};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = sizeof(ShaderDataUBO);
	buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(display->device, &buffer_info, NULL, &display->shaderDataUBO) != VK_SUCCESS) {
		printf("failed to create shader data UBO.\n");
		return;
	}

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(display->device, display->shaderDataUBO, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_reqs.size;
	alloc_info.memoryTypeIndex = findMemoryTypeIndex(display, mem_reqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(display->device, &alloc_info, NULL, &display->shaderDataUBOMemory) != VK_SUCCESS) {
		printf("failed to allocate shader data UBO memory.\n");
		return;
	}
	vkBindBufferMemory(display->device, display->shaderDataUBO, display->shaderDataUBOMemory, 0);

	vkMapMemory(display->device, display->shaderDataUBOMemory, 0, sizeof(ShaderDataUBO), 0, &display->shaderDataMapped);

	printf("shader data UBO created.\n");
}

void uploadWorldToSSBO(Display_t* display, World* world) {
	VkDeviceSize size = (VkDeviceSize)world->world_size * sizeof(uint8_t);

	VkBufferCreateInfo staging_buffer_info = {0};
	staging_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	staging_buffer_info.size = size;
	staging_buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	staging_buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer staging_buffer;
	if (vkCreateBuffer(display->device, &staging_buffer_info, NULL, &staging_buffer) != VK_SUCCESS) {
		printf("failed to create world staging buffer.\n");
		return;
	}

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(display->device, staging_buffer, &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_reqs.size;
	alloc_info.memoryTypeIndex = findMemoryTypeIndex(display, mem_reqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VkDeviceMemory staging_memory;
	if (vkAllocateMemory(display->device, &alloc_info, NULL, &staging_memory) != VK_SUCCESS) {
		printf("failed to allocate world staging buffer memory.\n");
		vkDestroyBuffer(display->device, staging_buffer, NULL);
		return;
	}
	vkBindBufferMemory(display->device, staging_buffer, staging_memory, 0);

	void* mapped;
	vkMapMemory(display->device, staging_memory, 0, size, 0, &mapped);
	memcpy(mapped, world->voxels, size);
	vkUnmapMemory(display->device, staging_memory);

	VkCommandPoolCreateInfo pool_info = {0};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	pool_info.queueFamilyIndex = display->gfxQueueFamIdx;

	VkCommandPool upload_pool;
	vkCreateCommandPool(display->device, &pool_info, NULL, &upload_pool);

	VkCommandBufferAllocateInfo cmd_alloc_info = {0};
	cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_alloc_info.commandPool = upload_pool;
	cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_alloc_info.commandBufferCount = 1;

	VkCommandBuffer cmd;
	vkAllocateCommandBuffers(display->device, &cmd_alloc_info, &cmd);

	VkCommandBufferBeginInfo begin_info = {0};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(cmd, &begin_info);

	VkBufferCopy copy_region = {0};
	copy_region.size = size;
	vkCmdCopyBuffer(cmd, staging_buffer, display->SSBO, 1, &copy_region);

	vkEndCommandBuffer(cmd);

	VkSubmitInfo submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd;

	vkQueueSubmit(display->gfxQueue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(display->gfxQueue);

	vkFreeCommandBuffers(display->device, upload_pool, 1, &cmd);
	vkDestroyCommandPool(display->device, upload_pool, NULL);
	vkDestroyBuffer(display->device, staging_buffer, NULL);
	vkFreeMemory(display->device, staging_memory, NULL);

	printf("world uploaded to SSBO.\n");
}



void update_camera(Display_t* display, Camera* c) {
	ShaderDataUBO* ubo = (ShaderDataUBO*)display->shaderDataMapped;

	ubo->camera_position[0] = c->pos[0];
	ubo->camera_position[1] = c->pos[1];
	ubo->camera_position[2] = c->pos[2];

	ubo->camera_rotation[0] = c->angle[0];
	ubo->camera_rotation[1] = c->angle[1];
	ubo->camera_rotation[2] = c->angle[2];
}





void set_properties(Display_t* display, const char* name, uint32_t width, uint32_t height) {
    strcpy(display->name, name);
    display->width = width;
	display->height = height;

    display->swapchainWidth = width;
    display->swapchainHeight= height;
}