//
//  CLManager.h
//  OpenCL
//
//  Created by Alberto Ottimo on 22/12/14.
//
//

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#define CHECK_EXIT 1
#define CHECK_NOT_EXIT 0

void CLErrorCheck(cl_int error, const char * function, const char * message, int exit);
cl_platform_id CLSelectPlatform(int platformIndex);
cl_device_id CLSelectDevice(cl_platform_id platform, int deviceIndex);
cl_context CLCreateContext(cl_platform_id platform, cl_device_id device);
cl_command_queue CLCreateQueue(cl_context context, cl_device_id device);
cl_program CLCreateProgram(cl_context context, cl_device_id device, const char * fileName);
cl_kernel CLCreateKernel(cl_program program, const char * kernelName);
size_t CLGetPreferredWorkGroupSizeMultiple(cl_kernel kernel, cl_device_id device, const char * name);
cl_mem CLCreateBufferHostVar(cl_context context, cl_mem_flags flags, size_t size, void * hostVar, const char * name);
cl_mem CLCreateBuffer(cl_context context, cl_mem_flags flags, size_t size, const char * name);
void CLSetKernelArg(cl_kernel kernel, cl_uint index, size_t size, const void * arg, const char * name);
void CLEnqueueNDRangeKernel(cl_command_queue queue, cl_kernel kernel, const size_t * globalWorkOffset, const size_t * globalWorkSize, const size_t * localWorkSize, cl_uint numberOfEventsWaitList, const cl_event * eventsWaitList, cl_event * event, const char * name);
void CLFinish(cl_command_queue queue);
void CLReleaseMemObject(cl_mem var, const char * name);
void CLReleaseEvent(cl_event event);

void printStatsKernel(cl_event event, size_t numberOfElements, size_t totalBytes, const char * name);
void printTimeBetweenEvents(cl_event start, cl_event finish, const char * name);
