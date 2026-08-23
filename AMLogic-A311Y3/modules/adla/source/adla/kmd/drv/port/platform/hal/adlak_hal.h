/**
 * Copyright: (C) 2025 Amlogic, Inc. All rights reserved.
 */

/**
 * @file adlak_hal.h
 * @brief
 *
 * @author: shiwei.sun
 * Created: 2025-08-12 23:22:03
 */

#ifndef INCLUDE_E03658AE_ADLAK_HAL
#define INCLUDE_E03658AE_ADLAK_HAL

/***************************** Include Files *********************************/

#include "adlak_common.h"
#include "adlak_device.h"
#include "adlak_platform_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef ADLAK_HW_DEBUG_EN
#define ADLAK_HW_DEBUG_EN (0)
#endif
#if (!(ADLAK_DEBUG))
#undef ADLAK_HW_DEBUG_EN
#define ADLAK_HW_DEBUG_EN (0)
#endif

#define ADLAK_BIT(data, n) (((data) >> (n)) & 0x1)
/**************************** Type Definitions *******************************/

typedef volatile unsigned long __IO;
/**
 * struct io_region - a general struct describe IO region
 *
 * @phys: physical address base of an IO region
 * @kern: kernel virtual address base remapped from phys
 * @size: size of an IO region in byte
 */
struct io_region {
    uint64_t pa_kernel;
    uint32_t size;
    void *   va_kernel;
};

enum ADLAK_HAL_IRQ_STATE {
    ADLAK_HAL_IRQ_STATE_NULL = 0,
    ADLAK_HAL_IRQ_STATE_NORMAL,
    ADLAK_HAL_IRQ_STATE_SKIP,
    ADLAK_HAL_IRQ_STATE_ERROR,
};

// Unified hardware operations interface
struct adlak_hw_ops {
    /* Basic operations */
    int (*init)(void **data, struct io_region *hw_region, uint32_t sram_addr, uint32_t sram_size);
    int (*deinit)(void **data);

    /* Device operations */
    int (*device_reset)(void *data);
    int (*device_start)(void *data);
    int (*device_stop)(void *data);

    /* Power management */
    int (*device_suspend)(void *data);
    int (*device_resume)(void *data);

    int (*wait_device_idle)(void *data);

    /* progress parser */
    int (*parser_submit)(void *data, uint32_t base_addr, uint32_t size, uint32_t rpt, uint32_t wpt);

    /* progress smmu */
    int (*smmu_config)(void *data, bool en, uint64_t entry);
    int (*smmu_refresh)(void *data);

    /* progress irq */
    int (*get_irq_status)(void *data, uint32_t *extra_status);

    /* pm control*/
    int (*pm_start)(void *data, uint64_t profile_iova, uint32_t profile_buf_size, uint32_t rpt,
                    uint32_t wpt);
    int (*pm_stop)(void *data);

    /* Debug functions */
    void (*dbg_dump_status_regs)(void *data);
    int (*dbg_dump_reg)(void *data, uint32_t offset);
    int (*dbg_dump_master_reg)(void *data, uint32_t offset);
    int (*dbg_dump_core_reg)(void *data, uint32_t core, uint32_t offset);
    int (*dbg_write_reg)(void *data, uint32_t offset, uint32_t value);
    int (*dbg_write_master_reg)(void *data, uint32_t offset, uint32_t value);
    int (*dbg_write_core_reg)(void *data, uint32_t core, uint32_t offset, uint32_t value);
    void (*dbg_dump_all_regs)(void *data);
    void (*dbg_dump_extern)(void *data, void *context);
    void (*secure_entry)(void *data, void *cmq_mem_handle);

    /* Hardware version info */
    union {
        struct hw_ver_detail {
            uint8_t minor;
            uint8_t major;
        } detail;
        uint32_t all;
    } hw_ver;
    const char *name;
};

// Declare version-specific operations
extern struct adlak_hw_ops gen2_ops;
extern struct adlak_hw_ops gen3b_ops;
extern struct adlak_hw_ops emu_ops;

/**************************  Function Declarations *****************************/

/**
 * @brief create ADLAK IO region using physical base address
 *
 * @param dev: device pointer
 * @param phys_base: base address
 * @param size: region size
 *
 * @return io_region pointer if successful; NULL if failed;
 */
struct io_region *adlak_create_ioregion(uintptr_t phys_base, uint32_t size);
/**
 * @brief destroy an ADLAK IO region
 *
 * @param region: region pointer created by adlak_create_ioregion
 */
void adlak_destroy_ioregion(struct io_region *region);

int adlak_hal_device_reset(void *data);

int adlak_hal_device_start(void *data);

int adlak_hal_device_stop(void *data);

int adlak_hal_device_suspend(void *data);

int adlak_hal_device_resume(void *data);

int adlak_hal_wait_device_idle(void *data);

int adlak_hal_parser_submit(void *data, uint32_t base_addr, uint32_t size, uint32_t rpt,
                            uint32_t wpt);

int adlak_hal_smmu_config(void *data, bool en, uint64_t entry);

int adlak_hal_get_irq_status(void *data, uint32_t *extra_status);

int adlak_hal_pm_start(void *data, uint32_t base_addr, uint32_t size, uint32_t rpt, uint32_t wpt);

int adlak_hal_pm_stop(void *data);

int adlak_hal_dbg_dump_reg(void *data, uint32_t offset);
int adlak_hal_dbg_write_reg(void *data, uint32_t offset, uint32_t value);

void adlak_hal_dbg_dump_status_regs(void *data);

void adlak_hal_dbg_dump_all_regs(void *data);

void adlak_hal_dbg_dump_extern(void *data, void *context);
void adlak_hal_secure_entry(void *data);

void adlak_hal_deinit(void *data);
int  adlak_hal_init(void *data);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_1A9EF716_ADLAK_HAL */
