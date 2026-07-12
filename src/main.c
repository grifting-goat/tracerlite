#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

int main(int argc, char* argv[]) {
    //if (argc > 1) {if (strcmp(argv[1], "--Lan") == 0) {}}
    glfwInit();

    //create application info
    VkApplicationInfo app_info;

    app_info.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext=NULL;
	char app_name[VK_MAX_EXTENSION_NAME_SIZE];
	strcpy(app_name,"tracerlite");
	app_info.pApplicationName=app_name;
	app_info.applicationVersion=VK_MAKE_VERSION(0,0,1);
	char app_engine_name[VK_MAX_EXTENSION_NAME_SIZE];
	strcpy(app_engine_name,"vulkan_engine");
	app_info.pEngineName=app_engine_name;
	app_info.engineVersion=VK_MAKE_VERSION(0,0,1);
	app_info.apiVersion=VK_API_VERSION_1_4;

	// create instance info
	VkInstanceCreateInfo instance_create_info;
	instance_create_info.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pNext=NULL;
	instance_create_info.flags=0;
	instance_create_info.pApplicationInfo=&app_info;
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

	//create instance
	VkInstance instance;

	vkCreateInstance(&instance_create_info, NULL, &instance);
	printf("instance created.\n");


	//enum physical device
	//
	uint32_t physical_device_count=0;
	vkEnumeratePhysicalDevices(instance,&physical_device_count,NULL);

	VkPhysicalDevice physical_device[physical_device_count];
	vkEnumeratePhysicalDevices(instance,&physical_device_count,physical_device);

	/*
	select physical device
	*/
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

	VkPhysicalDevice *physical_device_best = &(physical_device[physical_device_best_index]);

	//query que families
	uint32_t queue_family_prop_count=0;
	vkGetPhysicalDeviceQueueFamilyProperties(*physical_device_best,&queue_family_prop_count,NULL);
	VkQueueFamilyProperties queue_family_props[queue_family_prop_count];
	vkGetPhysicalDeviceQueueFamilyProperties(*physical_device_best,&queue_family_prop_count,queue_family_props);

	uint32_t queue_family_queue_count[queue_family_prop_count];
	for(uint32_t i=0;i<queue_family_prop_count;i++){
		queue_family_queue_count[i]=queue_family_props[i].queueCount;
	}

	//create logical device
	VkDeviceQueueCreateInfo device_queue_create_infos[queue_family_prop_count];
	float *queue_priors[queue_family_prop_count];
	for(uint32_t i=0;i<queue_family_prop_count;i++){
		device_queue_create_infos[i].sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		device_queue_create_infos[i].pNext=NULL;
		device_queue_create_infos[i].flags=0;
		device_queue_create_infos[i].queueFamilyIndex=i;
		device_queue_create_infos[i].queueCount=queue_family_queue_count[i];
		queue_priors[i]=malloc(queue_family_queue_count[i]*sizeof(float));
		for(uint32_t j=0;j<queue_family_queue_count[i];j++){
			queue_priors[i][j]=1.0f;
		}
		device_queue_create_infos[i].pQueuePriorities=queue_priors[i];
	}
	printf("using %d queue families.\n",queue_family_prop_count);

	VkDeviceCreateInfo device_create_info;
	device_create_info.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_create_info.pNext=NULL;
	device_create_info.flags=0;
	device_create_info.queueCreateInfoCount=queue_family_prop_count;
	device_create_info.pQueueCreateInfos=device_queue_create_infos;
	device_create_info.enabledLayerCount=0;
	device_create_info.ppEnabledLayerNames=NULL;

	uint32_t device_extension_count=1;
	device_create_info.enabledExtensionCount=device_extension_count;
	char pp_device_extensions[device_extension_count][VK_MAX_EXTENSION_NAME_SIZE];
	strcpy(pp_device_extensions[0],"VK_KHR_swapchain");
	char *pp_device_extension_names[device_extension_count];
	for(uint32_t i=0;i<device_extension_count;i++){
		pp_device_extension_names[i]=pp_device_extensions[i];
	}
	device_create_info.ppEnabledExtensionNames=
		(const char * const *)pp_device_extension_names;

	VkPhysicalDeviceFeatures physical_device_features;
	vkGetPhysicalDeviceFeatures(*physical_device_best,&physical_device_features);
	device_create_info.pEnabledFeatures=&physical_device_features;

	VkDevice device;
	vkCreateDevice(*physical_device_best,&device_create_info,NULL,&device);
	printf("logical device created.\n");

	for(uint32_t i=0;i<queue_family_prop_count;i++){free(queue_priors[i]);}


	//select best queue
	uint32_t queue_family_graph_count=0;
	uint32_t queue_family_graph_list[queue_family_prop_count];
	for(uint32_t i=0;i<queue_family_prop_count;i++){
		if((queue_family_props[i].queueFlags&VK_QUEUE_GRAPHICS_BIT) != 0){
			queue_family_graph_list[queue_family_graph_count]=i;
			queue_family_graph_count++;
		}
	}

	uint32_t max_queue_count=0;
	uint32_t queue_family_best_index=0;
	for(uint32_t i=0;i<queue_family_graph_count;i++){
		if(queue_family_props[queue_family_graph_list[i]].queueCount>max_queue_count){
			queue_family_best_index=queue_family_graph_list[i];
			max_queue_count=queue_family_props[queue_family_graph_list[i]].queueCount;
		}
	}
	printf("best queue family index:%d\n",queue_family_best_index);

	VkQueue queue_graph, queue_present;
	vkGetDeviceQueue(device,queue_family_best_index,0,&queue_graph);
	char single_queue=1;
	if(queue_family_props[queue_family_best_index].queueCount<2){
		vkGetDeviceQueue(device,queue_family_best_index,0,&queue_present);
		printf("using single queue for drawing.\n");
	}else{
		single_queue=0;
		vkGetDeviceQueue(device,queue_family_best_index,1,&queue_present);
		printf("using double queues for drawing.\n");
	}



	/*
	
	window and surface creation part

	*/

	//create window and surface

	glfwWindowHint(GLFW_CLIENT_API,GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE,GLFW_FALSE);
	GLFWwindow *window = glfwCreateWindow(1280,720,"Tracerlite",NULL,NULL);
	printf("window created.\n");

	GLFWimage icons[3];
	icons[0].pixels = stbi_load("icon_32.png", &icons[0].width, &icons[0].height, 0, 4);
	icons[1].pixels = stbi_load("icon_64.png", &icons[1].width, &icons[1].height, 0, 4);
	icons[2].pixels = stbi_load("icon_128.png", &icons[2].width, &icons[2].height, 0, 4);

	if (icons[0].pixels && icons[1].pixels && icons[2].pixels) {
		glfwSetWindowIcon(window, 3, icons); 
	} else {printf("Icon failed.\n");}
	
	if (icons[0].pixels) stbi_image_free(icons[0].pixels);
	if (icons[1].pixels) stbi_image_free(icons[1].pixels);
	if (icons[2].pixels) stbi_image_free(icons[2].pixels);


	VkSurfaceKHR surface;
	glfwCreateWindowSurface(instance,window,NULL,&surface);
	printf("surface created.\n");

	//verify surface support
    VkBool32 phyical_surface_supported;
	vkGetPhysicalDeviceSurfaceSupportKHR(*physical_device_best,queue_family_best_index,surface,&phyical_surface_supported);

	if(phyical_surface_supported==VK_TRUE){printf("surface supported.\n");}
	else{printf("warning:surface unsupported!\n");}

	//fetch surface capabilities

	VkSurfaceCapabilitiesKHR surface_capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*physical_device_best,surface,&surface_capabilities);
	printf("fetched caps from surface.\n");
	char extent_suitable=1;
	int window_w,window_h;
	glfwGetFramebufferSize(window,&window_w,&window_h);
	VkExtent2D actual_extent;
	actual_extent.width=window_w;
	actual_extent.height=window_h;
	if(surface_capabilities.currentExtent.width != window_w || surface_capabilities.currentExtent.height != window_h){

		extent_suitable=0;
		printf("actual extent size doesn't match framebuffers, resizing...\n");
		actual_extent.width=
			window_w>surface_capabilities
			.maxImageExtent.width?
			surface_capabilities
			.maxImageExtent.width
			:window_w;
		actual_extent.width=
			window_w<surface_capabilities
			.minImageExtent.width?
			surface_capabilities
			.minImageExtent.width
			:window_w;
		actual_extent.height=
			window_h>surface_capabilities
			.maxImageExtent.height?
			surface_capabilities
			.maxImageExtent.height
			:window_h;
		actual_extent.height=
			window_h<surface_capabilities
			.minImageExtent.height?
			surface_capabilities
			.minImageExtent.height
			:window_h;
	}

	//fetch surface formats
	uint32_t surface_form_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(*physical_device_best,surface,&surface_form_count,NULL);
	VkSurfaceFormatKHR surface_formats[surface_form_count];
	vkGetPhysicalDeviceSurfaceFormatsKHR(*physical_device_best,surface,&surface_form_count,surface_formats);
	printf("fetched %d surface formats.\n",surface_form_count);
	for(uint32_t i=0;i<surface_form_count;i++){
		printf("format:%d\tcolorspace:%d\n",surface_formats[i].format,surface_formats[i].colorSpace);
	}

	//fetch surface present mode
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(*physical_device_best,surface,&present_mode_count,NULL);
	VkPresentModeKHR present_modes[present_mode_count];
	vkGetPhysicalDeviceSurfacePresentModesKHR(*physical_device_best,surface,&present_mode_count,present_modes);
	printf("fetched %d present modes.\n",present_mode_count);
	char mailbox_mode_supported=0;
	for(uint32_t i=0;i<present_mode_count;i++){
		printf("present mode:%d\n",present_modes[i]);
		if(present_modes[i]==VK_PRESENT_MODE_MAILBOX_KHR){
			printf("mailbox present mode supported.\n");
			mailbox_mode_supported=1;
		}
	}

	//create swapchain

	VkSwapchainCreateInfoKHR swap_create_info;
	swap_create_info.sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swap_create_info.pNext=NULL;
	swap_create_info.flags=0;
	swap_create_info.surface=surface;
	swap_create_info.minImageCount=surface_capabilities.minImageCount+1;
	swap_create_info.imageFormat=surface_formats[0].format;
	swap_create_info.imageColorSpace=surface_formats[0].colorSpace;
	swap_create_info.imageExtent = extent_suitable ? surface_capabilities.currentExtent : actual_extent;
	swap_create_info.imageArrayLayers=1;
	swap_create_info.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swap_create_info.imageSharingMode = single_queue? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
	swap_create_info.queueFamilyIndexCount = single_queue ? 0 : 2;
	uint32_t qf_indices[2]={0,1};
	swap_create_info.pQueueFamilyIndices = single_queue ? NULL : qf_indices;
	swap_create_info.preTransform=surface_capabilities.currentTransform;
	swap_create_info.compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swap_create_info.presentMode = mailbox_mode_supported ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
	swap_create_info.clipped=VK_TRUE;
	swap_create_info.oldSwapchain=VK_NULL_HANDLE;

	VkSwapchainKHR swap;
	vkCreateSwapchainKHR(device,&swap_create_info,NULL,&swap);
	printf("swapchain created.\n");

	//fetch image from swapchain
	
	uint32_t swap_image_count=0;
	vkGetSwapchainImagesKHR(device,swap,&swap_image_count,NULL);
	VkImage swap_images[swap_image_count];
	vkGetSwapchainImagesKHR(device,swap,&swap_image_count,swap_images);
	printf("%d images fetched from swapchain.\n",swap_image_count);


	//create image view

	VkImageView image_views[swap_image_count];
	VkImageViewCreateInfo image_view_cre_infos[swap_image_count];

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

	for(uint32_t i=0;i<swap_image_count;i++){
		image_view_cre_infos[i].sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_cre_infos[i].pNext=NULL;
		image_view_cre_infos[i].flags=0;
		image_view_cre_infos[i].image=swap_images[i];
		image_view_cre_infos[i].viewType=VK_IMAGE_VIEW_TYPE_2D;
		image_view_cre_infos[i].format=surface_formats[0].format;
		image_view_cre_infos[i].components=image_view_rgba_component;
		image_view_cre_infos[i].subresourceRange=image_view_subresource;

		vkCreateImageView(device,&image_view_cre_infos[i],NULL,&image_views[i]);

		printf("image view %d created.\n",i);
	}

	/*
	render pass creation
	*/

	//fill attachment description

	VkAttachmentDescription attachment_descption;
	attachment_descption.flags=0;
	attachment_descption.format=swap_create_info.imageFormat;
	attachment_descption.samples=VK_SAMPLE_COUNT_1_BIT;
	attachment_descption.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment_descption.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
	attachment_descption.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment_descption.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment_descption.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
	attachment_descption.finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	printf("attachment description filled.\n");


	//fill attachment reference
	
	VkAttachmentReference attach_reference;
	attach_reference.attachment=0;
	attach_reference.layout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	printf("attachment reference filled.\n");

	//fill subpass description
	
	VkSubpassDescription subpass_description;
	subpass_description.flags=0;
	subpass_description.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_description.inputAttachmentCount=0;
	subpass_description.pInputAttachments=NULL;
	subpass_description.colorAttachmentCount=1;
	subpass_description.pColorAttachments=&attach_reference;
	subpass_description.pResolveAttachments=NULL;
	subpass_description.pDepthStencilAttachment=NULL;
	subpass_description.preserveAttachmentCount=0;
	subpass_description.pPreserveAttachments=NULL;

	printf("subpass description filled.\n");

	//fill subpass dependency

	VkSubpassDependency subpass_dependency;
	subpass_dependency.srcSubpass=VK_SUBPASS_EXTERNAL;
	subpass_dependency.dstSubpass=0;
	subpass_dependency.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.srcAccessMask=0;
	subpass_dependency.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpass_dependency.dependencyFlags=0;
	printf("subpass dependency created.\n");


	//create render pass
	
	VkRenderPassCreateInfo renderpass_creation_info;
	renderpass_creation_info.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderpass_creation_info.pNext=NULL;
	renderpass_creation_info.flags=0;
	renderpass_creation_info.attachmentCount=1;
	renderpass_creation_info.pAttachments=&attachment_descption;
	renderpass_creation_info.subpassCount=1;
	renderpass_creation_info.pSubpasses=&subpass_description;
	renderpass_creation_info.dependencyCount=1;
	renderpass_creation_info.pDependencies=&subpass_dependency;

	VkRenderPass renderpass;
	vkCreateRenderPass(device,&renderpass_creation_info,NULL,&renderpass);
	printf("render pass created.\n");

	/*
	pipeline creation part
	*/

	//load shader
	
	FILE *fp_vert=NULL,
		 *fp_frag=NULL;
	fp_vert=fopen("shaders/vert.spv","rb+");
	fp_frag=fopen("shaders/frag.spv","rb+");

	char shader_loaded=1;
	if(fp_vert==NULL||fp_frag==NULL){
		shader_loaded=0;
		printf("can't find SPIR-V binaries.\n");
	}

	fseek(fp_vert,0,SEEK_END);
	fseek(fp_frag,0,SEEK_END);
	uint32_t vert_size=ftell(fp_vert);
	uint32_t frag_size=ftell(fp_frag);

	char *p_vert_code=(char *)malloc(vert_size*sizeof(char));
	char *p_frag_code=(char *)malloc(frag_size*sizeof(char));

	rewind(fp_vert);
	rewind(fp_frag);
	fread(p_vert_code,1,vert_size,fp_vert);
	printf("vertex shader binaries loaded.\n");
	fread(p_frag_code,1,frag_size,fp_frag);
	printf("fragment shader binaries loaded.\n");

	fclose(fp_vert);
	fclose(fp_frag);


	//create shader modules
	VkShaderModuleCreateInfo vertex_shader_module_create_info;
	vertex_shader_module_create_info.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vertex_shader_module_create_info.pNext=NULL;
	vertex_shader_module_create_info.flags=0;
	vertex_shader_module_create_info.codeSize=shader_loaded? vert_size:0;
	vertex_shader_module_create_info.pCode= shader_loaded ? (const uint32_t *)p_vert_code : NULL;

	VkShaderModuleCreateInfo fragment_shader_module_create_info;
	fragment_shader_module_create_info.sType= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	fragment_shader_module_create_info.pNext=NULL;
	fragment_shader_module_create_info.flags=0;
	fragment_shader_module_create_info.codeSize=shader_loaded?frag_size:0;
	fragment_shader_module_create_info.pCode= shader_loaded ? (const uint32_t *)p_frag_code : NULL;

	VkShaderModule vertex_shader_module, fragment_shader_module;
	vkCreateShaderModule (device,&vertex_shader_module_create_info,NULL,&vertex_shader_module);
	printf("vertex shader module created.\n");
	vkCreateShaderModule(device,&fragment_shader_module_create_info,NULL,&fragment_shader_module);
	printf("fragment shader module created.\n");




	//fill shader stage info

	VkPipelineShaderStageCreateInfo vertex_shader_stage_creation_info, fragment_shader_stage_creation_info, shader_stage_creation_infos[2];

	vertex_shader_stage_creation_info.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertex_shader_stage_creation_info.pNext=NULL;
	vertex_shader_stage_creation_info.flags=0;
	vertex_shader_stage_creation_info.stage=VK_SHADER_STAGE_VERTEX_BIT;
	vertex_shader_stage_creation_info.module=vertex_shader_module;
	char vertex_entry[VK_MAX_EXTENSION_NAME_SIZE];
	strcpy(vertex_entry,"main");
	vertex_shader_stage_creation_info.pName=vertex_entry;
	vertex_shader_stage_creation_info.pSpecializationInfo=NULL;
	printf("vertex shader stage info filled.\n");

	fragment_shader_stage_creation_info.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragment_shader_stage_creation_info.pNext=NULL;
	fragment_shader_stage_creation_info.flags=0;
	fragment_shader_stage_creation_info.stage=VK_SHADER_STAGE_FRAGMENT_BIT;
	fragment_shader_stage_creation_info.module=fragment_shader_module;
	char fragment_entry[VK_MAX_EXTENSION_NAME_SIZE];
	strcpy(fragment_entry,"main");
	fragment_shader_stage_creation_info.pName=fragment_entry;
	fragment_shader_stage_creation_info.pSpecializationInfo=NULL;
	printf("fragment shader stage info filled.\n");

	shader_stage_creation_infos[0]=vertex_shader_stage_creation_info;
	shader_stage_creation_infos[1]=fragment_shader_stage_creation_info;


	//fill vertex input state info
	
	VkPipelineVertexInputStateCreateInfo vertex_input_info;

	vertex_input_info.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.pNext=NULL;
	vertex_input_info.flags=0;
	vertex_input_info.vertexBindingDescriptionCount=0;
	vertex_input_info.pVertexBindingDescriptions=NULL;
	vertex_input_info.vertexAttributeDescriptionCount=0;
	vertex_input_info.pVertexAttributeDescriptions=NULL;

	printf("vertex input state info filled.\n");


	//fill input assembly state info
	
	VkPipelineInputAssemblyStateCreateInfo input_asm_info;
	input_asm_info.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_asm_info.pNext=NULL;
	input_asm_info.flags=0;
	input_asm_info.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_asm_info.primitiveRestartEnable=VK_FALSE;
	printf("input assembly info filled.\n");

	//fill viewport
	
	VkViewport viewport;
	viewport.x=0.0f;
	viewport.y=0.0f;
	viewport.width=(float)swap_create_info.imageExtent.width;
	viewport.height=(float)swap_create_info.imageExtent.height;
	viewport.minDepth=0.0f;
	viewport.maxDepth=1.0f;
	printf("viewport filled.\n");


	//fill scissor

	VkRect2D scissor;
	VkOffset2D sci_offset;
	sci_offset.x=0;
	sci_offset.y=0;
	scissor.offset=sci_offset;
	scissor.extent=swap_create_info.imageExtent;
	printf("scissor filled.\n");

	//fill viewport state info
	
	VkPipelineViewportStateCreateInfo pipeline_viewport_state_creation_info;
	pipeline_viewport_state_creation_info.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipeline_viewport_state_creation_info.pNext=NULL;
	pipeline_viewport_state_creation_info.flags=0;
	pipeline_viewport_state_creation_info.viewportCount=1;
	pipeline_viewport_state_creation_info.pViewports=&viewport;
	pipeline_viewport_state_creation_info.scissorCount=1;
	pipeline_viewport_state_creation_info.pScissors=&scissor;
	printf("viewport state filled.\n");


	//fill rasterizer state info
	VkPipelineRasterizationStateCreateInfo rasterization_create_info;
	rasterization_create_info.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_create_info.pNext=NULL;
	rasterization_create_info.flags=0;
	rasterization_create_info.depthClampEnable=VK_FALSE;
	rasterization_create_info.rasterizerDiscardEnable=VK_FALSE;
	rasterization_create_info.polygonMode=VK_POLYGON_MODE_FILL;
	rasterization_create_info.cullMode=VK_CULL_MODE_BACK_BIT;
	rasterization_create_info.frontFace=VK_FRONT_FACE_CLOCKWISE;
	rasterization_create_info.depthBiasEnable=VK_FALSE;
	rasterization_create_info.depthBiasConstantFactor=0.0f;
	rasterization_create_info.depthBiasClamp=0.0f;
	rasterization_create_info.depthBiasSlopeFactor=0.0f;
	rasterization_create_info.lineWidth=1.0f;

	printf("rasterization info filled.\n");

	//fill multisampling state info
	
	VkPipelineMultisampleStateCreateInfo multiple_sample_create_info;
	multiple_sample_create_info.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiple_sample_create_info.pNext=NULL;
	multiple_sample_create_info.flags=0;
	multiple_sample_create_info.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
	multiple_sample_create_info.sampleShadingEnable=VK_FALSE;
	multiple_sample_create_info.minSampleShading=1.0f;
	multiple_sample_create_info.pSampleMask=NULL;
	multiple_sample_create_info.alphaToCoverageEnable=VK_FALSE;
	multiple_sample_create_info.alphaToOneEnable=VK_FALSE;

	printf("multisample info filled.\n");

	//fill color blend attachment state

	VkPipelineColorBlendAttachmentState color_blend_attach;
	color_blend_attach.blendEnable=VK_FALSE;
	color_blend_attach.srcColorBlendFactor=VK_BLEND_FACTOR_ONE;
	color_blend_attach.dstColorBlendFactor=VK_BLEND_FACTOR_ZERO;
	color_blend_attach.colorBlendOp=VK_BLEND_OP_ADD;
	color_blend_attach.srcAlphaBlendFactor=VK_BLEND_FACTOR_ONE;
	color_blend_attach.dstAlphaBlendFactor=VK_BLEND_FACTOR_ZERO;
	color_blend_attach.alphaBlendOp=VK_BLEND_OP_ADD;

	color_blend_attach.colorWriteMask
		=VK_COLOR_COMPONENT_R_BIT
		|VK_COLOR_COMPONENT_G_BIT
		|VK_COLOR_COMPONENT_B_BIT
		|VK_COLOR_COMPONENT_A_BIT;

	printf("color blend attachment state filled.\n");


	//fill color blend state info
	
	VkPipelineColorBlendStateCreateInfo color_blend_create_info;
	color_blend_create_info.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_create_info.pNext=NULL;
	color_blend_create_info.flags=0;
	color_blend_create_info.logicOpEnable=VK_FALSE;
	color_blend_create_info.logicOp=VK_LOGIC_OP_COPY;
	color_blend_create_info.attachmentCount=1;
	color_blend_create_info.pAttachments=&color_blend_attach;

	for(uint32_t i=0;i<4;i++){
		color_blend_create_info.blendConstants[i]=0.0f;
	}

	printf("color blend state info filled.\n");


	//create pipeline layout
	
	VkPipelineLayoutCreateInfo pipe_layout_create_info;
	pipe_layout_create_info.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipe_layout_create_info.pNext=NULL;
	pipe_layout_create_info.flags=0;
	pipe_layout_create_info.setLayoutCount=0;
	pipe_layout_create_info.pSetLayouts=NULL;
	pipe_layout_create_info.pushConstantRangeCount=0;
	pipe_layout_create_info.pPushConstantRanges=NULL;

	VkPipelineLayout pipe_layout;
	vkCreatePipelineLayout(device,&pipe_layout_create_info,NULL,&pipe_layout);

	printf("pipeline layout created.\n");


	//create pipeline
	
	VkGraphicsPipelineCreateInfo pipe_create_info;
	pipe_create_info.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipe_create_info.pNext=NULL;
	pipe_create_info.flags=0;
	pipe_create_info.stageCount=2;
	pipe_create_info.pStages=shader_stage_creation_infos;
	pipe_create_info.pVertexInputState=&vertex_input_info;
	pipe_create_info.pInputAssemblyState=&input_asm_info;
	pipe_create_info.pTessellationState=NULL;
	pipe_create_info.pViewportState=&pipeline_viewport_state_creation_info;
	pipe_create_info.pRasterizationState=&rasterization_create_info;
	pipe_create_info.pMultisampleState=&multiple_sample_create_info;
	pipe_create_info.pDepthStencilState=NULL;
	pipe_create_info.pColorBlendState=&color_blend_create_info;
	pipe_create_info.pDynamicState=NULL;

	pipe_create_info.layout=pipe_layout;
	pipe_create_info.renderPass=renderpass;
	pipe_create_info.subpass=0;
	pipe_create_info.basePipelineHandle=VK_NULL_HANDLE;
	pipe_create_info.basePipelineIndex=-1;

	VkPipeline pipe;
	vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pipe_create_info,NULL,&pipe);
	printf("graphics pipeline created.\n");

	//destroy shader module
	
	vkDestroyShaderModule(device,fragment_shader_module,NULL);
	printf("fragment shader module destroyed.\n");
	vkDestroyShaderModule(device,vertex_shader_module,NULL);
	printf("vertex shader module destroyed.\n");
	free(p_frag_code);
	printf("fragment shader binaries released.\n");
	free(p_vert_code);
	printf("vertex shader binaries released.\n");

	/*
	framebuffer creation
	*/

	//create framebuffer
	VkFramebufferCreateInfo frame_buffer_creation_infos[swap_image_count];
	VkFramebuffer frame_buffers[swap_image_count];
	VkImageView image_attachs[swap_image_count];
	for(uint32_t i=0;i<swap_image_count;i++){
		image_attachs[i]=image_views[i];
		frame_buffer_creation_infos[i].sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frame_buffer_creation_infos[i].pNext=NULL;
		frame_buffer_creation_infos[i].flags=0;
		frame_buffer_creation_infos[i].renderPass=renderpass;
		frame_buffer_creation_infos[i].attachmentCount=1;
		frame_buffer_creation_infos[i].pAttachments=&(image_attachs[i]);
		frame_buffer_creation_infos[i].width=swap_create_info.imageExtent.width;
		frame_buffer_creation_infos[i].height=swap_create_info.imageExtent.height;
		frame_buffer_creation_infos[i].layers=1;

		vkCreateFramebuffer(device,&(frame_buffer_creation_infos[i]),NULL,&(frame_buffers[i]));
		printf("framebuffer %d created.\n",i);
	}


	/*
	command buffer creation
	*/

	//create command pool
	
	VkCommandPoolCreateInfo cmd_pool_create_info;
	cmd_pool_create_info.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmd_pool_create_info.pNext=NULL;
	cmd_pool_create_info.flags=0;
	cmd_pool_create_info.queueFamilyIndex=queue_family_best_index;

	VkCommandPool cmd_pool;
	vkCreateCommandPool(device,&cmd_pool_create_info,NULL,&cmd_pool);
	printf("command pool created.\n");
	
	//allocate command buffers
	
	VkCommandBufferAllocateInfo cmd_buffer_alloc_info;
	cmd_buffer_alloc_info.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmd_buffer_alloc_info.pNext=NULL;
	cmd_buffer_alloc_info.commandPool=cmd_pool;
	cmd_buffer_alloc_info.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_buffer_alloc_info.commandBufferCount=swap_image_count;

	VkCommandBuffer cmd_buffers[swap_image_count];
	vkAllocateCommandBuffers(device,&cmd_buffer_alloc_info,cmd_buffers);
	printf("command buffers allocated.\n");

	/*
	render preparation
	*/
	VkCommandBufferBeginInfo cmd_buffer_begin_infos[swap_image_count];
	VkRenderPassBeginInfo renderpass_begin_infos[swap_image_count];
	VkRect2D renderpass_area;
	renderpass_area.offset.x=0;
	renderpass_area.offset.y=0;
	renderpass_area.extent=swap_create_info.imageExtent;
	VkClearValue clear_val={0.6f,0.2f,0.8f,0.0f};
	for(uint32_t i=0;i<swap_image_count;i++){

		cmd_buffer_begin_infos[i].sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmd_buffer_begin_infos[i].pNext=NULL;
		cmd_buffer_begin_infos[i].flags=0;
		cmd_buffer_begin_infos[i].pInheritanceInfo=NULL;
		printf("command buffer begin info %d filled.\n",i);

		renderpass_begin_infos[i].sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderpass_begin_infos[i].pNext=NULL;
		renderpass_begin_infos[i].renderPass=renderpass;
		renderpass_begin_infos[i].framebuffer=frame_buffers[i];
		renderpass_begin_infos[i].renderArea=renderpass_area;
		renderpass_begin_infos[i].clearValueCount=1;
		renderpass_begin_infos[i].pClearValues=&clear_val;

		printf("render pass begin info %d filled.\n",i);

		vkBeginCommandBuffer(cmd_buffers[i],&cmd_buffer_begin_infos[i]);

		vkCmdBeginRenderPass(cmd_buffers[i],&(renderpass_begin_infos[i]),VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(cmd_buffers[i],VK_PIPELINE_BIND_POINT_GRAPHICS,pipe);

		vkCmdDraw(cmd_buffers[i],3,1,0,0);

		vkCmdEndRenderPass(cmd_buffers[i]);

		vkEndCommandBuffer(cmd_buffers[i]);

		printf("command buffer drawing recorded.\n");
	}

	//semaphores and fences creation part

	uint32_t max_frames=2;
	VkSemaphore semaphore_img_avl[max_frames];
	VkSemaphore semaphore_rend_fin[swap_image_count];
	VkFence fences[max_frames];

	VkSemaphoreCreateInfo semaphore_create_info;
	semaphore_create_info.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphore_create_info.pNext=NULL;
	semaphore_create_info.flags=0;

	VkFenceCreateInfo fence_create_info;
	fence_create_info.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_create_info.pNext=NULL;
	fence_create_info.flags=VK_FENCE_CREATE_SIGNALED_BIT;

	for(uint32_t i=0;i<max_frames;i++){
		vkCreateSemaphore(device,&semaphore_create_info,NULL,&(semaphore_img_avl[i]));
		vkCreateFence(device,&fence_create_info,NULL,&(fences[i]));
	}
	for(uint32_t i=0;i<swap_image_count;i++){
		vkCreateSemaphore(device,&semaphore_create_info,NULL,&(semaphore_rend_fin[i]));
	}
	printf("semaphores and fences created.\n");

	uint32_t current_frame=0;
	VkFence fence_img[swap_image_count];
	for(uint32_t i=0;i<swap_image_count;i++){
		fence_img[i]=VK_NULL_HANDLE;
	}


	//main present part

	printf("\n");
	while(!glfwWindowShouldClose(window)){
		glfwPollEvents();
	
		//submit
	
		vkWaitForFences(device,1,&(fences[current_frame]),VK_TRUE,UINT64_MAX);

		uint32_t img_index=0;
		vkAcquireNextImageKHR(device,swap,UINT64_MAX,semaphore_img_avl[current_frame],VK_NULL_HANDLE,&img_index);

		if(fence_img[img_index]!=VK_NULL_HANDLE){
			vkWaitForFences(device,1,&(fence_img[img_index]),VK_TRUE,UINT64_MAX);
		}

		fence_img[img_index]=fences[current_frame];

		VkSubmitInfo submit_info;
		submit_info.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.pNext=NULL;

		VkSemaphore semaphore_wait[1];
		semaphore_wait[0]=semaphore_img_avl[current_frame];
		VkPipelineStageFlags wait_stages[1];
		wait_stages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		submit_info.waitSemaphoreCount=1;
		submit_info.pWaitSemaphores=&(semaphore_wait[0]);
		submit_info.pWaitDstStageMask=&(wait_stages[0]);
		submit_info.commandBufferCount=1;
		submit_info.pCommandBuffers=&(cmd_buffers[img_index]);

		VkSemaphore semaphore_sig[1];
		semaphore_sig[0]=semaphore_rend_fin[img_index];

		submit_info.signalSemaphoreCount=1;
		submit_info.pSignalSemaphores=&(semaphore_sig[0]);

		vkResetFences(device,1,&(fences[current_frame]));

		vkQueueSubmit(queue_graph,1,&submit_info,fences[current_frame]);

		//present
		//
		VkPresentInfoKHR present_info;

		present_info.sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.pNext=NULL;
		present_info.waitSemaphoreCount=1;
		present_info.pWaitSemaphores=&(semaphore_sig[0]);

		VkSwapchainKHR swaps[1];
		swaps[0]=swap;
		present_info.swapchainCount=1;
		present_info.pSwapchains=&(swaps[0]);
		present_info.pImageIndices=&img_index;
		present_info.pResults=NULL;

		vkQueuePresentKHR(queue_present,&present_info);

		current_frame=(current_frame+1)%max_frames;
	}

	vkDeviceWaitIdle(device);
	printf("command buffers finished.\n");

	/*
	destroy everythin
	*/

	//free command buffer

	vkFreeCommandBuffers(device,cmd_pool,swap_image_count,cmd_buffers);
	printf("command buffers freed.\n");


	//destroy semaphores and fences

	for(uint32_t i=0;i<max_frames;i++){
		vkDestroySemaphore(device,semaphore_img_avl[i],NULL);
		vkDestroyFence(device,fences[i],NULL);
	}
	for(uint32_t i=0;i<swap_image_count;i++){
		vkDestroySemaphore(device,semaphore_rend_fin[i],NULL);
	}

	printf("semaphores and fences destroyed.\n");


	//destroy command pool

	vkDestroyCommandPool(device,cmd_pool,NULL);
	printf("command pool destroyed.\n");


	//destroy frambuffer
	for(uint32_t i=0;i<swap_image_count;i++){
		vkDestroyFramebuffer(device,frame_buffers[i],NULL);
		printf("frambuffer %d destroyed.\n",i);
	}

	//destroy pipeline

	vkDestroyPipeline(device,pipe,NULL);
	printf("graphics pipeline destroyed.\n");

	//destroy pipeline layout

	vkDestroyPipelineLayout(device,pipe_layout,NULL);
	printf("pipeline layout destroyed.\n");

	//destroy render pass

	vkDestroyRenderPass(device,renderpass,NULL);
	printf("render pass destroyed.\n");

	//destroy imageview

	for(uint32_t i=0;i<swap_image_count;i++){
		vkDestroyImageView(device,image_views[i],NULL);
		printf("image view %d destroyed.\n",i);
	}

	//destroy swapchain

	vkDestroySwapchainKHR(device,swap,NULL);
	printf("swapchain destroyed.\n");

	//destroy surface and window

	vkDestroySurfaceKHR(instance,surface,NULL);
	printf("surface destroyed.\n");
	glfwDestroyWindow(window);
	printf("window destroyed.\n");

	//destroy device

	vkDestroyDevice(device,NULL);
	printf("logical device destroyed.\n");

	//destroy instance

	vkDestroyInstance(instance,NULL);
	printf("instance destroyed.\n");

	glfwTerminate();
	return 0;
}