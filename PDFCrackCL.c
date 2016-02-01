//
//  main.c
//  OpenCL
//
//  Created by Alberto Ottimo on 21/12/14.
//
//

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CLPrint.h"

#define BUFFER_SIZE 2048
#define N 8

#define PUTCHAR(buf, index, val) (buf)[(index)>>2] = ((buf)[(index)>>2] & ~(0xffU << (((index) & 3) << 3))) + ((val) << (((index) & 3) << 3))
#define GETCHAR(buf, index) (((unsigned char*)(buf))[(index)])

void printHash(unsigned int i, unsigned int * hash)
{
    printf("%d = ", i);
    
    for (int i = 0; i < 16; i++) {
        if (i != 0 && i % 4 == 0)
            printf(" ");
        
        printf("%02x", GETCHAR(hash, i));
    }
    printf("\n");
}

int main(int argc, const char * argv[]) {
    
    int platformIndex = -1;
    int deviceIndex = -1;
    char buffer[BUFFER_SIZE];
    
    cl_platform_id platform = NULL;
    cl_device_id device;
    
    //Parameter #1: platform
    if (argc > 1) {
        platformIndex = atoi(argv[1]);
        if ((platform = CLSelectPlatform(platformIndex)) == NULL)
            platformIndex = -1;
    }
    //Parameter #2: device
    if (argc > 2) {
        deviceIndex = atoi(argv[2]);
        if ((device = CLSelectDevice(platform, deviceIndex)) == NULL)
            deviceIndex = -1;
    }
    
    //If user not select platform
    if (platformIndex == -1) {
        do {
            CLPrintPlatforms();
            
            printf("Select Platform: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            
            platformIndex = atoi(buffer);
            printf("\n");
        } while ((platform = CLSelectPlatform(platformIndex)) == NULL);
    }
    
    //If user not select device
    if (deviceIndex == -1) {
        do {
            CLPrintDevices(platform);
            printf("Select Device: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            
            deviceIndex = atoi(buffer);
            printf("\n");
        } while ((device = CLSelectDevice(platform, deviceIndex)) == NULL);
    }
    
    cl_context context = CLCreateContext(platform, device);
    cl_command_queue queue = CLCreateQueue(context, device);
    cl_program program = CLCreateProgram(context, device, "Kernels.ocl");
    
    unsigned int numberOfWords = 1024 * 1024;
    size_t dataSize = sizeof(unsigned int) * N * numberOfWords;
    char charset[36] = "abcdefghijklmnopqrstuvwxyz0123456789";
    size_t charsetLength = strlen(charset);

    
    cl_mem charset_d = CLCreateBufferHostVar(context, CL_MEM_READ_ONLY, sizeof(charset), charset, "charset_d");
    cl_mem words_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, dataSize, "words_d");
    cl_mem wordsLength_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, numberOfWords * sizeof(unsigned int), "wordsLength_d");
    
    
    size_t gws = numberOfWords;
    //size_t lws = 16;
    
    cl_event eventInitKernel;
    cl_kernel initKernel = CLCreateKernel(program, "initWordsWithPad");
    CLSetKernelArg(initKernel, 0, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
    CLSetKernelArg(initKernel, 1, sizeof(charset_d), &charset_d, "charset_d");
    CLSetKernelArg(initKernel, 2, sizeof(charsetLength), &charsetLength, "charsetLength");
    CLSetKernelArg(initKernel, 3, sizeof(words_d), &words_d, "words_d");
    CLSetKernelArg(initKernel, 4, sizeof(wordsLength_d), &wordsLength_d, "wordsLength_d");

    CLEnqueueNDRangeKernel(queue, initKernel, NULL, &gws, NULL, 0, NULL, &eventInitKernel, "initKernel");
    CLFinish(queue);

    
    //KERNEL MD5
    cl_mem hashes_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, dataSize, "hashes_d");
    
    cl_event eventMD5Kernel;
    cl_kernel md5Kernel = CLCreateKernel(program, "MD5");
    CLSetKernelArg(md5Kernel, 0, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
    CLSetKernelArg(md5Kernel, 1, sizeof(words_d), &words_d, "words_d");
    CLSetKernelArg(md5Kernel, 2, sizeof(wordsLength_d), &wordsLength_d, "wordsLength_d");
    CLSetKernelArg(md5Kernel, 3, sizeof(hashes_d), &hashes_d, "hashes_d");
    
    CLEnqueueNDRangeKernel(queue, md5Kernel, NULL, &gws, NULL, 0, NULL, &eventMD5Kernel, "MD5Kernel");
    CLFinish(queue);

    unsigned int * hashes = malloc(dataSize);
    clEnqueueReadBuffer(queue, hashes_d, CL_TRUE, 0, dataSize, hashes, 1, &eventMD5Kernel, NULL);
    
    for (int i = 0; i < 3; i++) {
        printHash(i, &hashes[i * 4]);
    }
    printf("\n");

    size_t initDataSize = sizeof(numberOfWords) + sizeof(charset) + sizeof(charsetLength) + (numberOfWords * N * sizeof(unsigned int)) + numberOfWords * sizeof(unsigned int);
    printStatsKernel(eventInitKernel, numberOfWords, initDataSize, "initKernel");
    
    size_t md5DataSize = sizeof(numberOfWords) + (2 * (numberOfWords * N * sizeof(unsigned int))) + numberOfWords * sizeof(unsigned int);
    printStatsKernel(eventMD5Kernel, numberOfWords, md5DataSize, "md5Kernel");
    
    CLReleaseMemObject(charset_d, "charset_d");
    CLReleaseMemObject(words_d, "words_d");
    CLReleaseMemObject(wordsLength_d, "wordsLength_d");
    CLReleaseMemObject(hashes_d, "hashes_d");
    
    free(hashes);
    
    return EXIT_SUCCESS;
}