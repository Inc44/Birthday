// gcc -o birthday_vulkan birthday_vulkan.c -lvulkan -Ofast && ./birthday_vulkan
#define BLOCK_SIZE 32
#define DAYS_IN_YEAR 365
#define NUM_THREADS 768	 // for GTX 1660 SUPER
// #define NUM_THREADS 2176 for RTX 4060 TI and Intel Arc
#define PEOPLE 24
#define TOTAL_SIMULATIONS 1000000
#define MULTIPLIER 1664525
#define INCREMENT 1013904223
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
typedef struct {
	int simulations;
	unsigned int seed;
	int days_in_year;
	int people;
	int num_threads;
	int multiplier;
	int increment;
} ThreadData;
const char* SIMULATE =
	"#version 450\n"
	"layout(push_constant) uniform ThreadData {"
	"	int simulations;"
	"	uint seed;"
	"	int days_in_year;"
	"	int people;"
	"	int num_threads;"
	"	int multiplier;"
	"	int increment;"
	"}"
	"pushConstants;"
	"layout(local_size_x = 32) in;"
	"layout(std430, binding = 0) buffer OutBuf {"
	"	int successCount[];"
	"};"
	"void main() {"
	"	uint tid = gl_GlobalInvocationID.x;"
	"	int simulationsPerThread ="
	"		pushConstants.simulations / pushConstants.num_threads;"
	"	int localSuccessCount = 0;"
	"	uint state = pushConstants.seed ^ tid;"
	"	int birthdays[365];"
	"	for (int sim = 0; sim < simulationsPerThread; ++sim) {"
	"		for (int i = 0; i < pushConstants.days_in_year; ++i)"
	"			birthdays[i] = 0;"
	"		for (int i = 0; i < pushConstants.people; ++i) {"
	"			state = state * pushConstants.multiplier + "
	"pushConstants.increment;"
	"			uint birthday = state % pushConstants.days_in_year;"
	"			birthdays[birthday]++;"
	"		}"
	"		int exactlyTwoCount = 0;"
	"		for (int i = 0; i < pushConstants.days_in_year; ++i)"
	"			if (birthdays[i] == 2)"
	"				exactlyTwoCount++;"
	"		if (exactlyTwoCount == 1)"
	"			localSuccessCount++;"
	"	}"
	"	successCount[tid] = localSuccessCount;"
	"}";
size_t strlen(const char* str) {
	size_t n = 0;
	while (str[n])
		n++;
	return n;
}
void write_shader() {
	FILE* file = fopen("birthday.comp", "wb");
	size_t n = strlen(SIMULATE);
	fwrite(SIMULATE, 1, n, file);
	fclose(file);
}
void* read_spv(size_t* n) {
	FILE* file = fopen("birthday.comp.spv", "rb");
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	void* spv = malloc(size);
	fread(spv, 1, size, file);
	fclose(file);
	*n = size;
	return spv;
}
int find_memory_type(int memoryTypeBits,
					 VkMemoryPropertyFlags flags,
					 VkPhysicalDeviceMemoryProperties* memoryProperties) {
	for (int i = 0; i < memoryProperties->memoryTypeCount; i++)
		if ((memoryTypeBits & (1u << i)) &&
			(memoryProperties->memoryTypes[i].propertyFlags & flags) == flags)
			return i;
	exit(1);
}
int main() {
	struct timespec start_time, end_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	unsigned long long seed = time.tv_sec * 1e9 + time.tv_nsec;
	int* h_successCount;
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkPhysicalDeviceProperties physical_device_properties;
	VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
	VkDevice device;
	VkQueue queue;
	VkBuffer buffer;
	VkMemoryRequirements memory_requirements;
	VkDeviceMemory device_memory;
	VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSet descriptor_set;
	VkShaderModule shader_module;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
	VkCommandPool command_pool;
	VkCommandBuffer command_buffer;
	write_shader();
	system("glslangValidator -o birthday.comp.spv birthday.comp -V");
	size_t spvlen;
	void* spv = read_spv(&spvlen);
	VkApplicationInfo application_info = {0};
	application_info.apiVersion = VK_API_VERSION_1_0;
	application_info.pApplicationName = "birthday_vulkan";
	application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	VkInstanceCreateInfo instance_info = {0};
	instance_info.pApplicationInfo = &application_info;
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vkCreateInstance(&instance_info, NULL, &instance);
	int physical_device_count;
	vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL);
	VkPhysicalDevice* physical_devices = (VkPhysicalDevice*)malloc(
		sizeof(VkPhysicalDevice) * physical_device_count);
	vkEnumeratePhysicalDevices(instance, &physical_device_count,
							   physical_devices);
	int queue_family_index = -1;
	for (int pass = 0; pass < 2 && physical_device == VK_NULL_HANDLE; pass++) {
		for (int p = 0; p < physical_device_count; p++) {
			vkGetPhysicalDeviceProperties(physical_devices[p],
										  &physical_device_properties);
			if (pass == 0 && physical_device_properties.vendorID != 0x10DE)
				continue;
			int queue_family_property_count;
			vkGetPhysicalDeviceQueueFamilyProperties(
				physical_devices[p], &queue_family_property_count, NULL);
			VkQueueFamilyProperties* queue_family_properties =
				(VkQueueFamilyProperties*)malloc(
					sizeof(VkQueueFamilyProperties) *
					queue_family_property_count);
			vkGetPhysicalDeviceQueueFamilyProperties(
				physical_devices[p], &queue_family_property_count,
				queue_family_properties);
			for (int i = 0; i < queue_family_property_count; i++) {
				if (queue_family_properties[i].queueFlags &
					VK_QUEUE_COMPUTE_BIT) {
					physical_device = physical_devices[p];
					queue_family_index = i;
					break;
				}
			}
			free(queue_family_properties);
			if (physical_device != VK_NULL_HANDLE)
				break;
		}
	}
	free(physical_devices);
	if (physical_device == VK_NULL_HANDLE || queue_family_index < 0)
		exit(1);
	vkGetPhysicalDeviceMemoryProperties(physical_device,
										&physical_device_memory_properties);
	float queue_priority = 1.0f;
	VkDeviceQueueCreateInfo device_queue_info = {0};
	device_queue_info.pQueuePriorities = &queue_priority;
	device_queue_info.queueCount = 1;
	device_queue_info.queueFamilyIndex = queue_family_index;
	device_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	VkDeviceCreateInfo device_info = {0};
	device_info.pQueueCreateInfos = &device_queue_info;
	device_info.queueCreateInfoCount = 1;
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	vkCreateDevice(physical_device, &device_info, NULL, &device);
	vkGetDeviceQueue(device, queue_family_index, 0, &queue);
	int group_count_x = (NUM_THREADS + BLOCK_SIZE - 1) / BLOCK_SIZE;
	VkDeviceSize device_size =
		(VkDeviceSize)group_count_x * BLOCK_SIZE * sizeof(int);
	VkBufferCreateInfo buffer_info = {0};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_info.size = device_size;
	buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	vkCreateBuffer(device, &buffer_info, NULL, &buffer);
	vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);
	int memory_type = find_memory_type(memory_requirements.memoryTypeBits,
									   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
										   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
									   &physical_device_memory_properties);
	VkMemoryAllocateInfo memory_allocation_info = {0};
	memory_allocation_info.allocationSize = memory_requirements.size;
	memory_allocation_info.memoryTypeIndex = memory_type;
	memory_allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	vkAllocateMemory(device, &memory_allocation_info, NULL, &device_memory);
	vkBindBufferMemory(device, buffer, device_memory, 0);
	VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {0};
	descriptor_set_layout_binding.binding = 0;
	descriptor_set_layout_binding.descriptorCount = 1;
	descriptor_set_layout_binding.descriptorType =
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {0};
	descriptor_set_layout_info.bindingCount = 1;
	descriptor_set_layout_info.pBindings = &descriptor_set_layout_binding;
	descriptor_set_layout_info.sType =
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	vkCreateDescriptorSetLayout(device, &descriptor_set_layout_info, NULL,
								&descriptor_set_layout);
	VkDescriptorPoolSize descriptor_pool_size = {0};
	descriptor_pool_size.descriptorCount = 1;
	descriptor_pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	VkDescriptorPoolCreateInfo descriptor_pool_info = {0};
	descriptor_pool_info.maxSets = 1;
	descriptor_pool_info.pPoolSizes = &descriptor_pool_size;
	descriptor_pool_info.poolSizeCount = 1;
	descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	vkCreateDescriptorPool(device, &descriptor_pool_info, NULL,
						   &descriptor_pool);
	VkDescriptorSetAllocateInfo descriptor_set_allocation_info = {0};
	descriptor_set_allocation_info.descriptorPool = descriptor_pool;
	descriptor_set_allocation_info.descriptorSetCount = 1;
	descriptor_set_allocation_info.pSetLayouts = &descriptor_set_layout;
	descriptor_set_allocation_info.sType =
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	vkAllocateDescriptorSets(device, &descriptor_set_allocation_info,
							 &descriptor_set);
	VkDescriptorBufferInfo descriptor_buffer_info = {0};
	descriptor_buffer_info.buffer = buffer;
	descriptor_buffer_info.offset = 0;
	descriptor_buffer_info.range = VK_WHOLE_SIZE;
	VkWriteDescriptorSet write_descriptor_set = {0};
	write_descriptor_set.descriptorCount = 1;
	write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write_descriptor_set.dstArrayElement = 0;
	write_descriptor_set.dstBinding = 0;
	write_descriptor_set.dstSet = descriptor_set;
	write_descriptor_set.pBufferInfo = &descriptor_buffer_info;
	write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);
	VkShaderModuleCreateInfo shader_module_info = {0};
	shader_module_info.codeSize = spvlen;
	shader_module_info.pCode = spv;
	shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vkCreateShaderModule(device, &shader_module_info, NULL, &shader_module);
	VkPushConstantRange push_constant_range = {0};
	push_constant_range.offset = 0;
	push_constant_range.size = sizeof(ThreadData);
	push_constant_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
	pipeline_layout_info.pPushConstantRanges = &push_constant_range;
	pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	vkCreatePipelineLayout(device, &pipeline_layout_info, NULL,
						   &pipeline_layout);
	VkPipelineShaderStageCreateInfo pipeline_shader_stage_info = {0};
	pipeline_shader_stage_info.module = shader_module;
	pipeline_shader_stage_info.pName = "main";
	pipeline_shader_stage_info.sType =
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pipeline_shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	VkComputePipelineCreateInfo compute_pipeline_info = {0};
	compute_pipeline_info.layout = pipeline_layout;
	compute_pipeline_info.sType =
		VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	compute_pipeline_info.stage = pipeline_shader_stage_info;
	vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &compute_pipeline_info,
							 NULL, &pipeline);
	VkCommandPoolCreateInfo command_pool_info = {0};
	command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_info.queueFamilyIndex = (int)queue_family_index;
	command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	vkCreateCommandPool(device, &command_pool_info, NULL, &command_pool);
	VkCommandBufferAllocateInfo command_buffer_allocation_info = {0};
	command_buffer_allocation_info.commandBufferCount = 1;
	command_buffer_allocation_info.commandPool = command_pool;
	command_buffer_allocation_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocation_info.sType =
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	vkAllocateCommandBuffers(device, &command_buffer_allocation_info,
							 &command_buffer);
	ThreadData push_constants = {
		TOTAL_SIMULATIONS, seed,	   DAYS_IN_YEAR, PEOPLE,
		NUM_THREADS,	   MULTIPLIER, INCREMENT};
	VkCommandBufferBeginInfo command_buffer_info = {0};
	command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginCommandBuffer(command_buffer, &command_buffer_info);
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
							pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
	vkCmdPushConstants(command_buffer, pipeline_layout,
					   VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ThreadData),
					   &push_constants);
	vkCmdDispatch(command_buffer, group_count_x, 1, 1);
	vkEndCommandBuffer(command_buffer);
	VkSubmitInfo submit_info = {0};
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);
	vkMapMemory(device, device_memory, 0, VK_WHOLE_SIZE, 0,
				(void**)&h_successCount);
	int totalSuccessCount = 0;
	for (int t = 0; t < NUM_THREADS; t++) {
		totalSuccessCount += h_successCount[t];
	}
	vkUnmapMemory(device, device_memory);
	double probability = (double)totalSuccessCount / TOTAL_SIMULATIONS;
	printf("Probability: %.9f\n", probability);
	clock_gettime(CLOCK_MONOTONIC, &end_time);
	double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
						  1e-9 * (end_time.tv_nsec - start_time.tv_nsec);
	printf("Execution Time: %.3f s\n", elapsed_time);
	vkDestroyCommandPool(device, command_pool, NULL);
	vkDestroyPipeline(device, pipeline, NULL);
	vkDestroyShaderModule(device, shader_module, NULL);
	vkDestroyPipelineLayout(device, pipeline_layout, NULL);
	vkDestroyDescriptorPool(device, descriptor_pool, NULL);
	vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);
	vkDestroyBuffer(device, buffer, NULL);
	vkFreeMemory(device, device_memory, NULL);
	vkDestroyDevice(device, NULL);
	vkDestroyInstance(instance, NULL);
	free(spv);
	return 0;
}