//
//  CryptoManager.h
//  PDFCrackCL
//
//  Created by Albertomac on 5/3/16.
//  Copyright © 2016 Albertomac. All rights reserved.
//

#ifndef CryptoManager_h
#define CryptoManager_h

#ifdef __APPLE__
    #define COMMON_DIGEST_FOR_OPENSSL
    #include <CommonCrypto/CommonDigest.h>
#else
    #include <CL/cl.h>
    #include <openssl/md5.h>
#endif

void md5(unsigned char * string, unsigned int length, unsigned char digest[16]);

#endif /* CryptoManager_h */
