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

#define BUFFER_SIZE (16 * 1024)
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
    size_t wordsHalfDataSize = sizeof(unsigned int) * 16 * numberOfWords;
    size_t hashesDataSize = sizeof(unsigned int) * 4 * numberOfWords;
    unsigned char charset[36] = "abcdefghijklmnopqrstuvwxyz0123456789";
    unsigned int charsetLength = 36;
    
    unsigned char O[32] = {
        0xB2, 0x19, 0x27, 0x81, 0x73, 0xB6, 0xE7, 0x88,
        0x8E, 0x1C, 0x17, 0xA2, 0x86, 0x31, 0xDD, 0x23,
        0x42, 0x0C, 0x94, 0xD5, 0x93, 0xC8, 0x83, 0xCA,
        0x44, 0xCA, 0xE5, 0x02, 0xDD, 0xF2, 0xE3, 0x74
    };

    unsigned char P[4] = {
        0xC0, 0xF0, 0xFF, 0xFF
    };

    unsigned char fileID[16] = {
        0xD7, 0xB1, 0x1C, 0xA0, 0x47, 0xEB, 0x61, 0x71,
        0xE6, 0xE8, 0xFE, 0x8A, 0x64, 0xB0, 0x4C, 0xFA
    };

    
    cl_mem charset_d = CLCreateBufferHostVar(context, CL_MEM_READ_ONLY, sizeof(charset), charset, "charset_d");
    cl_mem O_d = CLCreateBufferHostVar(context, CL_MEM_READ_ONLY, sizeof(O), O, "O_d");
    cl_mem P_d = CLCreateBufferHostVar(context, CL_MEM_READ_ONLY, sizeof(P), P, "P_d");
    cl_mem fileID_d = CLCreateBufferHostVar(context, CL_MEM_READ_ONLY, sizeof(fileID), fileID, "fileID_d");
    cl_mem wordsHalfOne_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, wordsHalfDataSize, "wordsHalfOne_d");
    cl_mem wordsHalfTwo_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, wordsHalfDataSize, "wordsHalfTwo_d");
    cl_mem hashes_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, hashesDataSize, "hashes_d");

    
    size_t gws = numberOfWords;
    //size_t lws = 16;
    
    cl_event eventInitKernel;
    cl_kernel initKernel = CLCreateKernel(program, "initWords");
    CLSetKernelArg(initKernel, 0, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
    CLSetKernelArg(initKernel, 1, sizeof(charset_d), &charset_d, "charset_d");
    CLSetKernelArg(initKernel, 2, sizeof(charsetLength), &charsetLength, "charsetLength");
    CLSetKernelArg(initKernel, 3, sizeof(O_d), &O_d, "O_d");
    CLSetKernelArg(initKernel, 4, sizeof(P_d), &P_d, "P_d");
    CLSetKernelArg(initKernel, 5, sizeof(fileID_d), &fileID_d, "fileID_d");
    CLSetKernelArg(initKernel, 6, sizeof(wordsHalfOne_d), &wordsHalfOne_d, "wordsHalfOne_d");
    CLSetKernelArg(initKernel, 7, sizeof(wordsHalfTwo_d), &wordsHalfTwo_d, "wordsHalfTwo_d");
    CLSetKernelArg(initKernel, 8, sizeof(hashes_d), &hashes_d, "hashes_d");

    CLEnqueueNDRangeKernel(queue, initKernel, NULL, &gws, NULL, 0, NULL, &eventInitKernel, "initKernel");
    CLFinish(queue);

    
    
//    //RC4
//    cl_event eventRC4Kernel;
//    cl_kernel rc4Kernel = CLCreateKernel(program, "RC4");
//    unsigned int num = 1024 * 64;
//    char key[3] = "key";
//    size_t keyLen = (size_t)3;
//    char * msg = "porcodioporcodioporcodioporcodio";
//    unsigned int * msgLen = malloc(num * sizeof(unsigned int));
//    for (int i = 0; i < num; ++i) {
//        msgLen[i] = 32;
//    }
//    cl_mem key_d = CLCreateBufferHostVar(context, CL_MEM_READ_WRITE, sizeof(char) * 3, key, "key_d");
//    cl_mem msg_d = CLCreateBufferHostVar(context, CL_MEM_READ_WRITE, sizeof(char) * 32, msg, "msg_d");
//    cl_mem msgLen_d = CLCreateBufferHostVar(context, CL_MEM_READ_WRITE, sizeof(unsigned int) * num, msgLen, "msgLen_d");
//    cl_mem hash_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, 4 * sizeof(unsigned int), "hash_d");
//
//    CLSetKernelArg(rc4Kernel, 0, sizeof(num), &num, "num");
//    CLSetKernelArg(rc4Kernel, 1, sizeof(key_d), &key_d, "key_d");
//    CLSetKernelArg(rc4Kernel, 2, sizeof(keyLen), &keyLen, "keyLen");
//    CLSetKernelArg(rc4Kernel, 3, sizeof(msg_d), &msg_d, "msg_d");
//    CLSetKernelArg(rc4Kernel, 4, sizeof(msgLen_d), &msgLen_d, "msgLen_d");
//    CLSetKernelArg(rc4Kernel, 5, sizeof(hash_d), &hash_d, "hash");
//
//    
//    size_t gwsRC4 = num;
//    CLEnqueueNDRangeKernel(queue, rc4Kernel, NULL, &gwsRC4, NULL, 0, NULL, &eventRC4Kernel, "rc4Kernel");
//    CLFinish(queue);
//    
//    printStatsKernel(eventRC4Kernel, gwsRC4, gwsRC4 * sizeof(char) * 32 * 2, "rc4Kernel");
    
    
//    //KERNEL MD5
//    cl_mem hashes_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, dataSize, "hashes_d");
//    
//    cl_event eventMD5Kernel;
//    cl_kernel md5Kernel = CLCreateKernel(program, "MD5");
//    CLSetKernelArg(md5Kernel, 0, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
//    CLSetKernelArg(md5Kernel, 1, sizeof(words_d), &words_d, "words_d");
//    CLSetKernelArg(md5Kernel, 2, sizeof(wordsLength_d), &wordsLength_d, "wordsLength_d");
//    CLSetKernelArg(md5Kernel, 3, sizeof(hashes_d), &hashes_d, "hashes_d");
//    
//    CLEnqueueNDRangeKernel(queue, md5Kernel, NULL, &gws, NULL, 0, NULL, &eventMD5Kernel, "MD5Kernel");
//    CLFinish(queue);
//
//    unsigned int * hashes = malloc(dataSize);
//    clEnqueueReadBuffer(queue, hashes_d, CL_TRUE, 0, dataSize, hashes, 1, &eventMD5Kernel, NULL);
//    
//    for (int i = 0; i < 3; i++) {
//        printHash(i, &hashes[i * 4]);
//    }
//    printf("\n");


    cl_event eventMD5KernelFirst;
    cl_kernel md5Kernel = CLCreateKernel(program, "MD5");
    CLSetKernelArg(md5Kernel, 0, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
    CLSetKernelArg(md5Kernel, 1, sizeof(wordsHalfOne_d), &wordsHalfOne_d, "wordsHalfOne_d");
    CLSetKernelArg(md5Kernel, 2, sizeof(hashes_d), &hashes_d, "hashes_d");
    CLEnqueueNDRangeKernel(queue, md5Kernel, NULL, &gws, NULL, 0, NULL, &eventMD5KernelFirst, "MD5Kernel");
    CLFinish(queue);
    
    cl_event eventMD5KernelSecond;
    CLSetKernelArg(md5Kernel, 0, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
    CLSetKernelArg(md5Kernel, 1, sizeof(wordsHalfTwo_d), &wordsHalfTwo_d, "wordsHalfTwo_d");
    CLSetKernelArg(md5Kernel, 2, sizeof(hashes_d), &hashes_d, "hashes_d");
    CLEnqueueNDRangeKernel(queue, md5Kernel, NULL, &gws, NULL, 0, NULL, &eventMD5KernelSecond, "MD5Kernel");
    CLFinish(queue);

    
    unsigned int * hashes = malloc(hashesDataSize);
    clEnqueueReadBuffer(queue, hashes_d, CL_TRUE, 0, hashesDataSize, hashes, 1, &eventMD5KernelSecond, NULL);
    
    for (int i = 0; i < 3; i++) {
        printHash(i, &hashes[i * 4]);
    }
    printf("\n");

    
    md5Kernel = CLCreateKernel(program, "MD5_50");
    CLSetKernelArg(md5Kernel, 0, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
    CLSetKernelArg(md5Kernel, 1, sizeof(hashes_d), &hashes_d, "hashes_d");
    for (unsigned int i = 0; i < 50; ++i) {
        CLEnqueueNDRangeKernel(queue, md5Kernel, NULL, &gws, NULL, 0, NULL, &eventMD5KernelSecond, "MD5_50Kernel");
        CLFinish(queue);
    }
    
    //unsigned int * hashes = malloc(hashesDataSize);
    clEnqueueReadBuffer(queue, hashes_d, CL_TRUE, 0, hashesDataSize, hashes, 1, &eventMD5KernelSecond, NULL);
    
    for (int i = 0; i < 3; i++) {
        printHash(i, &hashes[i * 4]);
    }
    printf("\n");
    
    size_t initDataSize = sizeof(numberOfWords) + sizeof(charset) + sizeof(charsetLength) + wordsHalfDataSize * 2 + hashesDataSize;
    printStatsKernel(eventInitKernel, numberOfWords, initDataSize, "initKernel");
    
    size_t md5DataSize = sizeof(numberOfWords) + wordsHalfDataSize + hashesDataSize;
    printStatsKernel(eventMD5KernelFirst, numberOfWords, md5DataSize, "MD5_firstKernel");
    
    printStatsKernel(eventMD5KernelSecond, numberOfWords, md5DataSize, "MD5_SecondKernel");
    
    CLReleaseMemObject(charset_d, "charset_d");
    CLReleaseMemObject(wordsHalfOne_d, "wordsHalfOne_d");
    CLReleaseMemObject(wordsHalfTwo_d, "wordsHalfTwo_d");
    CLReleaseMemObject(hashes_d, "hashes_d");
    
    free(hashes);
    
    return EXIT_SUCCESS;
}