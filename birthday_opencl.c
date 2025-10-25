// gcc -o birthday_opencl birthday_opencl.c -lOpenCL -Ofast && ./birthday_opencl
// g++ -o birthday_opencl birthday_opencl.c -lOpenCL -Ofast && ./birthday_opencl
// zig cc -o birthday_opencl birthday_opencl.c -lOpenCL -Ofast &&
// ./birthday_opencl
// zig c++ -o birthday_opencl birthday_opencl.c -lOpenCL -Ofast &&
// ./birthday_opencl
#define CL_TARGET_OPENCL_VERSION 300
#define BLOCK_SIZE 32
#define DAYS_IN_YEAR 365
#define NUM_THREADS 768	 // for GTX 1660 SUPER
// #define NUM_THREADS 2176 for RTX 4060 TI and Intel Arc
#define PEOPLE 24
#define TOTAL_SIMULATIONS 1000000
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main() {
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);
	unsigned int seed = time(NULL);
	size_t block_size = (size_t)BLOCK_SIZE;
	int days_in_year = DAYS_IN_YEAR;
	int num_threads = NUM_THREADS;
	int people = PEOPLE;
	int totalSimulations = TOTAL_SIMULATIONS;
	size_t threads_size = (size_t)num_threads;
	cl_mem d_successCount;
	int* h_successCount;
	h_successCount = (int*)malloc(NUM_THREADS * sizeof(int));
	cl_platform_id platform;
	cl_device_id device;
	cl_context context;
	cl_command_queue queue;
	cl_program program;
	cl_kernel kernel;
	clGetPlatformIDs(1, &platform, NULL);
	clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, 1, &device, NULL);
	context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
	queue = clCreateCommandQueueWithProperties(context, device, NULL, NULL);
	const char* simulate =
		"__kernel void simulate(int simulations,"
		"					   __global int* d_successCount,"
		"					   unsigned int seed,"
		"					   int days_in_year,"
		"					   int people,"
		"					   int num_threads) {"
		"	size_t tid = get_global_id(0);"
		"	int simulationsPerThread = simulations / num_threads;"
		"	int successCount = 0;"
		"	unsigned int state = seed ^ (unsigned int)tid;"
		"	for (int sim = 0; sim < simulationsPerThread; sim++) {"
		"		int birthdays[365] = {0};"
		"		for (int i = 0; i < people; i++) {"
		"			state = state * 1664525U + 1013904223U;"
		"			int birthday = (int)(state % (unsigned int)days_in_year);"
		"			birthdays[birthday]++;"
		"		}"
		"		int exactlyTwoCount = 0;"
		"		for (int i = 0; i < days_in_year; i++) {"
		"			if (birthdays[i] == 2) {"
		"				exactlyTwoCount++;"
		"			}"
		"		}"
		"		if (exactlyTwoCount == 1) {"
		"			successCount++;"
		"		}"
		"	}"
		"	d_successCount[(int)tid] = successCount;"
		"}";
	program = clCreateProgramWithSource(context, 1, &simulate, NULL, NULL);
	clBuildProgram(program, 1, &device, NULL, NULL, NULL);
	kernel = clCreateKernel(program, "simulate", NULL);
	d_successCount = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
									num_threads * sizeof(int), NULL, NULL);
	clSetKernelArg(kernel, 0, sizeof(int), &totalSimulations);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_successCount);
	clSetKernelArg(kernel, 2, sizeof(unsigned int), &seed);
	clSetKernelArg(kernel, 3, sizeof(int), &days_in_year);
	clSetKernelArg(kernel, 4, sizeof(int), &people);
	clSetKernelArg(kernel, 5, sizeof(int), &num_threads);
	clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &threads_size, &block_size,
						   0, NULL, NULL);
	clEnqueueReadBuffer(queue, d_successCount, CL_TRUE, 0,
						num_threads * sizeof(int), h_successCount, 0, NULL,
						NULL);
	int totalSuccessCount = 0;
	for (int t = 0; t < num_threads; t++) {
		totalSuccessCount += h_successCount[t];
	}
	double probability = (double)totalSuccessCount / totalSimulations;
	printf("Probability: %.9f\n", probability);
	clock_gettime(CLOCK_MONOTONIC, &end);
	double elapsed =
		(end.tv_sec - start.tv_sec) + 1e-9 * (end.tv_nsec - start.tv_nsec);
	printf("Execution Time: %.3f s\n", elapsed);
	free(h_successCount);
	clReleaseMemObject(d_successCount);
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(queue);
	clReleaseContext(context);
	return 0;
}