//
//  CLManager.c
//  OpenCL
//
//  Created by Alberto Ottimo on 22/12/14.
//
//

#include "CLManager.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE (16 * 1024)

void CLErrorCheck(cl_int error, const char * function, const char * message, int needExit)
{
    if (error != CL_SUCCESS) {
        fprintf(stderr, "%s - %s: %d\n", function, message, error);
        
        if (needExit == CHECK_EXIT) {
            exit(EXIT_FAILURE);
        }
    }
}

cl_platform_id CLSelectPlatform(int platformIndex)
{
    if (platformIndex >= 0) {
        
        cl_int err;
        cl_uint nPlatforms;
        cl_platform_id *platforms;
        
        err = clGetPlatformIDs(0, NULL, &nPlatforms);
        CLErrorCheck(err, "clGetPlatformIDs", "get nPlatforms", CHECK_EXIT);
        
        platforms = (cl_platform_id *)calloc(nPlatforms, sizeof(cl_platform_id));
        
        err = clGetPlatformIDs(nPlatforms, platforms, NULL);
        CLErrorCheck(err, "clGetPlatformIDs", "get platforms IDs", CHECK_EXIT);
        
        if (platformIndex < nPlatforms) {
            return platforms[platformIndex];
        }
    }
    return NULL;
}

cl_device_id CLSelectDevice(cl_platform_id platform, int deviceIndex)
{
    if (platform && deviceIndex >= 0) {
    
        cl_int err;
        cl_uint nDevices;
        cl_device_id *devices;
        
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &nDevices);
        CLErrorCheck(err, "clGetDeviceIDs", "get nDevices", CHECK_EXIT);
        
        devices = (cl_device_id *)calloc(nDevices, sizeof(cl_device_id));
        
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, nDevices, devices, NULL);
        CLErrorCheck(err, "clGetDeviceIDs", "get device IDs", CHECK_EXIT);
        
        
        if (deviceIndex < nDevices) {
            return devices[deviceIndex];
        }
    }
    return NULL;
}

cl_context CLCreateContext(cl_platform_id platform, cl_device_id device)
{
    cl_int err;
    cl_context_properties contextProperties[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0};
    cl_context context = clCreateContext(contextProperties, 1, &device, NULL, NULL, &err);
    CLErrorCheck(err, "clCreateContext", "create context", CHECK_EXIT);
    return context;
}

cl_command_queue CLCreateQueue(cl_context context, cl_device_id device)
{
    cl_int err;
    cl_command_queue queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
    CLErrorCheck(err, "clCreateCommandQueue", "create queue", CHECK_EXIT);
    return queue;
}

cl_program CLCreateProgram(cl_context context, cl_device_id device, const char * fileName)
{
    cl_int err;
    cl_program program;
    
    char *buffer = (char *)calloc(BUFFER_SIZE, sizeof(char));
    time_t now = time(NULL);
    snprintf(buffer, BUFFER_SIZE-1, "//%s#include \"%s\"\n", ctime(&now), fileName);
    
    printf("'codice':\n%s\n", buffer);
    const char *ptrBuff = buffer;
    program = clCreateProgramWithSource(context, 1, &ptrBuff, NULL, &err);
    CLErrorCheck(err, "clCreateProgramWithSource", "create program", CHECK_EXIT);
    
    err = clBuildProgram(program, 1, &device, "-I.", NULL, NULL);
    CLErrorCheck(err, "clBuildProgram", "build program", CHECK_NOT_EXIT);
    
    err = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, BUFFER_SIZE, buffer, NULL);
    CLErrorCheck(err, "clGetProgramBuildInfo", "get program build info", CHECK_NOT_EXIT);

    printf("=== BUILD LOG ===\n");
    printf("%s\n", buffer);
    
    return program;
}

cl_kernel CLCreateKernel(cl_program program, const char * kernelName)
{
    cl_int err;
    cl_kernel kernel;
    
    kernel = clCreateKernel(program, kernelName, &err);
    CLErrorCheck(err, "clCreateKernel", "create kernel", CHECK_EXIT);
    
    return kernel;
}

size_t CLGetPreferredWorkGroupSizeMultiple(cl_kernel kernel, cl_device_id device, const char * name)
{
    cl_int error;
    size_t preferredWorkGroupSizeMultiple;
    
    error = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(preferredWorkGroupSizeMultiple), &preferredWorkGroupSizeMultiple, NULL);
    printf("(%s) Preferred Work Group Size Multiple: %zu\n", name, preferredWorkGroupSizeMultiple);
    return preferredWorkGroupSizeMultiple;
}

cl_mem CLCreateBufferHostVar(cl_context context, cl_mem_flags flags, size_t size, void * hostVar, const char * name)
{
    cl_int error;
    cl_mem var = clCreateBuffer(context, flags | CL_MEM_COPY_HOST_PTR, size, hostVar, &error);
    CLErrorCheck(error, "clCreateBuffer", name, CHECK_EXIT);
    
    return var;
}

cl_mem CLCreateBuffer(cl_context context, cl_mem_flags flags, size_t size, const char * name)
{
    cl_int error;
    cl_mem var = clCreateBuffer(context, flags, size, NULL, &error);
    CLErrorCheck(error, "clCreateBuffer", name, CHECK_EXIT);
    
    return var;
}

void CLSetKernelArg(cl_kernel kernel, cl_uint index, size_t size, const void * arg, const char * name)
{
    cl_int error;
    error = clSetKernelArg(kernel, index, size, arg);
    CLErrorCheck(error, "clSetKernelArg", "", CHECK_NOT_EXIT);
}

void CLEnqueueNDRangeKernel(cl_command_queue queue, cl_kernel kernel, const size_t * globalWorkOffset, const size_t * globalWorkSize, const size_t * localWorkSize, cl_uint numberOfEventsWaitList, const cl_event * eventsWaitList, cl_event * event, const char * name)
{
    cl_int error;
    error = clEnqueueNDRangeKernel(queue, kernel, 1, globalWorkOffset, globalWorkSize, localWorkSize, numberOfEventsWaitList, eventsWaitList, event);
    CLErrorCheck(error, "clEnqueueNDRangeKernel", name, CHECK_EXIT);
}

void CLFinish(cl_command_queue queue)
{
    cl_int error;
    error = clFinish(queue);
    CLErrorCheck(error, "clFinish", "", CHECK_EXIT);
}

void CLReleaseMemObject(cl_mem var, const char * name)
{
    cl_int error;
    error = clReleaseMemObject(var);
    CLErrorCheck(error, "clReleaseMemObject", name, CHECK_NOT_EXIT);
}

void printStatsKernel(cl_event event, size_t numberOfElements, size_t totalBytes, const char * name)
{
    cl_ulong timeStart, timeEnd;
    double totalTimeNS, totalTimeMS, totalTimeS, bandwidth;
    size_t elementsPerSecond;
    
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(timeStart), &timeStart, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(timeEnd), &timeEnd, NULL);
    
    totalTimeNS = timeEnd-timeStart;
    totalTimeMS = totalTimeNS / 1000000.0;
    totalTimeS = totalTimeNS / 1000000000.0;
    
    elementsPerSecond = numberOfElements / totalTimeS;
    bandwidth = totalBytes / totalTimeNS;
    
    printf("%s:\n Time: %0.3f ms\n Elements/Second: %zu\n Bandwidth: %0.3f GB/s\n\n", name, totalTimeMS, elementsPerSecond, bandwidth);
}