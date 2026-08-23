/*******************************************************************************
 * Copyright (C) 2022 Amlogic, Inc. All rights reserved.
 ******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file adlak_api_base.h
 * @brief
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   	Who				Date				Changes
 * ----------------------------------------------------------------------------
 * 1.00a shiwei.sun@amlogic.com	2022/04/26	Initial release
 * </pre>
 *
 ******************************************************************************/

#ifndef __ADLAK_API_BASE_H__
#define __ADLAK_API_BASE_H__

/***************************** Include Files *********************************/
#include "adlak_errcode.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************** Constant Definitions *****************************/

/**************************Global Variable************************************/

/**************************Type Definition and Structure**********************/
enum adlak_extra_state {
    ADLAK_EXTRA_STATE_PM_FIFO_OVERFLOW    = (1 << 0),
    ADLAK_EXTRA_STATE_PM_ARBITER_OVERFLOW = (1 << 1),
} __packed;

enum adlak_pm_buffer_size_perlayer {
    ADLAK_PM_BUFFER_SIZE_PERLAYER_V1 = (256),
    ADLAK_PM_BUFFER_SIZE_PERLAYER_V2 = (512),
} __packed;

enum adlak_smmu_tlb_type {
    ADLAK_ENUM_SMMU_TLB_TYPE_PUBLIC_ONLY = 0,  // default setting
    ADLAK_ENUM_SMMU_TLB_TYPE_PRIVATE_ONLY,
    ADLAK_ENUM_SMMU_TLB_TYPE_PRIVATE_AND_PUBLIC,
} __packed;

struct adlak_buf_desc {
    uint64_t iova_addr; /* virtual address in smmu*/
    uint64_t va_user;   /* virtual address in user mode*/
    uint64_t phys_addr; /* physical base address if mem_type is contiguous*/
    uint64_t bytes;     /*return real size*/
    uint64_t uid;
} __packed;

enum adlak_mem_type {
    ADLAK_ENUM_MEMTYPE_CACHEABLE      = (1 << 0),
    ADLAK_ENUM_MEMTYPE_CONTIGUOUS     = (1 << 1),
    ADLAK_ENUM_MEMTYPE_INNER          = (1 << 2),  // For ADLA use only if value is true.
    ADLAK_ENUM_MEMTYPE_PA_WITHIN_4G   = (1 << 4),  // physical address less than 4Gbytes
    ADLAK_ENUM_MEMTYPE_SHARE          = (1 << 5),  // share between different models
    ADLAK_ENUM_MEMTYPE_SMMU_TLB_DEF   = (1 << 6),  //
    ADLAK_ENUM_MEMTYPE_SMMU_TLB_ID1   = (1 << 7),  //
    ADLAK_ENUM_MEMTYPE_SMMU_PRIV      = (1 << 8),  //
    ADLAK_ENUM_MEMTYPE_IOVA_WITHIN_4G = (1 << 10)  //
} __packed;

enum adlak_mem_direction {
    ADLAK_ENUM_MEM_DIR_READ_WRITE = 0,
    ADLAK_ENUM_MEM_DIR_READ_ONLY,
    ADLAK_ENUM_MEM_DIR_WRITE_ONLY
} __packed;

enum ADLAK_INVOKE_CTRL_FLAGS {
    ADLAK_INVOKE_CTRL_FLAGS_BIT_RESET_AFTER_TASK =
        0,  // must reset the adla dependency after this task completion
    ADLAK_INVOKE_CTRL_FLAGS_BIT_IS_LAST_PARTIAL =
        1,  // mask this task is the last hw partial of the model
    ADLAK_INVOKE_CTRL_FLAGS_BIT_CONTEXT_HOLD =
        2,  // hold the device continuously throughout the execution of the context
} __packed;

struct adlak_buf_req {
    uint64_t              mem_handle;    /* return memory info handle in kernel */
    uint64_t              bytes;         /* bytes requested to allocate */
    uint32_t              align_in_page; /* alignment requirements (in 4KB) */
    uint32_t              data_type;     /* type of data in the buffer to allocate */
    uint32_t              mem_type;      /*request info*/
    uint32_t              mem_direction; /*request info*/
    struct adlak_buf_desc ret_desc;      /* info of buffer successfully allocated */
    uint32_t              mmap_en;       /* the flag of mmap */
    int32_t               errcode;       /* return err number */
} __packed;

struct adlak_extern_buf_info {
    uint64_t              buf_handle;    /* buf handle */
    uint64_t              mem_handle;    /* return memory info handle in kernel */
    uint64_t              bytes;         /* bytes of buffer */
    uint32_t              buf_type;      /* type of buf handle */
    uint32_t              mem_type;      /*request info*/
    uint32_t              mem_direction; /*request info*/
    struct adlak_buf_desc ret_desc;      /* info of buffer successfully import */
    uint32_t              mmap_en;       /* the flag of mmap */
    int32_t               errcode;       /* return err number */
} __packed;

enum adlak_flush_cache_direction {
    FLUSH_TO_DEVICE   = 1,
    FLUSH_FROM_DEVICE = 2,
    FLUSH_NONE        = 3,
};

struct adlak_buf_flush {
    uint64_t mem_handle; /* info of buffer  */
    uint32_t direction;
    uint32_t is_partial; /* is dma sync partial*/
    uint64_t offset;
    uint64_t size;
    int32_t  errcode; /* return err number */
} __packed;

enum adlak_context_priority {
    ADLAK_CONTEXT_PRIORITY_DEFAULT = 0,
    ADLAK_CONTEXT_PRIORITY_LOW,
    ADLAK_CONTEXT_PRIORITY_HIGH,
};

struct adlak_network_desc {
    int32_t  profile_en;  // profilling enable
    uint64_t profile_iova;
    uint32_t profile_buf_size;
    uint32_t priority;          // submit priority
    int32_t  net_register_idx;  // return from kmd
    int64_t  macc_count;
    int32_t  first_hw_layer;
    int32_t  last_hw_layer;
} __packed;

struct adlak_networks_desc {
    uint32_t sub_tasks_count;
    uint64_t networks_desc_va;
    int32_t  net_register_idx;  // return from kmd
    int32_t  smmu_entry_count;
    int32_t  errcode; /* return err number */
} __packed;

struct adlak_network_del_desc {
    int32_t net_register_idx;
} __packed;

struct adlak_network_invoke_desc {
    int32_t  net_register_idx;
    uint32_t sub_tasks_idx;
    int32_t  invoke_register_idx;  // return from kmd
    uint32_t ctrl_flags;
    int32_t  layer_start;
    int32_t  layer_end;
    int32_t  smmu_entry_index;
    uint64_t cmq_base_iova;
    uint32_t cmq_size;
    int32_t  cmq_rpt;
    int32_t  cmq_wpt;
    int32_t  pm_wpt;
} __packed;

struct adlak_network_invoke_del_desc {
    int32_t net_register_idx;
    int32_t invoke_register_idx;

} __packed;

struct adlak_get_stat_desc {
    int32_t net_register_idx;
    int32_t invoke_register_idx;
    int32_t timeout_ms;
    int32_t layer_start;     // return from kmd
    int32_t layer_end;       // return from kmd
    int32_t ret_state;       // 0: success,1:running,-1: timeout, -3: other err
    int32_t profile_en;      // profilling enable
    int32_t invoke_time_us;  // invoke time which get from os

    uint64_t axi_freq_cur;      // adlak axi clock frequency currently
    uint64_t core_freq_cur;     // adlak core clock frequency currently
    uint64_t mem_alloced_base;  // alloced by kmd
    uint64_t mem_alloced_umd;   // alloced by umd in this context
    int64_t  mem_pool_size;     //-1:the limit base on the system
    uint64_t mem_pool_used;     // memory usage
    int32_t  efficiency;
    uint32_t exrta_status;
} __packed;

struct adlak_profile_cfg_desc {
    int32_t  net_register_idx;
    uint32_t sub_tasks_idx;
    int32_t  profile_en;  // profilling enable
    uint64_t profile_iova;
    uint32_t profile_buf_size;
    int32_t  errcode; /* return err number */

} __packed;

struct adlak_test_desc {
    uint64_t type;
} __packed;

struct adlak_caps_desc {
    uint32_t hw_ver;           /* adlak hardware version*/
    uint64_t axi_freq_max;     /* adlak axi clock frequency maximum */
    uint64_t core_freq_max;    /* adlak core clock frequency maximum */
    uint32_t cmq_size;         /* cmq buffer size*/
    uint64_t sram_base;        /* axi sram base addr*/
    uint32_t sram_size;        /* axi sram buffer size*/
    uint64_t hw_iova_max_size; /* tha maximum vaddr value allowed by the hardware*/
    uint64_t iova_max_size;    /* tha maximum vaddr of smmu*/
    uint64_t iova_free_size;   /* tha free size of vaddr*/
    uint32_t use_smmu;
} __packed;

struct adlak_context_attribute {
    uint32_t smmu_tlb_type : 8;
    uint32_t rsv : 24;
} __packed;

struct adlak_mem_available {
    int64_t bytes;
} __packed;

struct adlak_tee_network_desc {
    uint32_t priority;          // submit priority
    int32_t  net_register_idx;  // return from kmd
    uint64_t tee_ctx_handle;
} __packed;

struct adlak_tee_network_invoke_desc {
    int32_t net_register_idx;
    int32_t invoke_register_idx;  // return from kmd
    int32_t invoke_section_id;

} __packed;

struct adlak_tee_query_addr {
    int32_t  net_register_idx;
    uint64_t fd;
    uint64_t ret_addr;
    uint64_t ret_size;

} __packed;

struct adlak_tee_protect_addr {
    int32_t  net_register_idx;
    uint64_t phys_addr;
    uint64_t size;
} __packed;

struct adlak_dev_info_get_req {
    int32_t  info_type;
    int32_t  clk_core_freq_real;
    int32_t  clk_core_freq_set;
    int32_t  device_stat;
    uint32_t dpm_period_set;
    uint32_t dev_hw_version;

    uint64_t mem_pool_size; // total iova pool size, include used & unused
    uint64_t mem_pool_used; // total mem used
    uint64_t mem_kmd_used; // used by kmd, for example smmu tlb
    uint64_t mem_umd_used; // all contexts used from umd
    uint64_t mem_shard_mem;
    uint64_t mem_cur_context; //current context mem used from umd

    uint32_t tasks_sched_num;
    uint32_t tasks_pending_num;
} __packed;

struct adlak_dev_info_set_req {
    int32_t  info_type;
    int64_t  value;
} __packed;

/************************** Function Prototypes ******************************/

#ifdef __cplusplus
}
#endif

#endif /* __ADLAK_API_BASE_H__ end define*/
