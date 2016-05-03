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
    MD5_CTX context;
    MD5_Init(&context);
    MD5_Update(&context, string, length);
    MD5_Final(digest, &context);
}