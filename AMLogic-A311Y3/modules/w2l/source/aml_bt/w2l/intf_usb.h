/*
* Copyright (c) 202X Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*/

#ifndef __INTF_USB_H__
#define __INTF_USB_H__

#define GDSL_TX_Q_USED          (1)
#define GDSL_TX_Q_COMPLETE      (2)
#define GDSL_TX_Q_UNUSED        (0)

#define SRAM_FD_INIT_FLAG       (1 << 1)

enum bt_drv_state
{
    BT_DRV_NONE,
    BT_DRV_CLOSED,
    BT_DRV_SUSPEND_ENTRY,
    BT_DRV_SUSPEND,
    BT_DRV_RESUME_ENTRY,
    BT_DRV_RESUME,
    BT_DRV_WAIT_RECOVERY
};

enum wifi_cmd {
    CMD_DOWNLOAD_WIFI = 0xC1,
    CMD_START_WIFI,
    CMD_STOP_WIFI,
    CMD_READ_REG,
    CMD_WRITE_REG,
    CMD_READ_PACKET,
    CMD_WRITE_PACKET,
    CMD_WRITE_SRAM,
    CMD_READ_SRAM,
    CMD_DOWNLOAD_BT,
    CMD_GET_TX_CFM,
    CMD_OTHER_CMD,
    CMD_USB_IRQ
};

#define AML_SIG_CBW             0x43425355
#define AML_XFER_TO_DEVICE      0
#define AML_XFER_TO_HOST        0x80
#define AML_USB_CONTROL_MSG_TIMEOUT 3000
//wait usb recovery time 10s
#define MAX_TIMEOUT             10000000000

//bluez param
//Transmit states
#define XMIT_SENDING  1
#define XMIT_WAKEUP   2
#define XMIT_WAITING  8

//Receiver states
#define RECV_WAIT_PACKET_TYPE   0
#define RECV_WAIT_EVENT_HEADER  1
#define RECV_WAIT_ACL_HEADER    2
#define RECV_WAIT_SCO_HEADER    3
#define RECV_WAIT_DATA          4


//static struct mutex bt_usb_mutex;
extern struct mutex auc_usb_mutex;

#define USB_BEGIN_LOCK() do {\
    mutex_lock(&auc_usb_mutex);\
} while (0)

#define USB_END_LOCK() do {\
    mutex_unlock(&auc_usb_mutex);\
} while (0)


struct crg_msc_cbw {
    unsigned int sig;
    unsigned int tag;
    unsigned int data_len;
    unsigned char flag;
    unsigned char lun;
    unsigned char len;
    unsigned int cdb[4];
    unsigned char resv[481];
}__attribute__ ((packed));

enum bt_polling_interval{
    POLLING_LEVEL_1 = 1000000,
    POLLING_LEVEL_2 = 5000000,
    POLLING_LEVEL_3 = 8000000,
};

extern struct usb_device *g_udev;
extern struct crg_msc_cbw *g_cmd_buf;

int amlbt_intf_usb_check(void);
int amlbt_intf_usb_write_word(unsigned int addr,unsigned int data, unsigned int ep);
int amlbt_intf_usb_read_word(unsigned int addr, unsigned int ep, unsigned int *value);
int amlbt_intf_usb_write_sram(unsigned char *buf, unsigned char *sram_addr, unsigned int len, unsigned int ep);
int amlbt_intf_usb_read_sram(unsigned char *buf,unsigned char *sram_addr, unsigned int len, unsigned int ep);
void amlbt_intf_usb_exception_func(struct work_struct *work);
void amlbt_intf_usb_rx_work(struct work_struct *work);
int amlbt_intf_usb_send_hci_cmd(amlbt_t *p_bt, unsigned char *data, unsigned int len);
int amlbt_intf_usb_send_hci_data(amlbt_t *p_bt, unsigned char *data, unsigned int len);
int amlbt_intf_usb_send_15p4_data(amlbt_t *p_bt,unsigned char *data, unsigned int len);
void amlbt_intf_usb_write_work(amlbt_t *p_bt);
#if defined(CONFIG_AML_BT_CHIP_W1D)
int amlbt_intf_usb_shutdown(amlbt_t *p_bt);
#endif
int amlbt_hci_register_dev(amlbt_t *p_bt);
int amlbt_hci_dev_init(struct platform_device *dev);
void amlbt_hci_dev_deinit(void);

#endif

