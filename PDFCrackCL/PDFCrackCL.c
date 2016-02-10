//
//  main.c
//  PDFCrackCL
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

#include "pdfparser.h"

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
    
    
    FILE *file;
    EncData *e;
    e = malloc(sizeof(EncData));
    int ret;
    //char inputfile[128] = "/Volumes/RamDisk/pdfchallenge/pdfprotected_weak.pdf";
    if((file = fopen("/Volumes/RamDisk/pdfchallenge/pdfprotected_weak.pdf", "rb")) == 0) {
        fprintf(stderr,"Error: file not found\n");
        return -1;
    }
    
    
    if(!openPDF(file, e)) {
        fprintf(stderr, "Error: Not a valid PDF\n");
        return -2;
    }

    ret = getEncryptedInfo(file, e);
    if(ret) {
        if(ret == EENCNF)
            fprintf(stderr, "Error: Could not extract encryption information\n");
        else if(ret == ETRANF || ret == ETRENF || ret == ETRINF) {
            fprintf(stderr, "Error: Encryption not detected (is the document password protected?)\n");
            return -3;
        }
    } else if(e->revision < 2 || (strcmp(e->s_handler,"Standard") != 0 || e->revision > 5)) {
        fprintf(stderr, "The specific version is not supported (%s - %d)\n", e->s_handler, e->revision);
        return -5;
    }
    
    printEncData(e);

    if(fclose(file)) {
        fprintf(stderr, "Error: closing file \n");
    }

    int platformIndex = -1;
    int deviceIndex = -1;
    char buffer[BUFFER_SIZE];

    cl_platform_id platform = NULL;
    cl_device_id device = NULL;
    
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
    
    unsigned int numberOfWords = 36 * 36 * 36 * 36;
    size_t wordsHalfDataSize = sizeof(unsigned int) * 16 * numberOfWords;
    size_t hashesDataSize = sizeof(unsigned int) * 4 * numberOfWords;
    unsigned char charset[36] = "abcdefghijklmnopqrstuvwxyz0123456789";
    unsigned int charsetLength = 36;

    
    unsigned char otherPad[52];
    unsigned int i;
    for (i = 0; i < 32; ++i) {
        otherPad[i] = e->o_string[i];
    }
    
    otherPad[32] = e->permissions & 0xff;
    otherPad[33] = (e->permissions >> 8) & 0xff;
    otherPad[34] = (e->permissions >> 16) & 0xff;
    otherPad[35] = (e->permissions >> 24) & 0xff;
    
    for (i = 0; i < e->fileIDLen; ++i) {
        otherPad[i + 32 + 4] = e->fileID[i];
    }
    
    cl_mem charset_d = CLCreateBufferHostVar(context, CL_MEM_READ_ONLY, sizeof(charset), charset, "charset_d");
    cl_mem otherPad_d = CLCreateBufferHostVar(context, CL_MEM_READ_ONLY, sizeof(otherPad), otherPad, "otherPad_d");
    cl_mem wordsHalfOne_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, wordsHalfDataSize, "wordsHalfOne_d");
    cl_mem wordsHalfTwo_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, wordsHalfDataSize, "wordsHalfTwo_d");
    cl_mem hashes_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, hashesDataSize, "hashes_d");

    
    size_t gws = numberOfWords;
    size_t lws;
    
    cl_event eventInitWords;
    cl_event eventMD5First;
    cl_event eventMD5Second;
    cl_event eventMD5_50;
    
    cl_kernel kernelInitWords = CLCreateKernel(program, "initWords");
    cl_kernel kernelMD5 = CLCreateKernel(program, "MD5");
    cl_kernel kernelMD5_50 = CLCreateKernel(program, "MD5_50");

    
    //Init Words
//    CLSetKernelArg(kernelInitWords, 0, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
//    CLSetKernelArg(kernelInitWords, 1, sizeof(charset_d), &charset_d, "charset_d");
//    CLSetKernelArg(kernelInitWords, 2, sizeof(charsetLength), &charsetLength, "charsetLength");
//    CLSetKernelArg(kernelInitWords, 3, sizeof(O_d), &O_d, "O_d");
//    CLSetKernelArg(kernelInitWords, 4, sizeof(P_d), &P_d, "P_d");
//    CLSetKernelArg(kernelInitWords, 5, sizeof(fileID_d), &fileID_d, "fileID_d");
//    CLSetKernelArg(kernelInitWords, 6, sizeof(wordsHalfOne_d), &wordsHalfOne_d, "wordsHalfOne_d");
//    CLSetKernelArg(kernelInitWords, 7, sizeof(wordsHalfTwo_d), &wordsHalfTwo_d, "wordsHalfTwo_d");
//    CLSetKernelArg(kernelInitWords, 8, sizeof(hashes_d), &hashes_d, "hashes_d");
    CLSetKernelArg(kernelInitWords, 0, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
    CLSetKernelArg(kernelInitWords, 1, sizeof(charset_d), &charset_d, "charset_d");
    CLSetKernelArg(kernelInitWords, 2, sizeof(charsetLength), &charsetLength, "charsetLength");
    CLSetKernelArg(kernelInitWords, 3, sizeof(otherPad_d), &otherPad_d, "otherPad_d");
    CLSetKernelArg(kernelInitWords, 4, sizeof(wordsHalfOne_d), &wordsHalfOne_d, "wordsHalfOne_d");
    CLSetKernelArg(kernelInitWords, 5, sizeof(wordsHalfTwo_d), &wordsHalfTwo_d, "wordsHalfTwo_d");
    CLSetKernelArg(kernelInitWords, 6, sizeof(hashes_d), &hashes_d, "hashes_d");

    lws = CLGetPreferredWorkGroupSizeMultiple(kernelInitWords, device, "kernelInitWords");
    CLEnqueueNDRangeKernel(queue, kernelInitWords, NULL, &gws, &lws, 0, NULL, &eventInitWords, "kernelInitWords");

    //MD5 First
    CLSetKernelArg(kernelMD5, 0, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
    CLSetKernelArg(kernelMD5, 1, sizeof(wordsHalfOne_d), &wordsHalfOne_d, "wordsHalfOne_d");
    CLSetKernelArg(kernelMD5, 2, sizeof(hashes_d), &hashes_d, "hashes_d");
    
    lws = CLGetPreferredWorkGroupSizeMultiple(kernelMD5, device, "kernelMD5_First/Second");
    CLEnqueueNDRangeKernel(queue, kernelMD5, NULL, &gws, &lws, 1, &eventInitWords, &eventMD5First, "kernelMD5_First");
    
    
    //MD5 Second
    //CLSetKernelArg(kernelMD5, 0, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
    CLSetKernelArg(kernelMD5, 1, sizeof(wordsHalfTwo_d), &wordsHalfTwo_d, "wordsHalfTwo_d");
    //CLSetKernelArg(kernelMD5, 2, sizeof(hashes_d), &hashes_d, "hashes_d");
    CLEnqueueNDRangeKernel(queue, kernelMD5, NULL, &gws, &lws, 1, &eventMD5First, &eventMD5Second, "kernelMD5_Second");

    
    
    //MD5_50
    CLSetKernelArg(kernelMD5_50, 0, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
    CLSetKernelArg(kernelMD5_50, 1, sizeof(hashes_d), &hashes_d, "hashes_d");
    
    lws = CLGetPreferredWorkGroupSizeMultiple(kernelMD5_50, device, "kernelMD5_50");
    CLEnqueueNDRangeKernel(queue, kernelMD5_50, NULL, &gws, &lws, 1, &eventMD5Second, &eventMD5_50, "kernelMD5_50");

    for (unsigned int i = 1; i < 50; ++i) {
        CLEnqueueNDRangeKernel(queue, kernelMD5_50, NULL, &gws, &lws, 1, &eventMD5_50, &eventMD5_50, "kernelMD5_50");
    }
    
    unsigned int * hashes = malloc(hashesDataSize);
    clEnqueueReadBuffer(queue, hashes_d, CL_TRUE, 0, hashesDataSize, hashes, 1, &eventMD5_50, NULL);
    
    printf("after 50 times:\n");
    for (int i = 0; i < 3; i++) {
        printHash(i, &hashes[i * 4]);
    }
    printf("\n");
    
    
    
    size_t initDataSize = sizeof(numberOfWords) + sizeof(charset) + sizeof(charsetLength) + wordsHalfDataSize * 2 + hashesDataSize;
    printStatsKernel(eventInitWords, numberOfWords, initDataSize, "initKernel");
    
    size_t md5DataSize = sizeof(numberOfWords) + wordsHalfDataSize + hashesDataSize;
    printStatsKernel(eventMD5First, numberOfWords, md5DataSize, "MD5_firstKernel");
    printStatsKernel(eventMD5Second, numberOfWords, md5DataSize, "MD5_SecondKernel");
    
    size_t md5_50DataSize = sizeof(numberOfWords) + numberOfWords * 16 * sizeof(unsigned int);
    printStatsKernel(eventMD5_50, numberOfWords, md5_50DataSize, "MD5_50Kernel");
    
    CLReleaseMemObject(charset_d, "charset_d");
    CLReleaseMemObject(otherPad_d, "otherPad_d");
//    CLReleaseMemObject(O_d, "O_d");
//    CLReleaseMemObject(P_d, "P_d");
//    CLReleaseMemObject(fileID_d, "fileID_d");
    CLReleaseMemObject(wordsHalfOne_d, "wordsHalfOne_d");
    CLReleaseMemObject(wordsHalfTwo_d, "wordsHalfTwo_d");
    CLReleaseMemObject(hashes_d, "hashes_d");
    
    free(hashes);
    
    return EXIT_SUCCESS;
}


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

