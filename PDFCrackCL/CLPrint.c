//
//  CLPrint.c
//  OpenCL
//
//  Created by Alberto Ottimo on 22/12/14.
//
//

#include "CLPrint.h"

#include <stdio.h>

void CLPrintPlatformInfo(CLPlatform platformID)
{
    cl_int err;
    size_t nBuffer;
    char *charBuffer;
    
    const int nInfo = 5;
    char *infoNames[]= {"Profile", "Platform Name", "Vendor", "Version", "Extensions"};
    cl_platform_info info[] = {CL_PLATFORM_PROFILE, CL_PLATFORM_NAME, CL_PLATFORM_VENDOR, CL_PLATFORM_VERSION, CL_PLATFORM_EXTENSIONS};
    
    printf("Platforms:\n");
    for (int i = 0; i < nInfo; ++i) {
        err = clGetPlatformInfo(platformID, info[i], 0, NULL, &nBuffer);
        CLErrorCheck(err, "clGetPlatformInfo", "get nBuffer", CHECK_EXIT);
        charBuffer = calloc(nBuffer, sizeof(charBuffer));
        err = clGetPlatformInfo(platformID, info[i], nBuffer, charBuffer, NULL);
        CLErrorCheck(err, "clGetPlatformInfo", "get platform info", CHECK_EXIT);
        printf("%s: %s\n", infoNames[i], charBuffer);
    }
    printf("\n");
}

void CLPrintPlatforms()
{
    cl_int err;
    cl_uint nPlatforms;
    CLPlatform *platforms;
    
    //Chiedo quante piattaforme sono installate
    err = clGetPlatformIDs(0, NULL, &nPlatforms);
    CLErrorCheck(err, "clGetPlatformIDs", "get number of platforms", CHECK_EXIT);
    
    //Alloco l'array platforms
    platforms = (CLPlatform *)calloc(nPlatforms, sizeof(CLPlatform));
    
    //Chiedo gli IDs delle piattaforme installate
    err = clGetPlatformIDs(nPlatforms, platforms, NULL);
    CLErrorCheck(err, "clGetPlatformIDs", "get platform IDs", CHECK_EXIT);
    
    for (int i = 0; i < nPlatforms; ++i) {
        printf("PLATFORM #%d\n", i);
        CLPrintPlatformInfo(platforms[i]);
    }
}

void CLPrintDeviceInfo(CLDevice deviceID)
{
    cl_int err;
    size_t nBuffer;
    char *charBuffer;
    cl_ulong ulongBuffer;
    
    
    int charNInfo = 3;
    char *charInfoNames[] = {"Vendor", "Name", "OpenCL Ver."};
    cl_device_info charInfo[] = {CL_DEVICE_VENDOR, CL_DEVICE_NAME, CL_DEVICE_VERSION};
    
    int ulongNInfo = 5;
    char *ulongInfoNames[] = {"Frequency", "Global Memroy", "Global Memory Cache", "Global Memory CacheLine", "Local Memory", "Max Compute Units"};
    cl_device_info ulongInfo[] = {CL_DEVICE_MAX_CLOCK_FREQUENCY, CL_DEVICE_GLOBAL_MEM_SIZE, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, CL_DEVICE_LOCAL_MEM_SIZE, CL_DEVICE_MAX_COMPUTE_UNITS};
    
    printf("Devices Info\n");
    for (int i = 0; i < charNInfo; ++i) {
        err = clGetDeviceInfo(deviceID, charInfo[i], 0, NULL, &nBuffer);
        CLErrorCheck(err, "clGetDeviceInfo", "get nBuffer", CHECK_EXIT);
        
        charBuffer = calloc(nBuffer, sizeof(charBuffer));
        
        err = clGetDeviceInfo(deviceID, charInfo[i], nBuffer, charBuffer, NULL);
        CLErrorCheck(err, "clGetDeviceInfo", "get device info", CHECK_EXIT);
        printf("%s: %s\n", charInfoNames[i], charBuffer);
    }
    
    
    for (int i = 0; i < ulongNInfo; ++i) {
        err = clGetDeviceInfo(deviceID, ulongInfo[i], sizeof(ulongBuffer), &ulongBuffer, NULL);
        CLErrorCheck(err, "clGetDeviceInfo", "get device info", CHECK_EXIT);
        printf("%s: %llu\n", ulongInfoNames[i], ulongBuffer);
    }
    printf("\n");
    
}

void CLPrintDevices(CLPlatform platform)
{
    cl_int err;
    cl_uint nDevices;
    CLDevice *devices;
    
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL, &nDevices);
    CLErrorCheck(err, "clGetDeviceIDs", "get number of devices", CHECK_EXIT);
    
    devices = (CLDevice *)calloc(nDevices, sizeof(CLDevice));
    
    err = clGetDeviceIDs(NULL, CL_DEVICE_TYPE_ALL, nDevices, devices, NULL);
    CLErrorCheck(err, "clGetDeviceIDs", "get device IDs", CHECK_EXIT);
    
    for (int i = 0; i < nDevices; ++i) {
        printf("DEVICE #%d\n", i);
        CLPrintDeviceInfo(devices[i]);
    }
}
