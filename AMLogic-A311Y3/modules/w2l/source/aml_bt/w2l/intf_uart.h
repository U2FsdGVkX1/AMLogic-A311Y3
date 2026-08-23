/*
* Copyright (c) 202X Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*/

#ifndef N_HCI
#define N_HCI    15
#endif
#define N_AML    16 //define new number

/* Ioctls */
#define HCIUARTSETPROTO     _IOW('U', 200, int)
#define HCIUARTGETPROTO     _IOR('U', 201, int)
#define HCIUARTGETDEVICE    _IOR('U', 202, int)
#define HCIUARTSETFLAGS     _IOW('U', 203, int)
#define HCIUARTGETFLAGS     _IOR('U', 204, int)

/* UART protocols */
#define HCI_UART_MAX_PROTO  13

#define HCI_UART_H4     0
#define HCI_UART_BCSP   1
#define HCI_UART_3WIRE  2
#define HCI_UART_H4DS   3
#define HCI_UART_LL     4
#define HCI_UART_ATH3K  5
#define HCI_UART_INTEL  6
#define HCI_UART_BCM    7
#define HCI_UART_QCA    8
#define HCI_UART_AG6XX  9
#define HCI_UART_NOKIA  10
#define HCI_UART_MRVL   11
#define HCI_UART_AML    12

#define HCI_UART_RAW_DEVICE     0
#define HCI_UART_RESET_ON_INIT  1
#define HCI_UART_CREATE_AMP     2
#define HCI_UART_INIT_PENDING   3
#define HCI_UART_EXT_CONFIG     4
#define HCI_UART_VND_DETECT     5

struct hci_uart;
struct serdev_device;

struct hci_uart_proto {
    unsigned int id;
    const char *name;
    unsigned int manufacturer;
    unsigned int init_speed;
    unsigned int oper_speed;
    int (*open)(struct hci_uart *hu);
    int (*close)(struct hci_uart *hu);
    int (*flush)(struct hci_uart *hu);
    int (*setup)(struct hci_uart *hu);
    int (*set_baudrate)(struct hci_uart *hu, unsigned int speed);
    int (*recv)(struct hci_uart *hu, const void *data, int len);
    int (*enqueue)(struct hci_uart *hu, struct sk_buff *skb);
    struct sk_buff *(*dequeue)(struct hci_uart *hu);
};

struct hci_uart {
    struct tty_struct    *tty;
    struct serdev_device *serdev;
    struct hci_dev       *hdev;
    unsigned long       flags;
    unsigned long       hdev_flags;
    struct work_struct  init_ready;
    struct work_struct  write_work;
    const struct hci_uart_proto *proto;
    struct percpu_rw_semaphore proto_lock;	/* Stop work for proto close */
    void            *priv;
    struct sk_buff  *tx_skb;
    unsigned long    tx_state;
    unsigned int init_speed;
    unsigned int oper_speed;
    u8          alignment;
    u8          padding;
};

/* HCI_UART proto flag bits */
#define HCI_UART_PROTO_SET      0
#define HCI_UART_REGISTERED     1
#define HCI_UART_PROTO_READY    2
#define HCI_UART_NO_SUSPEND_NOTIFIER    3

/* TX states  */
#define HCI_UART_SENDING    1
#define HCI_UART_TX_WAKEUP  2


/* HCI controller types */
#define HCI_PRIMARY	0x00
#define HCI_AMP		0x01

//uart intf
#define HCI_UART_MAX_BUFF   4096
#define UART_WRITE_RETRY_MAX 10
#define RW_OPERATION_SIZE                       (248)

struct hci_cmd_request {
    unsigned short opcode;
    const unsigned char *cmd_data;
    unsigned int cmd_length;

    unsigned char rsp_buf[256];
    unsigned int rsp_len;
    struct completion done;
    int status;           // 0 for success, < 0 for error
    struct list_head list;
};

void amlbt_intf_uart_exception_func(struct work_struct *work);
void amlbt_intf_uart_write_work(amlbt_t *p_bt);
int amlbt_intf_uart_sw_read_word(unsigned int addr, unsigned int *reg);
int amlbt_intf_uart_sw_write_word(unsigned int addr, unsigned int data);
int amlbt_intf_uart_sw_read_sram(unsigned char* buf, unsigned int addr, unsigned int len);
unsigned int amlbt_intf_uart_sw_write_sram(unsigned char *data, unsigned int addr, unsigned char length);
int amlbt_intf_uart_hw_write_word(unsigned int addr, unsigned int data);
int amlbt_intf_uart_hw_read_word(unsigned int addr, unsigned int *reg);
void amlbt_intf_uart_early_suspend(amlbt_t *p_bt);
void amlbt_intf_uart_suspend(amlbt_t *p_bt);
void amlbt_intf_uart_resume(amlbt_t *p_bt);
void amlbt_intf_uart_later_resume(amlbt_t *p_bt);
void amlbt_intf_uart_shutdown(amlbt_t *p_bt);
unsigned int amlbt_intf_uart_sw_write_rclist(unsigned char *data, unsigned char cnt, unsigned char length);
int amlbt_intf_uart_sw_read_rclist(unsigned char* buf, unsigned int len);
int amlbt_intf_uart_check_hci_event(struct sk_buff *skb);
int amlbt_intf_uart_register(void);
void amlbt_intf_uart_unregister(void);

int amlbt_hci_uart_register_proto(const struct hci_uart_proto *p);
int amlbt_hci_uart_unregister_proto(const struct hci_uart_proto *p);

