/**
 ****************************************************************************************
 *
 * @file aml_msg_rx.h
 *
 * @brief RX function declarations
 *
 * Copyright (C) Amlogic 2012-2021
 *
 ****************************************************************************************
 */

#ifndef _AML_MSG_RX_H_
#define _AML_MSG_RX_H_

#define MAC_RS_LINK_LOSS_DISCONNECT              40

#define COEX_BT_IN_A2DP_STREAM BIT(15)
#define COEX_BT_ACL_WORK_FLAG  BIT(18)
#define COEX_BT_IN_SLAVE_MODE  BIT(19)
#define COEX_BT_IN_ESCO_MODE   BIT(21)
#define COEX_BT_SINK_FLAG      BIT(22)
#define COEX_BT_BLE_WORK_FLAG  BIT(25)
#define COEX_BT_REQ_TDD_FLAG   BIT(26)

#define COEX_15P4_ASSOCIATING  BIT(30)
#define COEX_15P4_REQ_TDD_FLAG BIT(26)

struct aml_ft_auth_timeout {
    uint8_t vif_idx;
};

struct resume_sync_ptr
{
    u32_l hw_rd;
};

void aml_rx_handle_msg(struct aml_hw *aml_hw, struct ipc_e2a_msg *msg);
void aml_rx_sdio_ind_msg_handle(struct aml_hw *aml_hw, struct ipc_e2a_msg *msg);
void aml_del_sta(struct aml_vif *aml_vif, const u8 *mac_addr, u32 freq);
int aml_send_me_shutdown(struct aml_hw *aml_hw);
void aml_sta_notify_csa_ch_switch(struct aml_hw *aml_hw, struct ipc_e2a_msg *msg);
int aml_rx_sm_disconnect_handler(struct aml_hw *aml_hw, struct sm_disconnect_ind *ind);

#endif /* _AML_MSG_RX_H_ */
