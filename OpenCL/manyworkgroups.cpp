#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <ctime>
 
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.hpp>
#endif
 
#define MAX_SOURCE_SIZE (0x100000)

int GetTimeMs()
{
 /* Linux */
 struct timeval tv;

 gettimeofday(&tv, NULL);

 unsigned int ret = tv.tv_usec;
 /* Convert from micro seconds (10^-6) to milliseconds (10^-3) */
 ret /= 1000;

 /* Adds the seconds (10^0) after converting them to milliseconds (10^-3) */
 ret += (tv.tv_sec * 1000);

 return ret;
}
 
int main(void) {
    // Create the variables for the time measure
    int starttime, stoptime;
    // Create the two input vectors and instance the output vector
    int i, N;
    const int LIST_SIZE = 1;
    float *sum = (float*)malloc(sizeof(int)*LIST_SIZE);
    
    for(i = 0; i < LIST_SIZE; i++) {
        sum[i] = 0.4;
    }

    //Ask to the user, how many interactions he wants to see
    printf("How many interactions:\n");
    scanf("%d",&N);
 
    // Load the kernel source code into the array source_str
    FILE *fp;
    char *source_str;
    size_t source_size;
 
    fp = fopen("simplefloat_kernels.cl", "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread( source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose( fp );

    //Get initial time
    starttime = GetTimeMs();
 
    cl_uint ret_num_devices = 0;
    cl_uint ret_num_platforms = 0;

    // Use this to check the output of each API call
    cl_int ret;

    // Retrieve the number of platforms
    ret = clGetPlatformIDs(0, NULL, &ret_num_platforms);

    // Allocate enough space for each platform
    cl_platform_id *platforms = NULL;
    platforms = (cl_platform_id*)malloc(ret_num_platforms*sizeof(cl_platform_id));

    // Fill in the platforms
    ret = clGetPlatformIDs(ret_num_platforms, platforms, NULL);

    // Retrieve the number of devices
    ret = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, 0, NULL, &ret_num_devices);

    // Allocate enough space for each device
    cl_device_id *devices;
    devices = (cl_device_id*)malloc(ret_num_devices*sizeof(cl_device_id));

    // Fill in the devices
    ret = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_ALL, ret_num_devices, devices,  NULL);
 
    // Create an OpenCL context
    cl_context context = clCreateContext( NULL, ret_num_devices, devices, NULL, NULL, &ret);
 
    // Create a command queue
    cl_command_queue command_queue = clCreateCommandQueue(context, devices[0], 0, &ret);
 
    // Create memory buffers on the device for each vector
    cl_mem sum_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
            LIST_SIZE * sizeof(int), NULL, &ret);
 
    // Copy the lists A and B to their respective memory buffers
    ret = clEnqueueWriteBuffer(command_queue, sum_mem_obj, CL_TRUE, 0, 
            LIST_SIZE * sizeof(int), sum, 0, NULL, NULL);
 
    // Create a program from the kernel source
    cl_program program = clCreateProgramWithSource(context, 1, 
            (const char **)&source_str, (const size_t *)&source_size, &ret);
 
    // Build the program
    ret = clBuildProgram(program, ret_num_devices, devices, NULL, NULL, NULL);
 
    // Create the OpenCL kernel
    cl_kernel kernel = clCreateKernel(program, "simplefloatadd", &ret);
 
    // Set the arguments of the kernel
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&sum_mem_obj);
    ret = clSetKernelArg(kernel, 1, sizeof(int), &N);
 
    // Execute the OpenCL kernel on the list
    size_t global_item_size = LIST_SIZE; // Process the entire lists
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, 
            &global_item_size, NULL, 0, NULL, NULL);
 
    // Read the memory buffer C on the device to the local variable C
    ret = clEnqueueReadBuffer(command_queue, sum_mem_obj, CL_TRUE, 0, 
            LIST_SIZE * sizeof(int), sum, 0, NULL, NULL);
 
    // Display the result to the screen
    for(i = 0; i < LIST_SIZE; i++)
        printf("%d += %.1f = %.1f\n", N, 0.3, sum[0]);

    //Get stop time
    stoptime = GetTimeMs();

    printf("Duration= %d ms\n", stoptime - starttime);
 
    // Clean up
    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(sum_mem_obj);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
    free(sum);
    return 0;
}