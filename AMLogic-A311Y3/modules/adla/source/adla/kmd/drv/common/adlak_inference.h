/*******************************************************************************
 * Copyright (C) 2022 Amlogic, Inc. All rights reserved.
 ******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file adlak_inference.c
 * @brief
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   	Who				Date				Changes
 * ----------------------------------------------------------------------------
 * 1.00a shiwei.sun@amlogic.com	2022/04/10	Initial release
 * </pre>
 *
 ******************************************************************************/

/***************************** Include Files *********************************/
#include "adlak_dpm.h"
#include "adlak_mm_common.h"
#include "adlak_profile.h"
#include "adlak_submit.h"

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

enum ADLAK_DEVICE_STATE {
    ADLAK_DEVICE_INIT = 1,
    ADLAK_DEVICE_ERR,
    ADLAK_DEVICE_IDLE,
    ADLAK_DEVICE_BUSY,
};

enum ADLAK_INFERENCE_STATE {
    ADLAK_INFERENCE_STATE_INIT = 0,
    ADLAK_INFERENCE_STATE_RESUME,
    ADLAK_INFERENCE_STATE_DEV_RESET,
    ADLAK_INFERENCE_STATE_WQ_RESET,
    ADLAK_INFERENCE_STATE_WQ_CHECK,
    ADLAK_INFERENCE_STATE_SLEEP_CHECK,
    ADLAK_INFERENCE_STATE_POST_TASK,
    ADLAK_INFERENCE_STATE_REPORT_TASK_PRE,
    ADLAK_INFERENCE_STATE_REPORT_TASK_PRE2,
    ADLAK_INFERENCE_STATE_SUBMIT_WAIT,
    ADLAK_INFERENCE_STATE_CHECK_TASK,
    ADLAK_INFERENCE_STATE_REPORT_TASK_CUR,
    ADLAK_INFERENCE_STATE_COUNT
};

/************************** Variable Definitions *****************************/
extern int *adlak_dev_state;

/************************** Function Prototypes ******************************/

static void adlak_irq_bottom_half(struct adlak_device *padlak, int *device_state,
                                  struct adlak_task *ptask);

int adlak_submit_wait(struct adlak_dev_inference *pinference, struct adlak_task *ptask) {
    int                      ret;
    int                      repeat = 0;
    uint32_t                 timeout;
    struct adlak_device *    padlak      = NULL;
    struct adlak_model_attr *pmodel_attr = NULL;
    AML_LOG_INFO("%s", __func__);
    /* make compiler happy */
    if (!pinference || !ptask) {
        return -1;
    }
    padlak = ptask->context->padlak;

#if defined(CONFIG_ADLAK_EMU_EN) && (CONFIG_ADLAK_EMU_EN == 1)
    /* Suppose ptask->sw_timeout_ms_ms is greater than 5 */
    adlak_os_timer_add(&pinference->emu_timer, 5);
#endif

    pmodel_attr = adlak_get_model_attr(ptask->context, ptask->sub_tasks_idx);
    AML_LOG_INFO("set submit hw_timeout_ms=%u ms", pmodel_attr->hw_timeout_ms);
    if (pmodel_attr->hw_timeout_ms) {
        do {
            ret = adlak_os_sema_take_timeout(pinference->sem_irq, pmodel_attr->hw_timeout_ms);
            if (ERR(NONE) == ret) {
                AML_LOG_INFO("%s\n", "sema_take success");
                timeout = false;
                break;
            } else {
                AML_LOG_WARN("%s\n", "sema_take_timeout");
                timeout = true;
#if defined(CONFIG_ADLAK_EMU_EN) && (CONFIG_ADLAK_EMU_EN == 1)
                adlak_os_timer_del(&pinference->emu_timer);
                break;
#endif
            }
            repeat++;
        } while (repeat < 2);
        ptask->hw_stat_timeout = timeout;
        if (true == timeout) {
            adlak_os_spinlock_lock(&padlak->spinlock);
            adlak_irq_proc(padlak);
            adlak_os_spinlock_unlock(&padlak->spinlock);
        }
    } else {
        ret = adlak_os_sema_take(pinference->sem_irq);
    }
    return ret;
}

static void adlak_dpm_timer_cb(adlak_os_timer_cb_t t) {
    adlak_os_ktime_t             current_time;
    uint64_t                     elapsed_us;
    struct adlak_workqueue *     pwq    = NULL;
    struct adlak_device *        padlak = NULL;
    struct adlak_os_timer_inner *data   = container_of(t, struct adlak_os_timer_inner, timer);
    padlak                              = (struct adlak_device *)(data->param);
    pwq                                 = &padlak->queue;

    adlak_os_spinlock_lock(&pwq->dev_inference.spinlock);
    if (pwq->submit_num == padlak->submit_num_pre) {
        pwq->dev_inference.wq_idel_cnt += 1;
        pwq->dev_inference.cnt_idel += 1;
    } else {
        padlak->submit_num_pre         = pwq->submit_num;
        pwq->dev_inference.wq_idel_cnt = 0;
        pwq->dev_inference.cnt_busy += 1;
    }
    pwq->dev_inference.cnt_elapsed++;
    if (pwq->dev_inference.cnt_elapsed == 0) {
        pwq->dev_inference.cnt_busy = 0;
        pwq->dev_inference.cnt_idel = 0;
    }

    /* nn_loading Statistics: If recording is in progress, check 1-second window and reset if needed.
     * The actual busy time is accumulated in adlak_profile_stop() from profile_data->time_elapsed_us.
     */
    if (pwq->dev_inference.nn_loading_flag) {
        current_time = adlak_os_ktime_get();

        /* Check if 1 second has passed since start, reset window if so */
        elapsed_us = adlak_os_ktime_us_delta(current_time, pwq->dev_inference.nn_loading_start_time);
        if (elapsed_us >= 1000000) { /* 1s = 1000000us */
            /* Reset the 1-second window: start a new window */
            pwq->dev_inference.nn_loading_start_time = current_time;
            pwq->dev_inference.nn_loading_busy_time_us = 0;
        }
    }

    adlak_os_timer_modify(&pwq->dev_inference.dpm_timer, pwq->dev_inference.dpm_period_set);

    adlak_os_spinlock_unlock(&pwq->dev_inference.spinlock);
}

static int adlak_check_netid_is_valid(struct adlak_device *padlak, int32_t net_id) {
    int                   ret = -1;
    struct adlak_context *context, *context_tmp;

    adlak_os_mutex_lock(&padlak->dev_mutex);
    list_for_each_entry_safe(context, context_tmp, &padlak->context_list, head) {
        if (context) {
            if (context->net_id == net_id) {
                ret = 0;
                break;
            }
        }
    }
    adlak_os_mutex_unlock(&padlak->dev_mutex);

    return ret;
}

static int32_t adlak_get_valid_num_locked(struct adlak_workqueue *pwq, int32_t *net_id_pre,
                                          int32_t *net_id_check) {
    /* if the net_id of context has been destroyed,the net_id will be set to -1*/
    int32_t            valid_num = 0;
    int                find      = 0;
    struct adlak_task *ptask = NULL, *ptask_tmp = NULL;
    int32_t            net_id = *net_id_pre;

    *net_id_check = -1;
    if (net_id < 0) {
        valid_num = pwq->pending_num;
    } else {
        /*the model not invoke to the end*/
        if (pwq->pending_num > 0) {
            list_for_each_entry_safe(ptask, ptask_tmp, &pwq->pending_list, head) {
                if (net_id == ptask->context->net_id) {
                    if (CONTEXT_STATE_CLOSED == ptask->context->state) {
                        /*the net_id of context has been destroyed.*/
                        *net_id_pre = -1;
                    } else {
                        find = 1;
                    }
                    break;
                }
            }
            if (1 != find) {
                *net_id_check = *net_id_pre;
            }
        }
        net_id = *net_id_pre;
        if (net_id < 0) {
            valid_num = pwq->pending_num;
        } else {
            valid_num = find;
        }
    }
    return valid_num;
}

static int32_t adlak_get_valid_num(struct adlak_workqueue *pwq, int32_t *net_id_pre,
                                   int32_t *pending_num_out) {
    int32_t valid_num;
    int32_t net_id_check;

    while (1) {
        adlak_os_mutex_lock(&pwq->wq_mutex);
        valid_num        = adlak_get_valid_num_locked(pwq, net_id_pre, &net_id_check);
        *pending_num_out = pwq->pending_num;
        adlak_os_mutex_unlock(&pwq->wq_mutex);

        if (net_id_check < 0) {
            break;
        }

        /*
         * Do not check context_list while holding wq_mutex. ioctl invoke path
         * holds dev_mutex first, then waits for wq_mutex, so taking dev_mutex
         * here under wq_mutex can deadlock with concurrent submissions.
         */
        if (0 == adlak_check_netid_is_valid(pwq->padlak, net_id_check)) {
            break;
        }

        *net_id_pre = -1;
        AML_LOG_WARN("the net id [%d] is invalid!", net_id_check);
    }

    return valid_num;
}

struct adlak_dpm_counter {
    adlak_os_ktime_t start;
    adlak_os_ktime_t finish;
    uint64_t         time_threshold;
    uint64_t         time_elapsed_us;
};

#ifndef CONFIG_ADLA_FREERTOS
static int adlak_dev_inference_cb(void *args) {
#else
static void *adlak_dev_inference_cb(void *args) {
#endif

    int                      ret;
    struct adlak_device *    padlak                 = args;
    struct adlak_workqueue * pwq                    = &padlak->queue;
    adlak_os_thread_t *      pthrd                  = &pwq->dev_inference.thrd_inference;
    struct adlak_task *      ptask_sch_cur          = NULL;
    int32_t                  net_id_invoke_previous = -1;
    int                      device_state, dpm_stategy, valid_num, pending_num = 0;
    struct adlak_model_attr *pmodel_attr = NULL;
    struct adlak_dpm_counter dpm_counter;
    bool                     reset_hw_enable = false;
    bool                     forbid_dpm      = true;

#ifdef CONFIG_PM
    int pm_suspend;
#endif
    padlak->submit_num_pre = -1;
    ret = adlak_os_timer_init(&pwq->dev_inference.dpm_timer, adlak_dpm_timer_cb, padlak);
    if (ret) {
        AML_LOG_ERR("dpm_timer init fail!\n");
#ifndef CONFIG_ADLA_FREERTOS
        return 0;
#else
        return NULL;
#endif
    }
#if CONFIG_ADLAK_DPM_EN
    dpm_counter.time_threshold = padlak->dpm_period_set * 1000 * 10;
    AML_LOG_WARN("dpm_timer_period: %d\n", padlak->dpm_period_set);
    dpm_counter.start = adlak_os_ktime_get();

    pwq->dev_inference.dpm_period_set = padlak->dpm_period_set;
    adlak_os_timer_add(&pwq->dev_inference.dpm_timer, pwq->dev_inference.dpm_period_set);
    dpm_stategy = ADLAK_DPM_STRATEGY_MIN;
    forbid_dpm  = false;
#endif
    adlak_dev_state = &device_state;
    while (!pthrd->thrd_should_stop) {
#ifdef CONFIG_PM
        adlak_os_mutex_lock(&padlak->dev_mutex);
        pm_suspend = padlak->pm_suspend;
        adlak_os_mutex_unlock(&padlak->dev_mutex);
        if (true == pm_suspend) {
            dpm_stategy = ADLAK_DPM_STRATEGY_MIN;
            adlak_dpm_stage_adjust(padlak, ADLAK_DPM_STRATEGY_MIN);
            adlak_os_mutex_lock(&padlak->dev_mutex);
            padlak->pm_suspend = false;
            adlak_os_mutex_unlock(&padlak->dev_mutex);
            adlak_os_sema_take(padlak->sem_pm_wakeup);
        }
#endif

        // check work queue
        do {
            adlak_os_sema_take_timeout(pwq->wk_update, 10);  // the CPU might be yielded here

            valid_num = adlak_get_valid_num(pwq, &net_id_invoke_previous, &pending_num);
            if (0 == valid_num) {
                if (0 == pending_num) {
#if CONFIG_ADLAK_DPM_EN
                    if (!forbid_dpm) {
                        // do dynamic power management
                        dpm_counter.finish          = adlak_os_ktime_get();
                        dpm_counter.time_elapsed_us = (uint32_t)adlak_os_ktime_us_delta(
                            dpm_counter.finish, dpm_counter.start);
                        if (dpm_counter.time_threshold <= dpm_counter.time_elapsed_us) {
                            // if (ADLAK_DPM_STRATEGY_MIN != dpm_stategy) {
                                dpm_stategy = ADLAK_DPM_STRATEGY_MIN;
                                adlak_dpm_stage_adjust(padlak, ADLAK_DPM_STRATEGY_MIN);
                            // }
                        }
                    }
#endif
                }
            } else {
                AML_LOG_INFO("valid_num is 0, but the pending num is %d", pending_num);
            }
            if (pthrd->thrd_should_stop) {
                break;
            }
        } while ((0 == valid_num) && (0 == pending_num));
        if (pthrd->thrd_should_stop) {
            break;
        }
        // do dpm
#if CONFIG_ADLAK_DPM_EN
        if (ADLAK_DPM_STRATEGY_MAX != dpm_stategy) {
            dpm_stategy = ADLAK_DPM_STRATEGY_MAX;
            adlak_dpm_stage_adjust(padlak, dpm_stategy);
        }
        dpm_counter.start = adlak_os_ktime_get();
#endif

        // get task
        ASSERT(NULL == ptask_sch_cur);
        ret = adlak_queue_schedule_update(padlak, &ptask_sch_cur, net_id_invoke_previous);
        ASSERT(ret == 0);
        if (ret < 0) {
            continue;
        }

        if (unlikely(!ptask_sch_cur)) {
            break;
        }
#ifdef CONFIG_ADLAK_DEBUG_INNNER
        adlak_dbg_inner_update(ptask_sch_cur->context, "schedule_start");
#endif
        if (reset_hw_enable) {
#ifdef CONFIG_ADLAK_DEBUG_INNNER
            adlak_dbg_inner_update(ptask_sch_cur->context, "device_reset");
#endif
            // reset device
            adlak_hal_device_reset(padlak);
            adlak_hal_device_start(padlak);
#ifdef CONFIG_ADLAK_DEBUG_INNNER
            adlak_dbg_inner_update(ptask_sch_cur->context, "device_reset_finished");
#endif
        }
        // submit task
        pwq->ptask_sch_cur = ptask_sch_cur;
        pwq->submit_num++;

#ifdef CONFIG_ADLAK_TEE
        if (NULL != ptask_sch_cur->context->ptee_model_attr) {
            adlak_hal_secure_entry(padlak);
            (void)adlak_submit_tee_task(ptask_sch_cur);
            net_id_invoke_previous = -1;
        } else
#endif
        {
            pmodel_attr =
                adlak_get_model_attr(ptask_sch_cur->context, ptask_sch_cur->sub_tasks_idx);
            (void)adlak_submit_patch_and_exec(ptask_sch_cur);
        }

        if (!ptask_sch_cur->blocking) {
            net_id_invoke_previous = -1;
            if (padlak->share_swap_en ||
                (ptask_sch_cur->ctrl_flags & (1 << ADLAK_INVOKE_CTRL_FLAGS_BIT_CONTEXT_HOLD))) {
                if (0 == (ptask_sch_cur->ctrl_flags &
                          (1 << ADLAK_INVOKE_CTRL_FLAGS_BIT_IS_LAST_PARTIAL))) {
                    net_id_invoke_previous = ptask_sch_cur->context->net_id;
                }
            }
            device_state = ADLAK_DEVICE_BUSY;
        } else {
            net_id_invoke_previous = -1;
            ptask_sch_cur->state   = ADLAK_SUBMIT_STATE_FINISHED;
        }

        // wait until finished
        if (!ptask_sch_cur->blocking) {
            if (ERR(NONE) != adlak_submit_wait(&pwq->dev_inference, ptask_sch_cur)) {
                AML_LOG_ERR("%s\n", "submit timeout");
                ptask_sch_cur->hw_stat_timeout = true;
            } else {
                AML_LOG_INFO("%s\n", "submit success");
            }
            adlak_irq_bottom_half(padlak, &device_state, ptask_sch_cur);
        } else {
            if (ptask_sch_cur->error_code == ADLAK_SUCCESS) {
                device_state = ADLAK_DEVICE_IDLE;
            } else {
                device_state = ADLAK_DEVICE_ERR;
            }
        }

        // report state
        adlak_os_mutex_lock(&pwq->wq_mutex);
        ret = adlak_queue_update_task_state(padlak, (struct adlak_task *)ptask_sch_cur);
        adlak_os_mutex_unlock(&pwq->wq_mutex);
        if (1 == ret) {
            adlak_to_umd_sinal_give(ptask_sch_cur->context->wait); /*give signal to umd*/
        }

        if (ptask_sch_cur->ctrl_flags & (1 << ADLAK_INVOKE_CTRL_FLAGS_BIT_RESET_AFTER_TASK) ||
            device_state == ADLAK_DEVICE_ERR) {
            // must reset hw
            reset_hw_enable = true;
        } else {
            reset_hw_enable = false;
        }
        if (!(ptask_sch_cur->ctrl_flags & (1 << ADLAK_INVOKE_CTRL_FLAGS_BIT_RESET_AFTER_TASK))) {
            forbid_dpm = true;
        } else {
            forbid_dpm = false;
        }
        ptask_sch_cur = NULL;
    }
    pthrd->thrd_should_stop = 0;

    if (pwq->dev_inference.dpm_timer) {
        adlak_os_timer_destroy(&pwq->dev_inference.dpm_timer);
    }
#ifndef CONFIG_ADLA_FREERTOS
    return 0;
#else
    return NULL;
#endif
}

#if defined(CONFIG_ADLAK_EMU_EN) && (CONFIG_ADLAK_EMU_EN == 1)

static void adlak_emu_irq_cb(adlak_os_timer_cb_t t) {
    struct adlak_dev_inference * pinference = NULL;
    struct adlak_task *          ptask      = NULL;
    struct adlak_workqueue *     pwq        = NULL;
    struct adlak_device *        padlak     = NULL;
    struct adlak_os_timer_inner *data       = container_of(t, struct adlak_os_timer_inner, timer);
    padlak                                  = (struct adlak_device *)(data->param);
    pwq                                     = &padlak->queue;
    AML_LOG_DEBUG("%s\n", __func__);
    adlak_cant_sleep();
    pinference     = &pwq->dev_inference;
    ptask          = (struct adlak_task *)(pwq->ptask_sch_cur);
    ptask->hw_stat = ADLAK_HAL_IRQ_STATE_NORMAL;

    adlak_os_sema_give_from_isr(pinference->sem_irq);
}
#endif

/**
 * @brief inference on adlak hardware
 *
 * @return int
 */
int adlak_dev_inference_init(struct adlak_device *padlak) {
    int ret;

    struct adlak_workqueue *pwq = &padlak->queue;

    struct adlak_dev_inference *pinference = &pwq->dev_inference;
    AML_LOG_DEBUG("%s\n", __func__);
    ret = adlak_os_mutex_lock(&pwq->wq_mutex);
    if (ret) {
        goto err;
    }

    ret = adlak_os_spinlock_init(&pinference->spinlock);
    ret = adlak_os_sema_init(&pinference->sem_irq, 1, 0);

#if defined(CONFIG_ADLAK_EMU_EN) && (CONFIG_ADLAK_EMU_EN == 1)
    ret = adlak_os_timer_init(&pinference->emu_timer, adlak_emu_irq_cb, padlak);
    if (ret) {
        AML_LOG_ERR("emu_timer init fail!\n");
    }
#endif
    ret =
        adlak_os_thread_create(&pinference->thrd_inference, adlak_dev_inference_cb, (void *)padlak);
    if (ret) {
        AML_LOG_ERR("Create inference thread fail!\n");
    }
    AML_LOG_INFO("Create inference thread success\n");
    ret = adlak_os_mutex_unlock(&pwq->wq_mutex);
    if (ret) {
        AML_LOG_ERR("wq_mutex unlock fail!\n");
    }
    return ret;
err:
    return ERR(EINTR);
}

static void adlak_dev_inference_finalize(void *args) {
    struct adlak_device *   padlak = args;
    struct adlak_workqueue *pwq    = &padlak->queue;
    adlak_os_sema_give(pwq->wk_update);
}

int adlak_dev_inference_deinit(struct adlak_device *padlak) {
    int                         ret;
    struct adlak_workqueue *    pwq        = &padlak->queue;
    struct adlak_dev_inference *pinference = &pwq->dev_inference;
    AML_LOG_DEBUG("%s\n", __func__);
    ret = adlak_os_thread_detach(&pinference->thrd_inference, adlak_dev_inference_finalize,
                                 (void *)padlak);
    if (ret) {
        AML_LOG_ERR("Detach inference thread fail!\n");
    }
    ret = adlak_os_mutex_lock(&pwq->wq_mutex);
    if (ret) {
        goto err;
    }
    if (pinference->emu_timer) {
        adlak_os_timer_destroy(&pinference->emu_timer);
    }
    if (pinference->sem_irq) {
        adlak_os_sema_destroy(&pinference->sem_irq);
    }
    if (pinference->spinlock) {
        adlak_os_spinlock_destroy(&pinference->spinlock);
    }
    adlak_os_mutex_unlock(&pwq->wq_mutex);
err:
    return 0;
}

static void adlak_irq_bottom_half(struct adlak_device *padlak, int *device_state,
                                  struct adlak_task *ptask) {
    struct adlak_model_attr *pmodel_attr = NULL;
    AML_LOG_INFO("%s", __func__);

    adlak_os_mutex_lock(&padlak->dev_mutex);

    pmodel_attr = adlak_get_model_attr(ptask->context, ptask->sub_tasks_idx);
    //    phw_stat    = &ptask->hw_stat;
    adlak_profile_stop(padlak, &pmodel_attr->pm_cfg, &pmodel_attr->pm_stat, &ptask->profilling,
                       ptask->layer_end);

    //    phw_info = phw_stat->hw_info;
    adlak_hal_dbg_dump_status_regs(padlak);

    ptask->state = ADLAK_SUBMIT_STATE_FAIL;
    if (ADLAK_HAL_IRQ_STATE_NORMAL == ptask->hw_stat) {
        *device_state     = ADLAK_DEVICE_IDLE;
        ptask->state      = ADLAK_SUBMIT_STATE_FINISHED;
        ptask->error_code = ADLAK_SUCCESS;
    } else {
        *device_state = ADLAK_DEVICE_ERR;

        if (ADLAK_HAL_IRQ_STATE_ERROR == ptask->hw_stat) {
            if (ptask->hw_stat_timeout) {
                ptask->error_code = ADLAK_SOFTWARE_TIMEOUT;
            } else {
                ptask->error_code = ADLAK_HARDWARE_TIMEOUT;
            }
        } else {
            if (CONTEXT_STATE_CLOSED != ptask->context->state) {
                AML_LOG_ERR("invoke_idx %d(%d~%d): Not support status %d!", ptask->invoke_idx,
                            ptask->layer_start, ptask->layer_end, ptask->hw_stat);
                ASSERT(0);
            } else {
                AML_LOG_WARN("Not support status[0x%08X],and the context has been closed!",
                             ptask->hw_stat);
            }
        }
    }
    adlak_os_mutex_unlock(&padlak->dev_mutex);
}
