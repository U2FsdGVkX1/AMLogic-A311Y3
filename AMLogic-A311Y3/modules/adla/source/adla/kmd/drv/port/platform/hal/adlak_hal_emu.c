/**
 * Copyright: (C) 2025 Amlogic, Inc. All rights reserved.
 */

/**
 * @file adlak_hal_emu.c
 * @brief
 *
 * @author: shiwei.sun
 * Created: 2025-08-14 15:30:07
 */

/***************************** Include Files *********************************/

#include "adlak_hal.h"

/***************** Macros (Inline Functions) Definitions *********************/
static int adlak_gen2_init(void **data, struct io_region *hw_region, uint32_t sram_addr,
                           uint32_t sram_size) {
    emu_ops.hw_ver.detail.major = 0;
    emu_ops.hw_ver.detail.minor = 0;
    return 0;
}

static int adlak_emu_get_irq_status(void *data, uint32_t *extra_status) {
    return ADLAK_HAL_IRQ_STATE_NORMAL;
}

struct adlak_hw_ops emu_ops = {
    .init   = adlak_gen2_init,
    .deinit = NULL,

    /* Device operations */
    .device_reset = NULL,
    .device_start = NULL,
    .device_stop  = NULL,

    /* Power management */
    .device_suspend = NULL,
    .device_resume  = NULL,

    .wait_device_idle = NULL,

    /* progress parser */
    .parser_submit = NULL,

    /* progress smmu */
    .smmu_config  = NULL,
    .smmu_refresh = NULL,

    /* progress irq */
    .get_irq_status = adlak_emu_get_irq_status,

    /* pm control*/
    .pm_start = NULL,
    .pm_stop  = NULL,

    /* Debug functions */
    .dbg_dump_status_regs = NULL,
    .dbg_dump_all_regs    = NULL,
    .dbg_dump_extern      = NULL,

    .name = "emu",
};