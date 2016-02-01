//
//  CLPrint.c
//  OpenCL
//
//  Created by Alberto Ottimo on 22/12/14.
//
//

#include "CLPrint.h"

#include <stdio.h>

void CLPrintPlatformInfo(cl_platform_id platformID)
{
    cl_int err;
    size_t nBuffer;
    char *charBuffer;
    
    const int nInfos = 5;
    char *infoNames[]= {"Profile", "Platform Name", "Vendor", "Version", "Extensions"};
    cl_platform_info infos[] = {CL_PLATFORM_PROFILE, CL_PLATFORM_NAME, CL_PLATFORM_VENDOR, CL_PLATFORM_VERSION, CL_PLATFORM_EXTENSIONS};
    
    printf("Platforms:\n");
    for (int i = 0; i < nInfos; ++i) {
        err = clGetPlatformInfo(platformID, infos[i], 0, NULL, &nBuffer);
        CLErrorCheck(err, "clGetPlatformInfo", "get nBuffer", CHECK_EXIT);
        charBuffer = calloc(nBuffer, sizeof(charBuffer));
        err = clGetPlatformInfo(platformID, infos[i], nBuffer, charBuffer, NULL);
        CLErrorCheck(err, "clGetPlatformInfo", "get platform info", CHECK_EXIT);
        printf("%s: %s\n", infoNames[i], charBuffer);
    }
    printf("\n");
}

void CLPrintPlatforms()
{
    cl_int err;
    cl_uint nPlatforms;
    cl_platform_id *platforms;
    
    //Chiedo quante piattaforme sono installate
    err = clGetPlatformIDs(0, NULL, &nPlatforms);
    CLErrorCheck(err, "clGetPlatformIDs", "get number of platforms", CHECK_EXIT);
    
    //Alloco l'array platforms
    platforms = (cl_platform_id *)calloc(nPlatforms, sizeof(cl_platform_id));
    
    //Chiedo gli IDs delle piattaforme installate
    err = clGetPlatformIDs(nPlatforms, platforms, NULL);
    CLErrorCheck(err, "clGetPlatformIDs", "get platform IDs", CHECK_EXIT);
    
    for (int i = 0; i < nPlatforms; ++i) {
        printf("PLATFORM #%d\n", i);
        CLPrintPlatformInfo(platforms[i]);
    }
}

void CLPrintDeviceInfo(cl_device_id deviceID)
{
    cl_int err;
    size_t nBuffer;
    char *charBuffer;
    cl_ulong ulongBuffer;
    
    
    int charNInfos = 3;
    char *charInfoNames[] = {"Vendor", "Name", "OpenCL Ver."};
    cl_device_info charInfos[] = {CL_DEVICE_VENDOR, CL_DEVICE_NAME, CL_DEVICE_VERSION};
    
    int ulongNInfos = 5;
    char *ulongInfoNames[] = {"Frequency", "Global Memroy", "Global Memory Cache", "Global Memory CacheLine", "Local Memory", "Max Compute Units"};
    cl_device_info ulongInfos[] = {CL_DEVICE_MAX_CLOCK_FREQUENCY, CL_DEVICE_GLOBAL_MEM_SIZE, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, CL_DEVICE_LOCAL_MEM_SIZE, CL_DEVICE_MAX_COMPUTE_UNITS};
    
    printf("Devices Infos\n");
    for (int i = 0; i < charNInfos; ++i) {
        err = clGetDeviceInfo(deviceID, charInfos[i], 0, NULL, &nBuffer);
        CLErrorCheck(err, "clGetDeviceInfo", "get nBuffer", CHECK_EXIT);
        
        charBuffer = calloc(nBuffer, sizeof(charBuffer));
        
        err = clGetDeviceInfo(deviceID, charInfos[i], nBuffer, charBuffer, NULL);
        CLErrorCheck(err, "clGetDeviceInfo", "get device info", CHECK_EXIT);
        printf("%s: %s\n", charInfoNames[i], charBuffer);
    }
    
    
    for (int i = 0; i < ulongNInfos; ++i) {
        err = clGetDeviceInfo(deviceID, ulongInfos[i], sizeof(ulongBuffer), &ulongBuffer, NULL);
        CLErrorCheck(err, "clGetDeviceInfo", "get device info", CHECK_EXIT);
        printf("%s: %llu\n", ulongInfoNames[i], ulongBuffer);
    }
    printf("\n");
    
}

void CLPrintDevices(cl_platform_id platform)
{
    cl_int err;
    cl_uint nDevices;
    cl_device_id *devices;
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &nDevices);
    CLErrorCheck(err, "clGetDeviceIDs", "get number of devices", CHECK_EXIT);
    
    devices = (cl_device_id *)calloc(nDevices, sizeof(cl_device_id));
    
    err = clGetDeviceIDs(NULL, CL_DEVICE_TYPE_ALL, nDevices, devices, NULL);
    CLErrorCheck(err, "clGetDeviceIDs", "get device IDs", CHECK_EXIT);
    
    for (int i = 0; i < nDevices; ++i) {
        printf("DEVICE #%d\n", i);
        CLPrintDeviceInfo(devices[i]);
    }
}
