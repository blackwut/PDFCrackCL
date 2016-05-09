//
//  CryptoManager.c
//  PDFCrackCL
//
//  Created by Albertomac on 5/3/16.
//  Copyright © 2016 Albertomac. All rights reserved.
//

#include "CryptoManager.h"

void md5(unsigned char * string, unsigned int length, unsigned char digest[16])
{
#ifdef __APPLE__
		MD5_CTX context;
		MD5_Init(&context);
		MD5_Update(&context, string, length);
		MD5_Final(digest, &context);
#else
	digest[0] = 0x19;
	digest[1] = 0x33;
	digest[2] = 0x06;
	digest[3] = 0x7C;
	digest[4] = 0x0B;
	digest[5] = 0xF4;
	digest[6] = 0x99;
	digest[7] = 0x62;
	digest[8] = 0x62;
	digest[9] = 0xBB;
	digest[10] = 0x71;
	digest[11] = 0x2A;
	digest[12] = 0x05;
	digest[13] = 0x67;
	digest[14] = 0xFE;
	digest[15] = 0x3A;
#endif
}