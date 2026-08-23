/*******************************************************************************
 * Copyright (C) 2021 Amlogic, Inc. All rights reserved.
 ******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file adlak_profile.c
 * @brief
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   	Who				Date				Changes
 * ----------------------------------------------------------------------------
 * 1.00a shiwei.sun@amlogic.com	2021/08/26	Initial release
 * </pre>
 *
 ******************************************************************************/

/***************************** Include Files *********************************/
#include "adlak_profile.h"

#include "adlak_submit.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/
extern int adlak_enable_save_context_time;
int adlak_profile_start(struct adlak_device *padlak, struct adlak_pm_cfg *pm_cfg,
                        struct adlak_pm_state *pm_stat, uint32_t wpt, int32_t layer_start) {
    struct adlak_model_attr *pmodel_attr = NULL;
    struct adlak_context    *context = NULL;
    uint32_t                is_layer_start = 0;

    AML_LOG_DEBUG("%s", __func__);
    ASSERT(padlak);
    ASSERT(pm_cfg);
    ASSERT(pm_stat);
    context = ((struct adlak_task *)padlak->queue.ptask_sch_cur)->context;
    pmodel_attr =
        adlak_get_model_attr(((struct adlak_task *)padlak->queue.ptask_sch_cur)->context,
                             ((struct adlak_task *)padlak->queue.ptask_sch_cur)->sub_tasks_idx);

    if (layer_start <= pmodel_attr->first_hw_layer) {
        is_layer_start = 1;
    }
    if (1 == pm_cfg->profile_en) {
        pm_stat->start = adlak_os_ktime_get();
        adlak_hal_pm_start(padlak, pm_cfg->profile_iova, pm_cfg->profile_buf_size, 0, wpt);
        if (is_layer_start) {
            context->invoke_time_elapsed_tmp = 0;
        }
    } else if (adlak_enable_save_context_time || padlak->queue.dev_inference.nn_loading_flag) {
        pm_stat->start = adlak_os_ktime_get();
        if (is_layer_start == 1) {
            context->invoke_time_elapsed_tmp = 0;
        }
    }
    return 0;
}

int adlak_profile_stop(struct adlak_device *padlak, struct adlak_pm_cfg *pm_cfg,
                       struct adlak_pm_state *pm_stat, struct adlak_profile *profile_data,
                       int32_t layer_end) {
    struct adlak_model_attr *pmodel_attr = NULL;
    struct adlak_context    *context = NULL;
    uint32_t                time_elapsed_us = 0;
    uint32_t                is_layer_end = 0;

    AML_LOG_DEBUG("%s", __func__);
    ASSERT(padlak);
    ASSERT(pm_cfg);
    ASSERT(pm_stat);
    context = ((struct adlak_task *)padlak->queue.ptask_sch_cur)->context;
    pmodel_attr =
        adlak_get_model_attr(((struct adlak_task *)padlak->queue.ptask_sch_cur)->context,
                             ((struct adlak_task *)padlak->queue.ptask_sch_cur)->sub_tasks_idx);

    if (layer_end >= pmodel_attr->last_hw_layer) {
        is_layer_end = 1;
    }

    if (1 == pm_cfg->profile_en) {
        adlak_hal_pm_stop(padlak);
        pm_stat->finish = adlak_os_ktime_get();
        profile_data->time_elapsed_us =
            (uint32_t)adlak_os_ktime_us_delta(pm_stat->finish, pm_stat->start);
        AML_LOG_DEBUG("pm used %d ms.", profile_data->time_elapsed_us / 1000);

        context->invoke_time_elapsed_tmp += profile_data->time_elapsed_us;
        if (is_layer_end == 1) {
            context->invoke_time_elapsed_total = context->invoke_time_elapsed_tmp;
        }

        /* Accumulate busy time for nn_loading statistics */
        if (padlak->queue.dev_inference.nn_loading_flag && pm_stat->start) {
            padlak->queue.dev_inference.nn_loading_busy_time_us += profile_data->time_elapsed_us;
        }
    } else if (adlak_enable_save_context_time || padlak->queue.dev_inference.nn_loading_flag) {
        pm_stat->finish = adlak_os_ktime_get();
        if (pm_stat->start) {
            time_elapsed_us =(uint32_t)adlak_os_ktime_us_delta(pm_stat->finish, pm_stat->start);
        }
        context->invoke_time_elapsed_tmp += time_elapsed_us;
        padlak->queue.dev_inference.nn_loading_busy_time_us += time_elapsed_us; // accumulate busy time for nn_loading statistics
        if (is_layer_end == 1) {
            context->invoke_time_elapsed_total = context->invoke_time_elapsed_tmp;
        }
    }

    return 0;
}
