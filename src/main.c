#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

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
	app_info.apiVersion=VK_API_VERSION_1_3;

	// create instance info
	VkInstanceCreateInfo inst_cre_info;
	inst_cre_info.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_cre_info.pNext=NULL;
	inst_cre_info.flags=0;
	inst_cre_info.pApplicationInfo=&app_info;
	uint32_t inst_layer_count = 1U;
	inst_cre_info.enabledLayerCount=inst_layer_count;
	char pp_inst_layers[inst_layer_count][VK_MAX_EXTENSION_NAME_SIZE];
	strcpy(pp_inst_layers[0],"VK_LAYER_KHRONOS_validation");
	char *pp_inst_layer_names[inst_layer_count];
	for(uint32_t i=0;i<inst_layer_count;i++){
		pp_inst_layer_names[i]=
			pp_inst_layers[i];
	}
	inst_cre_info.ppEnabledLayerNames=(const char * const *)pp_inst_layer_names;
	uint32_t inst_ext_count=0;
	const char * const *pp_inst_ext_names=
		glfwGetRequiredInstanceExtensions
		(&inst_ext_count);
	inst_cre_info.enabledExtensionCount=inst_ext_count;
	inst_cre_info.ppEnabledExtensionNames=pp_inst_ext_names;

	//create instance
	VkInstance inst;

	vkCreateInstance(&inst_cre_info, NULL, &inst);
	printf("instance created.\n");


	//enum physical device
	//
	uint32_t phy_devi_count=0;
	vkEnumeratePhysicalDevices(inst,&phy_devi_count,NULL);

	VkPhysicalDevice phy_devis[phy_devi_count];
	vkEnumeratePhysicalDevices(inst,&phy_devi_count,phy_devis);

	/*
	select physical device
	*/
	VkPhysicalDeviceProperties phy_devis_props[phy_devi_count];
	uint32_t discrete_gpu_list[phy_devi_count];
	uint32_t discrete_gpu_count=0;
	uint32_t intergrated_gpu_list[phy_devi_count];
	uint32_t intergrated_gpu_count=0;

	VkPhysicalDeviceMemoryProperties phy_devis_mem_props[phy_devi_count];
	uint32_t phy_devis_mem_count[phy_devi_count];
	VkDeviceSize phy_devis_mem_total[phy_devi_count];
	VkDeviceSize phy_devis_mem_vram[phy_devi_count];

	for(uint32_t i=0;i<phy_devi_count;i++){
		vkGetPhysicalDeviceProperties(phy_devis[i],&phy_devis_props[i]);
		if(phy_devis_props[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
			discrete_gpu_list[discrete_gpu_count]=i;
			discrete_gpu_count++;
		} else if(phy_devis_props[i].deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU){
			intergrated_gpu_list[intergrated_gpu_count]=i;
			intergrated_gpu_count++;
		}

		vkGetPhysicalDeviceMemoryProperties(phy_devis[i],&phy_devis_mem_props[i]);
		phy_devis_mem_count[i] = phy_devis_mem_props[i].memoryHeapCount;
		phy_devis_mem_total[i]=0;
		phy_devis_mem_vram[i]=0;
		for(uint32_t j=0;j<phy_devis_mem_count[i];j++){
			phy_devis_mem_total[i] += phy_devis_mem_props[i].memoryHeaps[j].size;
			if(phy_devis_mem_props[i].memoryHeaps[j].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT){
				phy_devis_mem_vram[i] += phy_devis_mem_props[i].memoryHeaps[j].size;
			}
		}
	}

	VkDeviceSize max_mem_size=0;
	uint32_t phy_devi_best_index=0;

	if(discrete_gpu_count!=0){
		for(uint32_t i=0;i<discrete_gpu_count;i++){
			if(phy_devis_mem_total[i]>max_mem_size){
				phy_devi_best_index=discrete_gpu_list[i];
				max_mem_size=phy_devis_mem_total[i];
			}
		}
	} else if(intergrated_gpu_count!=0){
		for(uint32_t i=0;i<intergrated_gpu_count;i++){
			if(phy_devis_mem_total[i]>max_mem_size){
				phy_devi_best_index=intergrated_gpu_list[i];
				max_mem_size=phy_devis_mem_total[i];
			}
		}
	}

	// Print Best Device
	printf("best device index:%u\n",phy_devi_best_index);
	printf("device name:%s\n",phy_devis_props[phy_devi_best_index].deviceName);
	printf("device type:");

	if(discrete_gpu_count!=0){printf("discrete gpu\n");}
	else if(intergrated_gpu_count!=0){printf("intergrated gpu\n");}
	else{printf("unknown\n");}

	printf("memory total:%llu\n",phy_devis_mem_total[phy_devi_best_index]);
	double mem_gb = (double)phy_devis_mem_vram[phy_devi_best_index] / (1024 * 1024 * 1024);
	printf("memory total:%.3f GB\n", mem_gb);

	VkPhysicalDevice *phy_best_devi=&(phy_devis[phy_devi_best_index]);

	//query que families
	uint32_t qf_prop_count=0;
	vkGetPhysicalDeviceQueueFamilyProperties(*phy_best_devi,&qf_prop_count,NULL);
	VkQueueFamilyProperties qf_props[qf_prop_count];
	vkGetPhysicalDeviceQueueFamilyProperties(*phy_best_devi,&qf_prop_count,qf_props);

	uint32_t qf_q_count[qf_prop_count];
	for(uint32_t i=0;i<qf_prop_count;i++){
		qf_q_count[i]=qf_props[i].queueCount;
	}

	//create logical device
	VkDeviceQueueCreateInfo dev_q_cre_infos[qf_prop_count];
	for(uint32_t i=0;i<qf_prop_count;i++){
		dev_q_cre_infos[i].sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		dev_q_cre_infos[i].pNext=NULL;
		dev_q_cre_infos[i].flags=0;
		dev_q_cre_infos[i].queueFamilyIndex=i;
		dev_q_cre_infos[i].queueCount=qf_q_count[i];
		float q_prior[1]={1.0f};
		dev_q_cre_infos[i].pQueuePriorities=q_prior;
	}
	printf("using %d queue families.\n",qf_prop_count);

	VkDeviceCreateInfo dev_cre_info;
	dev_cre_info.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	dev_cre_info.pNext=NULL;
	dev_cre_info.flags=0;
	dev_cre_info.queueCreateInfoCount=qf_prop_count;
	dev_cre_info.pQueueCreateInfos=dev_q_cre_infos;
	dev_cre_info.enabledLayerCount=0;
	dev_cre_info.ppEnabledLayerNames=NULL;

	uint32_t dev_ext_count=1;
	dev_cre_info.enabledExtensionCount=dev_ext_count;
	char pp_dev_exts[dev_ext_count][VK_MAX_EXTENSION_NAME_SIZE];
	strcpy(pp_dev_exts[0],"VK_KHR_swapchain");
	char *pp_dev_ext_names[dev_ext_count];
	for(uint32_t i=0;i<dev_ext_count;i++){
		pp_dev_ext_names[i]=pp_dev_exts[i];
	}
	dev_cre_info.ppEnabledExtensionNames=
		(const char * const *)pp_dev_ext_names;

	VkPhysicalDeviceFeatures phy_devi_feat;
	vkGetPhysicalDeviceFeatures(*phy_best_devi,&phy_devi_feat);
	dev_cre_info.pEnabledFeatures=&phy_devi_feat;

	VkDevice dev;
	vkCreateDevice(*phy_best_devi,&dev_cre_info,NULL,&dev);
	printf("logical device created.\n");


	//select best queue
	uint32_t qf_graph_count=0;
	uint32_t qf_graph_list[qf_prop_count];
	for(uint32_t i=0;i<qf_prop_count;i++){
		if((qf_props[i].queueFlags&VK_QUEUE_GRAPHICS_BIT) != 0){
			qf_graph_list[qf_graph_count]=i;
			qf_graph_count++;
		}
	}

	uint32_t max_q_count=0;
	uint32_t qf_best_index=0;
	for(uint32_t i=0;i<qf_graph_count;i++){
		if(qf_props[qf_graph_list[i]].queueCount>max_q_count){
			qf_best_index=qf_graph_list[i];
		}
	}
	printf("best queue family index:%d\n",qf_best_index);

	

    



}