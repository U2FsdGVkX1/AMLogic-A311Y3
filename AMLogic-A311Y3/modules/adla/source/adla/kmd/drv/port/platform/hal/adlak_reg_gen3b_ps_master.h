/**
 * Copyright: (C) 2025 Amlogic, Inc. All rights reserved.
 */

#ifndef __ADLAK_REG_GEN3B_PS_MASTER_H__
#define __ADLAK_REG_GEN3B_PS_MASTER_H__

#include "adlak_typedef.h"

/* Generated from sheet: ADLAM */

/* Register: REV (0x0000) */
#define ADLAK_REG_GEN3B_PS_MASTER_REV 0x0000
typedef union {
    uint32_t all;
    struct {
        uint32_t minor_rev : 8;
        uint32_t major_rev : 8;
        uint32_t rev0_rev : 8;
        uint32_t rev1_rev : 8;
    };
} adlak_reg_gen3b_ps_master_rev_t;

/* Register: WAIT_TIMER (0x0004) */
#define ADLAK_REG_GEN3B_PS_MASTER_WAIT_TIMER 0x0004
typedef union {
    uint32_t all;
    struct {
        uint32_t wait_timer_val : 16;
        uint32_t reserved_16_31 : 16;
    };
} adlak_reg_gen3b_ps_master_wait_timer_t;

/* Register: SECURE_MODE (0x0008) */
#define ADLAK_REG_GEN3B_PS_MASTER_SECURE_MODE 0x0008
typedef union {
    uint32_t all;
    struct {
        uint32_t security_mode : 1;
        uint32_t reserved_1_31 : 31;
    };
} adlak_reg_gen3b_ps_master_secure_mode_t;

/* Register: IRQ_MASKED (0x0010) */
#define ADLAK_REG_GEN3B_PS_MASTER_IRQ_MASKED 0x0010
typedef union {
    uint32_t all;
    struct {
        uint32_t irqsts_masked : 32;
    };
} adlak_reg_gen3b_ps_master_irq_masked_t;

/* Register: IRQ_MASK (0x0014) */
#define ADLAK_REG_GEN3B_PS_MASTER_IRQ_MASK 0x0014
typedef union {
    uint32_t all;
    struct {
        uint32_t irqsts_mask : 32;
    };
} adlak_reg_gen3b_ps_master_irq_mask_t;

/* Register: IRQ_RAW (0x0018) */
#define ADLAK_REG_GEN3B_PS_MASTER_IRQ_RAW 0x0018
typedef union {
    uint32_t all;
    struct {
        uint32_t irqsts_raw : 24;
        uint32_t reserved_24_31 : 8;
    };
} adlak_reg_gen3b_ps_master_irq_raw_t;

/* Register: STS_REPORT (0x001C) */
#define ADLAK_REG_GEN3B_PS_MASTER_STS_REPORT 0x001C
typedef union {
    uint32_t all;
    struct {
        uint32_t sts_report : 32;
    };
} adlak_reg_gen3b_ps_master_sts_report_t;

/* Register: SWRST (0x0020) */
#define ADLAK_REG_GEN3B_PS_MASTER_SWRST 0x0020
typedef union {
    uint32_t all;
    struct {
        uint32_t adla_swrst : 1;
        uint32_t reserved_1_31 : 31;
    };
} adlak_reg_gen3b_ps_master_swrst_t;

/* Register: ADLA_EN (0x0024) */
#define ADLAK_REG_GEN3B_PS_MASTER_ADLA_EN 0x0024
typedef union {
    uint32_t all;
    struct {
        uint32_t adla_en : 1;
        uint32_t reserved_1_31 : 31;
    };
} adlak_reg_gen3b_ps_master_adla_en_t;

/* Register: CLK_AUTOCLK (0x0028) */
#define ADLAK_REG_GEN3B_PS_MASTER_CLK_AUTOCLK 0x0028
typedef union {
    uint32_t all;
    struct {
        uint32_t adla_autoclk_en : 32;
    };
} adlak_reg_gen3b_ps_master_clk_autoclk_t;

/* Register: CLK_IDLE_CNT (0x002C) */
#define ADLAK_REG_GEN3B_PS_MASTER_CLK_IDLE_CNT 0x002C
typedef union {
    uint32_t all;
    struct {
        uint32_t adla_autoclk_idle_cnt : 5;
        uint32_t reserved_5_7 : 3;
        uint32_t adla_autoclk_busy_cnt : 5;
        uint32_t reserved_13_31 : 19;
    };
} adlak_reg_gen3b_ps_master_clk_idle_cnt_t;

/* Register: DBG_EN (0x0030) */
#define ADLAK_REG_GEN3B_PS_MASTER_DBG_EN 0x0030
typedef union {
    uint32_t all;
    struct {
        uint32_t dbg_en : 1;
        uint32_t reserved_1_31 : 31;
    };
} adlak_reg_gen3b_ps_master_dbg_en_t;

/* Register: DBG_SEL (0x0034) */
#define ADLAK_REG_GEN3B_PS_MASTER_DBG_SEL 0x0034
typedef union {
    uint32_t all;
    struct {
        uint32_t dbg_mdl_sel : 8;
        uint32_t reserved_8_31 : 24;
    };
} adlak_reg_gen3b_ps_master_dbg_sel_t;

/* Register: DBG_SUB_SEL (0x0038) */
#define ADLAK_REG_GEN3B_PS_MASTER_DBG_SUB_SEL 0x0038
typedef union {
    uint32_t all;
    struct {
        uint32_t dbg_sub_sel : 16;
        uint32_t reserved_16_31 : 16;
    };
} adlak_reg_gen3b_ps_master_dbg_sub_sel_t;

/* Register: DBG_DAT (0x003C) */
#define ADLAK_REG_GEN3B_PS_MASTER_DBG_DAT 0x003C
typedef union {
    uint32_t all;
    struct {
        uint32_t dbg_dat : 32;
    };
} adlak_reg_gen3b_ps_master_dbg_dat_t;

/* Register: PS_CTRL (0x0050) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_CTRL 0x0050
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_rst : 1;
        uint32_t ps_dep_rst : 1;
        uint32_t ps_start : 1;
        uint32_t ps_preempt : 1;
        uint32_t ps_pend_rst : 1;
        uint32_t reserved_5_31 : 27;
    };
} adlak_reg_gen3b_ps_master_ps_ctrl_t;

/* Register: PS_STS (0x0054) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_STS 0x0054
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_stop_cmd : 1;
        uint32_t ps_stop_err : 1;
        uint32_t ps_stop_pmt : 1;
        uint32_t ps_stop_sstep : 1;
        uint32_t ps_busy : 1;
        uint32_t ps_preempt_busy : 1;
        uint32_t ps_loop_uoflow : 5;
        uint32_t reserved_11_31 : 21;
    };
} adlak_reg_gen3b_ps_master_ps_sts_t;

/* Register: PS_ERR_DAT (0x0058) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_ERR_DAT 0x0058
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_error_data : 32;
    };
} adlak_reg_gen3b_ps_master_ps_err_dat_t;

/* Register: PS_IDLE_STS (0x005C) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_IDLE_STS 0x005C
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_idle_sts : 16;
        uint32_t reserved_16_31 : 16;
    };
} adlak_reg_gen3b_ps_master_ps_idle_sts_t;

/* Register: PS_TIME_STAMP (0x0060) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_TIME_STAMP 0x0060
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_time_stamp : 32;
    };
} adlak_reg_gen3b_ps_master_ps_time_stamp_t;

/* Register: PS_RBF_BASE (0x0064) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_RBF_BASE 0x0064
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_rbf_base : 32;
    };
} adlak_reg_gen3b_ps_master_ps_rbf_base_t;

/* Register: PS_RBF_SIZE (0x0068) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_RBF_SIZE 0x0068
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_rbf_size : 28;
        uint32_t reserved_28_31 : 4;
    };
} adlak_reg_gen3b_ps_master_ps_rbf_size_t;

/* Register: PS_RBF_WPT (0x006C) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_RBF_WPT 0x006C
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_rbf_wpt_ofst : 28;
        uint32_t reserved_28_31 : 4;
    };
} adlak_reg_gen3b_ps_master_ps_rbf_wpt_t;

/* Register: PS_RBF_RPT (0x0070) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_RBF_RPT 0x0070
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_rbf_rpt_ofst : 28;
        uint32_t reserved_28_31 : 4;
    };
} adlak_reg_gen3b_ps_master_ps_rbf_rpt_t;

/* Register: PS_RBF_PPT (0x0074) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_RBF_PPT 0x0074
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_rbf_ppt_ofst : 28;
        uint32_t reserved_28_31 : 4;
    };
} adlak_reg_gen3b_ps_master_ps_rbf_ppt_t;

/* Register: PS_FINISH_ID_0 (0x0078) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_FINISH_ID_0 0x0078
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_core0_fgid_dat : 8;
        uint32_t ps_core1_fgid_dat : 8;
        uint32_t ps_core2_fgid_dat : 8;
        uint32_t ps_core3_fgid_dat : 8;
    };
} adlak_reg_gen3b_ps_master_ps_finish_id_0_t;

/* Register: PS_FINISH_ID_1 (0x007C) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_FINISH_ID_1 0x007C
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_core4_fgid_dat : 8;
        uint32_t ps_core5_fgid_dat : 8;
        uint32_t ps_core6_fgid_dat : 8;
        uint32_t ps_core7_fgid_dat : 8;
    };
} adlak_reg_gen3b_ps_master_ps_finish_id_1_t;

/* Register: PS_OST (0x0080) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_OST 0x0080
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_core0_outstanding : 4;
        uint32_t ps_core1_outstanding : 4;
        uint32_t ps_core2_outstanding : 4;
        uint32_t ps_core3_outstanding : 4;
        uint32_t ps_core4_outstanding : 4;
        uint32_t ps_core5_outstanding : 4;
        uint32_t ps_core6_outstanding : 4;
        uint32_t ps_core7_outstanding : 4;
    };
} adlak_reg_gen3b_ps_master_ps_ost_t;

/* Register: PS_PEND_EN (0x0084) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_PEND_EN 0x0084
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_pend_timer_en : 1;
        uint32_t reserved_1_31 : 31;
    };
} adlak_reg_gen3b_ps_master_ps_pend_en_t;

/* Register: PS_PEND_VAL (0x0088) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_PEND_VAL 0x0088
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_pend_timer_val : 32;
    };
} adlak_reg_gen3b_ps_master_ps_pend_val_t;

/* Register: PS_M_IDLE_STS (0x008C) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_M_IDLE_STS 0x008C
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_mdl_idle_sts : 32;
    };
} adlak_reg_gen3b_ps_master_ps_m_idle_sts_t;

/* Register: PS_DBG_SW_GID (0x0090) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_DBG_SW_GID 0x0090
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_dbg_sw_gid : 24;
        uint32_t reserved_24_31 : 8;
    };
} adlak_reg_gen3b_ps_master_ps_dbg_sw_gid_t;

/* Register: PS_OPTION_0 (0x0094) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_OPTION_0 0x0094
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_force_single_group : 1;
        uint32_t ps_single_step_en : 1;
        uint32_t _94_dummy_31_2 : 30;
    };
} adlak_reg_gen3b_ps_master_ps_option_0_t;

/* Register: PS_DBG_HW_GID (0x0098) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_DBG_HW_GID 0x0098
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_dbg_hw_gid : 24;
        uint32_t reserved_24_31 : 8;
    };
} adlak_reg_gen3b_ps_master_ps_dbg_hw_gid_t;

/* Register: PS_FINISH_ID_2 (0x009C) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_FINISH_ID_2 0x009C
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_core0_fgid_vld : 1;
        uint32_t ps_core1_fgid_vld : 1;
        uint32_t ps_core2_fgid_vld : 1;
        uint32_t ps_core3_fgid_vld : 1;
        uint32_t ps_core4_fgid_vld : 1;
        uint32_t ps_core5_fgid_vld : 1;
        uint32_t ps_core6_fgid_vld : 1;
        uint32_t ps_core7_fgid_vld : 1;
        uint32_t reserved_8_27 : 20;
        uint32_t ps_ost_max : 4;
    };
} adlak_reg_gen3b_ps_master_ps_finish_id_2_t;

/* Register: HB_CTL (0x00A0) */
#define ADLAK_REG_GEN3B_PS_MASTER_HB_CTL 0x00A0
typedef union {
    uint32_t all;
    struct {
        uint32_t hb_clear : 1;
        uint32_t hb_clear_done : 1;
        uint32_t reserved_2_31 : 30;
    };
} adlak_reg_gen3b_ps_master_hb_ctl_t;

/* Register: HB_PRIO (0x00A4) */
#define ADLAK_REG_GEN3B_PS_MASTER_HB_PRIO 0x00A4
typedef union {
    uint32_t all;
    struct {
        uint32_t hb_l1_araw_arb_prio : 8;
        uint32_t reserved_8_31 : 24;
    };
} adlak_reg_gen3b_ps_master_hb_prio_t;

/* Register: PS_LOOP_LEVEL (0x0140) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_LOOP_LEVEL 0x0140
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_loop_level : 2;
        uint32_t reserved_2_15 : 14;
        uint32_t ps_loop_cnt1 : 16;
    };
} adlak_reg_gen3b_ps_master_ps_loop_level_t;

/* Register: PS_LOOP_CNT (0x0144) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_LOOP_CNT 0x0144
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_loop_cnt2 : 16;
        uint32_t ps_loop_cnt3 : 16;
    };
} adlak_reg_gen3b_ps_master_ps_loop_cnt_t;

/* Register: PS_SKIP (0x0148) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_SKIP 0x0148
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_skip_cnt : 8;
        uint32_t reserved_8_31 : 24;
    };
} adlak_reg_gen3b_ps_master_ps_skip_t;

/* Register: PS_CONDITION (0x014C) */
#define ADLAK_REG_GEN3B_PS_MASTER_PS_CONDITION 0x014C
typedef union {
    uint32_t all;
    struct {
        uint32_t ps_cond_sts : 1;
        uint32_t reserved_1_31 : 31;
    };
} adlak_reg_gen3b_ps_master_ps_condition_t;

/* Register: LK_CTL (0x0150) */
#define ADLAK_REG_GEN3B_PS_MASTER_LK_CTL 0x0150
typedef union {
    uint32_t all;
    struct {
        uint32_t lk_en : 1;
        uint32_t reserved_1_15 : 15;
        uint32_t lk_isp_autoclk_en : 4;
        uint32_t reserved_20_31 : 12;
    };
} adlak_reg_gen3b_ps_master_lk_ctl_t;

/* Register: LK_RST (0x0154) */
#define ADLAK_REG_GEN3B_PS_MASTER_LK_RST 0x0154
typedef union {
    uint32_t all;
    struct {
        uint32_t lk_rst : 1;
        uint32_t reserved_1_31 : 31;
    };
} adlak_reg_gen3b_ps_master_lk_rst_t;

/* Register: LK_ERROR (0x0158) */
#define ADLAK_REG_GEN3B_PS_MASTER_LK_ERROR 0x0158
typedef union {
    uint32_t all;
    struct {
        uint32_t lk_rx_t_rd_frame_ovf : 1;
        uint32_t lk_tx_t_wr_frame_ovf : 1;
        uint32_t lk_rx_s_rd_frame_ovf : 1;
        uint32_t lk_tx_s_wr_frame_ovf : 1;
        uint32_t reserved_4_31 : 28;
    };
} adlak_reg_gen3b_ps_master_lk_error_t;

/* Register: LK_RX_T_WR_ID (0x015C) */
#define ADLAK_REG_GEN3B_PS_MASTER_LK_RX_T_WR_ID 0x015C
typedef union {
    uint32_t all;
    struct {
        uint32_t lk_rx_t_wr_tile_id : 24;
        uint32_t lk_rx_t_wr_frame_id : 8;
    };
} adlak_reg_gen3b_ps_master_lk_rx_t_wr_id_t;

/* Register: LK_RX_T_RD_ID (0x0160) */
#define ADLAK_REG_GEN3B_PS_MASTER_LK_RX_T_RD_ID 0x0160
typedef union {
    uint32_t all;
    struct {
        uint32_t lk_rx_t_rd_tile_id : 24;
        uint32_t lk_rx_t_rd_frame_id : 8;
    };
} adlak_reg_gen3b_ps_master_lk_rx_t_rd_id_t;

/* Register: LK_TX_T_WR_ID (0x0164) */
#define ADLAK_REG_GEN3B_PS_MASTER_LK_TX_T_WR_ID 0x0164
typedef union {
    uint32_t all;
    struct {
        uint32_t lk_tx_t_wr_tile_id : 24;
        uint32_t lk_tx_t_wr_frame_id : 8;
    };
} adlak_reg_gen3b_ps_master_lk_tx_t_wr_id_t;

/* Register: LK_TX_T_RD_ID (0x0168) */
#define ADLAK_REG_GEN3B_PS_MASTER_LK_TX_T_RD_ID 0x0168
typedef union {
    uint32_t all;
    struct {
        uint32_t lk_tx_t_rd_tile_id : 24;
        uint32_t lk_tx_t_rd_frame_id : 8;
    };
} adlak_reg_gen3b_ps_master_lk_tx_t_rd_id_t;

/* Register: LK_RX_S_WR_ID (0x016C) */
#define ADLAK_REG_GEN3B_PS_MASTER_LK_RX_S_WR_ID 0x016C
typedef union {
    uint32_t all;
    struct {
        uint32_t lk_rx_s_wr_tile_id : 24;
        uint32_t lk_rx_s_wr_frame_id : 8;
    };
} adlak_reg_gen3b_ps_master_lk_rx_s_wr_id_t;

/* Register: LK_RX_S_RD_ID (0x0170) */
#define ADLAK_REG_GEN3B_PS_MASTER_LK_RX_S_RD_ID 0x0170
typedef union {
    uint32_t all;
    struct {
        uint32_t lk_rx_s_rd_tile_id : 24;
        uint32_t lk_rx_s_rd_frame_id : 8;
    };
} adlak_reg_gen3b_ps_master_lk_rx_s_rd_id_t;

/* Register: LK_TX_S_WR_ID (0x0174) */
#define ADLAK_REG_GEN3B_PS_MASTER_LK_TX_S_WR_ID 0x0174
typedef union {
    uint32_t all;
    struct {
        uint32_t lk_tx_s_wr_tile_id : 24;
        uint32_t lk_tx_s_wr_frame_id : 8;
    };
} adlak_reg_gen3b_ps_master_lk_tx_s_wr_id_t;

/* Register: LK_TX_S_RD_ID (0x0178) */
#define ADLAK_REG_GEN3B_PS_MASTER_LK_TX_S_RD_ID 0x0178
typedef union {
    uint32_t all;
    struct {
        uint32_t lk_tx_s_rd_tile_id : 24;
        uint32_t lk_tx_s_rd_frame_id : 8;
    };
} adlak_reg_gen3b_ps_master_lk_tx_s_rd_id_t;

#endif  // __ADLAK_REG_GEN3B_PS_MASTER_H__