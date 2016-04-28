//
//  main.c
//  PDFCrackCL
//
//  Created by Alberto Ottimo on 21/12/14.
//
//

#ifdef __APPLE__
#include <OpenCL/cl.h>
#define COMMON_DIGEST_FOR_OPENSSL
#include <CommonCrypto/CommonDigest.h>
#else
#include <CL/cl.h>
#include <openssl/md5.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "CLPrint.h"

#include "pdfparser.h"

#define BUFFER_SIZE (16 * 1024)
#define N 8

#define PUTCHAR(buf, index, val) (buf)[(index)>>2] = ((buf)[(index)>>2] & ~(0xffU << (((index) & 3) << 3))) + ((val) << (((index) & 3) << 3))
#define GETCHAR(buf, index) (((unsigned char*)(buf))[(index)])

unsigned char pad[32] = {
    0x28, 0xBF, 0x4E, 0x5E, 0x4E, 0x75, 0x8A, 0x41,
    0x64, 0x00, 0x4E, 0x56, 0xFF, 0xFA, 0x01, 0x08,
    0x2E, 0x2E, 0x00, 0xB6, 0xD0, 0x68, 0x3E, 0x80,
    0x2F, 0x0C, 0xA9, 0xFE, 0x64, 0x53, 0x69, 0x7A
};

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

void printWord(unsigned int id, unsigned char * charset, unsigned int charsetLength)
{
    unsigned int n = id;//203617; // = "bene"
    unsigned int x = 0;
    unsigned int wordLength = 0;
    unsigned char word[33];
    
    while (n != 0 && wordLength < 32) {
        x = n % charsetLength;
        n = n / charsetLength;
        word[wordLength++] = charset[x];
    }
    word[wordLength] = '\0';
    printf("Word: %s\n", word);
}

void md5(unsigned char * string, unsigned int length, unsigned char digest[16])
{
    MD5_CTX context;
    MD5_Init(&context);
    MD5_Update(&context, string, length);
    MD5_Final(digest, &context);
}

void loadDataFromPDFAtPath(const char * path, EncData * data) {
    
    FILE * file;
    
    if((file = fopen(path, "rb")) == 0) {
        fprintf(stderr,"Error: file not found\n");
        exit(-1);
    }
    
    if(!openPDF(file, data)) {
        fprintf(stderr, "Error: Not a valid PDF\n");
        exit(-2);
    }
    
    int ret = getEncryptedInfo(file, data);
    if(ret) {
        if(ret == EENCNF) {
            fprintf(stderr, "Error: Could not extract encryption information\n");
        } else if(ret == ETRANF || ret == ETRENF || ret == ETRINF) {
            fprintf(stderr, "Error: Encryption not detected (is the document password protected?)\n");
            exit(-3);
        }
    } else if(data->revision < 2 || (strcmp(data->s_handler,"Standard") != 0 || data->revision > 5)) {
        fprintf(stderr, "The specific version is not supported (%s - %d)\n", data->s_handler, data->revision);
        exit(-4);
    }
    
    if(fclose(file)) {
        fprintf(stderr, "Error: closing file \n");
        exit(-5);
    }
}

static void printHelp(char * program) {
    printf("Usage: %s -f filename [OPTIONS]\n"
           "\t-f, --filename filename\n"
           "\t-p, --platform\tnumber of platform\n"
           "\t-d, --device\tnumber of device\n"
           , program);
}

static struct option long_options[] = {
    {"filename", required_argument, 0, 'f'},
    {"platform", optional_argument, 0, 'p'},
    {"device",   optional_argument, 0, 'd'},
    {0, 0, 0, 0}
};

int main(int argc, char ** argv) {
    
    char path[BUFFER_SIZE] = {'\0'};// = "/Volumes/RamDisk/pdfchallenge/pdfprotected_weak.pdf";
    int platformIndex = -1;
    int deviceIndex = -1;
    char buffer[BUFFER_SIZE];
    
    cl_platform_id platform = NULL;
    cl_device_id device = NULL;
    
    
    //TODO: Sistemare meglio la situazione
    while (true) {
        
        int c;
        int option_index = 0;
        
        c = getopt_long(argc, argv, "f:p:d:", long_options, &option_index);
        
        printf("c: %d\n", c);
        printf("option_index: %d\n", option_index);
        
        if (c == -1){
            break;
        }
        
        switch (c) {
               
            case 0:
            case '?':
                printHelp(argv[0]);
                break;
                
            case 'f':
                if (path[0] == '\0') {
                    
                    if (access(optarg, F_OK) != -1) {
                        strcpy(path, optarg);
                    } else {
                        exit(1);
                    }
                }
                break;
            case 'p':
                if (platformIndex == -1) {
                    platformIndex = atoi(optarg);
                    if ((platform = CLSelectPlatform(platformIndex)) == NULL) {
                        exit(2);
                    }
                }
                break;
            case 'd':
                if (deviceIndex == -1) {
                    deviceIndex = atoi(optarg);
                    if ((device = CLSelectDevice(platform, deviceIndex)) == NULL) {
                        exit(3);
                    }
                }
                break;
            default:
                printHelp(argv[0]);
                exit(0);
        }
    }
    
    if (optind < argc) {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        putchar ('\n');
    }

    if (path[0] == '\0') {
        printHelp(argv[0]);
        exit(1);
    }

    
    EncData * data;
    data = malloc(sizeof(EncData));
    loadDataFromPDFAtPath(path, data);
    printEncData(data);
    
    unsigned char digest[16];
    unsigned char * buf;
    buf = malloc(sizeof(uint8_t) * 64);
    memcpy(buf, pad, 32);
    memcpy(buf + 32, data->fileID, data->fileIDLen);
    md5(buf, 32+data->fileIDLen, digest);
    
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
    memcpy(otherPad, data->o_string, data->o_length);
    otherPad[32] = data->permissions & 0xff;
    otherPad[33] = (data->permissions >> 8) & 0xff;
    otherPad[34] = (data->permissions >> 16) & 0xff;
    otherPad[35] = (data->permissions >> 24) & 0xff;
    memcpy(otherPad + 36, data->fileID, data->fileIDLen);
    
    
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
    cl_event eventMD5_50[50];
    cl_event eventFillBufferRC4;
    cl_event eventRC4[20];
    cl_event eventCheckPassword;
    
    cl_kernel kernelInitWords = CLCreateKernel(program, "initWords");
    cl_kernel kernelMD5 = CLCreateKernel(program, "MD5");
    cl_kernel kernelMD5_50 = CLCreateKernel(program, "MD5_50");
    cl_kernel kernelRC4 = CLCreateKernel(program, "RC4Local");
    cl_kernel kernelCheckPassword = CLCreateKernel(program, "checkPassword");

    CLSetKernelArg(kernelInitWords, 0, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
    CLSetKernelArg(kernelInitWords, 1, sizeof(charset_d), &charset_d, "charset_d");
    CLSetKernelArg(kernelInitWords, 2, sizeof(charsetLength), &charsetLength, "charsetLength");
    CLSetKernelArg(kernelInitWords, 3, sizeof(otherPad_d), &otherPad_d, "otherPad_d");
    CLSetKernelArg(kernelInitWords, 4, sizeof(wordsHalfOne_d), &wordsHalfOne_d, "wordsHalfOne_d");
    CLSetKernelArg(kernelInitWords, 5, sizeof(wordsHalfTwo_d), &wordsHalfTwo_d, "wordsHalfTwo_d");
    CLSetKernelArg(kernelInitWords, 6, sizeof(hashes_d), &hashes_d, "hashes_d");

    lws = CLGetPreferredWorkGroupSizeMultiple(kernelInitWords, device, "kernelInitWords");
    CLEnqueueNDRangeKernel(queue, kernelInitWords, NULL, &gws, &lws, 0, NULL, &eventInitWords, "kernelInitWords");

    CLReleaseMemObject(charset_d, "charset_d");
    CLReleaseMemObject(otherPad_d, "otherPad_d");

    
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
    CLEnqueueNDRangeKernel(queue, kernelMD5_50, NULL, &gws, &lws, 1, &eventMD5Second, eventMD5_50, "kernelMD5_50");

    for (unsigned int i = 1; i < 50; ++i) {
        CLEnqueueNDRangeKernel(queue, kernelMD5_50, NULL, &gws, &lws, 1, &eventMD5_50[i - 1], &eventMD5_50[i], "kernelMD5_50");
    }

    CLReleaseMemObject(wordsHalfOne_d, "wordsHalfOne_d");
    CLReleaseMemObject(wordsHalfTwo_d, "wordsHalfTwo_d");
    
    //RC4
    char iteration = 19;    
    cl_mem messages_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, hashesDataSize, "messages_d");
    cl_int error;
    error = clEnqueueFillBuffer(queue, messages_d, data->u_string, 16, 0, hashesDataSize, 1, &eventMD5_50[49], &eventFillBufferRC4);
    CLErrorCheck(error, "clEnqueueFillBuffer", "messages_d", CHECK_NOT_EXIT);
    
    
    lws = CLGetPreferredWorkGroupSizeMultiple(kernelRC4, device, "kernelRC4");
    CLSetKernelArg(kernelRC4, 0, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
    CLSetKernelArg(kernelRC4, 1, sizeof(hashes_d), &hashes_d, "keys_d");
    CLSetKernelArg(kernelRC4, 2, sizeof(messages_d), &messages_d, "messages_d");
    CLSetKernelArg(kernelRC4, 3, sizeof(iteration), &iteration, "iteration");
    CLSetKernelArg(kernelRC4, 4, sizeof(unsigned char) * 256 * lws, NULL, "state");

    CLEnqueueNDRangeKernel(queue, kernelRC4, NULL, &gws, &lws, 1, &eventFillBufferRC4, &eventRC4[19], "kernelRC4");

    for (char i = 18; i >= 0; --i) {
        CLSetKernelArg(kernelRC4, 3, sizeof(i), &i, "iteration");
        CLEnqueueNDRangeKernel(queue, kernelRC4, NULL, &gws, &lws, 1, &eventRC4[i+1], &eventRC4[i], "kernelRC4");
    }
    
    
    cl_mem test_d = CLCreateBufferHostVar(context, CL_MEM_READ_ONLY, sizeof(unsigned char) * 16, digest, "test_d");
    cl_mem index_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(unsigned int), "index_d");
    CLSetKernelArg(kernelCheckPassword, 0, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
    CLSetKernelArg(kernelCheckPassword, 1, sizeof(messages_d), &messages_d, "hashes_d");
    CLSetKernelArg(kernelCheckPassword, 2, sizeof(test_d), &test_d, "test_d");
    CLSetKernelArg(kernelCheckPassword, 3, sizeof(index_d), &index_d, "index_d");

    CLEnqueueNDRangeKernel(queue, kernelCheckPassword, NULL, &gws, &lws, 1, eventRC4, &eventCheckPassword, "kernelCheckPassowrd");
    CLFinish(queue);
    
    
    size_t initDataSize = sizeof(numberOfWords) + sizeof(charset) + sizeof(charsetLength) + wordsHalfDataSize * 2 + hashesDataSize;
    printStatsKernel(eventInitWords, numberOfWords, initDataSize, "initKernel");
    
    size_t md5DataSize = sizeof(numberOfWords) + wordsHalfDataSize + hashesDataSize;
    printStatsKernel(eventMD5First, numberOfWords, md5DataSize, "MD5_firstKernel");
    printStatsKernel(eventMD5Second, numberOfWords, md5DataSize, "MD5_SecondKernel");
    
    size_t md5_50DataSize = sizeof(numberOfWords) + numberOfWords * 16 * sizeof(unsigned int);
    printStatsKernel(eventMD5_50[49], numberOfWords, md5_50DataSize, "MD5_50Kernel");
    
    size_t rc4DataSize = sizeof(numberOfWords) + sizeof(data->u_string) + numberOfWords * 4 * sizeof(unsigned int) * 2;
    printStatsKernel(eventRC4[0], numberOfWords, rc4DataSize, "RC4");
    
    printTimeBetweenEvents(eventInitWords, eventCheckPassword, "Total Time");

    
    unsigned int index;
    clEnqueueReadBuffer(queue, index_d, CL_TRUE, 0, sizeof(unsigned int), &index, 0, NULL, NULL);
    printWord(index, charset, charsetLength);
    
    CLReleaseMemObject(messages_d, "messages_d");
    CLReleaseMemObject(hashes_d, "hashes_d");
    CLReleaseMemObject(test_d, "test_d");
    CLReleaseMemObject(index_d, "index_d");
        
    return EXIT_SUCCESS;
}