/*
* Copyright (c) 202X Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*/

#ifndef __DRIVER_H__
#define __DRIVER_H__

extern struct platform_device amlbt_device;
extern struct platform_driver amlbt_driver;

#define AML_BT_DRIVER_VERSION   0x03050028
#define VERSION_INFO        "TRUNK W2L 3.1"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#endif

