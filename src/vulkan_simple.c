#include "vulkan_simple.h"


//private
void set_properties(Display_t* display, const char* name, uint32_t width, uint32_t height);


bool display_init(Display_t* display) {
    glfwInit();
    
    set_properties(display, "Tracerlite", 1280U, 720U);

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
    createSwapchain(display);

    

    return true;
}

void display_run(Display_t* display) {

}

bool display_close(Display_t* display) {

    glfwDestroyWindow(display->window);
	printf("window destroyed.\n");

    glfwTerminate();
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
	GLFWwindow *window = glfwCreateWindow(display->width,display->height,display->name,NULL,NULL);
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


VkSwapchainKHR createSwapchain(Display_t* display) {

    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(display->physicalDevice,display->surface,&surface_capabilities);
    printf("fetched capabilities from surface.\n");

    uint32_t requestedImageCount = surface_capabilities.minImageCount < 2U ? 2U : surface_capabilities.minImageCount;
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

    //create swapchain

	VkSwapchainCreateInfoKHR swap_create_info;
    
	swap_create_info.sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swap_create_info.pNext=NULL;
	swap_create_info.flags=0;
	swap_create_info.surface=display->surface;
	swap_create_info.minImageCount=surface_capabilities.minImageCount+1;
	swap_create_info.imageFormat=surface_formats[0].format;
	swap_create_info.imageColorSpace=surface_formats[0].colorSpace;
	swap_create_info.imageExtent = actual_extent;
	swap_create_info.imageArrayLayers=1;
	swap_create_info.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

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
		return NULL;
	}
    printf("swapchain created\n");

    // ask for the swapchain images
	uint32_t swap_image_count = 0;
	vkGetSwapchainImagesKHR(display->device, display->swapchain, &swap_image_count, NULL);
	display->swapchainImages = calloc(swap_image_count, sizeof(VkImage));
	vkGetSwapchainImagesKHR(display->device, display->swapchain, &swap_image_count, display->swapchainImages);
	display->swapchainImageViews = calloc(swap_image_count, sizeof(VkImageView));

    //create image view
	VkImageView image_views[swap_image_count];
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
		image_view_creation_infos[i].format=surface_formats[0].format;
		image_view_creation_infos[i].components=image_view_rgba_component;
		image_view_creation_infos[i].subresourceRange=image_view_subresource;

		vkCreateImageView(display->device,&image_view_creation_infos[i],NULL,&image_views[i]);

		printf("image view %d created.\n",i);
    }
}




void set_properties(Display_t* display, const char* name, uint32_t width, uint32_t height) {
    strcpy(display->name, name);
    display->width = width;
	display->height = height;

    display->swapchainWidth = width;
    display->swapchainHeight= height;
}