/*******************************************************************************
 * Copyright (C) 2021 Amlogic, Inc. All rights reserved.
 ******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file adlak_platform_config.c
 * @brief
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   	Who				Date				Changes
 * ----------------------------------------------------------------------------
 * 1.00a shiwei.sun@amlogic.com	2021/06/05	Initial release
 * </pre>
 *
 ******************************************************************************/

/***************************** Include Files *********************************/
#include "adlak_platform_config.h"

#include "adlak_common.h"
#include "adlak_device.h"
#include "adlak_dpm.h"
#include "adlak_hal.h"
#include "adlak_interrupt.h"
#include "adlak_submit.h"

/************************** Constant Definitions *****************************/
#if !defined(CONFIG_ADLA_FREERTOS)
#ifndef CONFIG_OF
#error "Build failed: CONFIG_OF is required in linux OS"
#endif
#endif

// static int adlak_has_smmu = -1;

static int adlak_axi_freq = 800000000;

static int adlak_core_freq = 800000000;

static int adlak_sch_time_max_ms = 1000;

static int adlak_dpm_period = 300;

static int adlak_log_level = -1;

static uint adlak_share_swap = 0;

static uint adlak_share_buf_size = 0;

static uint adlak_smmu_iova_size = 4;

//#include "./adlak_platform_module_param.c"
/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Variable Definitions *****************************/

/************************** Function Prototypes ******************************/

int adlak_platform_device_init(void) {
    int ret = 0;
    AML_LOG_DEBUG("%s", __func__);
    return ret;
}

int adlak_platform_device_uninit(void) {
    int ret = 0;
    AML_LOG_DEBUG("%s", __func__);

    return ret;
}
static bool adlak_smmu_available(struct device *dev) {
    bool has_smmu = false;
#ifdef CONFIG_OF
    if (of_property_read_bool(dev->of_node, "smmu")) {
        has_smmu = true;
    }
#else
    if (1 == adlak_has_smmu) {
        has_smmu = true;
    }
#endif
    return has_smmu;
}

static int adlak_get_driver_vesion(struct device *dev, uint32_t *_ver) {
    int      ret = 0;
    uint32_t ver = ADLAK_HW_VER_DEFAULT;
#ifdef CONFIG_OF
    const char *compat_str;
    // Read compatible string from device tree
    if (of_property_read_string(dev->of_node, "compatible", &compat_str)) {
        AML_LOG_ERR("Missing compatible property\n");
        ret = ERR(ENODEV);
        goto err;
    }
    AML_LOG_INFO("driver compat_str %s\n", compat_str);
    if (strstr(compat_str, "gen3b")) {
        ver = ADLAK_HW_VER_GEN3B;
    } else if (strstr(compat_str, "gen2b")) {
        ver = ADLAK_HW_VER_GEN2B;
    } else {
        ver = ADLAK_HW_VER_DEFAULT;
    }
    AML_LOG_INFO("driver version %d\n", ver);
#endif
    if (_ver) {
        *_ver = ver;
    }
err:
    return ret;
}

int adlak_platform_get_resource(void *data) {
    int                  ret    = 0;
    struct resource *    res    = NULL;
    struct adlak_device *padlak = (struct adlak_device *)data;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
    int adla_irq = -1;
#endif
    AML_LOG_DEBUG("%s", __func__);

    padlak->smmu_en = adlak_smmu_available(padlak->dev);

    if (padlak->smmu_en) {
        AML_LOG_INFO("smmu available.\n");
    } else {
        AML_LOG_INFO("smmu not available.\n");
    }

    ret = adlak_get_driver_vesion(padlak->dev, &padlak->driver_ver);
    if (ret) {
        goto err;
    }

    /* get ADLAK IO */

    res = platform_get_resource_byname(padlak->pdev, IORESOURCE_MEM, "adla_reg");
    if (!res) {
        AML_LOG_ERR("get platform io region failed");
        ret = ERR(EINVAL);
        goto err;
    }
    AML_LOG_DEBUG("get ADLAK IO region: [0x%lX, 0x%lX]", (uintptr_t)res->start,
                  (uintptr_t)res->end);

    padlak->hw_res.adlak_reg_pa   = res->start;
    padlak->hw_res.adlak_reg_size = res->end - res->start + 1;

    res = platform_get_resource_byname(padlak->pdev, IORESOURCE_MEM, "adla_sram");
    if (!res) {
        AML_LOG_INFO("get platform sram region failed");
        padlak->hw_res.adlak_sram_pa   = 0;
        padlak->hw_res.adlak_sram_size = 0;
    } else {
        AML_LOG_DEBUG("get ADLAK SRAM region: [0x%lX, 0x%lX]", (uintptr_t)res->start,
                      (uintptr_t)res->end);
        padlak->hw_res.adlak_sram_pa   = res->start;
        padlak->hw_res.adlak_sram_size = res->end - res->start + 1;
    }
    padlak->hw_res.sram_wrap = 1;  // this configure must sync with adla-compiler,the default is
                                   // wrap enable in adla-compiler.

    /* get reserve-memory */

    res = platform_get_resource_byname(padlak->pdev, IORESOURCE_MEM, "adla_reserved_memory");
    if (!res) {
        AML_LOG_INFO("get platform reserved_memory region failed");
        padlak->hw_res.adlak_resmem_pa   = 0;
        padlak->hw_res.adlak_resmem_size = 0;
    } else {
        AML_LOG_DEBUG("get ADLA reserved_memory region: [0x%lX, 0x%lX]", (uintptr_t)res->start,
                      (uintptr_t)res->end);
        padlak->hw_res.adlak_resmem_pa   = res->start;
        padlak->hw_res.adlak_resmem_size = res->end - res->start + 1;
    }

/* get interrupt number */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
    adla_irq = platform_get_irq_byname(padlak->pdev, "adla");
    if (adla_irq <= 0) {
        AML_LOG_ERR("get irqnum failed");
        ret = ERR(EINVAL);
        goto err;
    }
    padlak->hw_res.irqline = adla_irq;
#else
    res = platform_get_resource_byname(padlak->pdev, IORESOURCE_IRQ, "adla");
    if (!res) {
        AML_LOG_ERR("get irqnum failed");
        ret = ERR(EINVAL);
        goto err;
    }
    padlak->hw_res.irqline = res->start;
#endif
    AML_LOG_DEBUG("get IRQ number: %d", padlak->hw_res.irqline);

    padlak->hw_timeout_ms = (adlak_sch_time_max_ms);
    AML_LOG_DEBUG("padlak->hw_timeout_ms =  %d ms", adlak_sch_time_max_ms);

#if 0
    padlak->clk_axi = devm_clk_get(padlak->dev, "adla_axi_clk");
    if (IS_ERR(padlak->clk_axi)) {
        AML_LOG_WARN("Failed to get adla_axi_clk\n");
    }
#else
    padlak->clk_axi        = NULL;
#endif
    padlak->clk_core = devm_clk_get(padlak->dev, "adla_core_clk");
    if (IS_ERR(padlak->clk_core)) {
        AML_LOG_ERR("Failed to get adla_core_clk\n");
    }
#ifdef CONFIG_OF
    // update  the core clock if defined in device tree
    of_property_read_s32(padlak->dev->of_node, "adla_core_clk_rate", &adlak_core_freq);
#endif
    padlak->clk_axi_freq_set  = adlak_axi_freq;
    padlak->clk_core_freq_set = adlak_core_freq;
    padlak->dpm_period_set    = adlak_dpm_period;

    if (adlak_log_level != -1) {
        g_adlak_log_level = adlak_log_level;
#if ADLAK_DEBUG
        g_adlak_log_level_pre = g_adlak_log_level;
#endif
    }

    padlak->share_swap_en  = 0;
    padlak->share_buf_size = 0;
    if (adlak_share_swap > 0) {
        padlak->share_swap_en  = 1;
        padlak->share_buf_size = adlak_share_buf_size;
    }

    padlak->iova_max_size_GB = adlak_smmu_iova_size;
#ifdef CONFIG_OF
    // update the smmu iova size if defined in device tree, the unit is GiB
    of_property_read_s32(padlak->dev->of_node, "smmu_iova_size", &padlak->iova_max_size_GB);
#endif
    if (padlak->driver_ver == ADLAK_HW_VER_DEFAULT) {
        if (padlak->iova_max_size_GB > 4) {
            padlak->iova_max_size_GB = 4;
        }
    }

    return 0;
err:
    return ret;
}

int adlak_platform_get_rsv_mem_size(void *dev, uint64_t *mem_size) {
    uint64_t size = 0;
#ifdef CONFIG_OF
    int                 ret = 0;
    struct resource     res;
    const __be32 *      ranges = NULL;
    int                 nsize;
    struct device_node *res_mem_dev;

    /* find a memory-region phandle */
    res_mem_dev = of_parse_phandle(((struct device *)dev)->of_node, "memory-region", 0);
    if (!res_mem_dev) {
        goto err;
    }
    ret = of_address_to_resource(res_mem_dev, 0, &res);
    if (!ret) {
        AML_LOG_DEBUG("get cma memory region: [0x%lX, 0x%lX]", (uintptr_t)res.start,
                      (uintptr_t)res.end);
        size = res.end - res.start + 1;
    } else {
        nsize  = of_n_size_cells(res_mem_dev);
        ranges = of_get_property(res_mem_dev, "size", NULL);
        if (!ranges) {
            AML_LOG_ERR("get cma size failed!\n");
            goto err;
        }
        size = of_read_number(ranges, nsize);
    }
#endif
    AML_LOG_DEBUG("get cma size=0x%lX", (uintptr_t)size);
    *mem_size = size;
    return 0;
err:
    return -1;
}

int adlak_platform_request_resource(void *data) {
    int                  ret    = 0;
    struct adlak_device *padlak = (struct adlak_device *)data;

    AML_LOG_DEBUG("%s", __func__);
    padlak->hw_res.preg = adlak_create_ioregion((uintptr_t)padlak->hw_res.adlak_reg_pa,
                                                padlak->hw_res.adlak_reg_size);
    if (NULL == padlak->hw_res.preg) {
        AML_LOG_ERR("create ioregion failed");
        ret = ERR(EINVAL);
        goto err;
    }

    return 0;
err:
    return ret;
}

int adlak_platform_free_resource(void *data) {
    int                  ret    = 0;
    struct adlak_device *padlak = (struct adlak_device *)data;

    if (padlak->hw_res.preg) {
        adlak_destroy_ioregion(padlak->hw_res.preg);
    }
    return ret;
}

void adlak_platform_set_clock(void *data, bool enable, int core_freq, int axi_freq) {
    int                  ret    = 0;
    struct adlak_device *padlak = (struct adlak_device *)data;
    AML_LOG_DEBUG("%s", __func__);

    if (false == enable) {
        if (true == padlak->is_clk_axi_enabled) {
            if (!ADLAK_IS_ERR_OR_NULL(padlak->clk_axi)) {
                clk_disable_unprepare(padlak->clk_axi);
            }
            padlak->is_clk_axi_enabled = false;
        }
        if (true == padlak->is_clk_core_enabled) {
            if (!ADLAK_IS_ERR_OR_NULL(padlak->clk_core)) {
                clk_disable_unprepare(padlak->clk_core);
            }
            padlak->is_clk_core_enabled = false;

            /*
                if adla clk has multi parent clk (T7c),
                switch adla clk parent to clk_parent1 when adla clk off;
            */
            if (!ADLAK_IS_ERR_OR_NULL(padlak->clk_parent1) &&
                !ADLAK_IS_ERR_OR_NULL(padlak->clk)) {
                clk_set_parent(padlak->clk, padlak->clk_parent1);
                AML_LOG_WARN("clk_set_parent to parent 1\n");
            }

        }
        padlak->clk_core_freq_real = 0;

    } else {
        // clk enable
        if (false == padlak->is_clk_axi_enabled) {
            if (!ADLAK_IS_ERR_OR_NULL(padlak->clk_axi)) {
                ret = clk_prepare_enable(padlak->clk_axi);
                if (ret) {
                    AML_LOG_ERR("Failed to enable adla_axi_clk\n");
                }
            }
            padlak->is_clk_axi_enabled = true;
        }
        if (false == padlak->is_clk_core_enabled) {
            /*
                if adla clk has multi parent clk(T7c),
                switch adla clk parent to clk_parent0 when adla clk on;
            */
            if (!ADLAK_IS_ERR_OR_NULL(padlak->clk_parent0) &&
                !ADLAK_IS_ERR_OR_NULL(padlak->clk)) {
                clk_set_parent(padlak->clk,padlak->clk_parent0);
                AML_LOG_WARN("clk_set_parent to parent 0\n");
            }

            if (!ADLAK_IS_ERR_OR_NULL(padlak->clk_core)) {
                ret = clk_prepare_enable(padlak->clk_core);
                if (ret) {
                    AML_LOG_ERR("Failed to enable adla_core_clk\n");
                }
                padlak->is_clk_core_enabled = true;
            }
        }
        if (!ADLAK_IS_ERR_OR_NULL(padlak->clk_axi)) {
            clk_set_rate(padlak->clk_axi, axi_freq);
            if (ret) {
                AML_LOG_ERR("Failed to set adla_axi_clk\n");
            }
            padlak->clk_axi_freq_real = (int)clk_get_rate(padlak->clk_axi);
            adlak_os_printf("adlak_axi clk requirement of %d Hz,and real val is %d Hz.", axi_freq,
                            padlak->clk_axi_freq_real);
        }
        if (!ADLAK_IS_ERR_OR_NULL(padlak->clk_core)) {
            ret = clk_set_rate(padlak->clk_core, core_freq);
            if (ret) {
                AML_LOG_ERR("Failed to set adla_core_clk\n");
            }
            padlak->clk_core_freq_real = (int)clk_get_rate(padlak->clk_core);

            AML_LOG_DEBUG("adlak_core clk requirement of %d Hz,and real val is %d Hz.", core_freq,
                            padlak->clk_core_freq_real);
        }
    }
    adlak_dpm_clk_update(padlak, core_freq, axi_freq);
}

void adlak_platform_set_power(void *data, bool enable) {
#if CONFIG_HAS_PM_DOMAIN
    int                  ret    = 0;
    struct adlak_device *padlak = (struct adlak_device *)data;
#endif
    AML_LOG_DEBUG("%s", __func__);
    if (false == enable) {
        AML_LOG_WARN("adla power off\n");
#if CONFIG_HAS_PM_DOMAIN
        pm_runtime_put_sync(padlak->dev);
        if (pm_runtime_enabled(padlak->dev)) {
            pm_runtime_disable(padlak->dev);
        }
#endif

    } else {
        AML_LOG_WARN("adla power on\n");
#if CONFIG_HAS_PM_DOMAIN
        pm_runtime_enable(padlak->dev);
        if (pm_runtime_enabled(padlak->dev)) {
            ret = pm_runtime_get_sync(padlak->dev);
            if (ret < 0) {
                AML_LOG_ERR("Getpower failed\n");
            }
        }
#endif
    }
}

int adlak_platform_pm_init(void *data) {
    int                  ret    = 0;
    struct adlak_device *padlak = (struct adlak_device *)data;
    AML_LOG_DEBUG("%s", __func__);
#if CONFIG_HAS_PM_DOMAIN
    ret = pm_runtime_set_active(padlak->dev);
#endif
    if (ret < 0) {
        AML_LOG_ERR("Get power failed\n");
        goto end;
    }

    adlak_os_sema_init(&padlak->sem_pm_wakeup, 1, 0);
    padlak->pm_suspend = false;
    // power on
    adlak_platform_set_power(padlak, true);
    // clk enable
    adlak_platform_set_clock(padlak, true, padlak->clk_core_freq_set, padlak->clk_axi_freq_set);
    padlak->is_suspend = false;
end:
    return ret;
}
void adlak_platform_pm_deinit(void *data) {
    struct adlak_device *padlak = (struct adlak_device *)data;
    AML_LOG_DEBUG("%s", __func__);
    // clk disable
    adlak_platform_set_clock(padlak, false, 0, 0);
    // power off
    adlak_platform_set_power(padlak, false);
    padlak->is_suspend = true;
    adlak_os_sema_destroy(&padlak->sem_pm_wakeup);
}

void adlak_platform_resume(void *data) {
#if CONFIG_ADLAK_DPM_EN
    struct adlak_device *padlak = (struct adlak_device *)data;
    AML_LOG_INFO("%s", __func__);
    if (false != padlak->is_suspend) {
        // power on
        adlak_platform_set_power(padlak, true);
        // clk enable
        adlak_platform_set_clock(padlak, true, padlak->clk_core_freq_set, padlak->clk_axi_freq_set);
        padlak->is_suspend = false;
        adlak_hal_device_resume(padlak);
    }
#endif
}

void adlak_platform_suspend(void *data) {
#if CONFIG_ADLAK_DPM_EN
    struct adlak_device *padlak = (struct adlak_device *)data;
    AML_LOG_INFO("%s", __func__);
    if (false == padlak->is_suspend) {
        adlak_hal_device_suspend(padlak);
        padlak->is_suspend = true;
        // clk disable
        adlak_platform_set_clock(padlak, false, 0, 0);
        // power off
        adlak_platform_set_power(padlak, false);
    }
#endif
}
