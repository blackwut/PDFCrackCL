//
//  CLManager.h
//  OpenCL
//
//  Created by Alberto Ottimo on 22/12/14.
//
//

#ifndef CLManager_h
#define CLManager_h

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#define CHECK_EXIT 1
#define CHECK_NOT_EXIT 0

#define CLPlatform  cl_platform_id
#define CLDevice    cl_device_id
#define CLContext   cl_context
#define CLQueue     cl_command_queue
#define CLMem       cl_mem
#define CLProgram   cl_program
#define CLKernel    cl_kernel
#define CLEvent     cl_event

void CLErrorCheck(cl_int error, const char * function, const char * message, int exit);
CLPlatform CLSelectPlatform(int platformIndex);
CLDevice CLSelectDevice(CLPlatform platform, int deviceIndex);
CLContext CLCreateContext(CLPlatform platform, CLDevice device);
CLQueue CLCreateQueue(CLContext context, CLDevice device);
CLProgram CLCreateProgram(CLContext context, CLDevice device, const char * fileName);
CLKernel CLCreateKernel(CLProgram program, const char * kernelName);
size_t CLGetPreferredWorkGroupSizeMultiple(CLKernel kernel, CLDevice device, const char * name);
size_t CLGetOptimalGlobalWorkItemsSize(size_t numberOfElements, size_t lws);
CLMem CLCreateBufferHostVar(CLContext context, cl_mem_flags flags, size_t size, void * hostVar, const char * name);
CLMem CLCreateBuffer(CLContext context, cl_mem_flags flags, size_t size, const char * name);
void CLSetKernelArg(CLKernel kernel, cl_uint index, size_t size, const void * arg, const char * name);
void CLEnqueueNDRangeKernel(CLQueue queue, CLKernel kernel, const size_t * globalWorkOffset, const size_t * globalWorkSize, const size_t * localWorkSize, cl_uint numberOfEventsWaitList, const CLEvent * eventsWaitList, CLEvent * event, const char * name);
void CLFinish(CLQueue queue);
void CLReleaseMemObject(CLMem var, const char * name);
void CLReleaseEvent(CLEvent event);

void printStatsKernel(CLEvent event, size_t numberOfElements, size_t totalBytes, const char * name);
void printTimeBetweenEvents(CLEvent start, CLEvent finish, const char * name);


#endif /* CLManager_h */