// gcc -o birthday_opencl birthday_opencl.c -lOpenCL -Ofast && ./birthday_opencl
// g++ -o birthday_opencl birthday_opencl.c -lOpenCL -Ofast && ./birthday_opencl
// zig cc -o birthday_opencl birthday_opencl.c -lOpenCL -Ofast &&
// ./birthday_opencl
// zig c++ -o birthday_opencl birthday_opencl.c -lOpenCL -Ofast &&
// ./birthday_opencl
// clang -o birthday_opencl birthday_opencl.c -lOpenCL -O3 -ffast-math &&
// ./birthday_opencl
// clang++ -o birthday_opencl birthday_opencl.c -lOpenCL -O3 -ffast-math &&
// ./birthday_opencl
#define CL_TARGET_OPENCL_VERSION 300
#define BLOCK_SIZE 32
#define DAYS_IN_YEAR 365
#define NUM_THREADS 768	 // for GTX 1660 SUPER
// #define NUM_THREADS 2176 for RTX 4060 TI and Intel Arc
#define PEOPLE 24
#define TOTAL_SIMULATIONS 1000000
#define MULTIPLIER 1664525
#define INCREMENT 1013904223
#include <CL/cl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
int main() {
	struct timespec start_time, end_time;
	clock_gettime(CLOCK_MONOTONIC, &start_time);
	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC, &time);
	uint64_t seed = time.tv_sec * 1e9 + time.tv_nsec;
	size_t block_size = (size_t)BLOCK_SIZE;
	uint32_t days_in_year = DAYS_IN_YEAR;
	uint32_t num_threads = NUM_THREADS;
	uint32_t people = PEOPLE;
	uint32_t totalSimulations = TOTAL_SIMULATIONS;
	uint32_t multiplier = MULTIPLIER;
	uint32_t increment = INCREMENT;
	size_t threads_size = (size_t)num_threads;
	cl_mem deviceSuccessCount;
	uint32_t* hostSuccessCount =
		(uint32_t*)malloc(num_threads * sizeof(uint32_t));
	cl_uint num_platforms;
	cl_platform_id platform;
	cl_uint num_devices;
	cl_device_id device;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
	cl_kernel kernel;
	clGetPlatformIDs(0, NULL, &num_platforms);
	cl_platform_id* platforms =
		(cl_platform_id*)malloc(num_platforms * sizeof(cl_platform_id));
	clGetPlatformIDs(num_platforms, platforms, NULL);
	platform = platforms[0];
	char vendor[block_size];
	for (cl_uint i = 0; i < num_platforms; ++i) {
		clGetPlatformInfo(platforms[i], CL_PLATFORM_VENDOR, sizeof(vendor),
						  vendor, NULL);
		if (strstr(vendor, "NVIDIA")) {
			platform = platforms[i];
			break;
		}
	}
	free(platforms);
	clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);
	cl_device_id* devices =
		(cl_device_id*)malloc(num_devices * sizeof(cl_device_id));
	clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, num_devices, devices, NULL);
	device = devices[0];
	free(devices);
	context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
	queue = clCreateCommandQueueWithProperties(context, device, NULL, NULL);
	const char* simulate =
		"__kernel void simulate(uint simulations,"
		"					   __global uint* deviceSuccessCount,"
		"					   uint seed,"
		"					   uint days_in_year,"
		"					   uint people,"
		"					   uint num_threads,"
		"					   uint multiplier,"
		"					   uint increment) {"
		"	size_t threadId = get_global_id(0);"
		"	uint simulationsPerThread = simulations / num_threads;"
		"	uint localSuccessCount = 0;"
		"	uint state = seed ^ threadId;"
		"	for (uint sim = 0; sim < simulationsPerThread; sim++) {"
		"		uchar birthdays[365] = {0};"
		"		for (int i = 0; i < people; i++) {"
		"			state = state * multiplier + increment;"
		"			uint birthday = state % days_in_year;"
		"			birthdays[birthday]++;"
		"		}"
		"		uchar exactlyTwoCount = 0;"
		"		for (int i = 0; i < days_in_year; i++) {"
		"			if (birthdays[i] == 2) {"
		"				exactlyTwoCount++;"
		"			}"
		"		}"
		"		if (exactlyTwoCount == 1) {"
		"			localSuccessCount++;"
		"		}"
		"	}"
		"	deviceSuccessCount[(int)threadId] = localSuccessCount;"
		"}";
	program = clCreateProgramWithSource(context, 1, &simulate, NULL, NULL);
	clBuildProgram(program, 1, &device, NULL, NULL, NULL);
	kernel = clCreateKernel(program, "simulate", NULL);
	deviceSuccessCount = clCreateBuffer(
		context, CL_MEM_WRITE_ONLY, num_threads * sizeof(uint32_t), NULL, NULL);
	clSetKernelArg(kernel, 0, sizeof(uint32_t), &totalSimulations);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), &deviceSuccessCount);
	clSetKernelArg(kernel, 2, sizeof(uint32_t), &seed);
	clSetKernelArg(kernel, 3, sizeof(uint32_t), &days_in_year);
	clSetKernelArg(kernel, 4, sizeof(uint32_t), &people);
	clSetKernelArg(kernel, 5, sizeof(uint32_t), &num_threads);
	clSetKernelArg(kernel, 6, sizeof(uint32_t), &multiplier);
	clSetKernelArg(kernel, 7, sizeof(uint32_t), &increment);
	clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &threads_size, &block_size,
						   0, NULL, NULL);
	clEnqueueReadBuffer(queue, deviceSuccessCount, CL_TRUE, 0,
						num_threads * sizeof(uint32_t), hostSuccessCount, 0,
						NULL, NULL);
	uint32_t totalSuccessCount = 0;
	for (uint32_t t = 0; t < num_threads; t++) {
		totalSuccessCount += hostSuccessCount[t];
	}
	double probability = (double)totalSuccessCount / totalSimulations;
	printf("Probability: %.9f\n", probability);
	clock_gettime(CLOCK_MONOTONIC, &end_time);
	double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
						  1e-9 * (end_time.tv_nsec - start_time.tv_nsec);
	printf("Execution Time: %.3f s\n", elapsed_time);
	free(hostSuccessCount);
	clReleaseMemObject(deviceSuccessCount);
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
	return 0;
}