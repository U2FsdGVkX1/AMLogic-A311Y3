/*******************************************************************************
 * Copyright (C) 2022 Amlogic, Inc. All rights reserved.
 ******************************************************************************/

/*****************************************************************************/
/**
 *
 * @file adlak_platform_module_param.c
 * @brief
 *
 * <pre>
 * MODIFICATION HISTORY:
 *
 * Ver   	Who				Date				Changes
 * ----------------------------------------------------------------------------
 * 1.00a shiwei.sun@amlogic.com	2022/09/22	Initial release
 * </pre>
 *
 ******************************************************************************/

module_param_named(has_smmu, adlak_has_smmu, int, 0644);
MODULE_PARM_DESC(has_smmu, "[ignored if device tree enabled]has smmu");

module_param_named(axi_freq, adlak_axi_freq, int, 0644);
MODULE_PARM_DESC(axi_freq, "max number of the axi clock in Hz");

module_param_named(core_freq, adlak_core_freq, int, 0644);
MODULE_PARM_DESC(core_freq, "max number of the core clock in Hz");

module_param_named(sch_time, adlak_sch_time_max_ms, int, 0440);
MODULE_PARM_DESC(sch_time, "soft-timeout per layer");

module_param_named(dpm_period, adlak_dpm_period, int, 0644);
MODULE_PARM_DESC(dpm_period, "the check-period of the dynamic power management in ms");

module_param_named(log_level, adlak_log_level, int, 0644);
MODULE_PARM_DESC(log_level, "the default log_level of kmd");

module_param_named(share_swap, adlak_share_swap, uint, 0644);
MODULE_PARM_DESC(share_swap, "share swap buffer between different models, disabled by default");

module_param_named(share_buf_size, adlak_share_buf_size, uint, 0644);
MODULE_PARM_DESC(share_buf_size, "share swap buffer size");

module_param_named(smmu_iova_size, adlak_smmu_iova_size, uint, 0644);
MODULE_PARM_DESC(smmu_iova_size,
                 "the iova size which supported by smmu,the unit is GByte,default value is 2");
