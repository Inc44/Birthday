// gcc -o birthday_vulkan birthday_vulkan.c -lvulkan -Ofast && ./birthday_vulkan
// g++ -o birthday_vulkan birthday_vulkan.c -lvulkan -Ofast && ./birthday_vulkan
// zig cc -o birthday_vulkan birthday_vulkan.c -lvulkan -Ofast &&
// ./birthday_vulkan
// zig c++ -o birthday_vulkan birthday_vulkan.c -lvulkan -Ofast &&
// ./birthday_vulkan
// clang -o birthday_vulkan birthday_vulkan.c -lvulkan -O3 -ffast-math &&
// ./birthday_vulkan
// clang++ -o birthday_vulkan birthday_vulkan.c -lvulkan -O3 -ffast-math &&
// ./birthday_vulkan
#define BLOCK_SIZE 32
#define DAYS_IN_YEAR 365
#define NUM_THREADS 768	 // for GTX 1660 SUPER
// #define NUM_THREADS 2176 for RTX 4060 TI and Intel Arc
#define PEOPLE 24
#define TOTAL_SIMULATIONS 1000000
#define MULTIPLIER 1664525
#define INCREMENT 1013904223
#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
typedef struct {
	uint32_t simulations;
	uint32_t seed;
	uint32_t days_in_year;
	uint32_t people;
	uint32_t num_threads;
	uint32_t multiplier;
	uint32_t increment;
} ThreadData;
const char* SIMULATE =
	"#version 450\n"
	"layout(push_constant) uniform ThreadData {"
	"	uint simulations;"
	"	uint seed;"
	"	uint days_in_year;"
	"	uchar people;"
	"	uint num_threads;"
	"	uint multiplier;"
	"	uint increment;"
	"}"
	"pushConstants;"
	"layout(local_size_x = 32) in;"
	"layout(std430, binding = 0) buffer OutBuf {"
	"	uint deviceSuccessCount[];"
	"};"
	"void main() {"
	"	uint threadId = gl_GlobalInvocationID.x;"
	"	uint simulationsPerThread ="
	"		pushConstants.simulations / pushConstants.num_threads;"
	"	uint localSuccessCount = 0;"
	"	uint state = pushConstants.seed ^ threadId;"
	"	uchar birthdays[365];"
	"	for (uint sim = 0; sim < simulationsPerThread; ++sim) {"
	"		for (int i = 0; i < pushConstants.days_in_year; ++i)"
	"			birthdays[i] = 0;"
	"		for (int i = 0; i < pushConstants.people; ++i) {"
	"			state = state * pushConstants.multiplier + "
	"pushConstants.increment;"
	"			uint birthday = state % pushConstants.days_in_year;"
	"			birthdays[birthday]++;"
	"		}"
	"		uchar exactlyTwoCount = 0;"
	"		for (int i = 0; i < pushConstants.days_in_year; ++i)"
	"			if (birthdays[i] == 2)"
	"				exactlyTwoCount++;"
	"		if (exactlyTwoCount == 1)"
	"			localSuccessCount++;"
	"	}"
	"	deviceSuccessCount[threadId] = localSuccessCount;"
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
	if (!file)
		exit(1);
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);
	void* spv = malloc(size);
	fread(spv, 1, size, file);
	fclose(file);
	*n = size;
	return spv;
}
uint8_t find_memory_type(uint8_t memoryTypeBits,
						 VkMemoryPropertyFlags flags,
						 VkPhysicalDeviceMemoryProperties* memoryProperties) {
	for (uint8_t i = 0; i < memoryProperties->memoryTypeCount; i++)
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
	uint64_t seed = time.tv_sec * 1e9 + time.tv_nsec;
	uint32_t* hostSuccessCount;
	VkInstance instance;
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
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
	system(
		"glslangValidator -o birthday.comp.spv birthday.comp -V > /dev/null "
		"2>&1");
	size_t spvlen;
	void* spv = read_spv(&spvlen);
	VkApplicationInfo application_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "birthday_vulkan",
		.apiVersion = VK_API_VERSION_1_0};
	VkInstanceCreateInfo instance_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &application_info};
	vkCreateInstance(&instance_info, NULL, &instance);
	uint32_t physical_device_count;
	vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL);
	VkPhysicalDevice* physical_devices = (VkPhysicalDevice*)malloc(
		sizeof(VkPhysicalDevice) * physical_device_count);
	vkEnumeratePhysicalDevices(instance, &physical_device_count,
							   physical_devices);
	int queue_family_index = -1;
	for (uint8_t pass = 0; pass < 2 && physical_device == VK_NULL_HANDLE;
		 pass++) {
		for (uint32_t p = 0; p < physical_device_count; p++) {
			vkGetPhysicalDeviceProperties(physical_devices[p],
										  &physical_device_properties);
			if (pass == 0 && physical_device_properties.vendorID != 0x10DE)
				continue;
			uint32_t queue_family_property_count;
			vkGetPhysicalDeviceQueueFamilyProperties(
				physical_devices[p], &queue_family_property_count, NULL);
			VkQueueFamilyProperties* queue_family_properties =
				(VkQueueFamilyProperties*)malloc(
					sizeof(VkQueueFamilyProperties) *
					queue_family_property_count);
			vkGetPhysicalDeviceQueueFamilyProperties(
				physical_devices[p], &queue_family_property_count,
				queue_family_properties);
			for (uint32_t i = 0; i < queue_family_property_count; i++) {
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
	VkDeviceQueueCreateInfo device_queue_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = (uint32_t)queue_family_index,
		.queueCount = 1,
		.pQueuePriorities = &queue_priority};
	VkDeviceCreateInfo device_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &device_queue_info};
	vkCreateDevice(physical_device, &device_info, NULL, &device);
	vkGetDeviceQueue(device, queue_family_index, 0, &queue);
	uint16_t group_count_x = (NUM_THREADS + BLOCK_SIZE - 1) / BLOCK_SIZE;
	VkDeviceSize device_size =
		(VkDeviceSize)group_count_x * BLOCK_SIZE * sizeof(uint32_t);
	VkBufferCreateInfo buffer_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = device_size,
		.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE};
	vkCreateBuffer(device, &buffer_info, NULL, &buffer);
	vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);
	uint8_t memory_type =
		find_memory_type(memory_requirements.memoryTypeBits,
						 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
							 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
						 &physical_device_memory_properties);
	VkMemoryAllocateInfo memory_allocation_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memory_requirements.size,
		.memoryTypeIndex = memory_type};
	vkAllocateMemory(device, &memory_allocation_info, NULL, &device_memory);
	vkBindBufferMemory(device, buffer, device_memory, 0);
	VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT};
	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &descriptor_set_layout_binding};
	vkCreateDescriptorSetLayout(device, &descriptor_set_layout_info, NULL,
								&descriptor_set_layout);
	VkDescriptorPoolSize descriptor_pool_size = {
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1};
	VkDescriptorPoolCreateInfo descriptor_pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 1,
		.poolSizeCount = 1,
		.pPoolSizes = &descriptor_pool_size};
	vkCreateDescriptorPool(device, &descriptor_pool_info, NULL,
						   &descriptor_pool);
	VkDescriptorSetAllocateInfo descriptor_set_allocation_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &descriptor_set_layout};
	vkAllocateDescriptorSets(device, &descriptor_set_allocation_info,
							 &descriptor_set);
	VkDescriptorBufferInfo descriptor_buffer_info = {
		.buffer = buffer, .offset = 0, .range = VK_WHOLE_SIZE};
	VkWriteDescriptorSet write_descriptor_set = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptor_set,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pBufferInfo = &descriptor_buffer_info};
	vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, NULL);
	VkShaderModuleCreateInfo shader_module_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = spvlen,
		.pCode = (const uint32_t*)spv};
	vkCreateShaderModule(device, &shader_module_info, NULL, &shader_module);
	VkPushConstantRange push_constant_range = {
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.offset = 0,
		.size = sizeof(ThreadData)};
	VkPipelineLayoutCreateInfo pipeline_layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &descriptor_set_layout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &push_constant_range};
	vkCreatePipelineLayout(device, &pipeline_layout_info, NULL,
						   &pipeline_layout);
	VkPipelineShaderStageCreateInfo pipeline_shader_stage_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.module = shader_module,
		.pName = "main"};
	VkComputePipelineCreateInfo compute_pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = pipeline_shader_stage_info,
		.layout = pipeline_layout};
	vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &compute_pipeline_info,
							 NULL, &pipeline);
	VkCommandPoolCreateInfo command_pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = (uint32_t)queue_family_index};
	vkCreateCommandPool(device, &command_pool_info, NULL, &command_pool);
	VkCommandBufferAllocateInfo command_buffer_allocation_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1};
	vkAllocateCommandBuffers(device, &command_buffer_allocation_info,
							 &command_buffer);
	ThreadData push_constants = {
		(uint32_t)TOTAL_SIMULATIONS, (uint32_t)seed,
		(uint16_t)DAYS_IN_YEAR,		 (uint8_t)PEOPLE,
		(uint16_t)NUM_THREADS,		 (uint32_t)MULTIPLIER,
		(uint32_t)INCREMENT};
	VkCommandBufferBeginInfo command_buffer_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	vkBeginCommandBuffer(command_buffer, &command_buffer_info);
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
							pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
	vkCmdPushConstants(command_buffer, pipeline_layout,
					   VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ThreadData),
					   &push_constants);
	vkCmdDispatch(command_buffer, group_count_x, 1, 1);
	vkEndCommandBuffer(command_buffer);
	VkSubmitInfo submit_info = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
								.commandBufferCount = 1,
								.pCommandBuffers = &command_buffer};
	vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);
	vkMapMemory(device, device_memory, 0, VK_WHOLE_SIZE, 0,
				(void**)&hostSuccessCount);
	uint32_t totalSuccessCount = 0;
	for (uint16_t t = 0; t < NUM_THREADS; t++) {
		totalSuccessCount += hostSuccessCount[t];
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