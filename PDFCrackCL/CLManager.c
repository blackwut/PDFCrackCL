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

#define BUFFER_SIZE (64 * 1024)

void CLErrorCheck(cl_int error, const char * function, const char * message, int needExit)
{
    if (error != CL_SUCCESS) {
        fprintf(stderr, "%s - %s: %d\n", function, message, error);
        
        if (needExit == CHECK_EXIT) {
            exit(EXIT_FAILURE);
        }
    }
}

CLPlatform CLSelectPlatform(int platformIndex)
{
    if (platformIndex >= 0) {
        
        cl_int err;
        cl_uint nPlatforms;
        cl_platform_id *platforms;
        
        err = clGetPlatformIDs(0, NULL, &nPlatforms);
        CLErrorCheck(err, "clGetPlatformIDs", "get nPlatforms", CHECK_EXIT);
        
        platforms = (CLPlatform *)calloc(nPlatforms, sizeof(CLPlatform));
        
        err = clGetPlatformIDs(nPlatforms, platforms, NULL);
        CLErrorCheck(err, "clGetPlatformIDs", "get platforms IDs", CHECK_EXIT);
        
        if (platformIndex < nPlatforms) {
            return platforms[platformIndex];
        }
    }
    return NULL;
}

CLDevice CLSelectDevice(CLPlatform platform, int deviceIndex)
{
    if (platform && deviceIndex >= 0) {
    
        cl_int err;
        cl_uint nDevices;
        CLDevice *devices;
        
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &nDevices);
        CLErrorCheck(err, "clGetDeviceIDs", "get nDevices", CHECK_EXIT);
        
        devices = (CLDevice *)calloc(nDevices, sizeof(CLDevice));
        
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, nDevices, devices, NULL);
        CLErrorCheck(err, "clGetDeviceIDs", "get device IDs", CHECK_EXIT);
        
        
        if (deviceIndex < nDevices) {
            return devices[deviceIndex];
        }
    }
    return NULL;
}

CLContext CLCreateContext(CLPlatform platform, CLDevice device)
{
    cl_int err;
    cl_context_properties contextProperties[] = {CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0};
    CLContext context = clCreateContext(contextProperties, 1, &device, NULL, NULL, &err);
    CLErrorCheck(err, "clCreateContext", "create context", CHECK_EXIT);
    return context;
}

CLQueue CLCreateQueue(CLContext context, CLDevice device)
{
    cl_int err;
    CLQueue queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
    CLErrorCheck(err, "clCreateCommandQueue", "create queue", CHECK_EXIT);
    return queue;
}

CLProgram CLCreateProgram(CLContext context, CLDevice device, const char * fileName)
{
    cl_int err;
    CLProgram program;
    
    char *buffer = (char *)calloc(BUFFER_SIZE, sizeof(char));
    time_t now = time(NULL);
    snprintf(buffer, BUFFER_SIZE-1, "//%s#include \"%s\"\n", ctime(&now), fileName);
    
    printf("'codice':\n%s\n", buffer);
    const char *ptrBuff = buffer;
    program = clCreateProgramWithSource(context, 1, &ptrBuff, NULL, &err);
    CLErrorCheck(err, "clCreateProgramWithSource", "create program", CHECK_EXIT);
    
    err = clBuildProgram(program, 1, &device, "-cl-unsafe-math-optimizations -cl-fast-relaxed-math -cl-finite-math-only -cl-no-signed-zeros -I.", NULL, NULL);
    CLErrorCheck(err, "clBuildProgram", "build program", CHECK_NOT_EXIT);
    
    err = clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, BUFFER_SIZE, buffer, NULL);
    CLErrorCheck(err, "clGetProgramBuildInfo", "get program build info", CHECK_NOT_EXIT);

    printf("=== BUILD LOG ===\n");
    printf("%s\n", buffer);
    
    return program;
}

CLKernel CLCreateKernel(CLProgram program, const char * kernelName)
{
    cl_int err;
    CLKernel kernel;
    
    kernel = clCreateKernel(program, kernelName, &err);
    CLErrorCheck(err, "clCreateKernel", "create kernel", CHECK_EXIT);
    
    return kernel;
}

size_t CLGetPreferredWorkGroupSizeMultiple(CLKernel kernel, CLDevice device, const char * name)
{
    cl_int error;
    size_t preferredWorkGroupSizeMultiple;
    
    error = clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(preferredWorkGroupSizeMultiple), &preferredWorkGroupSizeMultiple, NULL);
    printf("(%s) Preferred Work Group Size Multiple: %zu\n", name, preferredWorkGroupSizeMultiple);
    return preferredWorkGroupSizeMultiple;
}

size_t CLGetOptimalGlobalWorkItemsSize(size_t numberOfElements, size_t lws)
{
    return (numberOfElements + lws - 1) / lws * lws;
}

CLMem CLCreateBufferHostVar(CLContext context, cl_mem_flags flags, size_t size, void * hostVar, const char * name)
{
    cl_int error;
    CLMem var = clCreateBuffer(context, flags | CL_MEM_COPY_HOST_PTR, size, hostVar, &error);
    CLErrorCheck(error, "clCreateBuffer", name, CHECK_EXIT);
    
    return var;
}

CLMem CLCreateBuffer(CLContext context, cl_mem_flags flags, size_t size, const char * name)
{
    cl_int error;
    CLMem var = clCreateBuffer(context, flags, size, NULL, &error);
    CLErrorCheck(error, "clCreateBuffer", name, CHECK_EXIT);
    
    return var;
}

void CLSetKernelArg(CLKernel kernel, cl_uint index, size_t size, const void * arg, const char * name)
{
    cl_int error;
    error = clSetKernelArg(kernel, index, size, arg);
    CLErrorCheck(error, "clSetKernelArg", "", CHECK_NOT_EXIT);
}

void CLEnqueueNDRangeKernel(CLQueue queue, CLKernel kernel, const size_t * globalWorkOffset, const size_t * globalWorkSize, const size_t * localWorkSize, cl_uint numberOfEventsWaitList, const CLEvent * eventsWaitList, CLEvent * event, const char * name)
{
    cl_int error;
    error = clEnqueueNDRangeKernel(queue, kernel, 1, globalWorkOffset, globalWorkSize, localWorkSize, numberOfEventsWaitList, eventsWaitList, event);
    CLErrorCheck(error, "clEnqueueNDRangeKernel", name, CHECK_EXIT);
}

void CLFinish(CLQueue queue)
{
    cl_int error;
    error = clFinish(queue);
    CLErrorCheck(error, "clFinish", "", CHECK_EXIT);
}

void CLReleaseMemObject(CLMem var, const char * name)
{
    cl_int error;
    error = clReleaseMemObject(var);
    CLErrorCheck(error, "clReleaseMemObject", name, CHECK_NOT_EXIT);
}

void CLReleaseEvent(CLEvent event)
{
    clReleaseEvent(event);
}

void printStatsKernel(CLEvent event, size_t numberOfElements, size_t totalBytes, const char * name)
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

void printTimeBetweenEvents(CLEvent start, CLEvent finish, const char * name)
{
    cl_ulong timeStart, timeEnd;
    double totalTimeNS, totalTimeMS;
    
    clGetEventProfilingInfo(start, CL_PROFILING_COMMAND_START, sizeof(timeStart), &timeStart, NULL);
    clGetEventProfilingInfo(finish, CL_PROFILING_COMMAND_END, sizeof(timeEnd), &timeEnd, NULL);
    
    totalTimeNS = timeEnd-timeStart;
    totalTimeMS = totalTimeNS / 1000000.0;
    
    printf("%s: %0.5f ms\n", name, totalTimeMS);
}