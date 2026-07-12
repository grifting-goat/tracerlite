#include "vulkan_simple.h"




bool display_init(Display_t* display) {
    glfwInit();
    
    strcpy(display->name,"Tracerlite"); //temp

    //order matters

    display->app_info = createAppInfo(display);
    display->vulkanInstance = createVulkanInstance(display);
    display->physicalDevice = findPhysicalDevice(display);
    display->window = createWindow(display);

    display->surface = createSurface(display);
    display->gfxQueueFamIdx = findGraphicsQueue(display);

    display->device = createDevice(display);
    display->gfxQueue = createGraphicsQueue(display);

    
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
	GLFWwindow *window = glfwCreateWindow(1280,720,"Tracerlite",NULL,NULL);
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
	printf("memory total:%.3f GB\n", mem_gb);

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