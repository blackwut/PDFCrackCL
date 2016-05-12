//
//  main.c
//  PDFCrackCL
//
//  Created by Alberto Ottimo on 21/12/14.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "CLPrint.h"
#include "CryptoManager.h"
#include "pdfparser.h"

#define BUFFER_SIZE (16 * 1024)
#define N 8

#define PUTCHAR(buf, index, val) (buf)[(index)>>2] = ((buf)[(index)>>2] & ~(0xffU << (((index) & 3) << 3))) + ((val) << (((index) & 3) << 3))
#define GETCHAR(buf, index) (((unsigned char*)(buf))[(index)])


unsigned char pad[32] =
{
	0x28, 0xBF, 0x4E, 0x5E, 0x4E, 0x75, 0x8A, 0x41,
	0x64, 0x00, 0x4E, 0x56, 0xFF, 0xFA, 0x01, 0x08,
	0x2E, 0x2E, 0x00, 0xB6, 0xD0, 0x68, 0x3E, 0x80,
	0x2F, 0x0C, 0xA9, 0xFE, 0x64, 0x53, 0x69, 0x7A
};

void printHash(unsigned int index, unsigned int * hash)
{
    printf("%d = ", index);

    for (int i = 0; i < 16; i++) {
        if (i != 0 && i % 4 == 0)
            printf(" ");
        
        printf("%02x", GETCHAR(hash, i));
    }
    printf("\n");
}

void printWord(unsigned int id, unsigned char * charset, unsigned int charsetLength)
{
    unsigned int wordLength = 0;
    unsigned char word[33];
    
    unsigned int remainder = 0;
    unsigned int quoto = id;
    unsigned int quotoPrev;
    
    do {
        quotoPrev = quoto;
        quoto /= charsetLength;
        remainder = quotoPrev - (quoto * charsetLength);
        word[wordLength++] = charset[remainder];
    } while (quoto-- != 0 && wordLength < 32);
    
    word[wordLength] = '\0';
    printf("Word: %s\n", word);
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

static void printHelp(char * program)
{
    printf("Usage: %s -f filename [OPTIONS]\n"
           "\t-f, --filename filename\n"
           "\t-p, --platform\tnumber of platform\n"
           "\t-d, --device\tnumber of device\n"
           , program);
}

static struct option long_options[] =
{
    {"filename", required_argument, 0, 'f'},
    {"platform", optional_argument, 0, 'p'},
    {"device",   optional_argument, 0, 'd'},
    {0, 0, 0, 0}
};

unsigned int numberOfWordsToGenerate(unsigned int numberOfCharacters, size_t charsetLenght)
{
    unsigned int number = 0;
    unsigned int pow = 1;
    for (unsigned int i = 0; i < numberOfCharacters; ++i) {
        pow *= charsetLenght;
        number += pow;
    }
    return number;
}

CLEvent runInitWords(CLContext context, CLQueue queue, CLProgram program, CLDevice device,
					 size_t gws, size_t lws,
					 unsigned int numberOfWords, unsigned char * offsetString, unsigned char * charset, unsigned char * otherPad,
					 unsigned int offsetStringSize, unsigned int offsetStringLength, unsigned int charsetLength, unsigned int otherPadLength,
					 CLMem wordsHalfOne, CLMem wordsHalfTwo, CLMem hashes)
{
	//OpenCL
	CLKernel kernelInitWords = CLCreateKernel(program, "initWordsWithOffset");
	CLEvent eventInitWords;

	//CLMem
	CLMem offsetString_d = CLCreateBufferHostVar(context, CL_MEM_READ_ONLY, offsetStringSize, offsetString, "offsetString_d");
	CLMem charset_d = CLCreateBufferHostVar(context, CL_MEM_READ_ONLY, charsetLength, charset, "charset_d");
	CLMem otherPad_d = CLCreateBufferHostVar(context, CL_MEM_READ_ONLY, otherPadLength, otherPad, "otherPad_d");

	//Kernel Args
	CLSetKernelArg(kernelInitWords, 0, sizeof(offsetString_d), &offsetString_d, "offsetString");
	CLSetKernelArg(kernelInitWords, 1, sizeof(offsetStringLength), &offsetStringLength, "offsetLength");
	CLSetKernelArg(kernelInitWords, 2, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
	CLSetKernelArg(kernelInitWords, 3, sizeof(charset_d), &charset_d, "charset");
	CLSetKernelArg(kernelInitWords, 4, sizeof(charsetLength), &charsetLength, "charsetLength");
	CLSetKernelArg(kernelInitWords, 5, sizeof(otherPad_d), &otherPad_d, "otherPad");
	CLSetKernelArg(kernelInitWords, 6, sizeof(wordsHalfOne), &wordsHalfOne, "wordsHalfOne");
	CLSetKernelArg(kernelInitWords, 7, sizeof(wordsHalfTwo), &wordsHalfTwo, "wordsHalfTwo");
	CLSetKernelArg(kernelInitWords, 8, sizeof(hashes), &hashes, "hashes");

	CLEnqueueNDRangeKernel(queue, kernelInitWords, NULL, &gws, &lws, 0, NULL, &eventInitWords, "kernelInitWords");

	CLReleaseMemObject(offsetString_d, "offsetString_d");
	CLReleaseMemObject(charset_d, "charset_d");
	CLReleaseMemObject(otherPad_d, "otherPad_d");

	return eventInitWords;
}

int main(int argc, char ** argv)
{
    char path[BUFFER_SIZE] = {'\0'};
    int platformIndex = -1;
    int deviceIndex = -1;
    char buffer[BUFFER_SIZE];

    CLPlatform platform = NULL;
    CLDevice device = NULL;

    //TODO: Sistemare meglio la situazione
    while (true) {
        
        int c;
        int option_index = 0;
        
        c = getopt_long(argc, argv, "f:p:d:", long_options, &option_index);
        
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
    
    //If user doesn't select platform
    if (platformIndex == -1) {
        do {
            CLPrintPlatforms();
            
            printf("Select Platform: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            
            platformIndex = atoi(buffer);
            printf("\n");
        } while ((platform = CLSelectPlatform(platformIndex)) == NULL);
    }
    
    //If user doesn't select device
    if (deviceIndex == -1) {
        do {
            CLPrintDevices(platform);
            printf("Select Device: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            
            deviceIndex = atoi(buffer);
            printf("\n");
        } while ((device = CLSelectDevice(platform, deviceIndex)) == NULL);
    }

    CLContext context = CLCreateContext(platform, device);
    CLQueue queue = CLCreateQueue(context, device);
    CLProgram program = CLCreateProgram(context, device, "Kernels.ocl");

	CLEvent eventInitWords;
	CLEvent eventMD5First;
	CLEvent eventMD5Second;
	CLEvent eventMD5_50[50];
	CLEvent eventRC4[20];
	CLEvent eventCheckPassword;

	CLKernel kernelMD5 = CLCreateKernel(program, "MD5");
	CLKernel kernelMD5_50 = CLCreateKernel(program, "MD5_50");
	CLKernel kernelRC4 = CLCreateKernel(program, "RC4Local_Rev");
	CLKernel kernelCheckPassword = CLCreateKernel(program, "checkPassword");


	//OffsetString
	unsigned char offsetString[] = "\0";
	unsigned int offsetStringSize = sizeof(offsetStringSize);
	unsigned int offsetStringLength = offsetString[0] == '\0' ? 0: sizeof(offsetString) - 1;

	//Charset
	unsigned char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
	unsigned int charsetLength = sizeof(charset) - 1;

	//Number of Words
	unsigned int numberOfCharacters = 4;
	unsigned int numberOfWords = numberOfWordsToGenerate(numberOfCharacters, charsetLength);

	//Data Size
	size_t wordsHalfDataSize = sizeof(unsigned int) * 16 * numberOfWords;
	size_t hashesDataSize = sizeof(unsigned int) * 4 * numberOfWords;

	//Other Pad
	unsigned char otherPad[52];
	memcpy(otherPad, data->o_string, data->o_length);
	otherPad[32] = data->permissions & 0xff;
	otherPad[33] = (data->permissions >> 8) & 0xff;
	otherPad[34] = (data->permissions >> 16) & 0xff;
	otherPad[35] = (data->permissions >> 24) & 0xff;
	memcpy(otherPad + 36, data->fileID, data->fileIDLen);
	unsigned int otherPadLength = 52;

	CLMem wordsHalfOne_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, wordsHalfDataSize, "wordsHalfOne_d");
	CLMem wordsHalfTwo_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, wordsHalfDataSize, "wordsHalfTwo_d");
	CLMem hashes_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, hashesDataSize, "hashes_d");

	size_t lws = 32;
	size_t gws = CLGetOptimalGlobalWorkItemsSize(numberOfWords, lws);

	eventInitWords = runInitWords(context, queue, program, device,
								  gws, lws,
								  numberOfWords, offsetString, charset, otherPad,
								  offsetStringSize, offsetStringLength, charsetLength, otherPadLength,
								  wordsHalfOne_d, wordsHalfTwo_d, hashes_d);

    //MD5 First
    CLSetKernelArg(kernelMD5, 0, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
    CLSetKernelArg(kernelMD5, 1, sizeof(wordsHalfOne_d), &wordsHalfOne_d, "wordsHalfOne_d");
    CLSetKernelArg(kernelMD5, 2, sizeof(hashes_d), &hashes_d, "hashes_d");


    lws = CLGetPreferredWorkGroupSizeMultiple(kernelMD5, device, "kernelMD5_First/Second");
	gws = CLGetOptimalGlobalWorkItemsSize(numberOfWords, lws);

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

    for (int i = 1; i < 50; ++i) {
        CLEnqueueNDRangeKernel(queue, kernelMD5_50, NULL, &gws, &lws, 1, &eventMD5_50[i - 1], &eventMD5_50[i], "kernelMD5_50");
    }

    CLReleaseMemObject(wordsHalfOne_d, "wordsHalfOne_d");
    CLReleaseMemObject(wordsHalfTwo_d, "wordsHalfTwo_d");
    
    //RC4
	CLMem messages_d;
//#if CL_VERSION_1_1

	unsigned char * filledBuffer = malloc(hashesDataSize);
	for (int i = 0; i < hashesDataSize; ++i) {
		filledBuffer[i] = data->u_string[i % 16];
	}
	messages_d = CLCreateBufferHostVar(context, CL_MEM_READ_ONLY, hashesDataSize, filledBuffer, "messages_d");

//#else
//	messages_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, hashesDataSize, "messages_d");
//	cl_int error;
//	error = clEnqueueFillBuffer(queue, messages_d, data->u_string, 16, 0, hashesDataSize, 1, &eventMD5_50[49], &eventFillBufferRC4);
//	CLErrorCheck(error, "clEnqueueFillBuffer", "messages_d", CHECK_NOT_EXIT);
//#endif


    char iteration = 19;
    
	lws = CLGetPreferredWorkGroupSizeMultiple(kernelRC4, device, "kernelRC4");
    CLSetKernelArg(kernelRC4, 0, sizeof(numberOfWords), &numberOfWords, "numberOfWords");
	CLSetKernelArg(kernelRC4, 1, sizeof(unsigned char) * 256 * lws, NULL, "state");
    CLSetKernelArg(kernelRC4, 2, sizeof(hashes_d), &hashes_d, "keys_d");
    CLSetKernelArg(kernelRC4, 3, sizeof(messages_d), &messages_d, "messages_d");
    CLSetKernelArg(kernelRC4, 4, sizeof(iteration), &iteration, "iteration");

    CLEnqueueNDRangeKernel(queue, kernelRC4, NULL, &gws, &lws, 1, &eventMD5_50[49], &eventRC4[19], "kernelRC4");
	CLFinish(queue);
    for (int i = 18; i >= 0; --i) {
        CLSetKernelArg(kernelRC4, 4, sizeof(i), &i, "iteration");
        CLEnqueueNDRangeKernel(queue, kernelRC4, NULL, &gws, &lws, 1, &eventRC4[i+1], &eventRC4[i], "kernelRC4");
		CLFinish(queue);
    }
    
	unsigned char digest[16];
	unsigned char * buf;
	buf = malloc(sizeof(uint8_t) * 64);
	memcpy(buf, pad, 32);
	memcpy(buf + 32, data->fileID, data->fileIDLen);
	md5(buf, 32+data->fileIDLen, digest);

    CLMem test_d = CLCreateBufferHostVar(context, CL_MEM_READ_ONLY, sizeof(unsigned char) * 16, digest, "test_d");
    CLMem index_d = CLCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(unsigned int), "index_d");
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
    
    //Releasing MemObjects
    CLReleaseMemObject(messages_d, "messages_d");
    CLReleaseMemObject(hashes_d, "hashes_d");
    CLReleaseMemObject(test_d, "test_d");
    CLReleaseMemObject(index_d, "index_d");
    
    //Releasing Events
    CLReleaseEvent(eventInitWords);
    CLReleaseEvent(eventMD5First);
    CLReleaseEvent(eventMD5Second);
    
    for (int i = 0; i < 50; ++i) {
        CLReleaseEvent(eventMD5_50[i]);
    }
    
    for (int i = 0; i < 20; ++i) {
        CLReleaseEvent(eventRC4[i]);
    }
    
    CLReleaseEvent(eventCheckPassword);

    
    return EXIT_SUCCESS;
}