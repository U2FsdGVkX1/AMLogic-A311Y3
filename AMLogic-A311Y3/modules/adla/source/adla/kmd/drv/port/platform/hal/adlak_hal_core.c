/**
 * Copyright: (C) 2025 Amlogic, Inc. All rights reserved.
 */

/**
 * @file adlak_hal_core.c
 * @brief
 *
 * @author: shiwei.sun
 * Created: 2025-08-12 23:22:11
 */

/***************************** Include Files *********************************/

#include "adlak_hal.h"
#include "adlak_mm.h"

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/
struct adlak_hal {
    adlak_os_mutex_t       mutex;
    struct adlak_device *  padlak;
    struct adlak_hw_ops *  active_ops;
    struct adlak_caps_desc uapi_caps_data;
    void *                 hw_obj;
};

/************* Global Variable Definitions  (avoid if possible) **************/

/**************************  Function Prototypes *****************************/

static void inline adlak_hal_claim_dev(struct adlak_hal *hal) {
    AML_LOG_DEBUG("%s[+]", __func__);
    adlak_os_mutex_lock(&hal->mutex);
    AML_LOG_DEBUG("%s[-]", __func__);
}
static void inline adlak_hal_release_dev(struct adlak_hal *hal) {
    AML_LOG_DEBUG("%s[+]", __func__);
    adlak_os_mutex_unlock(&hal->mutex);
    AML_LOG_DEBUG("%s[-]", __func__);
}

int adlak_hal_device_reset(void *data) {
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal    = padlak->hal;
    AML_LOG_INFO("%s", __func__);
    adlak_hal_claim_dev(hal);
    if (hal->active_ops->device_reset) {
        hal->active_ops->device_reset(hal->hw_obj);
    }
    adlak_hal_release_dev(hal);
    return 0;
}

int adlak_hal_device_start(void *data) {
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal    = padlak->hal;
    AML_LOG_DEBUG("%s", __func__);
    adlak_hal_claim_dev(hal);
    if (hal->active_ops->device_start) {
        hal->active_ops->device_start(hal->hw_obj);
    }
    adlak_hal_release_dev(hal);
    return 0;
}

int adlak_hal_device_stop(void *data) {
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal    = padlak->hal;
    AML_LOG_DEBUG("%s", __func__);
    adlak_hal_claim_dev(hal);
    if (hal->active_ops->device_stop) {
        hal->active_ops->device_stop(hal->hw_obj);
    }
    adlak_hal_release_dev(hal);
    return 0;
}

int adlak_hal_device_suspend(void *data) {
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal    = padlak->hal;
    AML_LOG_DEBUG("%s", __func__);
    adlak_hal_claim_dev(hal);
    if (hal->active_ops->device_suspend) {
        hal->active_ops->device_suspend(hal->hw_obj);
    }
    adlak_hal_release_dev(hal);
    return 0;
}

int adlak_hal_device_resume(void *data) {
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal    = padlak->hal;
    AML_LOG_DEBUG("%s", __func__);
    adlak_hal_claim_dev(hal);
    if (hal->active_ops->device_resume) {
        hal->active_ops->device_resume(hal->hw_obj);
    }
    adlak_hal_release_dev(hal);
    return 0;
}

int adlak_hal_wait_device_idle(void *data) {
    int                  ret    = 0;
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal    = padlak->hal;
    AML_LOG_DEBUG("%s", __func__);
    adlak_hal_claim_dev(hal);
    if (hal->active_ops->wait_device_idle) {
        ret = hal->active_ops->wait_device_idle(hal->hw_obj);
    }
    adlak_hal_release_dev(hal);
    return ret;
}

int adlak_hal_parser_submit(void *data, uint32_t base_addr, uint32_t size, uint32_t rpt,
                            uint32_t wpt) {
    int                  ret    = 0;
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal    = padlak->hal;
    AML_LOG_DEBUG("%s", __func__);
    adlak_hal_claim_dev(hal);
    if (hal->active_ops->parser_submit) {
        ret = hal->active_ops->parser_submit(hal->hw_obj, base_addr, size, rpt, wpt);
    }
    adlak_hal_release_dev(hal);
    return ret;
}

int adlak_hal_smmu_config(void *data, bool en, uint64_t entry) {
    int                  ret    = 0;
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal    = padlak->hal;
    AML_LOG_DEBUG("%s", __func__);
    adlak_hal_claim_dev(hal);
    if (hal->active_ops->smmu_config) {
        ret = hal->active_ops->smmu_config(hal->hw_obj, en, entry);
    }
    adlak_hal_release_dev(hal);
    return ret;
}

int adlak_hal_get_irq_status(void *data, uint32_t *extra_status) {
    int                  ret    = ADLAK_HAL_IRQ_STATE_NULL;
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal    = padlak->hal;
    // this func will be call in the spin lock
    if (hal->active_ops->get_irq_status) {
        ret = hal->active_ops->get_irq_status(hal->hw_obj, extra_status);
    }
    return ret;
}

int adlak_hal_pm_start(void *data, uint32_t base_addr, uint32_t size, uint32_t rpt, uint32_t wpt) {
    int                  ret    = 0;
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal    = padlak->hal;
    AML_LOG_DEBUG("%s", __func__);
    adlak_hal_claim_dev(hal);
    if (hal->active_ops->pm_start) {
        ret = hal->active_ops->pm_start(hal->hw_obj, base_addr, size, rpt, wpt);
    }
    adlak_hal_release_dev(hal);
    return ret;
}

int adlak_hal_pm_stop(void *data) {
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal    = padlak->hal;
    AML_LOG_DEBUG("%s", __func__);
    adlak_hal_claim_dev(hal);
    if (hal->active_ops->pm_stop) {
        hal->active_ops->pm_stop(hal->hw_obj);
    }
    adlak_hal_release_dev(hal);
    return 0;
}

void adlak_hal_dbg_dump_status_regs(void *data) {
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal    = padlak->hal;
    AML_LOG_DEBUG("%s", __func__);
    adlak_hal_claim_dev(hal);
    if (hal->active_ops->dbg_dump_status_regs) {
        hal->active_ops->dbg_dump_status_regs(hal->hw_obj);
    }
    adlak_hal_release_dev(hal);
}

int adlak_hal_dbg_dump_reg(void *data, uint32_t offset) {
    int                  ret    = 0;
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal    = padlak->hal;
    if (hal->active_ops->dbg_dump_reg) {
        ret = hal->active_ops->dbg_dump_reg(hal->hw_obj, offset);
    }
    return ret;
}

int adlak_hal_dbg_write_reg(void *data, uint32_t offset, uint32_t value) {
    int                  ret    = 0;
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal    = padlak->hal;
    if (hal->active_ops->dbg_write_reg) {
        ret = hal->active_ops->dbg_write_reg(hal->hw_obj, offset, value);
    }
    return ret;
}

void adlak_hal_dbg_dump_all_regs(void *data) {
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal    = padlak->hal;
    AML_LOG_DEBUG("%s", __func__);
    adlak_hal_claim_dev(hal);
    if (hal->active_ops->dbg_dump_all_regs) {
        hal->active_ops->dbg_dump_all_regs(hal->hw_obj);
    }
    adlak_hal_release_dev(hal);
}

void adlak_hal_dbg_dump_extern(void *data, void *context) {
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal    = padlak->hal;
    AML_LOG_DEBUG("%s", __func__);
    adlak_hal_claim_dev(hal);
    if (hal->active_ops->dbg_dump_extern) {
        hal->active_ops->dbg_dump_extern(hal->hw_obj, context);
    }
    adlak_hal_release_dev(hal);
}

void adlak_hal_secure_entry(void *data) {
    struct adlak_device *    padlak  = (struct adlak_device *)data;
    struct adlak_hal *       hal     = padlak->hal;
    struct adlak_mem_handle *cmq_mem = padlak->public_cmq_mem;

    AML_LOG_DEBUG("%s", __func__);
    adlak_hal_claim_dev(hal);
    if (hal->active_ops->secure_entry) {
        if (cmq_mem && !cmq_mem->cpu_addr && padlak->mem_ctx) {
            (void)adlak_mem_vmap(padlak, cmq_mem);
        }
        if (cmq_mem && cmq_mem->cpu_addr) {
            hal->active_ops->secure_entry(hal->hw_obj, cmq_mem);
        } else {
            AML_LOG_ERR("%s: public cmq missing or no kernel mapping", __func__);
        }
    }
    adlak_hal_release_dev(hal);
}

static void adlak_hw_caps_update(struct adlak_hal *hal) {
    struct adlak_caps_desc *uapi_caps = &hal->uapi_caps_data;
    uapi_caps->hw_ver                 = hal->active_ops->hw_ver.all;
    uapi_caps->core_freq_max          = hal->padlak->clk_core_freq_set;
    uapi_caps->axi_freq_max           = hal->padlak->clk_axi_freq_set;
    uapi_caps->cmq_size               = 0xFFFFFFFF;  // deprecated
    uapi_caps->sram_base              = hal->padlak->hw_res.adlak_sram_pa;
    uapi_caps->sram_size              = hal->padlak->hw_res.adlak_sram_size;
    uapi_caps->hw_iova_max_size       = 0;
    uapi_caps->iova_max_size          = 0;
    uapi_caps->iova_free_size         = 0;
}

int adlak_hal_init(void *data) {
    int                  ret    = 0;
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal;

    AML_LOG_INFO("%s", __func__);
    if (!padlak->hal) {
        padlak->hal = adlak_os_zalloc(sizeof(struct adlak_hal), ADLAK_GFP_KERNEL);
        if (unlikely(!padlak->hal)) {
            ret = ERR(ENOMEM);
            goto end;
        }
    }
    hal = padlak->hal;
    adlak_os_mutex_init(&hal->mutex);

    adlak_hal_claim_dev(hal);
    hal->padlak = padlak;

    padlak->dev_caps.data = &hal->uapi_caps_data;
    padlak->dev_caps.size = sizeof(hal->uapi_caps_data);
    if (padlak->driver_ver == ADLAK_HW_VER_GEN3B || padlak->driver_ver == ADLAK_HW_VER_GEN2B) {
        hal->active_ops = &gen3b_ops;
    } else if (padlak->driver_ver == ADLAK_HW_VER_GEN2) {
        hal->active_ops = &gen2_ops;
    } else {
        hal->active_ops = &emu_ops;
    }

#if defined(CONFIG_ADLAK_EMU_EN) && (CONFIG_ADLAK_EMU_EN == 1)
    hal->active_ops = &emu_ops;
#endif
    // Initialize selected hardware
    if (hal->active_ops->init) {
        ret = hal->active_ops->init(&hal->hw_obj, padlak->hw_res.preg, padlak->hw_res.adlak_sram_pa,
                                    padlak->hw_res.adlak_sram_size);
        if (ret) {
            hal->active_ops = NULL;
            ret             = ERR(ENODEV);
            goto unlock;
        }
    }

    adlak_hw_caps_update(hal);
unlock:

    adlak_hal_release_dev(hal);
end:
    return ret;
}

void adlak_hal_deinit(void *data) {
    struct adlak_device *padlak = (struct adlak_device *)data;
    struct adlak_hal *   hal    = padlak->hal;
    AML_LOG_INFO("%s", __func__);
    if (hal) {
        adlak_hal_claim_dev(hal);
        if (likely(hal->active_ops && hal->active_ops->deinit)) {
            hal->active_ops->deinit(&hal->hw_obj);
        }

        adlak_hal_release_dev(hal);
        adlak_os_mutex_destroy(&hal->mutex);
        hal->active_ops = NULL;

        adlak_os_free(hal);
        hal = NULL;
    }
}
