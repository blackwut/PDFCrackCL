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

void CLPrintPlatformInfo(cl_platform_id platformID);
void CLPrintPlatforms();
void CLPrintDeviceInfo(cl_device_id deviceID);
void CLPrintDevices(cl_platform_id platform);
