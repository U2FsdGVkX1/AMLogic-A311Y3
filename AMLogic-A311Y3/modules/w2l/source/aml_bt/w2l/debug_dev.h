/*
* Copyright (c) 202X Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*/
#ifndef __DEBUG_DEV_H__
#define __DEBUG_DEV_H__

#define AML_BT_CHAR_DEBUG_DEVICE "aml_bt_debug"
#define AML_BT_CHAR_RECYDBG_NAME "aml_recy_dbg"

//recovery dbg use
#define MAX_DBG_BUF              64
#define BT_DRV_STATE_RECOVERY    BIT(3)

typedef struct
{
    struct dentry *debug_dir;
    char recy_dbg_buf[MAX_DBG_BUF];
} debug_dev_t;

int amlbt_debug_dev_init(void);
void amlbt_debug_dev_deinit(amlbt_t *p_bt);

#endif

