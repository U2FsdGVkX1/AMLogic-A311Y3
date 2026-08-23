/*******************************************************************************
 * Copyright (C) 2022 Amlogic, Inc. All rights reserved.
 ******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file adlak_device.c
 * @brief
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   	Who				Date				Changes
 * ----------------------------------------------------------------------------
 * 1.00a shiwei.sun@amlogic.com	2022/04/11	Initial release
 * </pre>
 *
 ******************************************************************************/

/***************************** Include Files *********************************/

#include "adlak_common.h"
#include "adlak_dpm.h"
#include "adlak_hal.h"
#include "adlak_interrupt.h"
#include "adlak_mm.h"
#include "adlak_platform_config.h"
#include "adlak_profile.h"
#include "adlak_submit.h"
/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

#ifdef CONFIG_ADLAK_DEBUG_INNNER
#include "adlak_dbg.c"
#endif
/**
 * @brief init device in kernel mode
 *
 * @param padlak
 * @return int
 */
int adlak_device_init(struct adlak_device *padlak) {
    int ret = 0;
    adlak_os_printf("%s kmd version: %s\n", DEVICE_NAME" linux", ADLAK_VERSION);
    adlak_os_mutex_init(&padlak->dev_mutex);
    adlak_os_spinlock_init(&padlak->spinlock);
    INIT_LIST_HEAD(&padlak->context_list);

    ret = adlak_os_mutex_lock(&padlak->dev_mutex);
    if (ret) {
        goto err_lock;
    }
    padlak->net_id_bitmap.size = ADLAK_MAX_NET_IDS;
    ret                        = adlak_simple_bitmap_pool_init(&padlak->net_id_bitmap);
    if (ret) {
        goto err_netid_pool_init;
    }
    ret = adlak_platform_pm_init(padlak);
    if (ret) {
        goto err_pm_init;
    }

    ret = adlak_mem_init(padlak);
    if (ret) {
        goto err_mem_init;
    }

    ret = adlak_queue_init(padlak);
    if (ret) {
        goto err_queue_init;
    }

    ret = adlak_dpm_init(padlak);
    if (ret) {
        goto err_dpm_init;
    }

    ret = adlak_hal_init(padlak);
    if (ret) {
        goto err_hw_init;
    }
    ret = adlak_dev_inference_init(padlak);
    if (ret) {
        AML_LOG_ERR("inference init fail!");
        goto err_inference_init;
    }

    ret = adlak_irq_init(padlak);
    if (ret) {
        AML_LOG_ERR("irq init fail!");
        ret = ERR(EINVAL);
        goto err_irq_init;
    }
    adlak_platform_suspend(padlak);

    adlak_os_mutex_unlock(&padlak->dev_mutex);

    return 0;

err_irq_init:
    adlak_dev_inference_deinit(padlak);
err_inference_init:
    adlak_hal_deinit(padlak);
err_hw_init:
    adlak_dpm_deinit(padlak);

err_dpm_init:
    adlak_queue_deinit(padlak);
err_queue_init:
    adlak_mem_deinit(padlak);

err_mem_init:
    adlak_platform_pm_deinit(padlak);
err_pm_init:
    adlak_simple_bitmap_pool_deinit(&padlak->net_id_bitmap);
err_netid_pool_init:
    adlak_os_mutex_unlock(&padlak->dev_mutex);
err_lock:
    return -1;
}

/**
 * @brief deinit device in kernel mode
 *
 * @param padlak
 * @return int
 */
int adlak_device_deinit(struct adlak_device *padlak) {
    int ret = 0;
    ret     = adlak_os_mutex_lock(&padlak->dev_mutex);
    if (ret) {
        goto err_lock;
    }
    adlak_os_mutex_unlock(&padlak->dev_mutex);
    adlak_dev_inference_deinit(padlak); /*the inference thread will internally call the dev_mutex*/
    adlak_os_mutex_lock(&padlak->dev_mutex);
    adlak_platform_resume(padlak);
    adlak_irq_deinit(padlak);
    adlak_hal_deinit(padlak);
    adlak_queue_deinit(padlak);
    adlak_os_mutex_unlock(&padlak->dev_mutex);
    adlak_destroy_all_context(padlak);
    adlak_os_mutex_lock(&padlak->dev_mutex);
    adlak_mem_deinit(padlak);
    adlak_platform_pm_deinit(padlak);
    adlak_dpm_deinit(padlak);
    adlak_simple_bitmap_pool_deinit(&padlak->net_id_bitmap);
    adlak_os_mutex_unlock(&padlak->dev_mutex);

    return 0;
err_lock:
    return -1;
}

/**
 * @brief called in top-half IRQ handler
 *
 * @param padlak
 * @return int
 */
int adlak_irq_proc(struct adlak_device *const padlak) {
    int                         irqstatus;
    uint32_t                    extra_status;
    struct adlak_task *         ptask      = NULL;
    struct adlak_dev_inference *pinference = NULL;
    // adlak_cant_sleep();
    ptask = padlak->queue.ptask_sch_cur;
    if (NULL == ptask) {
        AML_LOG_WARN("irq ptask is NULL");
        return -1;
    }
    pinference = &padlak->queue.dev_inference;

    irqstatus           = adlak_hal_get_irq_status(padlak, &extra_status);
    ptask->hw_stat      = irqstatus;
    ptask->extra_status = extra_status;
    if (ptask->hw_stat_timeout) {
        adlak_os_sema_give_from_isr(pinference->sem_irq);
        return 0;
    }

    AML_LOG_INFO("irq proc: invoke_idx %d(%d~%d), irq state %d", ptask->invoke_idx,
                 ptask->layer_start, ptask->layer_end, irqstatus);
    if (ADLAK_HAL_IRQ_STATE_NORMAL == irqstatus) {
        if (1 != ptask->invoke_partial) {
            adlak_os_sema_give_from_isr(pinference->sem_irq);
        } else {
            adlak_os_sema_give_from_isr(ptask->context->sem_irq);
        }
        return 0;
    } else if (ADLAK_HAL_IRQ_STATE_ERROR == irqstatus) {
        adlak_os_sema_give_from_isr(pinference->sem_irq);
        return 0;
    } else if (ADLAK_HAL_IRQ_STATE_SKIP == irqstatus) {
        return 0;
    } else {
        // maybe irq deinit
        AML_LOG_ERR("Not supported irq state %d", irqstatus);
        return -1;
    }
}
