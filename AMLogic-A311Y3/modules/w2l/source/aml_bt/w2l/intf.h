/*
* Copyright (c) 202X Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*/

#ifndef __INTF_H__
#define __INTF_H__

#define AML_BT_NOTE                     "stpbt"
#define AML_BT_USB_NOTE                 "aml_btusb"
#define AML_ZIGBEE_NOTE                 "aml_zigbee"
#define AML_THREAD_NOTE                 "aml_thread"
#define AML_COEX_NOTE                   "aml_coex"
#define AML_BT_DIAG_NOTE                "aml_bt_debug"

#if defined(CONFIG_AML_BT_CHIP_W2L)
extern void aml_bus_state_detect_deinit(void);
#endif
#if defined(CONFIG_AML_BT_CHIP_W1D)
extern unsigned char g_sdio_wifi_bt_alive;
#endif
extern int register_bt_event_notifier(struct notifier_block *nb);
extern int unregister_bt_event_notifier(struct notifier_block *nb);
extern unsigned char g_sdio_after_porbe;
extern unsigned char g_chip_function_ctrl;
extern unsigned char g_sdio_driver_insmoded;
extern struct notifier_block bt_nb;
extern void aml_sdio_exit(void);
extern int  aml_sdio_init(void);
extern struct aml_bus_state_detect bus_state_detect;
extern struct aml_pm_type g_wifi_pm;
extern const struct file_operations amlbt_intf_bt_fops;
extern const struct file_operations amlbt_intf_zigbee_fops;
extern const struct file_operations amlbt_intf_thread_fops;
extern const struct file_operations amlbt_intf_coex_fops;

/* | HCI_15P4_FLAG   | MHDL |           MID        | BODY LENGTH | checksum |
*  |   0x10 1 Byts   | 0xFA | command id 1 Bytes   |    2 Bytes  |  2 Bytes |
*/

#define MAX_DIAG_SKB_QUEUED     2048
#define MAX_DIAG_CMD_LENGTH     64

#define MAX_DOWNLOAD_RETRY      10

#ifndef BIT
#define BIT(_n)  (1 << (_n))
#endif

#define BIT_PHY                 1
#define BIT_MAC                 (1 << 1)
#define BIT_CPU                 (1 << 2)
#define BIT_RF_NUM              28
#define BT_SINK_MODE            25

#if defined(CONFIG_AML_BT_CHIP_W1D)
#define CHIP_BT_PMU_REG_BASE                      (0xf6f83000)
#define CHIP_INTF_REG_BASE                        (0xf6f80000)

#define REG_DEV_RESET                             (0xf6f83030)
#define REG_PMU_POWER_CFG                         (0xf6f83078)
#define REG_FW_PC                                 (0xf7780034)
#define REG_UART_ENABLE                           (0xf7783000)
#define REG_INTERRUPT_BT_SET                      (0xf7780168)
#define REG_INTERRUPT_SW_SET                      (0xf7780180)

//#define W2L_DF_REG_A188                           (0x00f062f0)
//#define W2L_RG_PMU_A16                            (0x00f02040)
//#define REG_RAM_PD_SHUTDWONW_SW                   (0x00f03050)
//#define REG_FW_MODE                               (0x00f000e0)
//#define REG_DF_A194                               (0x00f06308)

#else
#define CHIP_BT_PMU_REG_BASE                      (0x00f03000)
#define CHIP_INTF_REG_BASE                        (0x00f00000)

#define W2L_DF_REG_A188                           (0x00f062f0)
#define W2L_RG_PMU_A16                            (0x00f02040)
#define REG_DEV_RESET                             (0x00f03058)
#define REG_PMU_POWER_CFG                         (0x00f03040)
#define REG_RAM_PD_SHUTDWONW_SW                   (0x00f03050)
#define REG_FW_MODE                               (0x00f000e0)
#define REG_FW_PC                                 (0x00200034)
#define REG_DF_A194                               (0x00f06308)

#endif
//AON BT PMU REG
#define RG_BT_PMU_A0                              (CHIP_BT_PMU_REG_BASE + 0x00)
#define RG_BT_PMU_A1                              (CHIP_BT_PMU_REG_BASE + 0x04)
#define RG_BT_PMU_A2                              (CHIP_BT_PMU_REG_BASE + 0x08)
#define RG_BT_PMU_A3                              (CHIP_BT_PMU_REG_BASE + 0x0c)
#define RG_BT_PMU_A4                              (CHIP_BT_PMU_REG_BASE + 0x10)
#define RG_BT_PMU_A5                              (CHIP_BT_PMU_REG_BASE + 0x14)
#define RG_BT_PMU_A6                              (CHIP_BT_PMU_REG_BASE + 0x18)
#define RG_BT_PMU_A7                              (CHIP_BT_PMU_REG_BASE + 0x1c)
#define RG_BT_PMU_A8                              (CHIP_BT_PMU_REG_BASE + 0x20)
#define RG_BT_PMU_A9                              (CHIP_BT_PMU_REG_BASE + 0x24)
#define RG_BT_PMU_A10                             (CHIP_BT_PMU_REG_BASE + 0x28)
#define RG_BT_PMU_A11                             (CHIP_BT_PMU_REG_BASE + 0x2c)
#define RG_BT_PMU_A12                             (CHIP_BT_PMU_REG_BASE + 0x30)
#define RG_BT_PMU_A13                             (CHIP_BT_PMU_REG_BASE + 0x34)
#define RG_BT_PMU_A14                             (CHIP_BT_PMU_REG_BASE + 0x38)
#define RG_BT_PMU_A15                             (CHIP_BT_PMU_REG_BASE + 0x3c)
#define RG_BT_PMU_A16                             (CHIP_BT_PMU_REG_BASE + 0x40)
#define RG_BT_PMU_A17                             (CHIP_BT_PMU_REG_BASE + 0x44)
#define RG_BT_PMU_A18                             (CHIP_BT_PMU_REG_BASE + 0x48)
#define RG_BT_PMU_A20                             (CHIP_BT_PMU_REG_BASE + 0x50)
#define RG_BT_PMU_A22                             (CHIP_BT_PMU_REG_BASE + 0x58)
#define RG_BT_PMU_A30                             (CHIP_BT_PMU_REG_BASE + 0x78)

//AON CHIP REG
#define RG_AON_A15                                (CHIP_INTF_REG_BASE + 0x3c)
#define RG_AON_A16                                (CHIP_INTF_REG_BASE + 0x40)
#define RG_AON_A17                                (CHIP_INTF_REG_BASE + 0x44)
#define RG_AON_A24                                (CHIP_INTF_REG_BASE + 0x60)
#define RG_AON_A30                                (CHIP_INTF_REG_BASE + 0x78)
#define RG_AON_A33                                (CHIP_INTF_REG_BASE + 0x84)
#define RG_AON_A52                                (CHIP_INTF_REG_BASE + 0xd0)
#define RG_AON_A53                                (CHIP_INTF_REG_BASE + 0xd4)
#define RG_AON_A55                                (CHIP_INTF_REG_BASE + 0xdc)
#define RG_AON_A56                                (CHIP_INTF_REG_BASE + 0xe0)
#define RG_AON_A57                                (CHIP_INTF_REG_BASE + 0xe4)
#define RG_AON_A58                                (CHIP_INTF_REG_BASE + 0xe8)
#define RG_AON_A59                                (CHIP_INTF_REG_BASE + 0xec)
#define RG_AON_A60                                (CHIP_INTF_REG_BASE + 0xf0)
#define RG_AON_A61                                (CHIP_INTF_REG_BASE + 0xf4)
#define RG_AON_A62                                (CHIP_INTF_REG_BASE + 0xf8)
#define RG_AON_A93                                (CHIP_INTF_REG_BASE + 0x174)
#define RG_AON_A94                                (CHIP_INTF_REG_BASE + 0x178)

// pmu status
#define PMU_PWR_OFF       0x0
#define PMU_PWR_XOSC      0x1
#define PMU_XOSC_WAIT     0x2
#define PMU_XOSC_DPLL     0x3
#define PMU_DPLL_WAIT     0x4
#define PMU_DPLL_ACT      0x5
#define PMU_ACT_MODE      0x6
#define PMU_ACT_SLEEP     0x7
#define PMU_SLEEP_MODE    0x8
#define PMU_SLEEP_WAKE    0x9
#define PMU_WAKE_WAIT     0xa
#define PMU_WAKE_XOSC     0xb

#ifndef BIT
#define BIT(_n)  (1 << (_n))
#endif

#define BT_DRV_STATE_SUSPEND_ENTRY     BIT(0)
#define BT_DRV_STATE_SUSPEND           BIT(1)
#define BT_DRV_STATE_RESUME            BIT(2)
#define BT_DRV_STATE_RECOVERY          BIT(3)
//#define BT_DRV_STATE_SEND              BIT(4)
#define BT_DRV_STATE_WAIT_RECOVERY     BIT(5)

struct hci_uart_rx {
    enum bt_rx_state state;
    uint8_t type;
    uint8_t header_len;
    uint8_t header[4];
    uint8_t *payload;
    int expected_length;
    int received_length;
};

typedef struct
{
    struct cdev dev_cdev[AMLBT_MAX_COEX_DEVICES];
    int         dev_major;
    struct class *dev_class;
    struct device *dev_device[AMLBT_MAX_COEX_DEVICES];
} amlbt_res_driver_t;

typedef struct
{
    unsigned int rom_size;
    unsigned int iccm_size;
    unsigned int dccm_size;
    unsigned int add_size;
    unsigned int iccm_ahb_base;
    unsigned int dccm_ahb_base;
    unsigned int iccm_ram_base;
    unsigned int dccm_ram_base;
    unsigned int add_ram_base;
    unsigned int download_size;
    unsigned int poll_len;
    unsigned int poll_addr;
    unsigned long rx_q_addr;
    unsigned int rx_q_len;
    unsigned int rx_q_r;
    unsigned int rx_q_w;
    unsigned long rx_type_addr;
    unsigned int rx_type_len;
    unsigned int rx_type_r;
    unsigned int rx_type_w;
    unsigned long evt_addr;
    unsigned int evt_len;
    unsigned int evt_r;
    unsigned int evt_w;
    unsigned long cmd_addr;
    unsigned int cmd_len;
    unsigned int cmd_r;
    unsigned int cmd_w;
    unsigned long _15p4_rx_addr;
    unsigned int _15p4_rx_len;
    unsigned int _15p4_rx_r;
    unsigned int _15p4_rx_w;
    unsigned long _15p4_tx_addr;
    unsigned int _15p4_tx_len;
    unsigned int _15p4_tx_r;
    unsigned int _15p4_tx_w;
    unsigned long tx_q_addr;
    unsigned long tx_q_prio_addr;
    unsigned int driver_fw_status_reg;
    const unsigned char *iccm_buf;
    const unsigned char *dccm_buf;
    const unsigned char *add_buf;
} amlbt_res_fw_t;

typedef struct
{
    unsigned int antenna;
    unsigned int fw_mode;
    unsigned int bt_sink;
    unsigned int pin_mux;
    unsigned int br_digit_gain;
    unsigned int edr_digit_gain;
    unsigned int fw_log;
    unsigned int driver_log;
    unsigned int factory;
    unsigned char manf_data[256]; //sdio not used
    unsigned int manfdata_len;      //sdio not used
    unsigned int system;
    unsigned int isolation;
    unsigned int manf_cnt;
} amlbt_res_conf_t;

typedef struct
{
    unsigned char bt_start;
    wait_queue_head_t bt_wait_queue;
    struct sk_buff_head bt_rx_queue;
    struct sk_buff *current_skb;
    struct sk_buff *hw_error_skb;
} amlbt_res_bt_t;

typedef struct
{
    unsigned char zigbee_start;
    struct sk_buff *current_skb;
    struct sk_buff *hw_error_skb;
    wait_queue_head_t zigbee_wait_queue;
    struct sk_buff_head zigbee_rx_queue;
} amlbt_res_zigbee_t;

typedef struct
{
    unsigned char thread_start;
    struct sk_buff *current_skb;
    struct sk_buff *hw_error_skb;
    wait_queue_head_t thread_wait_queue;
    struct sk_buff_head thread_rx_queue;
} amlbt_res_thread_t;

typedef struct
{
    struct workqueue_struct *resume_wq;
    struct work_struct resume_work;
    int irq;
    int gpio_num;                       /**< GPIO pin number for bt wake host*/
    unsigned int irq_handle;
    struct workqueue_struct *wake_work_wq;
    struct work_struct wake_work;       //usb not used
    struct input_dev *input_dev;
    struct device_link *link;
#ifdef CONFIG_AMLOGIC_LEGACY_EARLY_SUSPEND
    struct early_suspend early_suspend;
#endif
    unsigned int dr_state;
} amlbt_res_pm_t;           //power manager

typedef struct
{
    struct workqueue_struct *exception_work_wq;
    struct work_struct exception_work;
    struct completion notify_comp;
    unsigned int notify_trig;
    unsigned int notify_recy;
} amlbt_res_exception_t;

typedef struct
{
    struct sk_buff *rx_skb;
    struct sk_buff_head txq;
    //struct mutex txq_mutex;
    //struct sk_buff_head rxq;
    struct hci_uart *hu;
    bool initialized;
}amlbt_res_uart_linux_t;

enum
{
    HDEV_RUNNING = 0,    /* BIT0:HCI Open  */
    HDEV_SUSPENDED,      /* BIT1:SUSPEND */
    HDEV_RESUMING,       /* BIT2:RESUMING */
    HDEV_RECOVERING,     /* BIT3:RECOVERY */
};

typedef struct
{
    struct hci_dev *hdev;//driver get hci dev info
    struct platform_device *pdev;
    struct workqueue_struct *workqueue;
    //struct work_struct receive_work;
    struct delayed_work receive_work;
    unsigned int reg_flag;
    spinlock_t lock;    /* For serializing operations */
    struct sk_buff_head txq;
    unsigned long tx_state;
    unsigned long rx_state;
    unsigned long rx_count;
    unsigned char *bluez_buf;
    unsigned long hdev_flags;
    struct sk_buff *rx_skb;
} amlbt_res_usb_linux_t;

typedef struct
{
    //rx fifo
    amlbt_common_gdsl_fifo_t *fw_type_fifo;
    amlbt_common_gdsl_fifo_t *fw_evt_fifo;
    amlbt_common_gdsl_fifo_t *fw_data_fifo;
    //tx fifo
    amlbt_common_gdsl_fifo_t *tx_cmd_fifo;
    amlbt_common_gdsl_tx_q_t tx_q[8]; //USB_TX_Q_NUM
    //15.4 fifo
    amlbt_common_gdsl_fifo_t *_15p4_tx_fifo;
    amlbt_common_gdsl_fifo_t *_15p4_rx_fifo;
    unsigned char *usb_rx_buf;
    unsigned int usb_rx_len;
    struct hrtimer poll_timer;
    ktime_t ktime;
    struct work_struct check_fw;
    u64 wait_start;
    struct work_struct rx_work;
    struct workqueue_struct *rx_work_wq;
    unsigned char usb_irq_task_quit;
} amlbt_res_usb_t;

typedef struct
{
    struct hci_uart *hu;
    struct hci_uart_rx rx;
    int wake_gpio;
    struct list_head hci_pending_list;
    unsigned char sw_op;
} amlbt_res_uart_t;

typedef struct
{
    unsigned char mac_addr[6];
    unsigned int sink_mode;
    unsigned long fw_log_cnt;
    unsigned long cmd_cnt;
    unsigned long acl_cnt;
    unsigned long fw_interrupt_cnt;
    unsigned long current_time;
    struct mutex bt_debug_mutex;
    struct sk_buff_head diag_queue;
    unsigned char flush_skb;
    unsigned int chip_family_id;
    unsigned int chip_rev_id;
    unsigned int chip_wireless_config;
    unsigned int chip_intf_id;
} amlbt_res_diag_t;

typedef struct
{
    unsigned char recovery_value;
    unsigned char shutdown_value;
    unsigned int res_init;
    struct work_struct  write_work;
    struct workqueue_struct *write_work_wq;
    struct sk_buff_head tx_queue;
} amlbt_res_common_t;

typedef struct
{
    amlbt_res_driver_t drv_res;
    amlbt_res_fw_t fw_res;
    amlbt_res_conf_t conf_res;
    amlbt_res_bt_t bt_res;
    amlbt_res_zigbee_t zigbee_res;
    amlbt_res_thread_t thread_res;
    amlbt_res_pm_t pm_res;
    amlbt_res_exception_t excp_res;
    amlbt_res_uart_t uart_res;
    amlbt_res_uart_linux_t uart_res_linux;
    amlbt_res_usb_t usb_res;
    amlbt_res_usb_linux_t usb_res_linux;
    amlbt_res_diag_t diag_res;
    amlbt_res_common_t common_res;
    unsigned int rw_intf;
    unsigned int bt_intf;
} amlbt_t;

#ifndef CONFIG_AML_BT_USB_HOTPLUG
struct aml_bus_state_detect {
    unsigned char bus_err;
    unsigned char usb_disconnect;
    unsigned char is_drv_load_finished;
    unsigned char bus_reset_ongoing;
    unsigned char is_load_by_timer;
    unsigned char is_recy_ongoing;
    struct timer_list timer;
    struct work_struct detect_work;
    int (*insmod_drv)(void);
    unsigned char usb_suspend;
};
#else
struct aml_bus_state_detect {
    unsigned char bus_err;
    unsigned char usb_disconnect;
    unsigned char is_drv_load_finished;
    unsigned char bus_reset_ongoing;
    unsigned char is_load_by_timer;
    unsigned char is_recy_ongoing;
    struct timer_list timer;
    struct work_struct detect_work;
    int (*insmod_drv)(void);
    unsigned char usb_suspend;
    unsigned char usb_unplug;
    void (*auc_wifi_enable_func)(void);
    void (*auc_wifi_disable_func)(void);
};
#endif

struct amlbt_diag_entry {
    unsigned char  type;
    unsigned int w;           // write pointer
    unsigned int r;           // read pointer

    unsigned char  mon;
    unsigned char  day;
    unsigned char  hour;
    unsigned char  min;
    unsigned char  sec;
    unsigned short ms;          // [0-999]

    unsigned char  opcode;      //
    unsigned char  info[7];     //
    unsigned int fw_log_cnt;  //
    unsigned int hci_cmd_cnt;  //
} __packed;

struct amlbt_diag_buf
{
    unsigned int count;
    unsigned int max;
    unsigned char fw_log[516];
    struct amlbt_diag_entry entries[];
} __packed;

struct amlbt_diag_remain_buf
{
    unsigned int count;
    unsigned int max;
    struct amlbt_diag_entry entries[];
};

enum fops_mode_t
{
    INTF_FOPS_BT,
    INTF_FOPS_ZIGBEE,
    INTF_FOPS_THREAD,
};


enum
{
    INTF_SDIO,
    INTF_USB,
    INTF_UART,
    INTF_PCIE,
    INTF_UNKNOWN = 0xff,
};

enum
{
    BT_INTF_KERNEL_TTY,
    BT_INTF_DRIVER_USB,
    BT_INTF_DRIVER_TTY,
    BT_INTF_UNKNOWN = 0xff,
};

enum
{
    CHIP_INTF_SDIO,
    CHIP_INTF_USB2,
    CHIP_INTF_PCIE2,
    CHIP_INTF_USB3,
    CHIP_INTF_PCIE3,
    CHIP_INTF_UNKNOWN = 0xff,
};

enum
{
    CHIP_W1 = 1,
    CHIP_W1U,
    CHIP_W2,
    CHIP_W2L,
    CHIP_W3,
    CHIP_W1D,
    CHIP_UNKNOWN = 0xff,
};

enum
{
    CHIP_REVA,
    CHIP_REVB,
    CHIP_REVC,
    CHIP_REVD,
    CHIP_REVE,
    CHIP_REVF,
    CHIP_REVG,
    CHIP_REV_UNKNOWN = 0xff,
};

enum
{
    MODULE_RESTART,
    MODULE_POWER_OFF,
};

#define DUMP_BUF(tag, buf, len)                                         \
    do {                                                                \
        unsigned int __dump_len = (unsigned int)(len);                  \
        const unsigned char *__dump_ptr = (const unsigned char *)(buf); \
        unsigned int __max_len = (__dump_len > 64) ? 64 : __dump_len;   \
        char __dump_str[256];                                           \
        char *__p = __dump_str;                                         \
        int __n, __i;                                                   \
        for (__i = 0; __i < __max_len; __i++) {                         \
            __n = snprintf(__p, sizeof(__dump_str) - (__p - __dump_str), "%02X ", __dump_ptr[__i]); \
            if (__n <= 0 || (__p - __dump_str) + __n >= sizeof(__dump_str)) \
                break;                                                  \
            __p += __n;                                                 \
        }                                                               \
        *__p = '\0';                                                    \
        BTE("%s: buf=%p len=%u,%s%s\n",                                  \
            (tag), __dump_ptr, __dump_len,                              \
            (__dump_len > __max_len ? " (truncated)" : ""), __dump_str);\
    } while (0)


unsigned int amlbt_intf_rw_get(void);
unsigned int amlbt_intf_bt_get(void);
amlbt_t * amlbt_intf_get_p_bt(void);

int amlbt_intf_write_word(unsigned int addr, unsigned int data);
int amlbt_intf_read_word(unsigned int addr, unsigned int *data);
int amlbt_intf_read_sram(unsigned char* buf, unsigned int addr, unsigned int len);
int amlbt_intf_write_sram(unsigned char* buf, unsigned int addr, unsigned int len);
int amlbt_intf_register(void);
void amlbt_intf_version(void);
void amlbt_intf_fw_info(void);
void amlbt_intf_drv_state_set(unsigned int bit);
void amlbt_intf_drv_state_clr(unsigned int bit);
int amlbt_intf_suspend(amlbt_t *p_bt);
int amlbt_intf_resume(amlbt_t *p_bt);
void amlbt_intf_shutdown(amlbt_t *p_bt);
void amlbt_intf_remove(amlbt_t *p_bt);
void amlbt_intf_register_early_suspend(amlbt_t *p_bt, struct platform_device *dev);
void amlbt_intf_unregister_early_suspend(amlbt_t *p_bt, struct platform_device *dev);
int amlbt_intf_input_device_init(struct platform_device *pdev);
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
int amlbt_intf_bind_bus(amlbt_t *p_bt, struct device *consumer, struct device *supplier, u32 flags);
void amlbt_intf_unbind_bus(amlbt_t *p_bt, struct device *consumer, struct device *supplier);
int amlbt_intf_ops_bind_bus(amlbt_t *p_bt);
void amlbt_intf_ops_unbind_bus(amlbt_t *p_bt);
#endif
void amlbt_intf_wakeup_key_process(amlbt_t *p_bt);
void amlbt_intf_exception_func(amlbt_t *p_bt);
int amlbt_intf_diag_filter_event(unsigned char *evt_buf);
void amlbt_intf_diag_add(amlbt_t *p_bt, unsigned char type,
    unsigned int w, unsigned int r, unsigned char *data, unsigned int fw_log_cnt, unsigned int data_cnt);
void amlbt_intf_diag_rssi_print(amlbt_t *p_bt);
void amlbt_intf_diag_skb_print(amlbt_t *p_bt);
void amlbt_intf_diag_rx_remain_print(amlbt_t *p_bt);
unsigned int amlbt_intf_vendor_write_rclist(unsigned char *data, unsigned char cnt, unsigned char length);
void amlbt_intf_queue_work(struct workqueue_struct *wq , struct work_struct *work);
void amlbt_intf_flush_workqueue(struct workqueue_struct *wq, struct work_struct *work);
int amlbt_intf_create_device(amlbt_t *p_bt);
int amlbt_intf_destroy_device(amlbt_t *p_bt);


#endif

