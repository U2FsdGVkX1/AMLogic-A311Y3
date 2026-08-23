/*
* Copyright (c) 202X Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*/

#include <linux/debugfs.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/poll.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/signal.h>
#include <linux/ioctl.h>
#include <linux/skbuff.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/poll.h>
#include <linux/platform_device.h>
#include <linux/amlogic/pm.h>
#include <linux/proc_fs.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 0, 0)
    #include <asm/unaligned.h>
#elif LINUX_VERSION_CODE < KERNEL_VERSION(6, 10, 0)
    #include <asm-generic/unaligned.h>
#else
    #include <linux/unaligned.h>
#endif

//#include "hci_uart.h"
#include "common.h"
#include "intf.h"
#include "intf_uart.h"
#include "intf_uart_coex.h"
#include "rc_list.h"
#include "debug_dev.h"
#include "driver.h"
#include "chip.h"

#define AML_COEX_VERSION    "2026-0203,1430 LEA"
#define IS_VALID_PKT_TYPE(t)  ( (t >= HCI_COMMAND_PKT && t <= HCI_ISO_PKT) || (t == HCI_15P4_PKT) )
static int amlbt_coex_recv_event(struct hci_dev *hdev, struct sk_buff *skb);
static int aml_15p4_recv_frame(struct hci_dev *hdev, struct sk_buff *skb);

static const struct h4_recv_pkt aml_coex_recv_pkts[] = {
    { H4_RECV_ACL,   .recv = hci_recv_frame },
    { H4_RECV_SCO,   .recv = hci_recv_frame },
    { H4_RECV_EVENT, .recv = amlbt_coex_recv_event },
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    { H4_RECV_ISO,    .recv = hci_recv_frame },
#endif
    { H4_RECV_15P4,  .recv = aml_15p4_recv_frame },
};

static int aml_thread_recv_frame(struct hci_dev *hdev, struct sk_buff *skb)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    //unsigned char pkt_type;

    if (!skb) {
        BTE("%s: invalid param", __func__);
        return -EINVAL;
    }

    //pkt_type = hci_skb_pkt_type(skb);
    //BTI("%s: Thread pkt, type %#x, len %d", __func__, pkt_type, skb->len);
    BTD("%s: Thread pkt, len %d", __func__, skb->len);

    if (skb_headroom(skb) < 1) {
        BTE("no headroom\n");
        kfree_skb(skb);
        return -ENOMEM;
    }

    skb_push(skb, 1);
    skb->data[0] = HCI_15P4_PKT;
    BTD("%s:thread rx: %02x %02x\n", __func__,skb->data[0], skb->data[1]);
    skb_queue_tail(&p_bt->thread_res.thread_rx_queue, skb);
    wake_up_interruptible(&p_bt->thread_res.thread_wait_queue);
    return 0;
}

static int aml_zigbee_recv_frame(struct hci_dev *hdev, struct sk_buff *skb)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    //unsigned char pkt_type;

    if (!skb) {
        BTE("%s: invalid param", __func__);
        return -EINVAL;
    }

    //pkt_type = hci_skb_pkt_type(skb);
    //BTI("%s: zigbee pkt, type %#x, len %d", __func__, pkt_type, skb->len);
    BTD("%s: zigbee pkt, len %d", __func__, skb->len);

    if (skb_headroom(skb) < 1) {
        BTE("no headroom\n");
        kfree_skb(skb);
        return -ENOMEM;
    }

    skb_push(skb, 1);
    skb->data[0] = HCI_15P4_PKT;
    BTD("%s:zigbee rx: %02x %02x\n", __func__,skb->data[0], skb->data[1]);
    skb_queue_tail(&p_bt->zigbee_res.zigbee_rx_queue, skb);
    wake_up_interruptible(&p_bt->zigbee_res.zigbee_wait_queue);
    return 0;
}

static int aml_15p4_recv_frame(struct hci_dev *hdev, struct sk_buff *skb)
{
    unsigned char pkt_type;
    unsigned char sub_pkt_type;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    if (!skb) {
        BTE("%s: invalid param",__func__);
        return -EINVAL;
    }

    pkt_type = hci_skb_pkt_type(skb);
    BTD("%s: 15.4 pkt, type %#x, len %d", __func__, pkt_type, skb->len);

    if (pkt_type != HCI_15P4_PKT) {
        BTE("%s: not 15.4 pkt (type %#x), drop", __func__, pkt_type);
        kfree_skb(skb);
        return -EINVAL;
    }

    sub_pkt_type = skb->data[0];
    switch (sub_pkt_type) {
        case HCI_15P4_THREAD_PKT:
            aml_thread_recv_frame(hdev, skb);
            break;
        case HCI_15P4_ZIGBEE_PKT:
            aml_zigbee_recv_frame(hdev, skb);
            break;
        case HCI_15P4_LOG_PKT:
            if (p_bt->zigbee_res.zigbee_start) {
                aml_zigbee_recv_frame(hdev, skb);
            } else if (p_bt->thread_res.thread_start) {
                aml_thread_recv_frame(hdev, skb);
            }  else {
                BTE("%s: No handler for sub-type %#x", __func__, sub_pkt_type);
                kfree_skb(skb);
                return -EINVAL;
            }
             break;
        default:
            BTE("%s: unknown 15.4 sub-type %#x, drop", __func__, sub_pkt_type);
            kfree_skb(skb);
            return -EINVAL;
    }
    return 0;
}

static int amlbt_coex_recv_event(struct hci_dev *hdev, struct sk_buff *skb)
{
    int ret = 0;

    ret = amlbt_intf_uart_check_hci_event(skb);
    if (ret == 0) {
        BTD("event skip -> stack\n");
        hci_recv_frame(hdev, skb);
    } else {
        BTD("event match (ret=%d)\n", ret);
        kfree_skb(skb);
    }
    return 0;
}

struct sk_buff *aml_h4_recv_buf(struct hci_dev *hdev, struct sk_buff *skb,
                const unsigned char *buffer, int count,
                const struct h4_recv_pkt *pkts, int pkts_count)
{
    struct hci_uart *hu = hci_get_drvdata(hdev);
    u8 alignment = hu->alignment ? hu->alignment : 1;

    /* Check for error from previous call */
    if (IS_ERR(skb)) {
        BTE("%s skb err\n", __func__);
        skb = NULL;
    }
    BTD("%s:count:%d\n", __func__, count);
    BTD("%s:buffer:[%#x,%#x,%#x,%#x]\n", __func__, buffer[0], buffer[1], buffer[2], buffer[3]);
    while (count) {
        int i, len;

        /* remove padding bytes from buffer */
        for (; hu->padding && count > 0; hu->padding--) {
            count--;
            buffer++;
        }
        if (!count)
            break;

        if (!skb) {
            for (i = 0; i < pkts_count; i++) {
                if (buffer[0] != (&pkts[i])->type)
                    continue;

                skb = bt_skb_alloc((&pkts[i])->maxlen,
                           GFP_ATOMIC);
                if (!skb) {
                    BTE("%s bt_skb_alloc failed\n", __func__);
                    return ERR_PTR(-ENOMEM);
                }
                hci_skb_pkt_type(skb) = (&pkts[i])->type;
                hci_skb_expect(skb) = (&pkts[i])->hlen;
                break;
            }
            BTD("aml_h4_recv_buf hci_skb_pkt_type(skb) %#x\n", hci_skb_pkt_type(skb));

            /* Check for invalid packet type */
            if (!skb) {
                BTE("%s invalid packet type: %#x\n", __func__, buffer[0]);
                return ERR_PTR(-EILSEQ);
            }
            count -= 1;
            buffer += 1;
        }

        BTD("buffer:[%#x,%#x,%#x,%#x]\n", buffer[0], buffer[1], buffer[2], buffer[3]);
        len = min_t(uint, hci_skb_expect(skb) - skb->len, count);
        skb_put_data(skb, buffer, len);

        count -= len;
        buffer += len;
        BTD("aml_h4_recv_buf len %#x skb->len %#x hci_skb_expect(skb) %#x\n", len, skb->len, hci_skb_expect(skb));

        /* Check for partial packet */
        if (skb->len < hci_skb_expect(skb))
            continue;

        for (i = 0; i < pkts_count; i++) {
            if (hci_skb_pkt_type(skb) == (&pkts[i])->type)
                break;
        }

        if (i >= pkts_count) {
            BTE("%s unknown packet type in completion: %#x\n", __func__, hci_skb_pkt_type(skb));
            kfree_skb(skb);
            return ERR_PTR(-EILSEQ);
        }

        if (skb->len == (&pkts[i])->hlen) {
            u16 dlen;

            switch ((&pkts[i])->lsize) {
            case 0:
                /* No variable data length */
                dlen = 0;
                break;
            case 1:
                /* Single octet variable length */
                dlen = skb->data[(&pkts[i])->loff];
                hci_skb_expect(skb) += dlen;
                BTD("1 dlen %#x\n", dlen);

                if (skb_tailroom(skb) < dlen) {
                    BTE("%s lsize=1, tailroom insufficient\n", __func__);
                    kfree_skb(skb);
                    return ERR_PTR(-EMSGSIZE);
                }
                break;
            case 2:
                /* Double octet variable length */
                dlen = get_unaligned_le16(skb->data +
                              (&pkts[i])->loff);
                hci_skb_expect(skb) += dlen;
                BTD("2 %#x\n", dlen);

                if (skb_tailroom(skb) < dlen) {
                    BTE("%s lsize=2, tailroom insufficient\n", __func__);
                    kfree_skb(skb);
                    return ERR_PTR(-EMSGSIZE);
                }
                break;
            case 3:
                /* Double octet variable length */
                dlen = get_unaligned_le16(skb->data +
                              (&pkts[i])->loff);
                dlen += 2;
                hci_skb_expect(skb) += dlen;
                BTD("3 %#x\n", dlen);

                if (skb_tailroom(skb) < dlen) {
                    BTE("%s lsize=3, tailroom insufficient\n", __func__);
                    kfree_skb(skb);
                    return ERR_PTR(-EMSGSIZE);
                }
                break;
            default:
                /* Unsupported variable length */
                BTE("%s unsupported lsize: %d\n", __func__, (&pkts[i])->lsize);
                kfree_skb(skb);
                return ERR_PTR(-EILSEQ);
            }

            if (!dlen) {
                hu->padding = (skb->len + 1) % alignment;
                hu->padding = (alignment - hu->padding) % alignment;

                /* No more data, complete frame */
                (&pkts[i])->recv(hdev, skb);
                skb = NULL;
                BTD("i %#x\n", i);
            }
        } else {
            hu->padding = (skb->len + 1) % alignment;
            hu->padding = (alignment - hu->padding) % alignment;

            /* Complete frame */
            (&pkts[i])->recv(hdev, skb);
            skb = NULL;
            BTD("J %#x\n", i);
        }
    }

   return skb;
}

static int aml_coex_common_txq_init(amlbt_t *p_bt)
{
    if (!p_bt)
        return -EINVAL;

    skb_queue_head_init(&p_bt->common_res.tx_queue);
    return 0;
}

static int aml_coex_open(struct hci_uart *hu)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    amlbt_res_uart_linux_t *uart_res_linux = &p_bt->uart_res_linux;

    BTI("%s, hu %p\n", __func__, hu);

    if (!uart_res_linux->initialized) {
        uart_res_linux->initialized = true;
        uart_res_linux->rx_skb = NULL;
    }

    skb_queue_purge(&p_bt->common_res.tx_queue);

    hu->priv = uart_res_linux;
    uart_res_linux->hu = hu;
    return 0;
}

static int aml_coex_flush(struct hci_uart *hu)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    amlbt_res_uart_linux_t *uart_res_linux = hu->priv;
    struct sk_buff *skb, *tmp;
    unsigned long flags;
    unsigned char pkt_type;

    if (!uart_res_linux) {
        BTE("%s: uart_linux_res is NULL\n", __func__);
        return -EINVAL;
    }

    if (&p_bt->uart_res_linux != uart_res_linux) {
        BTE("%s: uart_res_linux instance mismatch! expected: %p, actual: %p\n",
            __func__, &p_bt->uart_res_linux, uart_res_linux);
        return -EINVAL;
    }

    BTI("%s: flush bt pkts hu=%p\n", __func__, hu);

    spin_lock_irqsave(&p_bt->common_res.tx_queue.lock, flags);
    skb_queue_walk_safe(&p_bt->common_res.tx_queue, skb, tmp) {
        pkt_type = hci_skb_pkt_type(skb);

        switch (pkt_type) {
        case HCI_COMMAND_PKT:
        case HCI_ACLDATA_PKT:
        case HCI_SCODATA_PKT:
        case HCI_ISO_PKT:
            BTI(" flush BT pkt type 0x%02x\n", pkt_type);
            skb_unlink(skb, &p_bt->common_res.tx_queue);
            kfree_skb(skb);
            break;

        case HCI_15P4_PKT:
            BTI(" keep 15.4 pkt type 0x%02x\n", pkt_type);
            break;

        default:
            BTE(" unknown pkt type 0x%02x, keep\n", pkt_type);
            break;
        }
    }
    spin_unlock_irqrestore(&p_bt->common_res.tx_queue.lock, flags);
    return 0;
}

/* Close protocol */
static int aml_coex_close(struct hci_uart *hu)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    amlbt_res_uart_linux_t *uart_res_linux = hu->priv;

    BTI("%s, close hu %p\n", __func__, hu);

    if (!uart_res_linux) {
        BTE("uart_linux_res already freed!\n");
        return 0;
    }

    skb_queue_purge(&p_bt->common_res.tx_queue);

    if (uart_res_linux->rx_skb) {
      kfree_skb(uart_res_linux->rx_skb);
      uart_res_linux->rx_skb = NULL;
    }

    if (uart_res_linux) {
        uart_res_linux->initialized = false;
        uart_res_linux->hu = NULL;
        hu->priv = NULL;
    }

    return 0;
}

/* Enqueue frame for transmittion (padding, crc, etc) */
static int aml_coex_enqueue(struct hci_uart *hu, struct sk_buff *skb)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    amlbt_res_uart_linux_t *uart_res_linux = hu->priv;
    unsigned char pkt_type;

    if (!uart_res_linux || !skb) {
        BTE("%s: invalid param (uart_linux_res %p, skb %p)", __func__, uart_res_linux, skb);
        return -EINVAL;
    }

    pkt_type = hci_skb_pkt_type(skb);
    BTD("%s: pkt_type %#x, skb %p", __func__, pkt_type, skb);

    if (!IS_VALID_PKT_TYPE(pkt_type)) {
        BTE("%s: invalid pkt_type %#x, drop skb %p", __func__, pkt_type, skb);
        kfree_skb(skb);
        return -EINVAL;
    }

    /* Prepend skb with frame type */
    memcpy(skb_push(skb, 1), &pkt_type, 1);
    skb_queue_tail(&p_bt->common_res.tx_queue, skb);

    BTD("%s, pkt_type %#x, skb len %d", __func__, pkt_type, skb->len);
    return 0;
}

/* Recv data */
static int aml_coex_recv(struct hci_uart *hu, const void *data, int count)
{
    amlbt_res_uart_linux_t *uart_res_linux = hu->priv;

    if (!uart_res_linux || !data || count <= 0) {
        BTE("%s: invalid param (uart_res_linux %p, data %p, count %d)",
            __func__, uart_res_linux, data, count);
        return -EINVAL;
    }

    if (!test_bit(HCI_UART_REGISTERED, &hu->flags)) {
        BTE("aml_coex_recv HCI_UART_REGISTERED flag err!\n");
        return -EUNATCH;
    }

    uart_res_linux->rx_skb = aml_h4_recv_buf(hu->hdev, uart_res_linux->rx_skb, data, count,
                 aml_coex_recv_pkts, ARRAY_SIZE(aml_coex_recv_pkts));
    if (IS_ERR(uart_res_linux->rx_skb)) {
        int err = PTR_ERR(uart_res_linux->rx_skb);
        BTE("%s uart_res_linux->rx_skb err!\n",__func__);
        bt_dev_err(hu->hdev, "Frame reassembly failed (%d)", err);
        uart_res_linux->rx_skb = NULL;
        return err;
    }

    return count;
}

static struct sk_buff *aml_coex_dequeue(struct hci_uart *hu)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    amlbt_res_uart_linux_t *uart_res_linux = hu->priv;

    BTD("%s: hu %p", __func__, hu);
    if (!uart_res_linux) {
        BTE("%s: uart_res_linux is NULL (hu->priv invalid)", __func__);
        return NULL;
    }

    return skb_dequeue(&p_bt->common_res.tx_queue);
}

static const struct hci_uart_proto aml_coex_proto = {
    .id     = HCI_UART_LL,
    .name       = "AML",
    .open       = aml_coex_open,
    .close      = aml_coex_close,
    .recv       = aml_coex_recv,
    .enqueue    = aml_coex_enqueue,
    .dequeue    = aml_coex_dequeue,
    .flush      = aml_coex_flush,
};

int aml_coex_init(void)
{
    BTI("%s, %s", __func__, AML_COEX_VERSION);
    return amlbt_hci_uart_register_proto(&aml_coex_proto);
}

void aml_coex_deinit(void)
{
    BTI("%s", __func__);
    amlbt_hci_uart_unregister_proto(&aml_coex_proto);
}

