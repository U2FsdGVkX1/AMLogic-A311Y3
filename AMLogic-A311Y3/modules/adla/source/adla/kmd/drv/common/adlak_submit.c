/*******************************************************************************
 * Copyright (C) 2021 Amlogic, Inc. All rights reserved.
 ******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file adlak_submit.c
 * @brief
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   	Who				Date				Changes
 * ----------------------------------------------------------------------------
 * 1.00a shiwei.sun@amlogic.com	2021/06/13	Initial release
 * </pre>
 *
 ******************************************************************************/

/***************************** Include Files *********************************/
#include "adlak_submit.h"

#include "adlak_api.h"
#include "adlak_common.h"
#include "adlak_context.h"
#include "adlak_device.h"
#include "adlak_dpm.h"
#include "adlak_hal.h"
#include "adlak_mm.h"
#include "adlak_queue.h"
/************************** Constant Definitions *****************************/
#ifndef ADLAK_DEBUG_CMQ_PATTTCHING_EN
#define ADLAK_DEBUG_CMQ_PATTTCHING_EN (0)
#endif
/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/
/*per-invoke:[fence-all,ps-reset],*/
#define ADLAK_PS_CMD_EXTERN_LEN \
    (8) /*per-layer:[swid,dep,output,active,timestamp-flag,timestamp,fence]*/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

int adlak_queue_schedule_update(struct adlak_device *padlak, struct adlak_task **ptask_sch_cur,
                                int32_t net_id_pre) {
    struct adlak_workqueue *pwq   = &padlak->queue;
    struct adlak_task *     ptask = NULL, *ptask_tmp = NULL;
    int                     ret = 0;
    // iterator priority
    struct adlak_task *iter_ptask   = NULL;
    struct adlak_task *ptask_high   = NULL;
    struct adlak_task *ptask_medium = NULL;
    AML_LOG_INFO("%s", __func__);

    adlak_os_mutex_lock(&pwq->wq_mutex);
    if (list_empty(&pwq->pending_list)) {
        AML_LOG_WARN("pending_list is empty!,pwq->pending_num=%d", pwq->pending_num);
        *ptask_sch_cur = NULL;
        ret            = -1;
        pwq->pending_num--;
        goto end;
    }
    if (net_id_pre < 0) {
        ptask = list_first_entry(&pwq->pending_list, typeof(struct adlak_task), head);
        list_for_each_entry_safe(iter_ptask, ptask_tmp, &pwq->pending_list, head) {
            if (ADLAK_CONTEXT_PRIORITY_HIGH == iter_ptask->priority) {
                ptask_high = iter_ptask;
                break;
            } else if (ADLAK_CONTEXT_PRIORITY_DEFAULT == iter_ptask->priority &&
                       NULL == ptask_medium) {
                ptask_medium = iter_ptask;
            }
        }

        if (NULL != ptask_high) {
            ptask = ptask_high;
        } else if (NULL != ptask_medium) {
            ptask = ptask_medium;
        }

        if (ptask) {
            ret = 0;
        } else {
            ret = -1;
        }

    } else {
        ret = -1;
        list_for_each_entry_safe(ptask, ptask_tmp, &pwq->pending_list, head) {
            if (net_id_pre == ptask->context->net_id) {
                ret = 0;
                break;
            }
        }
    }
    if (!ret) {
        list_move_tail(&ptask->head, &pwq->scheduled_list);

        pwq->pending_num--;
        pwq->sched_num++;
        *ptask_sch_cur = ptask;
    }

end:
    adlak_os_mutex_unlock(&pwq->wq_mutex);
    return ret;
}

/**
 * adlak_queue_schedule() - Schedule a queue inference.
 * @core:	adlak core.
 *
 * Pop the inference queue until either the queue is empty or an inference has
 * been successfully scheduled.
 */

static struct adlak_model_attr *adlak_model_create(struct adlak_context *     context,
                                                   struct adlak_network_desc *psubmit_desc) {
    struct adlak_model_attr *pmodel_attr;

    AML_LOG_INFO("%s", __func__);
    pmodel_attr = adlak_os_zalloc(sizeof(struct adlak_model_attr), ADLAK_GFP_KERNEL);
    if (!pmodel_attr) {
        return ADLAK_ERR_PTR(ERR(ENOMEM));
    }

    pmodel_attr->context = context;

#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_reset(context);
    adlak_dbg_inner_update(context, "task_create");
#endif
    psubmit_desc->net_register_idx = context->net_id;

    pmodel_attr->pm_cfg.profile_en       = psubmit_desc->profile_en;
    pmodel_attr->pm_cfg.profile_iova     = psubmit_desc->profile_iova;
    pmodel_attr->pm_cfg.profile_buf_size = psubmit_desc->profile_buf_size;
    pmodel_attr->macc_count              = psubmit_desc->macc_count;
    pmodel_attr->pm_stat.pm_rpt          = 0;
    pmodel_attr->pm_stat.pm_wpt          = 0;
    pmodel_attr->priority                = psubmit_desc->priority;
    pmodel_attr->first_hw_layer          = psubmit_desc->first_hw_layer;
    pmodel_attr->last_hw_layer           = psubmit_desc->last_hw_layer;

#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(context, "task_create done");
#endif
    return pmodel_attr;
}
void adlak_model_destroy(struct adlak_model_attr *pmodel_attr) {
    AML_LOG_DEBUG("%s", __func__);

#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(pmodel_attr->context, "task destroy");
#endif

    adlak_os_free(pmodel_attr);
}

static struct adlak_task *adlak_invoke_create(struct adlak_context *            context,
                                              struct adlak_network_invoke_desc *pinvoke_desc) {
    struct adlak_model_attr *pmodel_attr;
    struct adlak_task *      pinvoke_attr;
    AML_LOG_DEBUG("%s", __func__);
    pmodel_attr = context->pmodel_attr_list[pinvoke_desc->sub_tasks_idx];
    if (!pmodel_attr) {
        AML_LOG_ERR("not found network!");
        return ADLAK_ERR_PTR(ERR(ENXIO));
    }
    ASSERT(context->net_id == pinvoke_desc->net_register_idx);
    if (pinvoke_desc->cmq_rpt < 0 || pinvoke_desc->cmq_rpt >= pinvoke_desc->cmq_size ||
        pinvoke_desc->cmq_wpt >= pinvoke_desc->cmq_size ||
        pinvoke_desc->cmq_rpt > pinvoke_desc->cmq_wpt || pinvoke_desc->smmu_entry_index > 1 ||
        pinvoke_desc->layer_start > pinvoke_desc->layer_end) {
        AML_LOG_ERR("invoke params is invalid!");
        return ADLAK_ERR_PTR(ERR(EINVAL));
    }
    if (!pmodel_attr->invoke_attr_rsv) {
        pinvoke_attr = adlak_os_zalloc(sizeof(struct adlak_task), ADLAK_GFP_KERNEL);
        if (!pinvoke_attr) {
            AML_LOG_ERR("adlak_os_zalloc fail!");
            return ADLAK_ERR_PTR(ERR(ENOMEM));
        }
        pmodel_attr->invoke_attr_rsv = pinvoke_attr;
    } else {
        pinvoke_attr = pmodel_attr->invoke_attr_rsv;
    }

    pinvoke_attr->context = context;

    ++context->invoke_count;
    if (context->invoke_count < 0) {
        context->invoke_count = 0;
    }

    pinvoke_attr->sub_tasks_idx       = pinvoke_desc->sub_tasks_idx;
    pinvoke_attr->invoke_idx          = context->invoke_count;
    pinvoke_desc->invoke_register_idx = pinvoke_attr->invoke_idx;  // return invoke index
    pinvoke_attr->ctrl_flags          = pinvoke_desc->ctrl_flags;
    pinvoke_attr->layer_start         = pinvoke_desc->layer_start;
    pinvoke_attr->layer_end           = pinvoke_desc->layer_end;
    if (pinvoke_desc->smmu_entry_index >= 1) {
        pinvoke_attr->smmu_entry_phys_addr =
            adlak_mem_get_smmu_entry(context, ADLAK_ENUM_SMMU_TLB_TYPE_PRIVATE_ONLY);
    } else {
        pinvoke_attr->smmu_entry_phys_addr =
            adlak_mem_get_smmu_entry(context, ADLAK_ENUM_SMMU_TLB_TYPE_PUBLIC_ONLY);
    }
    pinvoke_attr->cmq_base_iova = pinvoke_desc->cmq_base_iova;
    pinvoke_attr->cmq_size      = pinvoke_desc->cmq_size;
    pinvoke_attr->cmq_rpt       = pinvoke_desc->cmq_rpt;
    pinvoke_attr->cmq_wpt       = pinvoke_desc->cmq_wpt;
    pinvoke_attr->pm_wpt        = pinvoke_desc->pm_wpt;
    pinvoke_attr->priority      = pmodel_attr->priority;

    INIT_LIST_HEAD(&pinvoke_attr->head);

    return pinvoke_attr;
}

void adlak_invoke_destroy(struct adlak_task *ptask) {
    AML_LOG_DEBUG("%s", __func__);
    if (ptask) {
        ptask->context->invoke_cnt--;
        // adlak_os_free(ptask); //free the task memory will be done in model destroy.
        ptask = NULL;
    }
}

// return deleted count
static int adlak_invoke_del_from_nosch_list(struct list_head *hd, int32_t net_id,
                                            int32_t invoke_id) {
    int                ret   = 0;
    struct adlak_task *ptask = NULL, *ptask_tmp = NULL;
    AML_LOG_DEBUG("%s", __func__);
    if (!list_empty(hd)) {
        list_for_each_entry_safe(ptask, ptask_tmp, hd, head) {
            if (ptask) {
                if ((net_id != -1) && (net_id != ptask->context->net_id)) {
                    continue;
                }
                if ((invoke_id != -1) && (invoke_id != ptask->invoke_idx)) {
                    continue;
                }
                ret++;
                list_del(&ptask->head);
                adlak_invoke_destroy(ptask);
            }
        }
    }
    return ret;
}

// return deleted count
static int adlak_invoke_del_from_sch_list(struct list_head *hd, int32_t net_id, int32_t invoke_id) {
    int                ret   = 0;
    struct adlak_task *ptask = NULL, *ptask_tmp = NULL;
    AML_LOG_DEBUG("%s", __func__);
    if (!list_empty(hd)) {
        list_for_each_entry_safe(ptask, ptask_tmp, hd, head) {
            if (ptask) {
                if ((net_id != -1) && (net_id != ptask->context->net_id)) {
                    continue;
                }
                if ((invoke_id != -1) && (invoke_id != ptask->invoke_idx)) {
                    continue;
                }
                ret++;
                ptask->flag = ptask->flag | ADLAK_TASK_CANCELED;
            }
        }
    }
    return ret;
}

int adlak_invoke_del_with_invokeid(struct adlak_device *padlak, int32_t net_id, int32_t invoke_id) {
    /*
   - if in pendding list,
     - mutex lock;
     - remove from it,and release buffer
     - mutex unlock
   - if in schedule list,set invalid flag.
   - if in done list ? del or **TODO**
     */

    int                     ret         = 0;
    struct adlak_workqueue *pwq         = &padlak->queue;
    bool                    net_is_used = false;
    adlak_os_mutex_lock(&pwq->wq_mutex);
    AML_LOG_DEBUG("%s", __func__);
    AML_LOG_INFO("invoke del: net_id=%d,invoke_id=%d.", net_id, invoke_id);

    ret = adlak_invoke_del_from_nosch_list(&pwq->finished_list, net_id, invoke_id);
    if (ret > 0) {
        ret = 0;
        if (invoke_id >= 0) {
            goto end;
        }
    }

    ret = adlak_invoke_del_from_nosch_list(&pwq->pending_list, net_id, invoke_id);
    if (0 < ret) {
        ret              = 0;
        pwq->pending_num = pwq->pending_num - ret;
        if (invoke_id >= 0) {
            goto end;
        }
    }

    ret = adlak_invoke_del_from_sch_list(&pwq->scheduled_list, net_id, invoke_id);
    if (0 < ret) {
        ret         = -1;
        net_is_used = true;
    }
end:
    adlak_os_mutex_unlock(&pwq->wq_mutex);
    return ret;
}

int adlak_invoke_del_all(struct adlak_device *padlak, int32_t net_id) {
    AML_LOG_DEBUG("%s", __func__);
    return adlak_invoke_del_with_invokeid(padlak, net_id, -1);
}

static int adlak_net_attach(struct adlak_context *context, struct adlak_network_desc *psubmit_desc,
                            uint32_t sub_tasks_idx) {
    int                      ret         = 0;
    struct adlak_model_attr *pmodel_attr = NULL;

    AML_LOG_DEBUG("%s", __func__);
    /*create task*/
    pmodel_attr = adlak_model_create(context, psubmit_desc);
    if (!pmodel_attr) {
        AML_LOG_ERR("adlak task create fail!");
        ret = -1;
        goto err;
    }
    context->pmodel_attr_list[sub_tasks_idx] = pmodel_attr;
    return 0;
err:
    return ret;
}

static int adlak_invoke_add_queue(struct adlak_context *            context,
                                  struct adlak_network_invoke_desc *pinvoke_desc) {
    int                     ret    = 0;
    struct adlak_device *   padlak = context->padlak;
    struct adlak_workqueue *pwq    = &padlak->queue;
    struct adlak_task *     ptask  = NULL;
    // struct adlak_network_desc *psubmit_desc = NULL;
    AML_LOG_INFO("%s net_id[%d]", __func__, context->net_id);

    /*create task*/

    ptask = adlak_invoke_create(context, pinvoke_desc);
    if (ADLAK_IS_ERR_OR_NULL(ptask)) {
        AML_LOG_ERR("adlak task create fail!");
        ret = -1;
        goto err;
    }

    /*add list to workqueue*/

    if (ret) {
        AML_LOG_ERR("mutex lock fail!");
        ret = -1;
        goto err;
    }
    ptask->state      = ADLAK_SUBMIT_STATE_PENDING;
    ptask->error_code = ADLAK_SUCCESS;

    context->state = CONTEXT_STATE_USED;
    context->invoke_cnt++;

    adlak_os_mutex_lock(&pwq->wq_mutex);
    list_add_tail(&ptask->head, &pwq->pending_list);
    pwq->pending_num++;

    AML_LOG_INFO("pend++,pwq->pending_num = %d\n", pwq->pending_num);
    adlak_os_mutex_unlock(&pwq->wq_mutex);

    return 0;
err:
    return ret;
}

int adlak_nets_register_request(struct adlak_context *      context,
                                struct adlak_networks_desc *nets_desc) {
    int                        ret = 0;
    uint32_t                   sub_tasks_idx;
    struct adlak_network_desc  submit_desc;
    struct adlak_network_desc *psubmit_desc = &submit_desc;
#ifndef CONFIG_ADLA_FREERTOS
    void __user *desc_va_base = (void __user *)(uintptr_t)nets_desc->networks_desc_va;
#else
    void *desc_va_base = (void *)(uintptr_t)nets_desc->networks_desc_va;
#endif
    AML_LOG_DEBUG("%s", __func__);

#ifndef CONFIG_ADLA_FREERTOS

#ifdef CONFIG_COMPAT
    if (in_compat_syscall()) {
        desc_va_base = compat_ptr((compat_uptr_t)nets_desc->networks_desc_va);
    }
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
    if (!adlak_access_ok((void __user *)(uintptr_t)desc_va_base,
                         sizeof(struct adlak_network_desc) * nets_desc->sub_tasks_count))
        return ERR(EFAULT);

#else
    if (!adlak_access_ok(VERIFY_READ, (void __user *)((uintptr_t)desc_va_base),
                         sizeof(struct adlak_network_desc) * nets_desc->sub_tasks_count))
        return ERR(EFAULT);
#endif

    if (nets_desc->sub_tasks_count > 10000) {
        return ERR(ENOMEM);
    }
#endif

    context->sub_tasks_count = nets_desc->sub_tasks_count;
    context->pmodel_attr_list =
        adlak_os_zalloc(sizeof(void *) * nets_desc->sub_tasks_count, ADLAK_GFP_KERNEL);
    if (!context->pmodel_attr_list) {
        return ERR(ENOMEM);
    }

    for (sub_tasks_idx = 0; sub_tasks_idx < context->sub_tasks_count; sub_tasks_idx++) {
#ifndef CONFIG_ADLA_FREERTOS
        /*****copy data from user*****/
        ret =
            copy_from_user((void *)psubmit_desc,
                           (void __user *)(uintptr_t)(
                               desc_va_base + (sizeof(struct adlak_network_desc) * sub_tasks_idx)),
                           sizeof(struct adlak_network_desc));
        if (ret) {
            AML_LOG_ERR("copy from user failed!");
            ret = ERR(EFAULT);
            goto err_copy_from_user;
        }
#else
        psubmit_desc =
            (void *)(uintptr_t)(desc_va_base + (sizeof(struct adlak_network_desc) * sub_tasks_idx));
#endif
        ret = adlak_net_register_request(context, psubmit_desc, sub_tasks_idx);
        if (ret) {
            break;
        }
    }
    nets_desc->net_register_idx = context->net_id;

err_copy_from_user:
    if (ret) {
        if (context->pmodel_attr_list) {
            adlak_net_dettach(context);
            adlak_os_free(context->pmodel_attr_list);
            context->pmodel_attr_list = NULL;
        }
    }
    return ret;
}

int adlak_net_register_request(struct adlak_context *     context,
                               struct adlak_network_desc *psubmit_desc, uint32_t sub_tasks_idx) {
    int ret = 0;

    AML_LOG_INFO("%s", __func__);

    /*2.add to workqueue*/
    ret = adlak_net_attach(context, psubmit_desc, sub_tasks_idx);
    if (ret) {
        goto err;
    }

    /*flush context memory DMA_TO_DEVICE*/
    //     adlak_context_flush_cache(context); //no need to execute

    return 0;
err:
    return ret;
}

int adlak_net_unregister_request(struct adlak_context *         context,
                                 struct adlak_network_del_desc *submit_del) {
    int                  ret    = 0;
    struct adlak_device *padlak = context->padlak;

    AML_LOG_DEBUG("%s", __func__);

    ret = adlak_invoke_del_with_invokeid(padlak, submit_del->net_register_idx, -1);
    if (0 == ret) {
        if (context->pmodel_attr_list) {
            adlak_net_dettach(context);
            adlak_os_free(context->pmodel_attr_list);
            context->pmodel_attr_list = NULL;
        }
    }
    return 0;
}

int adlak_invoke_request(struct adlak_context *            context,
                         struct adlak_network_invoke_desc *pinvoke_desc) {
    int                     ret    = 0;
    struct adlak_device *   padlak = NULL;
    struct adlak_workqueue *pwq    = NULL;
    // struct adlak_device *padlak = context->padlak;
    AML_LOG_INFO("%s", __func__);
    AML_LOG_DEBUG("net_id=%d sub_id=%d, ctrl_flags%d", pinvoke_desc->net_register_idx,
                  pinvoke_desc->sub_tasks_idx, pinvoke_desc->ctrl_flags);
    AML_LOG_DEBUG("invoke_id=%d", pinvoke_desc->invoke_register_idx);
#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(context, "invoke_request");
#endif
#ifndef CONFIG_ADLA_FREERTOS
    if (pinvoke_desc->sub_tasks_idx >= context->sub_tasks_count) {
        return ERR(ENOMEM);
    }
#endif

    adlak_os_mutex_lock(&context->context_mutex);
    ret = adlak_invoke_add_queue(context, pinvoke_desc);

    adlak_os_mutex_unlock(&context->context_mutex);

    padlak = context->padlak;
    pwq    = &padlak->queue;

#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(context, "invoke_request done");
#endif
    adlak_os_sema_give(pwq->wk_update);
    adlak_os_thread_yield();

    if (ret) {
        goto err;
    }
    return 0;
err:
    return ret;
}
int adlak_uninvoke_request(struct adlak_context *                context,
                           struct adlak_network_invoke_del_desc *pinvoke_del) {
    int                  ret    = 0;
    struct adlak_device *padlak = context->padlak;
#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(context, "uninvoke_request");
#endif

    ret = adlak_invoke_del_with_invokeid(padlak, pinvoke_del->net_register_idx,
                                         pinvoke_del->invoke_register_idx);
#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(context, "uninvoke_request done");
#endif
    return 0;
}
int adlak_get_status_request(struct adlak_context *context, struct adlak_get_stat_desc *stat_desc) {
    struct adlak_device *   padlak = context->padlak;
    struct adlak_workqueue *pwq    = &padlak->queue;
    struct list_head *      hd;
    struct adlak_task *     ptask = NULL, *ptask_tmp = NULL;
    struct adlak_mem_usage  mem_usage;
    AML_LOG_DEBUG("%s", __func__);
#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(context, "status_request");
#endif
    hd                   = &pwq->finished_list;
    stat_desc->ret_state = 1;  // invoke busy

    if (!list_empty(hd)) {
        list_for_each_entry_safe(ptask, ptask_tmp, hd, head) {
            if (ptask) {
                if (stat_desc->net_register_idx != ptask->context->net_id) {
                    continue;
                }
                if (stat_desc->invoke_register_idx != ptask->invoke_idx) {
                    continue;
                }
                stat_desc->profile_en      = ptask->profilling.profile_en;
                stat_desc->invoke_time_us  = ptask->profilling.time_elapsed_us;
                stat_desc->layer_start     = ptask->layer_start;
                stat_desc->layer_end       = ptask->layer_end;
                stat_desc->axi_freq_cur    = padlak->clk_axi_freq_real;
                stat_desc->core_freq_cur   = padlak->clk_core_freq_real;
                stat_desc->mem_alloced_umd = context->smmu_attr.alloc_byte;

                adlak_mem_get_usage(padlak, &mem_usage);
                stat_desc->mem_alloced_base = mem_usage.alloced_kmd;
                stat_desc->mem_pool_size    = mem_usage.pool_size;
                stat_desc->mem_pool_used =
                    mem_usage.alloced_kmd + mem_usage.alloced_umd + mem_usage.share_buf_size;
                stat_desc->efficiency   = adlak_dmp_get_efficiency(padlak);
                stat_desc->exrta_status = ptask->extra_status;
                if (ADLAK_SUBMIT_STATE_FINISHED == ptask->state) {
                    stat_desc->ret_state = 0;
                } else {
                    stat_desc->ret_state = -3;  // TODO
                }
                break;
            }
        }
    }
    return 0;
}

int adlak_profile_config(struct adlak_context *         context,
                         struct adlak_profile_cfg_desc *profile_cfg) {
    struct adlak_model_attr *pmodel_attr;
    AML_LOG_DEBUG("%s", __func__);
    AML_LOG_DEBUG("net_idx=%d", profile_cfg->net_register_idx);
    AML_LOG_INFO("profile_en=%d", profile_cfg->profile_en);
    AML_LOG_INFO("profile_iova=0x%lX", (uintptr_t)profile_cfg->profile_iova);
    AML_LOG_INFO("profile_buf_size=%lu KByte", (uintptr_t)(profile_cfg->profile_buf_size / 1024));

    adlak_os_mutex_lock(&context->context_mutex);
    pmodel_attr = adlak_get_model_attr(context, profile_cfg->sub_tasks_idx);
    if (pmodel_attr) {
        pmodel_attr->pm_cfg.profile_en       = profile_cfg->profile_en;
        pmodel_attr->pm_cfg.profile_iova     = profile_cfg->profile_iova;
        pmodel_attr->pm_cfg.profile_buf_size = profile_cfg->profile_buf_size;
    }
    adlak_os_mutex_unlock(&context->context_mutex);

    profile_cfg->errcode = 0;
    return 0;
}

int adlak_queue_update_task_state(struct adlak_device *padlak, struct adlak_task *ptask) {
    int                     ret     = 0;
    struct adlak_workqueue *pwq     = &padlak->queue;
    struct adlak_context *  context = NULL;
    AML_LOG_DEBUG("%s", __func__);

    ASSERT(ptask);
    if (ADLAK_SUBMIT_STATE_FINISHED != ptask->state && ADLAK_SUBMIT_STATE_FAIL != ptask->state) {
        return -1;
    }

    adlak_hal_dbg_dump_extern(padlak, ptask->context);
    {
        // adlak_context_invalid_cache(ptask->context);

        mb();
        pwq->sched_num--;
        context = ptask->context;
        if (ptask->flag & ADLAK_TASK_CANCELED) {
            AML_LOG_DEBUG("delete the task had canceled which the owner's net_id is %d.",
                          ptask->context->net_id);
            list_del(&ptask->head);
            adlak_invoke_destroy(ptask);

        } else {
            /*move to finished queue*/
            list_move_tail(&ptask->head, &pwq->finished_list);

            pwq->finished_num++;
#ifdef CONFIG_ADLAK_DEBUG_INNNER
            adlak_dbg_inner_update(context, "submit_done");
#endif
            adlak_os_sema_give(context->invoke_state);

            ret = 1;
        }

        if (CONTEXT_STATE_CLOSED == context->state) {
            adlak_os_sema_give(context->ctx_idle);
        }
    }
    return ret;
}

int adlak_wait_until_finished(struct adlak_context *      context,
                              struct adlak_get_stat_desc *stat_desc) {
    struct adlak_device *    padlak = context->padlak;
    struct adlak_workqueue * pwq    = &padlak->queue;
    struct adlak_task *      ptask = NULL, *ptask_tmp = NULL;
    int32_t                  finished   = 0;
    int32_t                  find_netid = -1;
    struct adlak_mem_usage   mem_usage;
    struct adlak_model_attr *pmodel_attr = NULL;
    AML_LOG_DEBUG("%s", __func__);
    while (1) {
        if (ERR(NONE) == adlak_os_sema_take_timeout(context->invoke_state, stat_desc->timeout_ms)) {
            find_netid = 0;
            adlak_os_mutex_lock(&pwq->wq_mutex);
            list_for_each_entry_safe(ptask, ptask_tmp, &pwq->finished_list, head) {
                if (ptask && ptask->context->net_id == stat_desc->net_register_idx) {
                    find_netid = 1;
                    if (ptask->invoke_idx == stat_desc->invoke_register_idx) {
                        {
                            finished = 1;
#ifdef CONFIG_ADLAK_DEBUG_INNNER
                            adlak_dbg_inner_update(context, "poll to umd");
#endif
                            break;
                        }
                    }
                }
            }
            adlak_os_mutex_unlock(&pwq->wq_mutex);
        } else {
            finished = -1;
            AML_LOG_WARN("wait timeout");
#ifdef CONFIG_ADLAK_DEBUG_INNNER
            adlak_dbg_inner_update(context, "poll error to umd");
#endif
        }
        if (find_netid == 0) {
            AML_LOG_ERR("Not find valid net_id in finished queue!");
            ASSERT(0);
        }

        if (finished) {
            break;
        }
    }

    if (1 == finished) {
        ASSERT(ptask);
        pmodel_attr = adlak_get_model_attr(ptask->context, ptask->sub_tasks_idx);
        adlak_os_mutex_lock(&padlak->dev_mutex);
        if (pmodel_attr) {
            stat_desc->profile_en      = pmodel_attr->pm_cfg.profile_en;
            stat_desc->invoke_time_us  = ptask->profilling.time_elapsed_us;
            stat_desc->layer_start     = ptask->layer_start;
            stat_desc->layer_end       = ptask->layer_end;
            stat_desc->axi_freq_cur    = ptask->clk_axi_freq_real;
            stat_desc->core_freq_cur   = ptask->clk_core_freq_real;
            stat_desc->mem_alloced_umd = context->smmu_attr.alloc_byte;

            adlak_mem_get_usage(padlak, &mem_usage);
            stat_desc->mem_alloced_base = mem_usage.alloced_kmd;
            stat_desc->mem_pool_size    = mem_usage.pool_size;
            stat_desc->mem_pool_used =
                mem_usage.alloced_kmd + mem_usage.alloced_umd + mem_usage.share_buf_size;
            stat_desc->efficiency   = adlak_dmp_get_efficiency(padlak);
            stat_desc->exrta_status = ptask->extra_status;
        } else if (ptask->context->ptee_model_attr) {
            stat_desc->profile_en      = 0;
            stat_desc->axi_freq_cur    = 0;
            stat_desc->core_freq_cur   = 0;
            stat_desc->mem_alloced_umd = 0;
            stat_desc->mem_pool_used   = 0;
        }
        stat_desc->ret_state = ptask->error_code;
        adlak_os_mutex_unlock(&padlak->dev_mutex);

    } else if (-1 == finished) {
        stat_desc->ret_state = ADLAK_INVOKE_TIMEOUT;  // timeout
    } else {
        // not go here
        ASSERT(0);
    }
    return ERR(NONE);
}

int adlak_submit_patch_and_exec(struct adlak_task *ptask) {
    struct adlak_device *    padlak = ptask->context->padlak;
    struct adlak_model_attr *pmodel_attr;
    pmodel_attr = adlak_get_model_attr(ptask->context, ptask->sub_tasks_idx);
    AML_LOG_INFO("%s", __func__);
    if (ptask->state != ADLAK_SUBMIT_STATE_PENDING) {
        ASSERT(0);
        return -1;
    }
    ptask->blocking = 0;
    adlak_mem_smmu_tlb_invalidate(ptask->context);
#ifdef CONFIG_ADLAK_DEBUG_SMMU_TLB_DUMP
    adlak_mem_debug_smmu_tlb_dump(ptask->context);
#endif
#ifdef CONFIG_ADLAK_DEBUG_INNNER
    adlak_dbg_inner_update(ptask->context, "submit_start");
#endif
    if (ADLAK_INVALID_ADDR != ptask->smmu_entry_phys_addr) {  // update smmu table
        adlak_hal_smmu_config(padlak, true, ptask->smmu_entry_phys_addr);
    }

    adlak_profile_start(padlak, &pmodel_attr->pm_cfg, &pmodel_attr->pm_stat, ptask->pm_wpt,
                        ptask->layer_start);

    ptask->state        = ADLAK_SUBMIT_STATE_RUNNING;
    ptask->extra_status = 0;

    ptask->invoke_partial = 0;

#ifdef CONFIG_ADLAK_TEE
    if (padlak->submit_blocking) {
        adlak_submit_tee_task(ptask);
    } else
#endif
    {
        ptask->time_stamp      = ptask->layer_end;
        ptask->hw_stat_timeout = false;
        adlak_hal_parser_submit(padlak, ptask->cmq_base_iova, ptask->cmq_size, ptask->cmq_rpt,
                                ptask->cmq_wpt);
    }

    ptask->clk_axi_freq_real  = padlak->clk_axi_freq_real;
    ptask->clk_core_freq_real = padlak->clk_core_freq_real;
    ptask->hw_stat_timeout    = false;

    pmodel_attr->hw_timeout_ms =
        padlak->hw_timeout_ms * (ptask->layer_end + 1 - ptask->layer_start);
    if (pmodel_attr->hw_timeout_ms < 3000 && padlak->hw_timeout_ms > 0) {
        pmodel_attr->hw_timeout_ms = 3000;
    }

    AML_LOG_DEBUG("%s End", __func__);
    return ERR(NONE);
}

int adlak_set_context_attribute(struct adlak_context *          context,
                                struct adlak_context_attribute *context_attr) {
    int ret = ERR(NONE);
    AML_LOG_DEBUG("%s", __func__);
    AML_LOG_INFO("Set smmu_tlb_type %u", context_attr->smmu_tlb_type);

    context->smmu_attr.smmu_tlb_type = context_attr->smmu_tlb_type;
    if ((context->smmu_attr.smmu_tlb_type != ADLAK_ENUM_SMMU_TLB_TYPE_PUBLIC_ONLY) &&
        (context->smmu_attr.smmu_tlb_type != ADLAK_ENUM_SMMU_TLB_TYPE_PRIVATE_ONLY) &&
        (context->smmu_attr.smmu_tlb_type != ADLAK_ENUM_SMMU_TLB_TYPE_PRIVATE_AND_PUBLIC)) {
        context->smmu_attr.smmu_tlb_type = ADLAK_ENUM_SMMU_TLB_TYPE_PUBLIC_ONLY;
        AML_LOG_ERR("invalid smmu tlb type %u", context_attr->smmu_tlb_type);
        ret = ERR(EINVAL);
    }

    if (NULL == context->smmu_attr.smmu_public) {
        adlak_mem_create_smmu(context->padlak, &context->smmu_attr.smmu_public,
                              ADLAK_ENUM_SMMU_TLB_TYPE_PUBLIC_ONLY);
    }
    if (context->smmu_attr.smmu_tlb_type != ADLAK_ENUM_SMMU_TLB_TYPE_PUBLIC_ONLY) {
        if (NULL == context->smmu_attr.smmu_private) {
            adlak_mem_create_smmu(context->padlak, &context->smmu_attr.smmu_private,
                                  ADLAK_ENUM_SMMU_TLB_TYPE_PRIVATE_ONLY);
        }
    }
    return ret;
}
