/**
 * Copyright: (C) 2025 Amlogic, Inc. All rights reserved.
 */

/**
 * @file adlak_hal_gen3b.c
 * @brief
 *
 * @author: shiwei.sun
 * Created: 2025-08-15 15:35:47
 */

/***************************** Include Files *********************************/

#include "adlak_hal.h"
#include "adlak_io.h"
#include "adlak_reg_gen3b_ps_core.h"
#include "adlak_reg_gen3b_ps_master.h"
/***************** Macros (Inline Functions) Definitions *********************/

// master irq mask
#define ADLAK_MASTER_IRQ_MASK_PARSER_STOP_CMD (1 << 0)      /* [0]: parser stop for command*/
#define ADLAK_MASTER_IRQ_MASK_PARSER_STOP_ERR (1 << 1)      /* [1]: parser stop for error*/
#define ADLAK_MASTER_IRQ_MASK_PARSER_STOP_PMT (1 << 2)      /* [2]: parser stop for preempt*/
#define ADLAK_MASTER_IRQ_MASK_PEND_TIMEOUT (1 << 3)         /* [3]: pending timer timeout*/
#define ADLAK_MASTER_IRQ_MASK_GROUP_END (1 << 4)            /* [4]: group end event*/
#define ADLAK_MASTER_IRQ_MASK_TIM_STAMP (1 << 5)            /* [5]: time_stamp irq event*/
#define ADLAK_MASTER_IRQ_MASK_APB_WAIT_TIMEOUT (1 << 6)     /* [6]: apb wait timer timeout*/
#define ADLAK_MASTER_IRQ_MASK_PARSER_STOP_FOR_STEP (1 << 7) /* [7]: parser stop for single step */
#define ADLAK_MASTER_IRQ_MASK_PARSER_STOP_FOR_LOOP \
    (1 << 8) /* [8]: parser stop for loop under/over-flow */
#define ADLAK_MASTER_IRQ_MASK_ISP_LINK_OVERFLOW (1 << 9) /* [9]: isp link overflow */

// core irq mask
#define ADLAK_CORE_IRQ_MASK_PARSER_STOP_ERR (1 << 1)       /* [1]: parser stop for error*/
#define ADLAK_CORE_IRQ_MASK_PEND_TIMEOUT (1 << 3)          /* [3]: pending timer timeout*/
#define ADLAK_CORE_IRQ_MASK_LAYER_END (1 << 4)             /* [4]: layer end event*/
#define ADLAK_CORE_IRQ_MASK_TIM_STAMP (1 << 5)             /* [5]: time_stamp irq event*/
#define ADLAK_CORE_IRQ_MASK_APB_WAIT_TIMEOUT (1 << 6)      /* [6]: apb wait timer timeout*/
#define ADLAK_CORE_IRQ_MASK_PM_DRAM_OVF (1 << 7)           /* [7]: pm dram overflow*/
#define ADLAK_CORE_IRQ_MASK_PM_FIFO_OVF (1 << 8)           /* [8]: pm fifo overflow*/
#define ADLAK_CORE_IRQ_MASK_PM_ARBITER_OVF (1 << 9)        /* [9]: pm arbiter overflow*/
#define ADLAK_CORE_IRQ_MASK_INVALID_IOVA (1 << 10)         /* [10]: smmu has an invalid-va*/
#define ADLAK_CORE_IRQ_MASK_AB_RESPONSE_ERR (1 << 11)      /* [11]: ab has response error*/
#define ADLAK_CORE_IRQ_MASK_PARSER_STOP_FOR_STEP (1 << 12) /* [12]: parser stop for single step*/
#define ADLAK_CORE_IRQ_MASK_PARSER_MODIF_CMQ_ERR \
    (1 << 13) /* [13]: parser modified queue command error*/
#define ADLAK_CORE_IRQ_MASK_PARSER_NORMAL_QUEUE_UNDERFLOW \
    (1 << 14) /* [14]: ps normal queue underflow*/
#define ADLAK_CORE_IRQ_MASK_PARSER_MODIFY_QUEUE_UNDERFLOW \
    (1 << 15) /* [15]: ps modified queue underflow*/

/************************** Constant Definitions *****************************/

/**************************** Type Definitions *******************************/

struct adlak_gen3b_irq_status {
    uint32_t irq_masked;
    uint32_t irq_raw;
    uint32_t time_stamp;
};

struct adlak_gen3b_hw_stat {
    uint32_t status_report;
    uint32_t ps_module_stat;
    uint32_t ps_err_dat;
};

struct adlak_gen3b_irq_cfg {
    uint32_t mask;
    uint32_t mask_err;
    uint32_t mask_normal;
};

struct adlak_gen3b_hw_obj {
    uint32_t                      sram_addr;
    uint32_t                      sram_size;
    uint32_t                      expect_time_stamp;
    struct adlak_gen3b_irq_cfg    core_irq_cfg;
    struct adlak_gen3b_irq_cfg    master_irq_cfg;
    struct adlak_gen3b_irq_status core_irq_status;
    struct adlak_gen3b_irq_status master_irq_status;
    struct adlak_gen3b_hw_stat    core_hw_stat;
    struct io_region *            region;
};
/************* Global Variable Definitions  (avoid if possible) **************/

/**************************  Function Prototypes *****************************/

/* Device operations */
static void adlak_gen3b_set_autoclock(void *data, uint32_t en);
static int  adlak_gen3b_device_reset(void *data);
static int  adlak_gen3b_device_start(void *data);
static int  adlak_gen3b_device_stop(void *data);
static void adlak_gen3b_parser_set_pend_timer(struct adlak_gen3b_hw_obj *hw_obj, uint32_t time);
static void adlak_gen3b_parser_set_apb_timeout(struct adlak_gen3b_hw_obj *hw_obj, uint32_t time);

static int adlak_gen3b_wait_device_idle(void *data);

/* progress parser */
static int adlak_gen3b_parser_submit(void *data, uint32_t base_addr, uint32_t size, uint32_t rpt,
                                     uint32_t wpt);
/* progress smmu */
static int adlak_gen3b_smmu_config(void *data, bool en, uint64_t entry);
static int adlak_gen3b_smmu_refresh(void *data);

/* progress irq */
static int  adlak_gen3b_get_irq_status(void *data, uint32_t *extra_status);
static void adlak_gen3b_clear_all_irqs(struct adlak_gen3b_hw_obj *hw_obj);
static void adlak_gen3b_irq_enable(struct adlak_gen3b_hw_obj *hw_obj);
static void adlak_gen3b_irq_disable(struct adlak_gen3b_hw_obj *hw_obj);

/* pm control*/
static int adlak_gen3b_pm_start(void *data, uint64_t profile_iova, uint32_t profile_buf_size,
                                uint32_t rpt, uint32_t wpt);
static int adlak_gen3b_pm_stop(void *data);

/* Debug functions */
static void adlak_gen3b_dbg_dump_status_regs(void *data);
static void adlak_gen3b_dbg_dump_all_regs(void *data);
static void adlak_gen3b_dbg_dump_extern(void *data, void *context);

static uint32_t adlak_gen3b_master_read32(struct io_region *region, uint32_t offset) {
    return adlak_read32(region, offset + 0x1000);
}

static void adlak_gen3b_master_write32(struct io_region *region, uint32_t offset, uint32_t value) {
    return adlak_write32(region, offset + 0x1000, value);
}

// core 0
static uint32_t adlak_gen3b_core0_read32(struct io_region *region, uint32_t offset) {
    return adlak_read32(region, offset);
}

static void adlak_gen3b_core0_write32(struct io_region *region, uint32_t offset, uint32_t value) {
    return adlak_write32(region, offset, value);
}

static int adlak_gen3b_init(void **data, struct io_region *hw_region, uint32_t sram_addr,
                            uint32_t sram_size) {
    int                             ret = 0;
    struct adlak_gen3b_hw_obj *     hw_obj;
    adlak_reg_gen3b_ps_master_rev_t d;
    AML_LOG_DEBUG("%s", __func__);
    ASSERT(hw_region);
    if (!data) {
        ret = ERR(ENODEV);
        goto end;
    }
    if (!*data) {
        *data = adlak_os_zalloc(sizeof(struct adlak_gen3b_hw_obj), ADLAK_GFP_KERNEL);
        if (unlikely(!*data)) {
            ret = ERR(ENOMEM);
            goto end;
        }
    }
    hw_obj            = (struct adlak_gen3b_hw_obj *)*data;
    hw_obj->region    = hw_region;
    hw_obj->sram_addr = sram_addr;
    hw_obj->sram_size = sram_size;
    d.all             = adlak_gen3b_master_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_REV);
    gen3b_ops.hw_ver.detail.major = d.major_rev;
    gen3b_ops.hw_ver.detail.minor = d.minor_rev;
    AML_LOG_INFO("ADLA HW Ver: r%dp%d", gen3b_ops.hw_ver.detail.major,
                 gen3b_ops.hw_ver.detail.minor);

    hw_obj->core_irq_cfg.mask_err =
        ADLAK_CORE_IRQ_MASK_PEND_TIMEOUT | ADLAK_CORE_IRQ_MASK_APB_WAIT_TIMEOUT |
        ADLAK_CORE_IRQ_MASK_PARSER_STOP_ERR | ADLAK_CORE_IRQ_MASK_INVALID_IOVA |
        ADLAK_CORE_IRQ_MASK_AB_RESPONSE_ERR | ADLAK_CORE_IRQ_MASK_PARSER_STOP_FOR_STEP |
        ADLAK_CORE_IRQ_MASK_PARSER_MODIF_CMQ_ERR |
        ADLAK_CORE_IRQ_MASK_PARSER_NORMAL_QUEUE_UNDERFLOW |
        ADLAK_CORE_IRQ_MASK_PARSER_MODIFY_QUEUE_UNDERFLOW;

    hw_obj->core_irq_cfg.mask_err =
        hw_obj->core_irq_cfg.mask_err |
        (ADLAK_CORE_IRQ_MASK_PM_DRAM_OVF | ADLAK_CORE_IRQ_MASK_PM_FIFO_OVF |
         ADLAK_CORE_IRQ_MASK_PM_ARBITER_OVF);

    hw_obj->core_irq_cfg.mask_normal = 0;
#if ADLAK_DEBUG
    // hw_obj->core_irq_cfg.mask_normal |= ADLAK_CORE_IRQ_MASK_TIM_STAMP;  // debug only
    // hw_obj->core_irq_cfg.mask_normal |= ADLAK_CORE_IRQ_MASK_LAYER_END;  // debug only
#endif
    hw_obj->core_irq_cfg.mask = (hw_obj->core_irq_cfg.mask_err | hw_obj->core_irq_cfg.mask_normal);

    hw_obj->master_irq_cfg.mask_err =
        ADLAK_MASTER_IRQ_MASK_PARSER_STOP_CMD | ADLAK_MASTER_IRQ_MASK_PARSER_STOP_ERR |
        ADLAK_MASTER_IRQ_MASK_PARSER_STOP_PMT | ADLAK_MASTER_IRQ_MASK_PEND_TIMEOUT |
        ADLAK_MASTER_IRQ_MASK_APB_WAIT_TIMEOUT | ADLAK_MASTER_IRQ_MASK_PARSER_STOP_FOR_STEP |
        ADLAK_MASTER_IRQ_MASK_PARSER_STOP_FOR_LOOP |
        ADLAK_CORE_IRQ_MASK_PARSER_NORMAL_QUEUE_UNDERFLOW | ADLAK_MASTER_IRQ_MASK_ISP_LINK_OVERFLOW;

    hw_obj->master_irq_cfg.mask_normal = ADLAK_MASTER_IRQ_MASK_TIM_STAMP;
    hw_obj->master_irq_cfg.mask =
        (hw_obj->master_irq_cfg.mask_err | hw_obj->master_irq_cfg.mask_normal);
    adlak_gen3b_device_reset(hw_obj);
    adlak_gen3b_device_start(hw_obj);
end:
    return ret;
}

static int adlak_gen3b_deinit(void **data) {
    struct adlak_gen3b_hw_obj *hw_obj;
    if (!data) {
        return -1;
    }
    AML_LOG_DEBUG("%s", __func__);
    hw_obj = (struct adlak_gen3b_hw_obj *)*data;
    if (hw_obj) {
        adlak_gen3b_device_stop(hw_obj);
        adlak_os_free(hw_obj);
        *data = NULL;
    }
    return 0;
}

/* Device control */
static void adlak_gen3b_device_enable(void *data, bool en) {
    struct adlak_gen3b_hw_obj *         hw_obj = (struct adlak_gen3b_hw_obj *)data;
    adlak_reg_gen3b_ps_master_adla_en_t d;
    AML_LOG_DEBUG("%s", __func__);
    // 1. set clock gating
    adlak_gen3b_set_autoclock(hw_obj, en);

    // 2. adla  enable
    d.adla_en = 0;
    if (en) {
        d.adla_en = 1;
    }
    adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_ADLA_EN, d.all);
    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_ADLA_EN, d.all);
}

/* Device operations */
static int adlak_gen3b_device_reset(void *data) {
    struct adlak_gen3b_hw_obj *       hw_obj = (struct adlak_gen3b_hw_obj *)data;
    adlak_reg_gen3b_ps_core_ab_ctl_t  d_ab;
    adlak_reg_gen3b_ps_master_swrst_t d;
    uint32_t                          cnt;
    AML_LOG_INFO("%s", __func__);

    adlak_gen3b_device_enable(hw_obj, true);  // alda enable

    /*1. Stop the memory access*/
    d_ab.all = adlak_gen3b_core0_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_AB_CTL);

    d_ab.ab_force_stop_en = 1;
    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_AB_CTL, d_ab.all);

    /*2. Wait until memory access complete*/
    cnt = 0;
    while (1) {
        d_ab.all = adlak_gen3b_core0_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_AB_CTL);
        if (1 == d_ab.ab_force_stop_idle) {
            break;
        }
        adlak_os_udelay(1);
        cnt++;
        if (cnt > 30000) {
            AML_LOG_ERR("wait ab_force_stop timeout!");
            ASSERT(0);
            return -1;
        }
    };

    /*3. Release the memory access*/
    d_ab.all = adlak_gen3b_core0_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_AB_CTL);
    d_ab.ab_force_stop_en   = 0;
    d_ab.ab_force_stop_idle = 0;
    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_AB_CTL, d_ab.all);

    /*4. Reset adlak*/
    d.adla_swrst = 1;
    adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_SWRST, d.all);

    d.adla_swrst = 0;
    adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_SWRST, d.all);

    adlak_gen3b_clear_all_irqs(hw_obj);
    return 0;
}

static void adlak_gen3b_set_autoclock(void *data, uint32_t en) {
    struct adlak_gen3b_hw_obj *hw_obj = (struct adlak_gen3b_hw_obj *)data;
    AML_LOG_DEBUG("%s", __func__);
    if (0 == en) {
        adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_CLK_AUTOCLK, 0x0);
        adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_CLK_AUTOCLK, 0x0);
        adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_MC_CTL, 0x0);
    } else {
        adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_CLK_AUTOCLK,
                                   0xFFFFFFFF);
        adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_CLK_AUTOCLK, 0xFFFFFFFF);
        adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_MC_CTL, 0xDF);
        adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_CLK_IDLE_CNT,
                                  0x0a08);  // TODO need confirm the value
    }
}

static int adlak_gen3b_set_axisram(void *data) {
    struct adlak_gen3b_hw_obj *      hw_obj = (struct adlak_gen3b_hw_obj *)data;
    uint32_t                         pa_start, va_start, va_end, wrap_en;
    adlak_reg_gen3b_ps_core_ab_ctl_t d_ab;
    AML_LOG_DEBUG("%s", __func__);

    if (hw_obj->sram_size) {
        wrap_en  = true;
        pa_start = hw_obj->sram_addr;
        va_start = hw_obj->sram_addr;
        va_end   = hw_obj->sram_addr + hw_obj->sram_size;
        if (wrap_en) {
            va_end += va_end - va_start;
        }
        AML_LOG_INFO("va_start = 0x%lx,va_end = 0x%lX", (uintptr_t)va_start, (uintptr_t)va_end);
        adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_AB_AXI_SADDR,
                                  va_start / 4096);
        adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_AB_AXI_EADDR,
                                  va_end / 4096);
        adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_AB_AXI_PADDR,
                                  pa_start / 4096);
        d_ab.all = adlak_gen3b_core0_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_AB_CTL);
        d_ab.ab_axi_addr_wrap_en = wrap_en;
        adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_AB_CTL, d_ab.all);
    }
    return 0;
}

static int adlak_gen3b_device_start(void *data) {
    struct adlak_gen3b_hw_obj *hw_obj = (struct adlak_gen3b_hw_obj *)data;
    AML_LOG_DEBUG("%s", __func__);
    adlak_gen3b_device_enable(hw_obj, true);  // alda enable
    adlak_gen3b_wait_device_idle(hw_obj);
    /*2.2. Set MMU disable */
    adlak_gen3b_smmu_config(hw_obj, false, 0);

    adlak_gen3b_set_axisram(hw_obj);
    adlak_gen3b_parser_set_apb_timeout(hw_obj, 0xFF);
    adlak_gen3b_parser_set_pend_timer(hw_obj, 0x1000000);
    adlak_gen3b_irq_enable(hw_obj);
    return 0;
}

static int adlak_gen3b_device_stop(void *data) {
    struct adlak_gen3b_hw_obj *hw_obj = (struct adlak_gen3b_hw_obj *)data;
    AML_LOG_DEBUG("%s", __func__);
    adlak_gen3b_irq_disable(hw_obj);
    adlak_gen3b_device_reset(hw_obj);
    adlak_gen3b_device_enable(hw_obj, false);  // alda disable
    return 0;
}

/* Power management */
static int adlak_gen3b_device_suspend(void *data) {
    AML_LOG_DEBUG("%s", __func__);
    // NULL
    return 0;
}

static int adlak_gen3b_device_resume(void *data) {
    struct adlak_gen3b_hw_obj *hw_obj = (struct adlak_gen3b_hw_obj *)data;
    AML_LOG_DEBUG("%s", __func__);
    adlak_gen3b_device_reset(hw_obj);
    adlak_gen3b_device_start(hw_obj);
    return 0;
}

static int adlak_gen3b_wait_device_idle(void *data) {
    struct adlak_gen3b_hw_obj *hw_obj = (struct adlak_gen3b_hw_obj *)data;
    uint32_t                   idel_sts, idel_sts_pre, cnt;
    AML_LOG_DEBUG("%s", __func__);
    cnt          = 0;
    idel_sts_pre = 0;
    while (1) {
        idel_sts =
            adlak_gen3b_master_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_PS_M_IDLE_STS);
        if (idel_sts != 0xFFFFFFFF) {
            if (idel_sts_pre != idel_sts) {
                idel_sts_pre = idel_sts;
                AML_LOG_WARN("REG_ADLAK_PS_MODULE_IDLE_STS   : 0x%08X", idel_sts);
            }
        } else {
            break;
        }
        adlak_os_udelay(1);
        cnt++;
        if (cnt > 30000) {
            AML_LOG_ERR("wait device idel timeout!");
            return ERR(EIO);
        }
    };
    cnt          = 0;
    idel_sts_pre = 0;
    while (1) {
        idel_sts = adlak_gen3b_core0_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_PS_M_IDLE_STS);
        if (idel_sts != 0xFFFFFFFF) {
            if (idel_sts_pre != idel_sts) {
                idel_sts_pre = idel_sts;
                AML_LOG_WARN("REG_ADLAK_PS_MODULE_IDLE_STS   : 0x%08X", idel_sts);
            }
        } else {
            break;
        }
        adlak_os_udelay(1);
        cnt++;
        if (cnt > 30000) {
            AML_LOG_ERR("wait device idel timeout!");
            return ERR(EIO);
        }
    };
    //     TODO if the parser wpt==rpt==ppt
    return 0;
}

/* progress parser */

static int adlak_gen3b_parser_submit(void *data, uint32_t base_addr, uint32_t size, uint32_t rpt,
                                     uint32_t wpt) {
    struct adlak_gen3b_hw_obj *         hw_obj = (struct adlak_gen3b_hw_obj *)data;
    int                                 ret    = 0;
    adlak_reg_gen3b_ps_master_ps_ctrl_t d;
    AML_LOG_DEBUG("%s", __func__);
    if (wpt % 16 != 0) {
        AML_LOG_ERR("rbf_wpt must align with 16 bytes");
        ret = -1;
        goto end;
    }
    AML_LOG_INFO("set parser base addr = 0x%08x", base_addr);
    AML_LOG_INFO("set parser size      = 0x%08x", size);
    AML_LOG_INFO("set parser rpt       = 0x%08x", rpt);
    AML_LOG_INFO("set parser ppt       = 0x%08x", rpt);
    AML_LOG_INFO("set parser wpt       = 0x%08x", wpt);
    // restore parser
    adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_PS_RBF_BASE, base_addr);
    adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_PS_RBF_SIZE, size);
    adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_PS_RBF_RPT, rpt);
    adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_PS_RBF_PPT, rpt);
    adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_PS_RBF_WPT,
                               wpt & 0x0FFFFFFF);

    //    parser_start
    d.all      = 0;
    d.ps_start = 1;
    adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_PS_CTRL, d.all);
end:
    return ret;
}

/* progress smmu */
static int adlak_gen3b_smmu_config(void *data, bool enable, uint64_t smmu_entry) {
    struct adlak_gen3b_hw_obj *       hw_obj = (struct adlak_gen3b_hw_obj *)data;
    adlak_reg_gen3b_ps_core_smmu_en_t d;
    AML_LOG_DEBUG("%s", __func__);

    adlak_gen3b_wait_device_idle(hw_obj);
    d.all = 0;
    if (enable) {
        d.smmu_en = true;
    } else {
        d.smmu_en = false;
    }
    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_SMMU_EN, d.all);

    if (enable) {
        AML_LOG_INFO("SMMU Enable, and set smmu_entry addr=0x%lX", (uintptr_t)smmu_entry);

        adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_SMMU_TTBR_L,
                                  (uint32_t)(smmu_entry & (0xFFFFFFFF)));
        adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_SMMU_TTBR_H,
                                  (uint32_t)((smmu_entry >> 32) & (0xFFFFFFFF)));
        adlak_gen3b_smmu_refresh(hw_obj);
    } else {
        AML_LOG_INFO("SMMU Disable");
    }
    return 0;
}

static int adlak_gen3b_smmu_refresh(void *data) {
    struct adlak_gen3b_hw_obj *            hw_obj = (struct adlak_gen3b_hw_obj *)data;
    int                                    ret    = 0;
    adlak_reg_gen3b_ps_core_smmu_inv_ctl_t d;
    uint32_t                               cnt;
    adlak_reg_gen3b_ps_core_smmu_inv_va_t  iova;
    AML_LOG_DEBUG("%s", __func__);
    iova.smmu_invalid_addr = 0;
    d.all                  = 0;
    // invalid all
    d.smmu_invalid_rdy = 1;
    d.smmu_invalid_all = 0x0F;

    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_SMMU_INV_VA,
                              (uint32_t)iova.smmu_invalid_addr);
    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_SMMU_INV_CTL,
                              (uint32_t)d.all);

    /*2. Wait until smmu_invalid complete*/
    cnt = 0;
    do {
        d.all = adlak_gen3b_core0_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_SMMU_INV_CTL);
        cnt++;
        if (cnt > 3000) {
            ret = -1;
            AML_LOG_ERR("wait smmu_invalid_rdy timeout!");
            break;
        }
    } while (d.smmu_invalid_rdy == 1);
    return ret;
}

static void adlak_gen3b_parser_set_pend_timer(struct adlak_gen3b_hw_obj *hw_obj, uint32_t time) {
    adlak_reg_gen3b_ps_master_ps_pend_en_t d;
    AML_LOG_DEBUG("%s", __func__);
    d.ps_pend_timer_en = 0;
    if (time) {
        d.ps_pend_timer_en = 1;
    }
    adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_PS_PEND_VAL, time);
    adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_PS_PEND_EN, d.all);
}

static void adlak_gen3b_parser_set_apb_timeout(struct adlak_gen3b_hw_obj *hw_obj, uint32_t time) {
    AML_LOG_DEBUG("%s", __func__);
    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_WAIT_TIMER, time);
    adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_WAIT_TIMER, time);
}

/* progress irq */

static void adlak_gen3b_clear_all_irqs(struct adlak_gen3b_hw_obj *hw_obj) {
    uint32_t clr_bits = 0xFFFFFFFF;
    AML_LOG_DEBUG("%s", __func__);
    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_IRQ_RAW, clr_bits);
    adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_IRQ_RAW, clr_bits);
}

static void adlak_gen3b_clear_core0_irqs(struct adlak_gen3b_hw_obj *hw_obj, uint32_t clr_bits) {
    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_IRQ_RAW, clr_bits);
}

static void adlak_gen3b_clear_master_irqs(struct adlak_gen3b_hw_obj *hw_obj, uint32_t clr_bits) {
    adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_IRQ_RAW, clr_bits);
}

static void adlak_core_irq_status_decode(uint32_t state) {
    if (state & ADLAK_CORE_IRQ_MASK_PARSER_STOP_ERR) {
        AML_LOG_WARN(" [1]: parser stop for error");
    }
    if (state & ADLAK_CORE_IRQ_MASK_PEND_TIMEOUT) {
        AML_LOG_WARN(" [3]: pending timer timeout");
    }
    if (state & ADLAK_CORE_IRQ_MASK_LAYER_END) {
        AML_LOG_WARN(" [4]: layer end event");
    }
    if (state & ADLAK_CORE_IRQ_MASK_TIM_STAMP) {
        AML_LOG_WARN(" [5]: time_stamp irq event");
    }
    if (state & ADLAK_CORE_IRQ_MASK_APB_WAIT_TIMEOUT) {
        AML_LOG_WARN(" [6]: apb wait timer timeout");
    }
    if (state & ADLAK_CORE_IRQ_MASK_PM_DRAM_OVF) {
        AML_LOG_WARN(" [7]: pm dram overflow");
    }
    if (state & ADLAK_CORE_IRQ_MASK_PM_FIFO_OVF) {
        AML_LOG_WARN(" [8]: pm fifo overflow");
    }
    if (state & ADLAK_CORE_IRQ_MASK_PM_ARBITER_OVF) {
        AML_LOG_WARN(" [9]: pm arbiter overflow");
    }
    if (state & ADLAK_CORE_IRQ_MASK_INVALID_IOVA) {
        AML_LOG_WARN(" [10]: smmu has an invalid-va");
    }
    if (state & ADLAK_CORE_IRQ_MASK_AB_RESPONSE_ERR) {
        AML_LOG_WARN(" [11]: ab has response error");
    }
    if (state & ADLAK_CORE_IRQ_MASK_PARSER_STOP_FOR_STEP) {
        AML_LOG_WARN(" [12]: parser stop for single step");
    }
    if (state & ADLAK_CORE_IRQ_MASK_PARSER_MODIF_CMQ_ERR) {
        AML_LOG_WARN(" [13]: parser modified queue command error");
    }
    if (state & ADLAK_CORE_IRQ_MASK_PARSER_NORMAL_QUEUE_UNDERFLOW) {
        AML_LOG_WARN(" [14]: ps normal queue underflow");
    }
    if (state & ADLAK_CORE_IRQ_MASK_PARSER_MODIFY_QUEUE_UNDERFLOW) {
        AML_LOG_WARN(" [15]: ps modified queue underflow");
    }
}

static void adlak_core_status_report_decode(uint32_t state) {
    adlak_reg_gen3b_ps_core_sts_report_t d;
    d.all = state;

    if (d.hang_dw_sramf) {
        AML_LOG_WARN(" [0]: dw sramf hang");
    }
    if (d.hang_dw_sramw) {
        AML_LOG_WARN(" [1]: dw sramw hang");
    }
    if (d.hang_pe_srama) {
        AML_LOG_WARN(" [2]: pe srama hang");
    }
    if (d.hang_pe_sramm) {
        AML_LOG_WARN(" [3]: pe sramm hang");
    }
    if (d.hang_px_srama) {
        AML_LOG_WARN(" [4]: px srama hang");
    }
    if (d.hang_px_sramm) {
        AML_LOG_WARN(" [5]: px sramm hang");
    }
    if (d.rsv1) {
        AML_LOG_WARN(" [6]: reserved");
    }
    if (d.hang_vlc_decoder) {
        AML_LOG_WARN(" [7]: vlc decoder hang rpid = %d.", d.vlc_decoder_rpid);
    }
    if (d.hang_ps_dep) {
        AML_LOG_WARN(" [16]: ps dependence hang");
    }
    if (d.hang_mc_dep) {
        AML_LOG_WARN(" [17]: mc dependence hang");
    }
    if (d.hang_dw_f_dep) {
        AML_LOG_WARN(" [18]: dw_f dependence hang");
    }
    if (d.hang_dw_w_dep) {
        AML_LOG_WARN(" [19]: dw_w dependence hang");
    }
    if (d.hang_rs_dep) {
        AML_LOG_WARN(" [20]: rs dependence hang");
    }
}

static void adlak_core_module_status_decode(uint32_t state) {
    AML_LOG_WARN(
        "Modules busy part1: pm[%d], smmu[%d], ab[%d], ps[%d], rs[%d], mc[%d], dmcf[%d], "
        "dmcw[%d], ",
        !(state & (1 << 14)), !(state & (1 << 13)), !(state & (1 << 12)), !(state & (1 << 11)),
        !(state & (1 << 10)), !(state & (1 << 9)), !(state & (1 << 8)), !(state & (1 << 7)));
    AML_LOG_WARN("Modules busy part2: pe[%d], dw[%d], dmdf[%d], dmdw[%d], px[%d], pwe[%d], pwx[%d]",
                 !(state & (1 << 6)), !(state & (1 << 5)), !(state & (1 << 4)), !(state & (1 << 3)),
                 !(state & (1 << 2)), !(state & (1 << 1)), !(state & (1 << 0)));
}

static int adlak_gen3b_get_irq_status(void *data, uint32_t *extra_status) {
    struct adlak_gen3b_hw_obj *hw_obj = (struct adlak_gen3b_hw_obj *)data;
    hw_obj->core_irq_status.irq_masked =
        adlak_gen3b_core0_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_IRQ_MASKED);
    hw_obj->core_irq_status.irq_raw =
        adlak_gen3b_core0_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_IRQ_RAW);
    hw_obj->core_irq_status.time_stamp =
        adlak_gen3b_core0_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_PS_TIME_STAMP);
    hw_obj->master_irq_status.irq_masked =
        adlak_gen3b_master_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_IRQ_MASKED);
    hw_obj->master_irq_status.irq_raw =
        adlak_gen3b_master_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_IRQ_RAW);
    hw_obj->master_irq_status.time_stamp =
        adlak_gen3b_master_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_PS_TIME_STAMP);

    if (extra_status) {
        *extra_status = 0;
    }
    if (unlikely(hw_obj->master_irq_status.irq_masked == 0)) {
        adlak_gen3b_irq_disable(hw_obj);
        adlak_gen3b_clear_master_irqs(hw_obj, hw_obj->master_irq_status.irq_masked);
        adlak_gen3b_clear_core0_irqs(hw_obj, hw_obj->core_irq_status.irq_masked);
        return ADLAK_HAL_IRQ_STATE_ERROR;
    }
    if ((hw_obj->core_irq_status.irq_masked & hw_obj->core_irq_cfg.mask_err) ||
        (hw_obj->master_irq_status.irq_masked & hw_obj->master_irq_cfg.mask_err)) {
        // AML_LOG_ERR("interrupt disabled temporary!");
        adlak_gen3b_irq_disable(hw_obj);
        adlak_gen3b_clear_master_irqs(hw_obj, hw_obj->master_irq_status.irq_masked);
        adlak_gen3b_clear_core0_irqs(hw_obj, hw_obj->core_irq_status.irq_masked);
        return ADLAK_HAL_IRQ_STATE_ERROR;
    } else {
        adlak_gen3b_clear_master_irqs(hw_obj, hw_obj->master_irq_status.irq_masked);
        adlak_gen3b_clear_core0_irqs(hw_obj, hw_obj->core_irq_status.irq_masked);
        if (hw_obj->master_irq_status.irq_masked & hw_obj->master_irq_cfg.mask_normal) {
            return ADLAK_HAL_IRQ_STATE_NORMAL;
        } else {
            return ADLAK_HAL_IRQ_STATE_SKIP;
        }
    }
}

static void adlak_gen3b_irq_enable(struct adlak_gen3b_hw_obj *hw_obj) {
    AML_LOG_DEBUG("%s", __func__);
    adlak_gen3b_clear_all_irqs(hw_obj);
    adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_IRQ_MASK,
                               hw_obj->master_irq_cfg.mask);
    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_IRQ_MASK,
                              hw_obj->core_irq_cfg.mask);
}

static void adlak_gen3b_irq_disable(struct adlak_gen3b_hw_obj *hw_obj) {
    AML_LOG_DEBUG("%s", __func__);
    adlak_gen3b_master_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_MASTER_IRQ_MASK, 0);
    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_IRQ_MASK, 0);
    adlak_gen3b_clear_all_irqs(hw_obj);
}

/* pm control*/

static void adlak_gen3b_pm_enable(struct adlak_gen3b_hw_obj *hw_obj) {
    adlak_reg_gen3b_ps_core_pm_en_t d;
    AML_LOG_DEBUG("%s", __func__);
    d.all   = 0;
    d.pm_en = 0x03;

    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_PM_EN, d.all);

    if (gen3b_ops.hw_ver.detail.major >= 3) {
        /*
            pm_ddr_unit bit[0-1]   00: 16Byte; 01: 32Byte; 10: 64Byte; 11: reserved
            pm_sram_unit bit[2-3]  00: 16Byte; 01: 32Byte; 10: 64Byte; 11: reserved
        */
        adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_PM_UNIT,
                                  0x04);  // dram:16bit, sram:32bit
        // adlak_gen3b_core0_write32(hw_obj->region,REG_ADLAK_PM_UNIT, 0x00);//16bit
    }
}

static void adlak_gen3b_pm_disable(struct adlak_gen3b_hw_obj *hw_obj) {
    adlak_reg_gen3b_ps_core_pm_en_t d;
    AML_LOG_DEBUG("%s", __func__);
    d.all = 0;

    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_PM_EN, d.all);
}

static void adlak_gen3b_pm_reset(struct adlak_gen3b_hw_obj *hw_obj) {
    adlak_reg_gen3b_ps_core_pm_en_t d;
    AML_LOG_DEBUG("%s", __func__);

    d.all      = adlak_gen3b_core0_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_PM_EN);
    d.pm_swrst = 1;
    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_PM_EN, d.all);
    d.pm_swrst = 0;
    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_PM_EN, d.all);
}

static void adlak_gen3b_pm_config(struct adlak_gen3b_hw_obj *hw_obj, uint32_t addr,
                                  uint32_t buf_size, uint32_t wpt) {
    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_PM_RBF_BASE, addr >> 8);
    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_PM_RBF_SIZE, buf_size >> 8);
    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_PM_RBF_WPT, wpt >> 8);
    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_PM_RBF_RPT, 0);
}

static int adlak_gen3b_pm_fush_until_empty(struct adlak_gen3b_hw_obj *hw_obj) {
    adlak_reg_gen3b_ps_core_pm_sts_t d;
    uint32_t                         cnt;
    AML_LOG_DEBUG("%s", __func__);
    // flush pm
    d.all      = 0;
    d.pm_flush = 1;
    adlak_gen3b_core0_write32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_PM_STS, d.all);

    /*2. Wait until pm empty*/
    cnt = 0;
    do {
        d.all = adlak_gen3b_core0_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_PM_STS);
        cnt++;
        if (cnt > 3000) {
            AML_LOG_WARN("wait pm empty timeout!");
            ASSERT(0);
            return -1;
        }
    } while (d.pm_fifo_empty == 0);
    return 0;
}

static int adlak_gen3b_pm_start(void *data, uint64_t profile_iova, uint32_t profile_buf_size,
                                uint32_t rpt, uint32_t wpt) {
    struct adlak_gen3b_hw_obj *hw_obj = (struct adlak_gen3b_hw_obj *)data;
    AML_LOG_DEBUG("%s", __func__);
    adlak_gen3b_pm_enable(hw_obj);
    adlak_gen3b_wait_device_idle(hw_obj);
    adlak_gen3b_pm_reset(hw_obj);
    adlak_gen3b_pm_config(hw_obj, profile_iova, profile_buf_size, wpt);
    return 0;
}

static int adlak_gen3b_pm_stop(void *data) {
    struct adlak_gen3b_hw_obj *hw_obj = (struct adlak_gen3b_hw_obj *)data;
    int                        ret;
    AML_LOG_DEBUG("%s", __func__);
    ret = adlak_gen3b_pm_fush_until_empty(hw_obj);
    adlak_gen3b_pm_disable(hw_obj);
    return ret;
}

/* Debug functions */
static void adlak_gen3b_dbg_dump_status_regs(void *data) {
    struct adlak_gen3b_hw_obj * hw_obj   = (struct adlak_gen3b_hw_obj *)data;
    struct adlak_gen3b_hw_stat *phw_stat = &hw_obj->core_hw_stat;
    AML_LOG_DEBUG("%s", __func__);
    if ((hw_obj->core_irq_status.irq_masked & hw_obj->core_irq_cfg.mask_err) ||
        (hw_obj->master_irq_status.irq_masked & hw_obj->master_irq_cfg.mask_err)) {
        AML_LOG_ERR("interrupt disabled temporary!");
        phw_stat->ps_err_dat =
            adlak_gen3b_core0_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_PS_ERR_DAT);
        phw_stat->status_report =
            adlak_gen3b_core0_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_STS_REPORT);
        phw_stat->ps_module_stat =
            adlak_gen3b_core0_read32(hw_obj->region, ADLAK_REG_GEN3B_PS_CORE_PS_M_IDLE_STS);

        adlak_core_irq_status_decode(hw_obj->core_irq_status.irq_raw);
        adlak_core_status_report_decode(phw_stat->status_report);
        adlak_core_module_status_decode(phw_stat->ps_module_stat);
        AML_LOG_WARN("REG_ADLAK_PS_ERR_DAT   : 0x%08X", phw_stat->ps_err_dat);
        adlak_gen3b_dbg_dump_all_regs(hw_obj);
    } else {
        AML_LOG_INFO("ADLAK_REG_M_IRQ_MASKED   : 0x%08X", hw_obj->master_irq_status.irq_masked);
        AML_LOG_INFO("ADLAK_REG_M_IRQ_RAW      : 0x%08X", hw_obj->master_irq_status.irq_raw);
        AML_LOG_INFO("ADLAK_REG_M_PS_TIME_STAMP: 0x%08X", hw_obj->master_irq_status.time_stamp);

        AML_LOG_INFO("ADLAK_REG_Core_IRQ_MASKED   : 0x%08X", hw_obj->core_irq_status.irq_masked);
        AML_LOG_INFO("ADLAK_REG_Core_IRQ_RAW      : 0x%08X", hw_obj->core_irq_status.irq_raw);
        AML_LOG_INFO("ADLAK_REG_Core_PS_TIME_STAMP: 0x%08X", hw_obj->core_irq_status.time_stamp);
    }
}

static int adlak_get_master_reg_name(int offset, char *buf, size_t buf_size) {
    switch (offset) {
        case ADLAK_REG_GEN3B_PS_MASTER_REV:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_REV");
        case ADLAK_REG_GEN3B_PS_MASTER_WAIT_TIMER:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_WAIT_TIMER");
        case ADLAK_REG_GEN3B_PS_MASTER_SECURE_MODE:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_SECURE_MODE");
        case ADLAK_REG_GEN3B_PS_MASTER_IRQ_MASKED:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_IRQ_MASKED");
        case ADLAK_REG_GEN3B_PS_MASTER_IRQ_MASK:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_IRQ_MASK");
        case ADLAK_REG_GEN3B_PS_MASTER_IRQ_RAW:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_IRQ_RAW");
        case ADLAK_REG_GEN3B_PS_MASTER_STS_REPORT:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_STS_REPORT");
        case ADLAK_REG_GEN3B_PS_MASTER_SWRST:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_SWRST");
        case ADLAK_REG_GEN3B_PS_MASTER_ADLA_EN:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_ADLA_EN");
        case ADLAK_REG_GEN3B_PS_MASTER_CLK_AUTOCLK:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_CLK_AUTOCLK");
        case ADLAK_REG_GEN3B_PS_MASTER_CLK_IDLE_CNT:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_CLK_IDLE_CNT");
        case ADLAK_REG_GEN3B_PS_MASTER_DBG_EN:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_DBG_EN");
        case ADLAK_REG_GEN3B_PS_MASTER_DBG_SEL:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_DBG_SEL");
        case ADLAK_REG_GEN3B_PS_MASTER_DBG_SUB_SEL:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_DBG_SUB_SEL");
        case ADLAK_REG_GEN3B_PS_MASTER_DBG_DAT:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_DBG_DAT");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_CTRL:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_CTRL");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_STS:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_STS");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_ERR_DAT:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_ERR_DAT");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_IDLE_STS:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_IDLE_STS");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_TIME_STAMP:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_TIME_STAMP");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_RBF_BASE:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_RBF_BASE");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_RBF_SIZE:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_RBF_SIZE");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_RBF_WPT:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_RBF_WPT");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_RBF_RPT:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_RBF_RPT");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_RBF_PPT:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_RBF_PPT");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_FINISH_ID_0:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_FINISH_ID_0");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_FINISH_ID_1:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_FINISH_ID_1");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_OST:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_OST");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_PEND_EN:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_PEND_EN");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_PEND_VAL:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_PEND_VAL");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_M_IDLE_STS:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_M_IDLE_STS");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_DBG_SW_GID:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_DBG_SW_GID");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_OPTION_0:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_OPTION_0");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_DBG_HW_GID:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_DBG_HW_GID");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_FINISH_ID_2:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_FINISH_ID_2");
        case ADLAK_REG_GEN3B_PS_MASTER_HB_CTL:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_HB_CTL");
        case ADLAK_REG_GEN3B_PS_MASTER_HB_PRIO:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_HB_PRIO");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_LOOP_LEVEL:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_LOOP_LEVEL");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_LOOP_CNT:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_LOOP_CNT");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_SKIP:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_SKIP");
        case ADLAK_REG_GEN3B_PS_MASTER_PS_CONDITION:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_PS_CONDITION");
        case ADLAK_REG_GEN3B_PS_MASTER_LK_CTL:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_LK_CTL");
        case ADLAK_REG_GEN3B_PS_MASTER_LK_RST:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_LK_RST");
        case ADLAK_REG_GEN3B_PS_MASTER_LK_ERROR:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_LK_ERROR");
        case ADLAK_REG_GEN3B_PS_MASTER_LK_RX_T_WR_ID:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_LK_RX_T_WR_ID");
        case ADLAK_REG_GEN3B_PS_MASTER_LK_RX_T_RD_ID:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_LK_RX_T_RD_ID");
        case ADLAK_REG_GEN3B_PS_MASTER_LK_TX_T_WR_ID:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_LK_TX_T_WR_ID");
        case ADLAK_REG_GEN3B_PS_MASTER_LK_TX_T_RD_ID:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_LK_TX_T_RD_ID");
        case ADLAK_REG_GEN3B_PS_MASTER_LK_RX_S_WR_ID:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_LK_RX_S_WR_ID");
        case ADLAK_REG_GEN3B_PS_MASTER_LK_RX_S_RD_ID:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_LK_RX_S_RD_ID");
        case ADLAK_REG_GEN3B_PS_MASTER_LK_TX_S_WR_ID:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_LK_TX_S_WR_ID");
        case ADLAK_REG_GEN3B_PS_MASTER_LK_TX_S_RD_ID:
            return adlak_os_snprintf(buf, buf_size, "%s", "M_LK_TX_S_RD_ID");
        default:
            return 0;
    }
}

static int adlak_get_core_reg_name(int offset, char *buf, size_t buf_size) {
    switch (offset) {
        case ADLAK_REG_GEN3B_PS_CORE_REV:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_REV");
        case ADLAK_REG_GEN3B_PS_CORE_WAIT_TIMER:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_WAIT_TIMER");
        case ADLAK_REG_GEN3B_PS_CORE_IRQ_MASKED:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_IRQ_MASKED");
        case ADLAK_REG_GEN3B_PS_CORE_IRQ_MASK:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_IRQ_MASK");
        case ADLAK_REG_GEN3B_PS_CORE_IRQ_RAW:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_IRQ_RAW");
        case ADLAK_REG_GEN3B_PS_CORE_STS_REPORT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_STS_REPORT");
        case ADLAK_REG_GEN3B_PS_CORE_SWRST:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_SWRST");
        case ADLAK_REG_GEN3B_PS_CORE_ADLA_EN:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_ADLA_EN");
        case ADLAK_REG_GEN3B_PS_CORE_CLK_AUTOCLK:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_CLK_AUTOCLK");
        case ADLAK_REG_GEN3B_PS_CORE_CLK_IDLE_CNT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_CLK_IDLE_CNT");
        case ADLAK_REG_GEN3B_PS_CORE_DBG_EN:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_DBG_EN");
        case ADLAK_REG_GEN3B_PS_CORE_DBG_SEL:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_DBG_SEL");
        case ADLAK_REG_GEN3B_PS_CORE_DBG_SUB_SEL:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_DBG_SUB_SEL");
        case ADLAK_REG_GEN3B_PS_CORE_DBG_DAT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_DBG_DAT");
        case ADLAK_REG_GEN3B_PS_CORE_PS_CTRL:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_CTRL");
        case ADLAK_REG_GEN3B_PS_CORE_PS_STS:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_STS");
        case ADLAK_REG_GEN3B_PS_CORE_PS_ERR_DAT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_ERR_DAT");
        case ADLAK_REG_GEN3B_PS_CORE_PS_IDLE_STS:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_IDLE_STS");
        case ADLAK_REG_GEN3B_PS_CORE_PS_TIME_STAMP:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_TIME_STAMP");
        case ADLAK_REG_GEN3B_PS_CORE_PS_RBF_BASE:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_RBF_BASE");
        case ADLAK_REG_GEN3B_PS_CORE_PS_RBF_SIZE:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_RBF_SIZE");
        case ADLAK_REG_GEN3B_PS_CORE_PS_RBF_WPT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_RBF_WPT");
        case ADLAK_REG_GEN3B_PS_CORE_PS_RBF_RPT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_RBF_RPT");
        case ADLAK_REG_GEN3B_PS_CORE_PS_RBF_PPT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_RBF_PPT");
        case ADLAK_REG_GEN3B_PS_CORE_PS_FINISH_ID:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_FINISH_ID");
        case ADLAK_REG_GEN3B_PS_CORE_PS_HCNT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_HCNT");
        case ADLAK_REG_GEN3B_PS_CORE_PS_OST:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_OST");
        case ADLAK_REG_GEN3B_PS_CORE_PS_PEND_EN:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_PEND_EN");
        case ADLAK_REG_GEN3B_PS_CORE_PS_PEND_VAL:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_PEND_VAL");
        case ADLAK_REG_GEN3B_PS_CORE_PS_M_IDLE_STS:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_M_IDLE_STS");
        case ADLAK_REG_GEN3B_PS_CORE_PS_DBG_SW_ID:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_DBG_SW_ID");
        case ADLAK_REG_GEN3B_PS_CORE_PS_OPTION_0:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_OPTION_0");
        case ADLAK_REG_GEN3B_PS_CORE_PS_DBG_HW_ID:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_DBG_HW_ID");
        case ADLAK_REG_GEN3B_PS_CORE_AB_AXI_PADDR:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_AB_AXI_PADDR");
        case ADLAK_REG_GEN3B_PS_CORE_AB_CTL:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_AB_CTL");
        case ADLAK_REG_GEN3B_PS_CORE_AB_AXI_SADDR:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_AB_AXI_SADDR");
        case ADLAK_REG_GEN3B_PS_CORE_AB_AXI_EADDR:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_AB_AXI_EADDR");
        case ADLAK_REG_GEN3B_PS_CORE_AB_R_CS_PRIO:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_AB_R_CS_PRIO");
        case ADLAK_REG_GEN3B_PS_CORE_AB_R_LS_PRIO:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_AB_R_LS_PRIO");
        case ADLAK_REG_GEN3B_PS_CORE_AB_R_L2_PRIO:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_AB_R_L2_PRIO");
        case ADLAK_REG_GEN3B_PS_CORE_AB_W_PRIO:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_AB_W_PRIO");
        case ADLAK_REG_GEN3B_PS_CORE_AB_RESP_ERR:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_AB_RESP_ERR");
        case ADLAK_REG_GEN3B_PS_CORE_SMMU_EN:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_SMMU_EN");
        case ADLAK_REG_GEN3B_PS_CORE_SMMU_TTBR_L:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_SMMU_TTBR_L");
        case ADLAK_REG_GEN3B_PS_CORE_SMMU_TTBR_H:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_SMMU_TTBR_H");
        case ADLAK_REG_GEN3B_PS_CORE_SMMU_PRIO_POW2_0:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_SMMU_PRIO_POW2_0");
        case ADLAK_REG_GEN3B_PS_CORE_SMMU_PRIO_POW2_1:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_SMMU_PRIO_POW2_1");
        case ADLAK_REG_GEN3B_PS_CORE_SMMU_INV_CTL:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_SMMU_INV_CTL");
        case ADLAK_REG_GEN3B_PS_CORE_SMMU_INV_VA:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_SMMU_INV_VA");
        case ADLAK_REG_GEN3B_PS_CORE_SMMU_DFT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_SMMU_DFT");
        case ADLAK_REG_GEN3B_PS_CORE_SMMU_IVD_MDL:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_SMMU_IVD_MDL");
        case ADLAK_REG_GEN3B_PS_CORE_SMMU_IVD_VA:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_SMMU_IVD_VA");
        case ADLAK_REG_GEN3B_PS_CORE_SMMU_IVD_VA_MSB:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_SMMU_IVD_VA_MSB");
        case ADLAK_REG_GEN3B_PS_CORE_PM_EN:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PM_EN");
        case ADLAK_REG_GEN3B_PS_CORE_PM_RBF_BASE:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PM_RBF_BASE");
        case ADLAK_REG_GEN3B_PS_CORE_PM_RBF_SIZE:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PM_RBF_SIZE");
        case ADLAK_REG_GEN3B_PS_CORE_PM_RBF_WPT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PM_RBF_WPT");
        case ADLAK_REG_GEN3B_PS_CORE_PM_RBF_RPT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PM_RBF_RPT");
        case ADLAK_REG_GEN3B_PS_CORE_PM_STS:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PM_STS");
        case ADLAK_REG_GEN3B_PS_CORE_PM_UNIT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PM_UNIT");
        case ADLAK_REG_GEN3B_PS_CORE_PM_BWCNT0:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PM_BWCNT0");
        case ADLAK_REG_GEN3B_PS_CORE_PM_BWCNT1:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PM_BWCNT1");
        case ADLAK_REG_GEN3B_PS_CORE_PM_PORT_EN_0:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PM_PORT_EN_0");
        case ADLAK_REG_GEN3B_PS_CORE_PM_PORT_EN_1:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PM_PORT_EN_1");
        case ADLAK_REG_GEN3B_PS_CORE_MC_CTL:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_MC_CTL");
        case ADLAK_REG_GEN3B_PS_CORE_PS_RDMA_N_BASE:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_RDMA_N_BASE");
        case ADLAK_REG_GEN3B_PS_CORE_PS_RDMA_N_WPT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_RDMA_N_WPT");
        case ADLAK_REG_GEN3B_PS_CORE_PS_RDMA_N_RPT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_RDMA_N_RPT");
        case ADLAK_REG_GEN3B_PS_CORE_PS_RDMA_M_BASE:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_RDMA_M_BASE");
        case ADLAK_REG_GEN3B_PS_CORE_PS_RDMA_M_WPT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_RDMA_M_WPT");
        case ADLAK_REG_GEN3B_PS_CORE_PS_RDMA_M_RPT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_RDMA_M_RPT");
        case ADLAK_REG_GEN3B_PS_CORE_PS_MRG_N_BASE:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_MRG_N_BASE");
        case ADLAK_REG_GEN3B_PS_CORE_PS_MRG_N_PPT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_MRG_N_PPT");
        case ADLAK_REG_GEN3B_PS_CORE_PS_MRG_M_BASE:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_MRG_M_BASE");
        case ADLAK_REG_GEN3B_PS_CORE_PS_MRG_M_PPT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_MRG_M_PPT");
        case ADLAK_REG_GEN3B_PS_CORE_PS_DEC_BASE:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_DEC_BASE");
        case ADLAK_REG_GEN3B_PS_CORE_PS_DEC_PPT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_DEC_PPT");
        case ADLAK_REG_GEN3B_PS_CORE_PS_BM_ERR_DAT:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_PS_BM_ERR_DAT");
        case ADLAK_REG_GEN3B_PS_CORE_AB_SRAM_SADDR:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_AB_SRAM_SADDR");
        case ADLAK_REG_GEN3B_PS_CORE_AB_SRAM_EADDR:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_AB_SRAM_EADDR");
        case ADLAK_REG_GEN3B_PS_CORE_AB_SRAM_PADDR:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_AB_SRAM_PADDR");
        case ADLAK_REG_GEN3B_PS_CORE_DBUF_CTL:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_DBUF_CTL");
        case ADLAK_REG_GEN3B_PS_CORE_CBUF_CTL:
            return adlak_os_snprintf(buf, buf_size, "%s", "Core_CBUF_CTL");
        default:
            return 0;
    }
}

static int inline adlak_gen3b_dbg_write_master_reg(void *data, uint32_t offset, uint32_t value) {
    struct adlak_gen3b_hw_obj *hw_obj = (struct adlak_gen3b_hw_obj *)data;
    int                        len;
    char                       reg_name[64];
    len = adlak_get_master_reg_name(offset, reg_name, sizeof(reg_name));
    if (len) {
        adlak_gen3b_master_write32(hw_obj->region, offset, value);
        adlak_os_printf("write reg [0x%x]=0x%x,confirm=0x%x\n", offset, value,
                        adlak_gen3b_master_read32(hw_obj->region, offset));
        return 0;
    } else {
        return -1;
    }
}

static int inline adlak_gen3b_dbg_write_core_reg(void *data, uint32_t core_id, uint32_t offset,
                                                 uint32_t value) {
    struct adlak_gen3b_hw_obj *hw_obj = (struct adlak_gen3b_hw_obj *)data;
    int                        len;
    char                       reg_name[64];
    len = adlak_get_core_reg_name(offset, reg_name, sizeof(reg_name));
    if (len && 0 == core_id) {
        adlak_gen3b_core0_write32(hw_obj->region, offset, value);
        adlak_os_printf("write reg [0x%x]=0x%x,confirm=0x%x\n", offset, value,
                        adlak_gen3b_core0_read32(hw_obj->region, offset));
        return 0;
    } else {
        return -1;
    }
}

static int inline adlak_gen3b_dbg_dump_master_reg(void *data, uint32_t offset) {
    struct adlak_gen3b_hw_obj *hw_obj = (struct adlak_gen3b_hw_obj *)data;
    int                        len;
    char                       reg_name[64];
    uint32_t                   reg_val;
    len = adlak_get_master_reg_name(offset, reg_name, sizeof(reg_name));
    if (len) {
        reg_val = adlak_gen3b_master_read32(hw_obj->region, offset);
        adlak_os_printf("0x%-*x%-*s0x%08x", 6, offset, 22, reg_name, reg_val);
        return 0;
    } else {
        return -1;
    }
}

static int inline adlak_gen3b_dbg_dump_core_reg(void *data, uint32_t core_id, uint32_t offset) {
    struct adlak_gen3b_hw_obj *hw_obj = (struct adlak_gen3b_hw_obj *)data;
    int                        len;
    char                       reg_name[64];
    uint32_t                   reg_val;
    len = adlak_get_core_reg_name(offset, reg_name, sizeof(reg_name));
    if (len && 0 == core_id) {
        reg_val = adlak_gen3b_core0_read32(hw_obj->region, offset);
        adlak_os_printf("0x%-*x%-*s0x%08x", 6, offset, 22, reg_name, reg_val);
        return 0;
    } else {
        return -1;
    }
}

static void adlak_gen3b_dbg_dump_all_regs(void *data) {
    struct adlak_gen3b_hw_obj *hw_obj = (struct adlak_gen3b_hw_obj *)data;
    uint32_t                   offset;
    AML_LOG_WARN("%s", __func__);
    for (offset = 0; offset < 0x200;) {
        adlak_gen3b_dbg_dump_master_reg(hw_obj, offset);
        offset += 4;
    }
    for (offset = 0; offset < 0x200;) {
        adlak_gen3b_dbg_dump_core_reg(hw_obj, 0, offset);
        offset += 4;
    }
}

static void adlak_gen3b_dbg_dump_extern(void *data, void *context) {
    struct adlak_gen3b_hw_obj *hw_obj = (struct adlak_gen3b_hw_obj *)data;
    if (hw_obj->core_irq_status.irq_masked & hw_obj->core_irq_cfg.mask_err) {
#ifdef CONFIG_ADLAK_DEBUG_INNNER
        adlak_dbg_dump_module_read_data(context);
        adlak_dbg_dump_module_dump_data(context);
#endif
    }
}

struct adlak_hw_ops gen3b_ops = {

    .init   = adlak_gen3b_init,
    .deinit = adlak_gen3b_deinit,

    /* Device operations */ /* Device operations */
    .device_reset = adlak_gen3b_device_reset,
    .device_start = adlak_gen3b_device_start,
    .device_stop  = adlak_gen3b_device_stop,

    /* Power management */ /* Power management */
    .device_suspend = adlak_gen3b_device_suspend,
    .device_resume  = adlak_gen3b_device_resume,

    .wait_device_idle = adlak_gen3b_wait_device_idle,

    /* progress parser */ /* progress parser */
    .parser_submit = adlak_gen3b_parser_submit,

    /* progress smmu */ /* progress smmu */
    .smmu_config  = adlak_gen3b_smmu_config,
    .smmu_refresh = adlak_gen3b_smmu_refresh,

    /* progress irq */ /* progress irq */
    .get_irq_status = adlak_gen3b_get_irq_status,

    /* pm control*/ /* pm control*/
    .pm_start = adlak_gen3b_pm_start,
    .pm_stop  = adlak_gen3b_pm_stop,

    /* Debug functions */ /* Debug functions */
    .dbg_dump_status_regs = adlak_gen3b_dbg_dump_status_regs,
    .dbg_dump_reg         = NULL,
    .dbg_dump_master_reg  = adlak_gen3b_dbg_dump_master_reg,
    .dbg_dump_core_reg    = adlak_gen3b_dbg_dump_core_reg,
    .dbg_write_reg        = NULL,
    .dbg_write_master_reg = adlak_gen3b_dbg_write_master_reg,
    .dbg_write_core_reg   = adlak_gen3b_dbg_write_core_reg,
    .dbg_dump_all_regs    = adlak_gen3b_dbg_dump_all_regs,
    .dbg_dump_extern      = adlak_gen3b_dbg_dump_extern,

    .name = "gen3b",
};