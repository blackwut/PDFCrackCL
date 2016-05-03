//
//  CLPrint.h
//  OpenCL
//
//  Created by Alberto Ottimo on 22/12/14.
//
//

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "CLManager.h"

void CLPrintPlatformInfo(CLPlatform platformID);
void CLPrintPlatforms();
void CLPrintDeviceInfo(CLDevice deviceID);
void CLPrintDevices(CLPlatform platform);
