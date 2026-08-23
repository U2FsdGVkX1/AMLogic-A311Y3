/*******************************************************************************
 * Copyright (C) 2024 Amlogic, Inc. All rights reserved.
 ******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file adlak_tee.c
 * @brief
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   	Who				Date				Changes
 * ----------------------------------------------------------------------------
 * 1.00a shiwei.sun@amlogic.com	2024/07/17	Initial release
 * </pre>
 *
 ******************************************************************************/

/***************************** Include Files *********************************/
#include "adlak_tee.h"

#include "adlak_api.h"
#include "adlak_common.h"
#include "adlak_context.h"
#include "adlak_device.h"
#include "adlak_dpm.h"
#include "adlak_hal.h"
#include "adlak_mm.h"
#include "adlak_queue.h"
#include "adlak_submit.h"
#include "adlak_os.h"
/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/
#define ADLAK_IRQ_MASK_LAYER_END (1 << 4) /* [4]: layer end event*/
#define ADLAK_IRQ_MASK_TIM_STAMP (1 << 5) /* [5]: time_stamp irq event*/

#ifndef TEE_MEM_TYPE_ADLA
#define TEE_MEM_TYPE_ADLA (0x10)
#endif

/***************** Macros (Inline Functions) Definitions *********************/

#ifdef CONFIG_ADLAK_TEE_DUMP_DRAM
#define CONFIG_ADLAK_TEE_MEM_PROTECT_EN (0)
#else
#define CONFIG_ADLAK_TEE_MEM_PROTECT_EN (1)
#endif

#if !(defined(CONFIG_ADLAK_EMU_EN) && (CONFIG_ADLAK_EMU_EN == 1))
extern uint32_t tee_register_mem(uint32_t type, phys_addr_t pa, size_t size);
extern uint32_t tee_protect_mem(uint32_t type, uint32_t level, phys_addr_t start, size_t size,
                                uint32_t *handle);
extern void     tee_unprotect_mem(uint32_t handle);
extern int      tee_config_device_state(int dev_id, int secure);
extern uint32_t tee_protect_mem_by_type(uint32_t type, phys_addr_t start, size_t size,
                                        uint32_t *handle);
#else

void     tee_unprotect_mem(uint32_t handle) {}
int      tee_config_device_state(int dev_id, int secure) { return 0; }
uint32_t tee_protect_mem_by_type(uint32_t type, phys_addr_t start, size_t size, uint32_t *handle) {
    return 0;
}

void            dma_buf_put(struct dma_buf *dmabuf){};
struct dma_buf *dma_buf_get(int fd) {
    return NULL;
};

#endif

void adlak_tee_mem_deinit(struct adlak_device *padlak) {
    AML_LOG_INFO("%s", __func__);

    if (padlak->tee_heap_handle) {
        tee_unprotect_mem(padlak->tee_heap_handle);
        padlak->tee_heap_handle = 0;
    }
    if (padlak->tee_cpu_addr) {
        dma_free_coherent(padlak->dev, padlak->tee_size, padlak->tee_cpu_addr, padlak->tee_dma_hd);
        padlak->tee_phy_base    = 0;
        padlak->tee_size = 0;
        padlak->tee_cpu_addr    = NULL;
    }
}

int adlak_tee_mem_init(struct adlak_device *padlak) {
    int           ret               = 0;
    uint64_t rsv_size         = 0;
    unsigned int size         = 0;
    dma_addr_t dma_hd = 0;
    int        try;
    size_t     size_dec;
    void *cpu_addr = NULL;

    /* Initialize CMA */
    ret = of_reserved_mem_device_init(padlak->dev);
    if (ret) {
        AML_LOG_WARN("secure cma mem not present or init failed !!!!\n");
        return 0;
    } else {
        AML_LOG_INFO("secure cma memory init ok\n");
    }

    ret = adlak_platform_get_rsv_mem_size(padlak->dev, &rsv_size);
    if (ret) {
        goto err;
    }
    size = rsv_size;
    try      = 10;
    size_dec = size / 16;
    while (try--) {
        cpu_addr = dma_alloc_coherent(padlak->dev, (size_t)size, &dma_hd, ADLAK_GFP_KERNEL);
        if (!cpu_addr) {
            AML_LOG_ERR("DMA alloc coherent failed: pa 0x%lX, size = %lu\n", (uintptr_t)dma_hd,
                        (uintptr_t)size);
            size = size - size_dec;
        } else {
            break;
        }
    }
    if (!cpu_addr) {
        goto err;
    }

    padlak->tee_dma_hd   = dma_hd;
    padlak->tee_phy_base = dma_to_phys(padlak->dev, dma_hd);
    padlak->tee_size     = size;
    padlak->tee_cpu_addr = cpu_addr;
    AML_LOG_DEBUG("[%s, %d], secure_zone base:0x%lx size:0x%x\n", __func__, __LINE__, padlak->tee_phy_base,
                    size);

    if (!ADLAK_IS_ALIGNED(padlak->tee_phy_base, 0x10000) || !ADLAK_IS_ALIGNED(padlak->tee_size, 0x10000)) {
        AML_LOG_ERR("%s: prepared memory not aligned 64K\n", __func__);
        AML_LOG_ERR("[%s, %d], secure_zone base:0x%lx size:0x%x\n", __func__, __LINE__, padlak->tee_phy_base,
                    size);
        goto prepare_memoy_fail;
    }

    /* register mem to tee */
    ret = tee_register_mem(TEE_MEM_TYPE_ADLA, padlak->tee_phy_base, padlak->tee_size);
    if (ret != 0) {
        ret = -1;
        AML_LOG_ERR("%s: tee register for adla mem fail!\n", __func__);
        goto protect_fail;

    }
    if (tee_protect_mem(TEE_MEM_TYPE_ADLA, 0, padlak->tee_phy_base, padlak->tee_size,
                        &padlak->tee_heap_handle)) {
        ret = -1;
        AML_LOG_ERR("%s: tee protect for adla mem fail!\n", __func__);
        goto protect_fail;
    }

    return 0;
protect_fail:
prepare_memoy_fail:
    dma_free_coherent(padlak->dev, padlak->tee_size, padlak->tee_cpu_addr, padlak->tee_dma_hd);
err:
    return ret;

}

int adlak_tee_net_register_request(struct adlak_context *         context,
                                   struct adlak_tee_network_desc *psubmit_desc) {
    int                          ret             = 0;
    struct adlak_tee_model_attr *ptee_model_attr = NULL;
    AML_LOG_INFO("%s", __func__);
    ptee_model_attr = adlak_os_zalloc(sizeof(struct adlak_tee_model_attr), ADLAK_GFP_KERNEL);
    if (!ptee_model_attr) {
        return ERR(ENOMEM);
    }

    ptee_model_attr->context        = context;
    ptee_model_attr->tee_ctx_handle = psubmit_desc->tee_ctx_handle;
    context->ptee_model_attr        = ptee_model_attr;

    psubmit_desc->net_register_idx = context->net_id;

    ret = adlak_os_mutex_lock(&context->context_mutex);
    if (ret) {
        AML_LOG_ERR("mutex lock fail!");
        ret = -1;
        goto err;
    }

    adlak_os_mutex_unlock(&context->context_mutex);

    return 0;
err:
    return ret;
}

int adlak_tee_net_unregister_request(struct adlak_context *         context,
                                     struct adlak_network_del_desc *submit_del) {
    int                  ret    = 0;
    struct adlak_device *padlak = context->padlak;

    AML_LOG_INFO("%s", __func__);

    if (context->ptee_model_attr) {
        ret = adlak_invoke_del_with_invokeid(padlak, submit_del->net_register_idx, -1);
        if (0 == ret) {
            adlak_os_free(context->ptee_model_attr);
            context->ptee_model_attr = NULL;
        }
    }
    if (context->secure_heap_handle) {
        tee_unprotect_mem(context->secure_heap_handle);
    }

    return 0;
}

static struct adlak_task *adlak_tee_invoke_add_queue(
    struct adlak_context *context, struct adlak_tee_network_invoke_desc *pinvoke_desc) {
    int                          ret             = 0;
    struct adlak_tee_model_attr *ptee_model_attr = NULL;
    struct adlak_task *          pinvoke_attr    = NULL;

    AML_LOG_INFO("%s net_id[%d]", __func__, context->net_id);

    ptee_model_attr = context->ptee_model_attr;
    if (!ptee_model_attr) {
        AML_LOG_ERR("not found network!");
        ret = (ERR(ENXIO));
        goto err;
    }
    if (!ptee_model_attr->invoke_attr_rsv) {
        pinvoke_attr = adlak_os_zalloc(sizeof(struct adlak_task), ADLAK_GFP_KERNEL);
        if (!pinvoke_attr) {
            AML_LOG_ERR("adlak_os_zalloc fail!");
            ret = (ERR(ENOMEM));
            goto err;
        }
        ptee_model_attr->invoke_attr_rsv = pinvoke_attr;
    } else {
        pinvoke_attr = ptee_model_attr->invoke_attr_rsv;
    }

    pinvoke_attr->context = context;

    ++ptee_model_attr->invoke_count;
    if (ptee_model_attr->invoke_count < 0) {
        ptee_model_attr->invoke_count = 0;
    }
    pinvoke_attr->invoke_idx          = ptee_model_attr->invoke_count;
    pinvoke_desc->invoke_register_idx = pinvoke_attr->invoke_idx;  // return invoke index
    pinvoke_attr->invoke_section_id   = pinvoke_desc->invoke_section_id;

    INIT_LIST_HEAD(&pinvoke_attr->head);

    pinvoke_attr->state      = ADLAK_SUBMIT_STATE_PENDING;
    pinvoke_attr->error_code = ADLAK_SUCCESS;
    context->state           = CONTEXT_STATE_USED;
    context->invoke_cnt++;

    return pinvoke_attr;
err:
    return NULL;
}

int adlak_tee_invoke_request(struct adlak_context *                context,
                             struct adlak_tee_network_invoke_desc *pinvoke_desc) {
    int                     ret    = 0;
    struct adlak_device *   padlak = context->padlak;
    struct adlak_task *     ptask;
    struct adlak_workqueue *pwq = NULL;
    AML_LOG_DEBUG("%s", __func__);
#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(context, "invoke_request");
#endif

    adlak_os_mutex_lock(&context->context_mutex);
    ptask = adlak_tee_invoke_add_queue(context, pinvoke_desc);
    adlak_os_mutex_unlock(&context->context_mutex);
    if (ADLAK_IS_ERR_OR_NULL(ptask)) {
        AML_LOG_ERR("adlak task create fail!");
        ret = -1;
        goto err;
    }

    pwq = &padlak->queue;
    adlak_os_mutex_lock(&pwq->wq_mutex);
    list_add_tail(&ptask->head, &pwq->pending_list);
    pwq->pending_num++;

    AML_LOG_INFO("pend++,pwq->pending_num = %d\n", pwq->pending_num);
    adlak_os_mutex_unlock(&pwq->wq_mutex);

#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(context, "invoke_request done");
#endif
    adlak_os_sema_give(pwq->wk_update);
    adlak_os_thread_yield();

    return 0;
err:
    return ret;
}

void adlak_tee_model_destroy(struct adlak_tee_model_attr *ptee_model_attr) {
    AML_LOG_DEBUG("%s", __func__);

#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(ptee_model_attr->context, "task destroy");
#endif
    adlak_os_free(ptee_model_attr);
}

int adlak_tee_query_addr(struct adlak_context *       context,
                         struct adlak_tee_query_addr *tee_query_addr) {
    int ret = 0;

    AML_LOG_DEBUG("%s", __func__);
    return ret;
}

int adlak_tee_protect_addr(struct adlak_context *         context,
                           struct adlak_tee_protect_addr *tee_protect_addr) {
    int ret = 0;

    AML_LOG_DEBUG("%s", __func__);
    return ret;
}

#include <linux/delay.h>
#include <linux/hw_random.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/tee_drv.h>
#include <linux/uuid.h>

static int optee_ctx_match(struct tee_ioctl_version_data *ver, const void *data) {
    if (ver->impl_id == TEE_IMPL_ID_OPTEE)
        return 1;
    else
        return 0;
}

#define TA_ADLA_CMD_INVOKE_HW (0x10 + 3)
#ifndef TA_ADLA_UUID
#define TA_ADLA_UUID \
    UUID_INIT(0x5dc33b6a, 0x216d, 0x4641, 0xa8, 0x34, 0xb3, 0xf3, 0xaa, 0x1a, 0xcc, 0xb8)
#endif

uint32_t adla_submit_through_tadla(uint64_t tee_ctx_handle, uint32_t invoke_section_id) {
    uint32_t                          hw_ret = 0;
    int                               ret = 0, err = -ENODEV;
    static const uuid_t               uuid     = TA_ADLA_UUID;
    struct tee_param                  param[4] = {0};
    struct tee_context *              ctx      = NULL;
    struct tee_ioctl_open_session_arg sess_arg;
    struct tee_ioctl_invoke_arg       inv_arg = {0};

    /* Open context with TEE driver */
    ctx = tee_client_open_context(NULL, optee_ctx_match, NULL, NULL);
    if (IS_ERR(ctx)) {
        AML_LOG_ERR("%s open context failed\n", __func__);
        hw_ret = 1;
        return hw_ret;
    }

    export_uuid(sess_arg.uuid, &uuid);
    sess_arg.clnt_login = TEE_IOCTL_LOGIN_PUBLIC;
    sess_arg.num_params = 2;
    /* Fill open cmd params */
    param[0].attr      = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INOUT;
    param[0].u.value.a = (uint32_t)TA_ADLA_CMD_INVOKE_HW;  // command type
    param[0].u.value.b = invoke_section_id;
    param[1].attr      = TEE_IOCTL_PARAM_ATTR_TYPE_VALUE_INPUT;
    param[1].u.value.a = (uint32_t)(tee_ctx_handle);        // low
    param[1].u.value.b = (uint32_t)(tee_ctx_handle >> 32);  // high

    ret = tee_client_open_session(ctx, &sess_arg, param);
    if ((ret < 0) || (sess_arg.ret != 0)) {
        AML_LOG_ERR("tee_client_open_session failed, err: %x\n", sess_arg.ret);
        err    = -EINVAL;
        hw_ret = 1;
        goto out_ctx;
    }

    /* Invoke function */
    inv_arg.func       = TA_ADLA_CMD_INVOKE_HW;
    inv_arg.session    = sess_arg.session;
    inv_arg.num_params = 2;
    ret                = tee_client_invoke_func(ctx, &inv_arg, param);
    if (ret < 0) {
        AML_LOG_ERR("%s invoke func failed, cmd = %u, ret= %d, res = 0x%x, origin = 0x%x\n",
                    __func__, TA_ADLA_CMD_INVOKE_HW, ret, inv_arg.ret, inv_arg.ret_origin);
        ret = inv_arg.ret;
    }
    AML_LOG_INFO("new tee_session_id %d\n", sess_arg.session);
    if (param[0].u.value.a == 0) {
        hw_ret = param[0].u.value.b;
    } else {
        AML_LOG_ERR("invoke hw failed, err: %llx\n", param[0].u.value.a);
        hw_ret = 0x1234;
    }

    tee_client_close_session(ctx, sess_arg.session);
out_ctx:
    tee_client_close_context(ctx);
    return hw_ret;
}

int adlak_submit_tee_task(struct adlak_task *ptask) {
    struct adlak_tee_model_attr *ptee_model_attr = NULL;
    uint32_t                     irq_raw         = 0;
    uint32_t                     invoke_section_id;
    uint64_t                     tee_ctx_handle;
    struct adlak_device *        padlak = NULL;

    ptee_model_attr = ptask->context->ptee_model_attr;
    AML_LOG_INFO("%s", __func__);
#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(ptask->context, "tee_submit_start");
#endif
    {
        ptask->blocking = 1;
#if defined(CONFIG_ADLAK_EMU_EN) && (CONFIG_ADLAK_EMU_EN == 1)
        tee_ctx_handle    = ptee_model_attr->tee_ctx_handle;
        invoke_section_id = ptask->invoke_section_id;
        AML_LOG_INFO("adla_submit_through_tadla tee_ctx_handle 0x%X , invoke_section_id 0x%X\n",
                     tee_ctx_handle, invoke_section_id);
        adlak_os_udelay(100000);
        irq_raw = adla_submit_through_tadla(tee_ctx_handle, invoke_section_id);
        AML_LOG_INFO("IRQ RAW[0x%08X] \n", irq_raw);
        irq_raw = irq_raw & (~ADLAK_IRQ_MASK_LAYER_END);
        if ((~ADLAK_IRQ_MASK_TIM_STAMP) & irq_raw) {
            ptask->state      = ADLAK_SUBMIT_STATE_FAIL;
            ptask->error_code = ADLAK_HARDWARE_TIMEOUT;
            AML_LOG_ERR("IRQ RAW[0x%08X]", irq_raw);
        } else {
            ptask->state      = ADLAK_SUBMIT_STATE_FINISHED;
            ptask->error_code = ADLAK_SUCCESS;
        }
#else
        if (ptee_model_attr) {
            invoke_section_id = ptask->invoke_section_id;
            tee_ctx_handle    = ptee_model_attr->tee_ctx_handle;
        } else {
            AML_LOG_ERR("Invalid args!");
            ASSERT(0);
            invoke_section_id = 0;
            tee_ctx_handle    = 0;
        }
        //    will blocking in teeos

        AML_LOG_INFO("adla_submit_through_tadla  tee_ctx_handle 0x%X , invoke_section_id 0x%X\n",
                     tee_ctx_handle, invoke_section_id);

        irq_raw = adla_submit_through_tadla(tee_ctx_handle, invoke_section_id);
        AML_LOG_INFO("IRQ RAW[0x%08X] \n", irq_raw);
        irq_raw = irq_raw & (~ADLAK_IRQ_MASK_LAYER_END);
        if ((~ADLAK_IRQ_MASK_TIM_STAMP) & irq_raw) {
            ptask->state      = ADLAK_SUBMIT_STATE_FAIL;
            ptask->error_code = ADLAK_HARDWARE_TIMEOUT;
            AML_LOG_ERR("IRQ RAW[0x%08X]", irq_raw);
        } else {
            ptask->state      = ADLAK_SUBMIT_STATE_FINISHED;
            ptask->error_code = ADLAK_SUCCESS;
        }

#endif
    }

#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(ptask->context, "tee_submit_end");
#endif

    ptask->hw_stat_timeout = false;
    padlak                 = ptask->context->padlak;
    adlak_hal_device_reset(padlak);
    adlak_hal_device_start(padlak);
    AML_LOG_DEBUG("%s End", __func__);
    return ERR(NONE);
}
