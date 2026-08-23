/*******************************************************************************
 * Copyright (C) 2024 Amlogic, Inc. All rights reserved.
 ******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file adlak_mm_mbd.c
 * @brief
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   	Who				Date				Changes
 * ----------------------------------------------------------------------------
 * 1.00a shiwei.sun@amlogic.com	2024/03/13	Initial release
 * </pre>
 *
 ******************************************************************************/

/***************************** Include Files *********************************/
#include "adlak_mm_mbd.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

struct adlak_mem_mbd {
    struct adlak_mem_usage usage;
    adlak_os_mutex_t       mutex;
    struct adlak_device *  padlak;
    struct device *        dev;
};

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

static void adlak_mbd_deinit(void) {}

static int adlak_mbd_init(void) { return 0; }

void adlak_mem_mbd_unregister(struct adlak_device *padlak, void **priv,
                              struct adlak_mem_operator *ops) {
    struct adlak_mem_mbd *ptr_mm_mbd = (struct adlak_mem_mbd *)*priv;
    AML_LOG_INFO("%s", __func__);
    if (ptr_mm_mbd) {
        adlak_mbd_deinit();
        adlak_os_free(ptr_mm_mbd);
        *priv = NULL;
    }
    ops->alloc         = NULL;
    ops->free          = NULL;
    ops->attach        = NULL;
    ops->dettach       = NULL;
    ops->mmap          = NULL;
    ops->unmap         = NULL;
    ops->vmap          = NULL;
    ops->vunmap        = NULL;
    ops->cache_flush   = NULL;
    ops->cache_invalid = NULL;
    ops->get_usage     = NULL;
}

int adlak_mem_mbd_register(struct adlak_device *padlak, void **priv,
                           struct adlak_mem_operator *ops) {
    struct adlak_mem_mbd *ptr_mm_mbd = NULL;
    int                   ret        = 0;
    if (!priv) {
        ret = ERR(ENODEV);
        goto end;
    }
    adlak_mem_mbd_unregister(padlak, priv, ops);
    AML_LOG_INFO("%s", __func__);
    if (!*priv) {
        *priv = adlak_os_zalloc(sizeof(struct adlak_mem_mbd), ADLAK_GFP_KERNEL);
        if (unlikely(!*priv)) {
            ret = ERR(ENOMEM);
            goto end;
        }
    }
    ptr_mm_mbd         = (struct adlak_mem_mbd *)*priv;
    ptr_mm_mbd->padlak = padlak;

    adlak_os_mutex_init(&ptr_mm_mbd->mutex);
    ptr_mm_mbd->padlak = padlak;
    // TODO bind your mbd function
    ops->alloc         = NULL;
    ops->free          = NULL;
    ops->attach        = NULL;
    ops->dettach       = NULL;
    ops->mmap          = NULL;
    ops->unmap         = NULL;
    ops->vmap          = NULL;
    ops->vunmap        = NULL;
    ops->cache_flush   = NULL;
    ops->cache_invalid = NULL;
    ops->get_usage     = NULL;
    adlak_mbd_init();

end:
    return ret;
}
