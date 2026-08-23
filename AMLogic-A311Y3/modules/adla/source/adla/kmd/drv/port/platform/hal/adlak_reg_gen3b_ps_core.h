/**
 * Copyright: (C) 2025 Amlogic, Inc. All rights reserved.
 */

#ifndef __ADLAK_REG_GEN3B_PS_CORE_H__
#define __ADLAK_REG_GEN3B_PS_CORE_H__

#include "adlak_typedef.h"

/* Generated from sheet: ADLA */

/* Register: REV (0x0000) */
#define ADLAK_REG_GEN3B_PS_CORE_REV 0x0000
typedef union {
    uint32_t all;
    struct {
        uint32_t minor_rev : 8;
        uint32_t major_rev : 8;
        uint32_t rev0_rev : 8;
        uint32_t rev1_rev : 8;
    };
} adlak_reg_gen3b_ps_core_rev_t;

/* Register: WAIT_TIMER (0x0004) */
#define ADLAK_REG_GEN3B_PS_CORE_WAIT_TIMER 0x0004
typedef union {
    uint32_t all;
    struct {
        uint32_t wait_timer_val : 16;
        uint32_t reserved_16_31 : 16;
    };
} adlak_reg_gen3b_ps_core_wait_timer_t;

/* Register: IRQ_MASKED (0x0010) */
#define ADLAK_REG_GEN3B_PS_CORE_IRQ_MASKED 0x0010
typedef union {
    uint32_t all;
    struct {
        uint32_t irqsts_masked : 32;
    };
} adlak_reg_gen3b_ps_core_irq_masked_t;

/* Register: IRQ_MASK (0x0014) */
#define ADLAK_REG_GEN3B_PS_CORE_IRQ_MASK 0x0014
typedef union {
    uint32_t all;
    struct {
        uint32_t irqsts_mask : 32;
    };
} adlak_reg_gen3b_ps_core_irq_mask_t;

/* Register: IRQ_RAW (0x0018) */
#define ADLAK_REG_GEN3B_PS_CORE_IRQ_RAW 0x0018
typedef union {
    uint32_t all;
    struct {
        uint32_t irqsts_raw : 32;
    };
} adlak_reg_gen3b_ps_core_irq_raw_t;

/* Register: STS_REPORT (0x001C) */
#define ADLAK_REG_GEN3B_PS_CORE_STS_REPORT 0x001C
typedef union {
    uint32_t all;
    struct {
        uint32_t hang_dw_sramf : 1;    /* [0]: dw sramf hang*/
        uint32_t hang_dw_sramw : 1;    /* [1]: dw sramw hang*/
        uint32_t hang_pe_srama : 1;    /* [2]: pe srama hang*/
        uint32_t hang_pe_sramm : 1;    /* [3]: pe sramm hang*/
        uint32_t hang_px_srama : 1;    /* [4]: px srama hang*/
        uint32_t hang_px_sramm : 1;    /* [5]: px sramm hang*/
        uint32_t rsv1 : 1;             /* [6]: reserved*/
        uint32_t hang_vlc_decoder : 1; /* [7]: vlc decoder hang*/
        uint32_t vlc_decoder_rpid : 8; /* [15:8] : vlc decoder rpid*/
        uint32_t hang_ps_dep : 1;      /*[16]: ps dependence hang*/
        uint32_t hang_mc_dep : 1;      /*[17]: mc dependence hang*/
        uint32_t hang_dw_f_dep : 1;    /*[18]: dw_f dependence hang*/
        uint32_t hang_dw_w_dep : 1;    /*[19]: dw_w dependence hang*/
        uint32_t hang_rs_dep : 1;      /*[20]: rs dependence hang*/
        uint32_t : 11;
    };
} adlak_reg_gen3b_ps_core_sts_report_t;

/* Register: SWRST (0x0020) */
#define ADLAK_REG_GEN3B_PS_CORE_SWRST 0x0020
typedef union {
    uint32_t all;
    struct {
        uint32_t reserved_0_0 : 1;
        uint32_t reserved_1_31 : 31;
    };
} adlak_reg_gen3b_ps_core_swrst_t;

/* Register: ADLA_EN (0x0024) */
#define ADLAK_REG_GEN3B_PS_CORE_ADLA_EN 0x0024
typedef union {
    uint32_t all;
    struct {
        uint32_t adla_en : 1;
        uint32_t reserved_1_31 : 31;
    };
} adlak_reg_gen3b_ps_core_adla_en_t;

/* Register: CLK_AUTOCLK (0x0028) */
#define ADLAK_REG_GEN3B_PS_CORE_CLK_AUTOCLK 0x0028
typedef union {
    uint32_t all;
    struct {
        uint32_t adla_autoclk_en : 32;
    };
} adlak_reg_gen3b_ps_core_clk_autoclk_t;

/* Register: CLK_IDLE_CNT (0x002C) */
#define ADLAK_REG_GEN3B_PS_CORE_CLK_IDLE_CNT 0x002C
typedef union {
    uint32_t all;
    struct {
        uint32_t adla_autoclk_idle_cnt : 5;
        uint32_t reserved_5_7 : 3;
        uint32_t adla_autoclk_busy_cnt : 5;
        uint32_t reserved_13_31 : 19;
    };
} adlak_reg_gen3b_ps_core_clk_idle_cnt_t;

/* Register: DBG_EN (0x0030) */
#define ADLAK_REG_GEN3B_PS_CORE_DBG_EN 0x0030
typedef union {
    uint32_t all;
    struct {
        uint32_t dbg_en : 1;
        uint32_t reserved_1_31 : 31;
    };
} adlak_reg_gen3b_ps_core_dbg_en_t;

/* Register: DBG_SEL (0x0034) */
#define ADLAK_REG_GEN3B_PS_CORE_DBG_SEL 0x0034
typedef union {
    uint32_t all;
    struct {
        uint32_t dbg_mdl_sel : 8;
        uint32_t reserved_8_31 : 24;
    };
} adlak_reg_gen3b_ps_core_dbg_sel_t;

/* Register: DBG_SUB_SEL (0x0038) */
#define ADLAK_REG_GEN3B_PS_CORE_DBG_SUB_SEL 0x0038
typedef union {
    uint32_t all;
    struct {
        uint32_t dbg_sub_sel : 16;
        uint32_t reserved_16_31 : 16;
    };
} adlak_reg_gen3b_ps_core_dbg_sub_sel_t;

/* Register: DBG_DAT (0x003C) */
#define ADLAK_REG_GEN3B_PS_CORE_DBG_DAT 0x003C
typedef union {
    uint32_t all;
    struct {
        uint32_t dbg_dat : 32;
    };
} adlak_reg_gen3b_ps_core_dbg_dat_t;

/* Register: PS_CTRL (0x0050) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_CTRL 0x0050
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_rst : 1;
        uint32_t ps_dep_rst : 1;
        uint32_t ps_start : 1;
        uint32_t reserved_3_3 : 1;
        uint32_t ps_pend_rst : 1;
        uint32_t reserved_5_31 : 27;
    };
} adlak_reg_gen3b_ps_core_ps_ctrl_t;

/* Register: PS_STS (0x0054) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_STS 0x0054
typedef union {
    uint32_t all;
    struct {
        uint32_t reserved_0_0 : 1;
        uint32_t ps_stop_err : 1;
        uint32_t reserved_2_2 : 1;
        uint32_t ps_stop_sstep : 1;
        uint32_t ps_busy : 1;
        uint32_t reserved_5_5 : 1;
        uint32_t ps_bm_cmd_err : 1;
        uint32_t ps_nq_underflow : 1;
        uint32_t ps_mq_underflow : 1;
        uint32_t reserved_9_31 : 23;
    };
} adlak_reg_gen3b_ps_core_ps_sts_t;

/* Register: PS_ERR_DAT (0x0058) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_ERR_DAT 0x0058
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_error_data : 32;
    };
} adlak_reg_gen3b_ps_core_ps_err_dat_t;

/* Register: PS_IDLE_STS (0x005C) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_IDLE_STS 0x005C
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_idle_sts : 16;
        uint32_t reserved_16_31 : 16;
    };
} adlak_reg_gen3b_ps_core_ps_idle_sts_t;

/* Register: PS_TIME_STAMP (0x0060) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_TIME_STAMP 0x0060
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_time_stamp : 32;
    };
} adlak_reg_gen3b_ps_core_ps_time_stamp_t;

/* Register: PS_RBF_BASE (0x0064) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_RBF_BASE 0x0064
typedef union {
    uint32_t all;
    struct {
        uint32_t reserved_0_31 : 32;
    };
} adlak_reg_gen3b_ps_core_ps_rbf_base_t;

/* Register: PS_RBF_SIZE (0x0068) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_RBF_SIZE 0x0068
typedef union {
    uint32_t all;
    struct {
        uint32_t reserved_0_27 : 28;
        uint32_t reserved_28_31 : 4;
    };
} adlak_reg_gen3b_ps_core_ps_rbf_size_t;

/* Register: PS_RBF_WPT (0x006C) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_RBF_WPT 0x006C
typedef union {
    uint32_t all;
    struct {
        uint32_t reserved_0_27 : 28;
        uint32_t reserved_28_31 : 4;
    };
} adlak_reg_gen3b_ps_core_ps_rbf_wpt_t;

/* Register: PS_RBF_RPT (0x0070) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_RBF_RPT 0x0070
typedef union {
    uint32_t all;
    struct {
        uint32_t reserved_0_27 : 28;
        uint32_t reserved_28_31 : 4;
    };
} adlak_reg_gen3b_ps_core_ps_rbf_rpt_t;

/* Register: PS_RBF_PPT (0x0074) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_RBF_PPT 0x0074
typedef union {
    uint32_t all;
    struct {
        uint32_t reserved_0_31 : 32;
    };
} adlak_reg_gen3b_ps_core_ps_rbf_ppt_t;

/* Register: PS_FINISH_ID (0x0078) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_FINISH_ID 0x0078
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_pwe_flid_dat : 4;
        uint32_t ps_pwx_flid_dat : 4;
        uint32_t ps_rs_flid_dat : 4;
        uint32_t reserved_12_19 : 8;
        uint32_t ps_pwe_flid_vld : 1;
        uint32_t ps_pwx_flid_vld : 1;
        uint32_t ps_rs_flid_vld : 1;
        uint32_t reserved_23_31 : 9;
    };
} adlak_reg_gen3b_ps_core_ps_finish_id_t;

/* Register: PS_HCNT (0x007C) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_HCNT 0x007C
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_pwe_hcnt : 10;
        uint32_t reserved_10_15 : 6;
        uint32_t ps_pwx_hcnt : 10;
        uint32_t reserved_26_31 : 6;
    };
} adlak_reg_gen3b_ps_core_ps_hcnt_t;

/* Register: PS_OST (0x0080) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_OST 0x0080
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_ost_max : 3;
        uint32_t reserved_3_3 : 1;
        uint32_t ps_pwe_outstanding : 3;
        uint32_t reserved_7_7 : 1;
        uint32_t ps_pwx_outstanding : 3;
        uint32_t reserved_11_11 : 1;
        uint32_t ps_rs_outstanding : 3;
        uint32_t reserved_15_31 : 17;
    };
} adlak_reg_gen3b_ps_core_ps_ost_t;

/* Register: PS_PEND_EN (0x0084) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_PEND_EN 0x0084
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_pend_timer_en : 1;
        uint32_t reserved_1_31 : 31;
    };
} adlak_reg_gen3b_ps_core_ps_pend_en_t;

/* Register: PS_PEND_VAL (0x0088) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_PEND_VAL 0x0088
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_pend_timer_val : 32;
    };
} adlak_reg_gen3b_ps_core_ps_pend_val_t;

/* Register: PS_M_IDLE_STS (0x008C) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_M_IDLE_STS 0x008C
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_mdl_idle_sts : 32;
    };
} adlak_reg_gen3b_ps_core_ps_m_idle_sts_t;

/* Register: PS_DBG_SW_ID (0x0090) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_DBG_SW_ID 0x0090
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_dbg_sw_id : 24;
        uint32_t reserved_24_31 : 8;
    };
} adlak_reg_gen3b_ps_core_ps_dbg_sw_id_t;

/* Register: PS_OPTION_0 (0x0094) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_OPTION_0 0x0094
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_force_single_layer : 1;
        uint32_t ps_single_step_en : 1;
        uint32_t _94_dummy_31_2 : 30;
    };
} adlak_reg_gen3b_ps_core_ps_option_0_t;

/* Register: PS_DBG_HW_ID (0x0098) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_DBG_HW_ID 0x0098
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_dbg_hw_id : 24;
        uint32_t reserved_24_31 : 8;
    };
} adlak_reg_gen3b_ps_core_ps_dbg_hw_id_t;

/* Register: AB_AXI_PADDR (0x009C) */
#define ADLAK_REG_GEN3B_PS_CORE_AB_AXI_PADDR 0x009C
typedef union {
    uint32_t all;
    struct {
        uint32_t ab_axi_paddr : 26;
        uint32_t reserved_26_31 : 6;
    };
} adlak_reg_gen3b_ps_core_ab_axi_paddr_t;

/* Register: AB_CTL (0x00A0) */
#define ADLAK_REG_GEN3B_PS_CORE_AB_CTL 0x00A0
typedef union {
    uint32_t all;
    struct {
        uint32_t ab_force_stop_en : 1;
        uint32_t ab_force_stop_idle : 1;
        uint32_t ab_axi_addr_wrap_en : 1;
        uint32_t ab_sram_addr_wrap_en : 1;
        uint32_t reserved_4_7 : 4;
        uint32_t ab_mem_burst_mode : 2;
        uint32_t ab_axi_burst_mode : 2;
        uint32_t ab_resp_err_stop_en : 1;
        uint32_t reserved_13_13 : 1;
        uint32_t ab_part_w_det_mode : 2;
        uint32_t ab_w_mport_mode : 2;
        uint32_t reserved_18_30 : 13;
        uint32_t ab_cmp_cache_dis : 1;
    };
} adlak_reg_gen3b_ps_core_ab_ctl_t;

/* Register: AB_AXI_SADDR (0x00A4) */
#define ADLAK_REG_GEN3B_PS_CORE_AB_AXI_SADDR 0x00A4
typedef union {
    uint32_t all;
    struct {
        uint32_t ab_axi_saddr : 26;
        uint32_t reserved_26_31 : 6;
    };
} adlak_reg_gen3b_ps_core_ab_axi_saddr_t;

/* Register: AB_AXI_EADDR (0x00A8) */
#define ADLAK_REG_GEN3B_PS_CORE_AB_AXI_EADDR 0x00A8
typedef union {
    uint32_t all;
    struct {
        uint32_t ab_axi_eaddr : 26;
        uint32_t reserved_26_31 : 6;
    };
} adlak_reg_gen3b_ps_core_ab_axi_eaddr_t;

/* Register: AB_R_CS_PRIO (0x00AC) */
#define ADLAK_REG_GEN3B_PS_CORE_AB_R_CS_PRIO 0x00AC
typedef union {
    uint32_t all;
    struct {
        uint32_t ab_r_cs_arb_prio : 20;
        uint32_t reserved_20_27 : 8;
        uint32_t ab_r_dec_arb_prio : 4;
    };
} adlak_reg_gen3b_ps_core_ab_r_cs_prio_t;

/* Register: AB_R_LS_PRIO (0x00B0) */
#define ADLAK_REG_GEN3B_PS_CORE_AB_R_LS_PRIO 0x00B0
typedef union {
    uint32_t all;
    struct {
        uint32_t ab_r_ls_arb_prio : 32;
    };
} adlak_reg_gen3b_ps_core_ab_r_ls_prio_t;

/* Register: AB_R_L2_PRIO (0x00B4) */
#define ADLAK_REG_GEN3B_PS_CORE_AB_R_L2_PRIO 0x00B4
typedef union {
    uint32_t all;
    struct {
        uint32_t ab_r_mem_l2_arb_prio : 8;
        uint32_t reserved_8_30 : 23;
        uint32_t ab_r_mem_hp_mode : 1;
    };
} adlak_reg_gen3b_ps_core_ab_r_l2_prio_t;

/* Register: AB_W_PRIO (0x00B8) */
#define ADLAK_REG_GEN3B_PS_CORE_AB_W_PRIO 0x00B8
typedef union {
    uint32_t all;
    struct {
        uint32_t ab_w_arb_prio : 10;
        uint32_t reserved_10_13 : 4;
        uint32_t ab_w_enc_arb_prio : 2;
        uint32_t reserved_16_31 : 16;
    };
} adlak_reg_gen3b_ps_core_ab_w_prio_t;

/* Register: AB_RESP_ERR (0x00BC) */
#define ADLAK_REG_GEN3B_PS_CORE_AB_RESP_ERR 0x00BC
typedef union {
    uint32_t all;
    struct {
        uint32_t ab_mem_rresp : 2;
        uint32_t ab_mem_bresp : 2;
        uint32_t ab_axi_rresp : 2;
        uint32_t ab_axi_bresp : 2;
        uint32_t ab_mem_rresp_pid : 8;
        uint32_t ab_mem_bresp_pid : 4;
        uint32_t ab_axi_rresp_pid : 4;
        uint32_t ab_axi_bresp_pid : 4;
        uint32_t reserved_28_31 : 4;
    };
} adlak_reg_gen3b_ps_core_ab_resp_err_t;

/* Register: SMMU_EN (0x00C0) */
#define ADLAK_REG_GEN3B_PS_CORE_SMMU_EN 0x00C0
typedef union {
    uint32_t all;
    struct {
        uint32_t smmu_en : 1;
        uint32_t reserved_1_7 : 7;
        uint32_t smmu_tlb_l2_safe_mode : 1;
        uint32_t smmu_tlb_l1_all_miss : 1;
        uint32_t smmu_tlb_l2_all_miss : 1;
        uint32_t smmu_walker_l2_all_miss : 1;
        uint32_t reserved_12_31 : 20;
    };
} adlak_reg_gen3b_ps_core_smmu_en_t;

/* Register: SMMU_TTBR_L (0x00C4) */
#define ADLAK_REG_GEN3B_PS_CORE_SMMU_TTBR_L 0x00C4
typedef union {
    uint32_t all;
    struct {
        uint32_t smmu_ttbr_l : 32;
    };
} adlak_reg_gen3b_ps_core_smmu_ttbr_l_t;

/* Register: SMMU_TTBR_H (0x00C8) */
#define ADLAK_REG_GEN3B_PS_CORE_SMMU_TTBR_H 0x00C8
typedef union {
    uint32_t all;
    struct {
        uint32_t smmu_ttbr_h : 32;
    };
} adlak_reg_gen3b_ps_core_smmu_ttbr_h_t;

/* Register: SMMU_PRIO_POW2_0 (0x00CC) */
#define ADLAK_REG_GEN3B_PS_CORE_SMMU_PRIO_POW2_0 0x00CC
typedef union {
    uint32_t all;
    struct {
        uint32_t smmu_tlb_vab_prio_pow2_0 : 32;
    };
} adlak_reg_gen3b_ps_core_smmu_prio_pow2_0_t;

/* Register: SMMU_PRIO_POW2_1 (0x00D0) */
#define ADLAK_REG_GEN3B_PS_CORE_SMMU_PRIO_POW2_1 0x00D0
typedef union {
    uint32_t all;
    struct {
        uint32_t smmu_tlb_vab_prio_pow2_1 : 32;
    };
} adlak_reg_gen3b_ps_core_smmu_prio_pow2_1_t;

/* Register: SMMU_INV_CTL (0x00D4) */
#define ADLAK_REG_GEN3B_PS_CORE_SMMU_INV_CTL 0x00D4
typedef union {
    uint32_t all;
    struct {
        uint32_t smmu_invalid_rdy : 1;
        uint32_t reserved_1_3 : 3;
        uint32_t smmu_invalid_all : 4;
        uint32_t smmu_invalid_one : 4;
        uint32_t smmu_invalid_mode : 1;
        uint32_t reserved_13_31 : 19;
    };
} adlak_reg_gen3b_ps_core_smmu_inv_ctl_t;

/* Register: SMMU_INV_VA (0x00D8) */
#define ADLAK_REG_GEN3B_PS_CORE_SMMU_INV_VA 0x00D8
typedef union {
    uint32_t all;
    struct {
        uint32_t smmu_invalid_addr : 25;
        uint32_t reserved_25_31 : 7;
    };
} adlak_reg_gen3b_ps_core_smmu_inv_va_t;

/* Register: SMMU_DFT (0x00DC) */
#define ADLAK_REG_GEN3B_PS_CORE_SMMU_DFT 0x00DC
typedef union {
    uint32_t all;
    struct {
        uint32_t smmu_dft_pa4kb : 25;
        uint32_t reserved_25_31 : 7;
    };
} adlak_reg_gen3b_ps_core_smmu_dft_t;

/* Register: SMMU_IVD_MDL (0x00E0) */
#define ADLAK_REG_GEN3B_PS_CORE_SMMU_IVD_MDL 0x00E0
typedef union {
    uint32_t all;
    struct {
        uint32_t smmu_ivd_mdl_id : 5;
        uint32_t reserved_5_31 : 27;
    };
} adlak_reg_gen3b_ps_core_smmu_ivd_mdl_t;

/* Register: SMMU_IVD_VA (0x00E4) */
#define ADLAK_REG_GEN3B_PS_CORE_SMMU_IVD_VA 0x00E4
typedef union {
    uint32_t all;
    struct {
        uint32_t smmu_ivd_mdl_va : 32;
    };
} adlak_reg_gen3b_ps_core_smmu_ivd_va_t;

/* Register: SMMU_IVD_VA_MSB (0x00E8) */
#define ADLAK_REG_GEN3B_PS_CORE_SMMU_IVD_VA_MSB 0x00E8
typedef union {
    uint32_t all;
    struct {
        uint32_t smmu_ivd_mdl_va_msb : 5;
        uint32_t reserved_5_31 : 27;
    };
} adlak_reg_gen3b_ps_core_smmu_ivd_va_msb_t;

/* Register: PM_EN (0x00F0) */
#define ADLAK_REG_GEN3B_PS_CORE_PM_EN 0x00F0
typedef union {
    uint32_t all;
    struct {
        uint32_t pm_en : 2;
        uint32_t pm_swrst : 1;
        uint32_t reserved_3_31 : 29;
    };
} adlak_reg_gen3b_ps_core_pm_en_t;

/* Register: PM_RBF_BASE (0x00F4) */
#define ADLAK_REG_GEN3B_PS_CORE_PM_RBF_BASE 0x00F4
typedef union {
    uint32_t all;
    struct {
        uint32_t pm_rbf_base : 29;
        uint32_t reserved_29_31 : 3;
    };
} adlak_reg_gen3b_ps_core_pm_rbf_base_t;

/* Register: PM_RBF_SIZE (0x00F8) */
#define ADLAK_REG_GEN3B_PS_CORE_PM_RBF_SIZE 0x00F8
typedef union {
    uint32_t all;
    struct {
        uint32_t pm_rbf_size : 20;
        uint32_t reserved_20_31 : 12;
    };
} adlak_reg_gen3b_ps_core_pm_rbf_size_t;

/* Register: PM_RBF_WPT (0x00FC) */
#define ADLAK_REG_GEN3B_PS_CORE_PM_RBF_WPT 0x00FC
typedef union {
    uint32_t all;
    struct {
        uint32_t pm_rbf_wpt_ofst : 20;
        uint32_t reserved_20_31 : 12;
    };
} adlak_reg_gen3b_ps_core_pm_rbf_wpt_t;

/* Register: PM_RBF_RPT (0x0100) */
#define ADLAK_REG_GEN3B_PS_CORE_PM_RBF_RPT 0x0100
typedef union {
    uint32_t all;
    struct {
        uint32_t pm_rbf_rpt_ofst : 20;
        uint32_t reserved_20_31 : 12;
    };
} adlak_reg_gen3b_ps_core_pm_rbf_rpt_t;

/* Register: PM_STS (0x0104) */
#define ADLAK_REG_GEN3B_PS_CORE_PM_STS 0x0104
typedef union {
    uint32_t all;
    struct {
        uint32_t pm_flush : 1;
        uint32_t pm_fifo_empty : 1;
        uint32_t reserved_2_31 : 30;
    };
} adlak_reg_gen3b_ps_core_pm_sts_t;

/* Register: PM_UNIT (0x0108) */
#define ADLAK_REG_GEN3B_PS_CORE_PM_UNIT 0x0108
typedef union {
    uint32_t all;
    struct {
        uint32_t pm_ddr_unit : 2;
        uint32_t pm_sram_unit : 2;
        uint32_t pm_chk_addr : 1;
        uint32_t reserved_5_31 : 27;
    };
} adlak_reg_gen3b_ps_core_pm_unit_t;

/* Register: PM_BWCNT0 (0x0110) */
#define ADLAK_REG_GEN3B_PS_CORE_PM_BWCNT0 0x0110
typedef union {
    uint32_t all;
    struct {
        uint32_t pm_axi_ddr_rd_bwcnt : 16;
        uint32_t pm_axi_ddr_wr_bwcnt : 16;
    };
} adlak_reg_gen3b_ps_core_pm_bwcnt0_t;

/* Register: PM_BWCNT1 (0x0114) */
#define ADLAK_REG_GEN3B_PS_CORE_PM_BWCNT1 0x0114
typedef union {
    uint32_t all;
    struct {
        uint32_t pm_sm_rd_bwcnt : 16;
        uint32_t pm_pm_wr_bwcnt : 16;
    };
} adlak_reg_gen3b_ps_core_pm_bwcnt1_t;

/* Register: PM_PORT_EN_0 (0x0118) */
#define ADLAK_REG_GEN3B_PS_CORE_PM_PORT_EN_0 0x0118
typedef union {
    uint32_t all;
    struct {
        uint32_t pm_port_en_0 : 32;
    };
} adlak_reg_gen3b_ps_core_pm_port_en_0_t;

/* Register: PM_PORT_EN_1 (0x011C) */
#define ADLAK_REG_GEN3B_PS_CORE_PM_PORT_EN_1 0x011C
typedef union {
    uint32_t all;
    struct {
        uint32_t pm_port_en_1 : 32;
    };
} adlak_reg_gen3b_ps_core_pm_port_en_1_t;

/* Register: MC_CTL (0x0120) */
#define ADLAK_REG_GEN3B_PS_CORE_MC_CTL 0x0120
typedef union {
    uint32_t all;
    struct {
        uint32_t mc_ctl : 32;
    };
} adlak_reg_gen3b_ps_core_mc_ctl_t;

/* Register: PS_RDMA_N_BASE (0x0140) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_RDMA_N_BASE 0x0140
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_rdma_n_base : 32;
    };
} adlak_reg_gen3b_ps_core_ps_rdma_n_base_t;

/* Register: PS_RDMA_N_WPT (0x0144) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_RDMA_N_WPT 0x0144
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_rdma_n_wpt_ofst : 28;
        uint32_t reserved_28_31 : 4;
    };
} adlak_reg_gen3b_ps_core_ps_rdma_n_wpt_t;

/* Register: PS_RDMA_N_RPT (0x0148) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_RDMA_N_RPT 0x0148
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_rdma_n_rpt_ofst : 28;
        uint32_t reserved_28_31 : 4;
    };
} adlak_reg_gen3b_ps_core_ps_rdma_n_rpt_t;

/* Register: PS_RDMA_M_BASE (0x014C) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_RDMA_M_BASE 0x014C
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_rdma_m_base : 32;
    };
} adlak_reg_gen3b_ps_core_ps_rdma_m_base_t;

/* Register: PS_RDMA_M_WPT (0x0150) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_RDMA_M_WPT 0x0150
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_rdma_m_wpt_ofst : 28;
        uint32_t reserved_28_31 : 4;
    };
} adlak_reg_gen3b_ps_core_ps_rdma_m_wpt_t;

/* Register: PS_RDMA_M_RPT (0x0154) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_RDMA_M_RPT 0x0154
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_rdma_m_rpt_ofst : 28;
        uint32_t reserved_28_31 : 4;
    };
} adlak_reg_gen3b_ps_core_ps_rdma_m_rpt_t;

/* Register: PS_MRG_N_BASE (0x0158) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_MRG_N_BASE 0x0158
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_mrg_n_base : 32;
    };
} adlak_reg_gen3b_ps_core_ps_mrg_n_base_t;

/* Register: PS_MRG_N_PPT (0x015C) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_MRG_N_PPT 0x015C
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_mrg_n_ppt : 32;
    };
} adlak_reg_gen3b_ps_core_ps_mrg_n_ppt_t;

/* Register: PS_MRG_M_BASE (0x0160) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_MRG_M_BASE 0x0160
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_mrg_m_base : 32;
    };
} adlak_reg_gen3b_ps_core_ps_mrg_m_base_t;

/* Register: PS_MRG_M_PPT (0x0164) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_MRG_M_PPT 0x0164
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_mrg_m_ppt : 32;
    };
} adlak_reg_gen3b_ps_core_ps_mrg_m_ppt_t;

/* Register: PS_DEC_BASE (0x0168) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_DEC_BASE 0x0168
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_dec_base : 32;
    };
} adlak_reg_gen3b_ps_core_ps_dec_base_t;

/* Register: PS_DEC_PPT (0x016C) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_DEC_PPT 0x016C
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_dec_ppt : 32;
    };
} adlak_reg_gen3b_ps_core_ps_dec_ppt_t;

/* Register: PS_BM_ERR_DAT (0x0170) */
#define ADLAK_REG_GEN3B_PS_CORE_PS_BM_ERR_DAT 0x0170
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_bm_error_data : 32;
    };
} adlak_reg_gen3b_ps_core_ps_bm_err_dat_t;

/* Register: AB_SRAM_SADDR (0x0190) */
#define ADLAK_REG_GEN3B_PS_CORE_AB_SRAM_SADDR 0x0190
typedef union {
    uint32_t all;
    struct {
        uint32_t ab_sram_saddr : 26;
        uint32_t reserved_26_31 : 6;
    };
} adlak_reg_gen3b_ps_core_ab_sram_saddr_t;

/* Register: AB_SRAM_EADDR (0x0194) */
#define ADLAK_REG_GEN3B_PS_CORE_AB_SRAM_EADDR 0x0194
typedef union {
    uint32_t all;
    struct {
        uint32_t ab_sram_eaddr : 26;
        uint32_t reserved_26_31 : 6;
    };
} adlak_reg_gen3b_ps_core_ab_sram_eaddr_t;

/* Register: AB_SRAM_PADDR (0x0198) */
#define ADLAK_REG_GEN3B_PS_CORE_AB_SRAM_PADDR 0x0198
typedef union {
    uint32_t all;
    struct {
        uint32_t ab_sram_paddr : 26;
        uint32_t reserved_26_31 : 6;
    };
} adlak_reg_gen3b_ps_core_ab_sram_paddr_t;

/* Register: DBUF_CTL (0x01A0) */
#define ADLAK_REG_GEN3B_PS_CORE_DBUF_CTL 0x01A0
typedef union {
    uint32_t all;
    struct {
        uint32_t dw_dbuf_clear : 1;
        uint32_t dw_dbuf_clear_done : 1;
        uint32_t reserved_2_31 : 30;
    };
} adlak_reg_gen3b_ps_core_dbuf_ctl_t;

/* Register: CBUF_CTL (0x01A4) */
#define ADLAK_REG_GEN3B_PS_CORE_CBUF_CTL 0x01A4
typedef union {
    uint32_t all;
    struct {
        uint32_t mc_cbuf_clear : 16;
        uint32_t mc_cbuf_clear_done : 16;
    };
} adlak_reg_gen3b_ps_core_cbuf_ctl_t;

#endif  // __ADLAK_REG_GEN3B_PS_CORE_H__