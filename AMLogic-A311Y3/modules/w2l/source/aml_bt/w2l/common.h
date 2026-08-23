/*
* Copyright (c) 202X Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*/
#ifndef __COMMON_H__
#define __COMMON_H__
#include <linux/version.h>

typedef enum
{
    GDSL_ERR_SUCCESS,
    GDSL_ERR_NULL_POINTER,
    GDSL_ERR_NOT_FULL,
    GDSL_ERR_FULL,
    GDSL_ERR_NOT_EMPTY,
    GDSL_ERR_EMPTY,
    GDSL_ERR_SPACE_INVALID,
    GDSL_ERR_SPACE_VALID,
} gdsl_err_t;

typedef struct
{
    unsigned char *r;
    unsigned char *w;
    unsigned char *base_addr;
    unsigned int size;
} amlbt_common_gdsl_fifo_t;

typedef struct
{
    unsigned int tx_q_addr;
    unsigned int tx_q_status_addr;
    unsigned int tx_q_prio_addr;
    unsigned int tx_q_dev_index_addr;

    unsigned int tx_q_status;
    unsigned int tx_q_prio;
    unsigned int tx_q_dev_index;
} amlbt_common_gdsl_tx_q_t;

enum
{
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_POINT,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_ALL,
};

typedef unsigned long SYS_TYPE;

#ifndef FALSE
#define FALSE  0
#endif

#ifndef TRUE
#define TRUE   (!FALSE)
#endif

struct auc_hif_ops {
    int (*hi_send_cmd)(unsigned int addr, unsigned int len);
    void (*hi_write_word)(unsigned int addr,unsigned int data, unsigned int ep);
    unsigned int (*hi_read_word)(unsigned int addr, unsigned int ep);
    void (*hi_write_sram)(unsigned char* buf, unsigned char* addr, unsigned int len, unsigned int ep);
    void (*hi_read_sram)(unsigned char* buf, unsigned char* addr, unsigned int len, unsigned int ep);

    void (*hi_rx_buffer_read)(unsigned char* buf, unsigned char* addr, unsigned int len, unsigned int ep);

    /*bt use*/
    void (*hi_write_word_for_bt)(unsigned int addr,unsigned int data, unsigned int ep);
    unsigned int (*hi_read_word_for_bt)(unsigned int addr, unsigned int ep);
    void (*hi_write_sram_for_bt)(unsigned char* buf, unsigned char* addr, unsigned int len, unsigned int ep);
    void (*hi_read_sram_for_bt)(unsigned char* buf, unsigned char* addr, unsigned int len, unsigned int ep);

    int (*hi_enable_scat)(void);
    void (*hi_cleanup_scat)(void);
    struct amlw_usb_hif_scatter_req * (*hi_get_scatreq)(void);
    int (*hi_scat_rw)(struct scatterlist *sg_list, unsigned int sg_num, unsigned int blkcnt,
        unsigned char func_num, unsigned int addr, unsigned char write);

    int (*hi_send_frame)(struct amlw_usb_hif_scatter_req *scat_req);
    void (*hi_rcv_frame)(unsigned char* buf, unsigned char* addr, unsigned long len);
};

struct amlw1_hif_ops {
	int				(*hi_bottom_write8)(unsigned char func_num, int addr, unsigned char data);
	unsigned char			(*hi_bottom_read8)(unsigned char func_num, int addr);
	int				(*hi_bottom_read)(unsigned char func_num, int addr, void *buf, size_t len, int incr_addr);
	int				(*hi_bottom_write)(unsigned char func_num, int addr, void *buf, size_t len, int incr_addr);

	unsigned char			(*hi_read8_func0)(unsigned long sram_addr);
	void				(*hi_write8_func0)(unsigned long sram_addr, unsigned char sramdata);

	unsigned long			(*hi_read_reg8)(unsigned long sram_addr);
	void				(*hi_write_reg8)(unsigned long sram_addr, unsigned long sramdata);
	unsigned long			(*hi_read_reg32)(unsigned long sram_addr);
	int				(*hi_write_reg32)(unsigned long sram_addr, unsigned long sramdata);

	void				(*hi_write_cmd)(unsigned long sram_addr, unsigned long sramdata);
	void				(*hi_write_sram)(unsigned char *buf, unsigned char *addr, SYS_TYPE len);
	void				(*hi_read_sram)(unsigned char *buf, unsigned char *addr, SYS_TYPE len);
	void				(*hi_write_word)(unsigned int addr, unsigned int data);
	unsigned int			(*hi_read_word)(unsigned int addr);

	void				(*hi_rcv_frame)(unsigned char *buf, unsigned char *addr, SYS_TYPE len);

	int				(*hi_enable_scat)(void);
	void				(*hi_cleanup_scat)(void);
	struct amlw_hif_scatter_req *	(*hi_get_scatreq)(void);
	int				(*hi_scat_rw)(struct scatterlist *sg_list, unsigned int sg_num, unsigned int blkcnt, unsigned char func_num, unsigned int addr, unsigned char write);
	int				(*hi_send_frame)(struct amlw_hif_scatter_req *scat_req);

	/*bt use*/
	void				(*bt_hi_write_sram)(unsigned char *buf, unsigned char *addr, SYS_TYPE len);
	void				(*bt_hi_read_sram)(unsigned char *buf, unsigned char *addr, SYS_TYPE len);
	void				(*bt_hi_write_word)(unsigned int addr, unsigned int data);
	unsigned int			(*bt_hi_read_word)(unsigned int addr);

    void				(*hif_get_sts)(unsigned int op_code, unsigned int ctrl_code);
	void				(*hif_pt_rx_start)(unsigned int qos);
	void				(*hif_pt_rx_stop)(void);

	int				(*hif_suspend)(unsigned int suspend_enable);
};


struct aml_pm_type {
    atomic_t bus_suspend_cnt;
    atomic_t drv_suspend_cnt;
    atomic_t is_shut_down;
    atomic_t wifi_enable;
    //atomic_t bt_enable; //wait wifi project_w2l_wifi_release_3_2
};

typedef void (*bt_shutdown_func)(void);

enum usb_endpoint_num{
    USB_EP0 = 0x0,
    USB_EP1,
    USB_EP2,
    USB_EP3,
    USB_EP4,
    USB_EP5,
    USB_EP6,
    USB_EP7,
};

enum usb_udev_state {
    USB_NOTATTACHED = 0,
    USB_ATTACHED,
    USB_POWERED,
    USB_RECONNECTING,
    USB_UNAUTHENTICATED,
    USB_DEFAULT,
    USB_ADDRESS,
    USB_CONFIGURED,
    USB_SUSPENDED
};

enum bt_rx_state{
    HCI_RX_TYPE,
    HCI_RX_HEADER,
    HCI_RX_PAYLOAD,
    HCI_RX_FATAL,
};

//read len
#define TYPE_SIZE         1
#define EVT_HEAD_SIZE     2
#define ACL_HEAD_SIZE     4

#define AMLBT_MAX_COEX_DEVICES   5

#define HCI_MAX_EVENT_SIZE    260
#define HCI_MAX_DATA_SIZE     1028
//#define HCI_MAX_ACL_SIZE      1021
//#define HCI_MAX_FRAME_SIZE    (HCI_MAX_ACL_SIZE + 7)
#define HCI_COMMAND_PKT       0x01
#define HCI_ACLDATA_PKT       0x02
#define HCI_SCODATA_PKT       0x03
#define HCI_EVENT_PKT         0x04
#define HCI_ISO_PKT           0x05
#define HCI_15P4_PKT          0x10
#define HCI_FWLOG_PKT         0x62
#define HCI_ADVIND_PKT        0x3e

#define HCI_15P4_ZIGBEE_PKT   0xf5
#define HCI_15P4_THREAD_PKT   0xfa
#define HCI_15P4_LOG_PKT      0xf0

#define HCI_NO_TYPE           0xfe
#define HCI_VENDOR_PKT        0xff

#define TCI_READ_REG                            0xfef0
#define TCI_WRITE_REG                           0xfef1
#define TCI_UPDATE_UART_BAUDRATE                0xfef2
#define TCI_DOWNLOAD_BT_FW                      0xfef3
#define SW_READ_REG                             0xfcf0
#define SW_WRITE_REG                            0xfcf1
#define SW_WRITE_SRAM                           0xfc6e
#define SW_READ_SRAM                            0xfc6f
#define SW_WRITE_RCLIST                         0xfc58
#define SW_READ_RCLIST                          0xfc59
#define SW_STR_CMD                              0xfc92
#define SW_SHUTDOWN_CMD                         0xfc93
#define SW_CAPTURE_CMD                          0xFC94

amlbt_common_gdsl_fifo_t *amlbt_common_gdsl_fifo_init(unsigned int len, unsigned char *base_addr);
void amlbt_common_gdsl_fifo_deinit(amlbt_common_gdsl_fifo_t *p_fifo);
unsigned int amlbt_common_gdsl_fifo_used(amlbt_common_gdsl_fifo_t *p_fifo);
unsigned int amlbt_common_gdsl_fifo_remain(amlbt_common_gdsl_fifo_t *p_fifo);
unsigned int amlbt_common_gdsl_fifo_copy_data(amlbt_common_gdsl_fifo_t *p_fifo, unsigned char *buff, unsigned int len);
unsigned int amlbt_common_gdsl_fifo_calc_r(amlbt_common_gdsl_fifo_t *p_fifo, unsigned char *buff, unsigned int len);
unsigned int amlbt_common_gdsl_fifo_get_data(amlbt_common_gdsl_fifo_t *p_fifo, unsigned char *buff, unsigned int len);
unsigned int amlbt_common_gdsl_fifo_update_r(amlbt_common_gdsl_fifo_t *p_fifo, unsigned int len);
int amlbt_common_gdsl_write_data(amlbt_common_gdsl_fifo_t *p_fifo, unsigned char *data, int len);
int amlbt_common_reg_bit_set(unsigned int addr, unsigned int bit);
int amlbt_common_reg_bit_clr(unsigned int addr, unsigned int bit);
int amlbt_common_reg_bit_get(unsigned int addr, unsigned int bit);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0)

#ifndef __poll_t_defined
typedef unsigned int __poll_t;
#define __poll_t_defined
#endif

void *skb_put_data(struct sk_buff *skb, const void *data, unsigned int len);

#endif

extern unsigned int g_dbg_level;
extern unsigned int amlbt_if_type;
extern unsigned int polling_time;
extern unsigned int amlbt_ft_mode;

struct amlbt_diag_buf;

#define BTUSB_IOC_MAGIC 'x'

#define IOCTL_GET_BT_RECOVERY       _IOR(BTUSB_IOC_MAGIC, 0, int)
#define IOCTL_GET_DEVICE_PID        _IOR(BTUSB_IOC_MAGIC, 1, int)
#define IOCTL_GET_BT_REG            _IOR(BTUSB_IOC_MAGIC, 2, int)
#define IOCTL_SET_BT_REG            _IOW(BTUSB_IOC_MAGIC, 3, int)
#define IOCTL_SET_HCI_CMD           _IOW(BTUSB_IOC_MAGIC, 4, int)
#define IOCTL_GET_BT_BUF            _IOR(BTUSB_IOC_MAGIC, 5, int)
#define IOCTL_GET_BT_VERSION        _IOR(BTUSB_IOC_MAGIC, 6, int)
#define IOCTL_SET_BT_SHUTDOWN       _IOW(BTUSB_IOC_MAGIC, 7, int)
#define IOCTL_GET_COEX_STATUS       _IOR(BTUSB_IOC_MAGIC, 8, int)
#define IOCTL_REGISTER_SDIO         _IOW(BTUSB_IOC_MAGIC, 9, int)
#define IOCTL_UNREGISTER_SDIO       _IOW(BTUSB_IOC_MAGIC, 10, int)
#define IOCTL_GET_SDIO_PROBE_STATUS _IOR(BTUSB_IOC_MAGIC, 11, int)
#define IOCTL_SET_BT_RECOVERY       _IOW(BTUSB_IOC_MAGIC, 12, int)
#define IOCTL_SET_BT_UART_RESET     _IO(BTUSB_IOC_MAGIC, 13)
#define IOCTL_GET_DEVICE_CID        _IOR(BTUSB_IOC_MAGIC, 14, int)
#define IOCTL_SET_BT_EN_ENABLE      _IO(BTUSB_IOC_MAGIC, 16)
#define IOCTL_REGISTER_HCI0         _IOW(BTUSB_IOC_MAGIC, 17, int)

#define IOCTL_GET_DRIVER_VERSION    _IOR(BTUSB_IOC_MAGIC, 20, int)
#define IOCTL_GET_DIAG_COUNT        _IOR(BTUSB_IOC_MAGIC, 21, int)
#define IOCTL_GET_DIAG_BUFF         _IOR(BTUSB_IOC_MAGIC, 22, struct amlbt_diag_buf)
#define IOCTL_GET_DIAG_REMAIN_COUNT _IOR(BTUSB_IOC_MAGIC, 23, int)
#define IOCTL_GET_DIAG_REMAIN_BUFF  _IOR(BTUSB_IOC_MAGIC, 24, struct amlbt_diag_remain_buf)

#define FAMILY_TYPE_IS_W1(x)        ((AMLBT_PD_ID_FAMILY & x) == AMLBT_FAMILY_W1)
#define FAMILY_TYPE_IS_W1U(x)       ((AMLBT_PD_ID_FAMILY & x) == AMLBT_FAMILY_W1U)
#define FAMILY_TYPE_IS_W2(x)        ((AMLBT_PD_ID_FAMILY & x) == AMLBT_FAMILY_W2)
#define FAMILY_TYPE_IS_W2L(x)       ((AMLBT_PD_ID_FAMILY & x) == AMLBT_FAMILY_W2L)
#define FAMILY_TYPE_IS_W1D(x)       ((AMLBT_PD_ID_FAMILY & x) == AMLBT_FAMILY_W1D)

#define INTF_TYPE_IS_SDIO(x)        ((AMLBT_PD_ID_INTF & x) == AMLBT_INTF_SDIO)
#define INTF_TYPE_IS_USB(x)         ((AMLBT_PD_ID_INTF & x) == AMLBT_INTF_USB)
#define INTF_TYPE_IS_PCIE(x)        ((AMLBT_PD_ID_INTF & x) == AMLBT_INTF_PCIE)

#define AMLBT_PD_ID_INTF            0x7
#define AMLBT_PD_ID_WIRELESS_CONFIG (0x7<<3)
#define AMLBT_PD_ID_FAMILY_VER      (0x7<<6)
#define AMLBT_PD_ID_FAMILY          (0x1f<<9)

#define INTF_CHIP_FAMILY_ID(x)      ((AMLBT_PD_ID_FAMILY & x) >> 9)
#define INTF_CHIP_FAMILY_REV_ID(x)  ((AMLBT_PD_ID_FAMILY_VER & x) >> 6)
#define INTF_WIRELESS_CONFIG(x)     ((AMLBT_PD_ID_WIRELESS_CONFIG & x) >> 3)
#define INTF_CHIP_INTF_ID(x)        ((AMLBT_PD_ID_INTF & x))

#define AMLBT_INTF_SDIO             0x0
#define AMLBT_INTF_USB              0x01
#define AMLBT_INTF_PCIE             0x02

#define AMLBT_FAMILY_W1             (0x01<<9)
#define AMLBT_FAMILY_W1U            (0x02<<9)
#define AMLBT_FAMILY_W2             (0x03<<9)
#define AMLBT_FAMILY_W2L            (0x04<<9)
#define AMLBT_FAMILY_W3             (0x05<<9)
#define AMLBT_FAMILY_W1D            (0x06<<9)

#define AMLBT_TRANS_UNKNOWN         0x00

#define AMLBT_TRANS_W1_UART         0x01
#define AMLBT_TRANS_W1U_UART        0x02
#define AMLBT_TRANS_W2_UART         0x03
#define AMLBT_TRANS_W1U_USB         0x04
#define AMLBT_TRANS_W2_USB          0x05
#define AMLBT_TRANS_W2L_USB         0x06

#define BTA(fmt, arg...) if (g_dbg_level >= LOG_LEVEL_ALL) printk(KERN_INFO "BTA:" fmt, ## arg)
#define BTD(fmt, arg...) if (g_dbg_level >= LOG_LEVEL_DEBUG) printk(KERN_INFO "BTD:" fmt, ## arg)
#define BTI(fmt, arg...) if (g_dbg_level >= LOG_LEVEL_INFO) printk(KERN_INFO "BTI:" fmt, ## arg)
#define BTP(fmt, arg...) if (g_dbg_level >= LOG_LEVEL_POINT) printk(KERN_INFO "BTP:" fmt, ## arg)
#define BTW(fmt, arg...) if (g_dbg_level >= LOG_LEVEL_WARN) printk(KERN_WARNING "BTW:" fmt, ## arg)
#define BTE(fmt, arg...) if (g_dbg_level >= LOG_LEVEL_ERROR) printk(KERN_ERR "BTE:" fmt, ## arg)
#define BTF(fmt, arg...) if (g_dbg_level >= LOG_LEVEL_FATAL) printk(KERN_ERR "BTF:" fmt, ## arg)

enum
{
    Rev_A,
    Rev_B,
    Rev_C,
    Rev_D,
    Rev_E,
    Rev_F,
    Rev_G,
    Rev_H,
};

//input device
#define INPUT_NAME                "input_btdrv"
#define INPUT_PHYS                "input_btdrv/input0"
//#define KEY_NETFLIX               133 android used
#ifndef KEY_NETFLIX
#define KEY_NETFLIX               468 //linux used
#endif

//wake source
#define REMOTE_WAKEUP           2 //infrared
#define BT_WAKEUP               4 //bt powerkey
#define REMOTE_CUS_WAKEUP       9 //bt netflixkey

#endif

