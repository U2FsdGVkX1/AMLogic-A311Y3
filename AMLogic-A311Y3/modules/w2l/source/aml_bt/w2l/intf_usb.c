/*
* Copyright (c) 202X Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/usb.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/io.h>
#include <linux/compat.h>
#include <linux/firmware.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/reboot.h>
#include <linux/time.h>
#include <linux/ktime.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/poll.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/amlogic/pm.h>
#include <linux/firmware.h>
#include <linux/jiffies.h>
#include <linux/wait.h>
#include <linux/input.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0))
#include <linux/panic_notifier.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/clock.h>
#endif
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#include "common.h"
#include "intf.h"
#include "intf_usb.h"
#include "rc_list.h"
#include "debug_dev.h"
#include "driver.h"
#include "chip.h"

struct crg_msc_cbw *g_cmd_buf;

static void amlbt_intf_usb_build_cbw(struct crg_msc_cbw *cbw_buf,
                               unsigned char dir,
                               unsigned int len,
                               unsigned char cdb1,
                               unsigned int cdb2,
                               unsigned long cdb3,
                               unsigned long cdb4)
{
    cbw_buf->sig = AML_SIG_CBW;
    cbw_buf->tag = 0x5da729a0;
    cbw_buf->data_len = len;
    cbw_buf->flag = dir; //direction
    cbw_buf->len = 16; //command length
    cbw_buf->lun = 0;

    cbw_buf->cdb[0] = cdb1;
    cbw_buf->cdb[1] = cdb2; // read or write addr
    cbw_buf->cdb[2] = (unsigned int)(unsigned long)cdb3;
    cbw_buf->cdb[3] = cdb4; //read or write data length
}

static void amlbt_intf_usb_build_cbw_add_data(struct crg_msc_cbw *cbw_buf,
                               unsigned char dir,
                               unsigned int len,
                               unsigned char cdb1,
                               unsigned int cdb2,
                               unsigned long cdb3,
                               SYS_TYPE cdb4,unsigned char *data)
{
    cbw_buf->sig = AML_SIG_CBW;
    cbw_buf->tag = 0x5da729a0;
    cbw_buf->data_len = len;
    cbw_buf->flag = dir; //direction
    cbw_buf->len = 16; //command length
    cbw_buf->lun = 0;

    cbw_buf->cdb[0] = cdb1;
    cbw_buf->cdb[1] = cdb2; // read or write addr
    cbw_buf->cdb[2] = (unsigned int)(unsigned long)cdb3;
    cbw_buf->cdb[3] = cdb4; //read or write data length
    memcpy(cbw_buf->resv + 1, (unsigned char *) data, len);
    /*in case call cmd and data mode but fw call cmd+data stage*/
    cbw_buf->resv[479] = cbw_buf->resv[480] = 0xFF;
}

int amlbt_intf_usb_check(void)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    if (g_udev == NULL)
    {
        BTE("interface NULL");
        return -1;
    }

    if (bus_state_detect.bus_err || bus_state_detect.bus_reset_ongoing || bus_state_detect.is_recy_ongoing
        || (enum usb_udev_state)g_udev->state != USB_CONFIGURED || p_bt->excp_res.notify_recy
#ifdef CONFIG_AML_BT_USB_HOTPLUG
        || bus_state_detect.usb_unplug
#endif
        )
    {
        if (!(p_bt->pm_res.dr_state & BT_DRV_STATE_WAIT_RECOVERY))
        {
            amlbt_intf_drv_state_set(BT_DRV_STATE_WAIT_RECOVERY);
        }
#ifdef CONFIG_AML_BT_USB_HOTPLUG
        BTE("bus_err %d reset %d recy %d udev %d, unplug %d notify %d", bus_state_detect.bus_err, bus_state_detect.bus_reset_ongoing,
                    bus_state_detect.is_recy_ongoing, g_udev->state, bus_state_detect.usb_unplug, p_bt->excp_res.notify_recy);
#else
        BTE("bus_err %d reset %d recy %d udev %d notify %d", bus_state_detect.bus_err, bus_state_detect.bus_reset_ongoing,
                    bus_state_detect.is_recy_ongoing, g_udev->state, p_bt->excp_res.notify_recy);
#endif
        return -1;
    }
    return 0;
}

static int amlbt_intf_usb_write_reg_by_ep(unsigned int addr, unsigned int value, unsigned int len, unsigned int ep)
{
    int ret = 0;
    int actual_length = 0;

    USB_BEGIN_LOCK();
    memset(g_cmd_buf, 0, sizeof(*g_cmd_buf));
    amlbt_intf_usb_build_cbw(g_cmd_buf, AML_XFER_TO_DEVICE, 0, CMD_WRITE_REG, addr, value, len);
    /* cmd stage */
    ret = usb_bulk_msg(g_udev, (unsigned int)usb_sndbulkpipe(g_udev, ep), (void *)g_cmd_buf,
    sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        BTE("amlbt_intf_usb_write_reg_by_ep Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d, value: 0x%x\n",
            ret, ep, addr, len, value);
        USB_END_LOCK();
        return ret;
    }
    USB_END_LOCK();

    return 0; //bt write maybe use the value
}

int amlbt_intf_usb_write_word(unsigned int addr,unsigned int data, unsigned int ep)
{
    int len = 4;
    int ret = 0;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    if (p_bt->pm_res.dr_state & BT_DRV_STATE_SUSPEND)
    {
        BTE("%d:suspend state %d\n", __LINE__, p_bt->pm_res.dr_state);
        return -2;
    }

    ret = amlbt_intf_usb_check();

    if (ret != 0)
    {
        BTE("%s:%d, error!\n", __func__, __LINE__);
        return -1;
    }


    switch (ep) {
        case USB_EP2:
        case USB_EP4:
            ret = amlbt_intf_usb_write_reg_by_ep(addr, data, len, ep);
            if (ret != 0)
            {
                return ret;
            }
            break;
        default:
            BTE("EP-%d unsupported!\n", ep);
            break;
    }
    return 0;
}

static int amlbt_intf_usb_read_reg_by_ep(unsigned int addr, unsigned int len, unsigned int ep, unsigned int *value)
{
    int ret = 0;
    int actual_length = 0;
    unsigned char *kmalloc_buf = NULL;

    USB_BEGIN_LOCK();

    kmalloc_buf = (unsigned char *)kzalloc(len, GFP_DMA | GFP_ATOMIC);

    if (!kmalloc_buf) {
        BTE("amlbt_intf_usb_read_reg_by_ep data malloc fail, ep: %d, addr: 0x%x, len: %d\n", ep, addr, len);
        goto err_unlock;
    }
    memset(g_cmd_buf, 0, sizeof(*g_cmd_buf));
    amlbt_intf_usb_build_cbw(g_cmd_buf, AML_XFER_TO_HOST, len, CMD_READ_REG, addr, 0, len);

    /* cmd stage */
    ret = usb_bulk_msg(g_udev, usb_sndbulkpipe(g_udev, ep), (void *)g_cmd_buf,
                    sizeof(*g_cmd_buf), &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        BTE("amlbt_intf_usb_read_reg_by_ep cmd Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d\n", ret, ep, addr, len);
        goto err_kfree;
    }

    /* data stage */
    ret = usb_bulk_msg(g_udev, usb_rcvbulkpipe(g_udev, ep), (void *)kmalloc_buf, len, &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        BTE("amlbt_intf_usb_read_reg_by_ep data Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d\n", ret ,ep, addr, len);
        goto err_kfree;
    }
    if (actual_length > len)
    {
        BTE("%s:%d, error! actual_length %#x\n", __func__, __LINE__, actual_length);
        goto err_kfree;
    }

    memcpy(value, kmalloc_buf, actual_length);
    kfree(kmalloc_buf);
    USB_END_LOCK();

    return ret;
err_kfree:
    kfree(kmalloc_buf);
err_unlock:
    USB_END_LOCK();
    return ret;
}

int amlbt_intf_usb_read_word(unsigned int addr, unsigned int ep, unsigned int *value)
{
    int len = 4;
    int ret = 0;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    if (p_bt->pm_res.dr_state & BT_DRV_STATE_SUSPEND)
    {
        BTE("%d:suspend state %d\n", __LINE__, p_bt->pm_res.dr_state);
        return -2;
    }

    ret = amlbt_intf_usb_check();

    if (ret != 0)
    {
        BTE("%s:%d, error!\n", __func__, __LINE__);
        return -1;
    }

    switch (ep) {
        case USB_EP2:
        case USB_EP4:
            ret = amlbt_intf_usb_read_reg_by_ep(addr, len, ep, value);
            if (ret != 0)
            {
                return ret;
            }
            break;
        default:
            BTE("EP-%d unsupported!\n", ep);
            break;
    }
    return ret;
}

static int amlbt_intf_usb_write_sram_by_ep(unsigned char *pdata, unsigned int addr, unsigned int len, unsigned int ep)
{
    int ret = 0;
    int actual_length = 0;
    unsigned char *kmalloc_buf = NULL;

    USB_BEGIN_LOCK();

    //if (len < WRITE_SRAM_DATA_LEN)
    if (0)
    {
        memset(g_cmd_buf, 0, sizeof(*g_cmd_buf));
        amlbt_intf_usb_build_cbw_add_data(g_cmd_buf, AML_XFER_TO_DEVICE, len, CMD_WRITE_SRAM, addr, 0, len,pdata);
        /* cmd stage */
        ret = usb_bulk_msg(g_udev, usb_sndbulkpipe(g_udev, ep), (void *)g_cmd_buf, sizeof(*g_cmd_buf),
                &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
        if (ret) {
            BTE("amlbt_intf_usb_write_sram_by_ep 1 Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d\n", ret, ep, addr, len);
            BTE("usb command transmit fail,g_cmd_buf->add is %d,len is %d\n", addr, len);
            goto err_unlock;
        }
        g_cmd_buf->resv[479] = g_cmd_buf->resv[480] = 0;
    }
    else
    {
        memset(g_cmd_buf, 0, sizeof(*g_cmd_buf));
        amlbt_intf_usb_build_cbw(g_cmd_buf, AML_XFER_TO_DEVICE, len, CMD_WRITE_SRAM, addr, 0, len);
        /* cmd stage */
        ret = usb_bulk_msg(g_udev, usb_sndbulkpipe(g_udev, ep), (void *)g_cmd_buf, sizeof(*g_cmd_buf),
                &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
        if (ret) {
            BTE("amlbt_intf_usb_write_sram_by_ep 2 Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d\n", ret, ep, addr, len);
            BTE("usb command transmit fail,g_cmd_buf->add is %d,len is %d\n", addr, len);
            goto err_unlock;
        }

        kmalloc_buf = (unsigned char *)kzalloc(len, GFP_DMA | GFP_ATOMIC);
        if (kmalloc_buf == NULL)
        {
            BTE("kmalloc buf fail, ep: %d, addr: 0x%x, len: %d\n", ep, addr, len);
            goto err_unlock;
        }

        memcpy(kmalloc_buf, pdata, len);
        /* data stage */
        ret = usb_bulk_msg(g_udev, usb_sndbulkpipe(g_udev, ep), (void *)kmalloc_buf, len,
                &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
        if (ret) {
            BTE("amlbt_intf_usb_write_sram_by_ep data Failed to usb_bulk_msg, ret %d, ep: %d,  addr: 0x%x, len: %d\n", ret, ep, addr, len);
            goto err_kfree;
        }
        kfree(kmalloc_buf);
    }
    USB_END_LOCK();

    return ret;
err_kfree:
    kfree(kmalloc_buf);
err_unlock:
    USB_END_LOCK();
    return ret;
}

int amlbt_intf_usb_write_sram(unsigned char *buf, unsigned char *sram_addr, unsigned int len, unsigned int ep)
{
    int ret = 0;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    if (p_bt->pm_res.dr_state & BT_DRV_STATE_SUSPEND)
    {
        BTE("%d:suspend state %d\n", __LINE__, p_bt->pm_res.dr_state);
        return -2;
    }

    if (len == 0)
    {
        BTE("EP-%d write len err!\n", ep);
        return -1;
    }

    ret = amlbt_intf_usb_check();

    if (ret != 0)
    {
        BTE("%s:%d, error!\n", __func__, __LINE__);
        return -1;
    }

    switch (ep) {
        case USB_EP2:
        case USB_EP4:
            ret = amlbt_intf_usb_write_sram_by_ep(buf, (unsigned int)(unsigned long)sram_addr, len, ep);
            if (ret != 0)
            {
                return ret;
            }
            break;
        default:
            BTE("EP-%d unsupported!\n", ep);
            break;
    }
    return ret;
}

static int amlbt_intf_usb_read_sram_by_ep(unsigned char *pdata, unsigned int addr, unsigned int len, unsigned int ep)
{
    int ret = 0;
    int actual_length = 0;
    unsigned char *kmalloc_buf = NULL;

    USB_BEGIN_LOCK();
    memset(g_cmd_buf, 0, sizeof(*g_cmd_buf));
    amlbt_intf_usb_build_cbw(g_cmd_buf,  AML_XFER_TO_HOST, len, CMD_READ_SRAM, addr, 0, len);
    /* cmd stage */
    ret = usb_bulk_msg(g_udev, usb_sndbulkpipe(g_udev, ep), (void *)g_cmd_buf, sizeof(*g_cmd_buf),
            &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        BTE("amlbt_intf_usb_read_sram_by_ep cmd Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d\n", ret, ep, addr, len);
        BTE("usb command transmit fail,g_cmd_buf->add is %d,len is %d\n",addr,len);
        goto err_unlock;
    }

    kmalloc_buf = (unsigned char *)kzalloc(len, GFP_DMA | GFP_ATOMIC);
    if (kmalloc_buf == NULL)
    {
        BTE("kmalloc buf fail, ep: %d, len: %d\n", ep, len);
        goto err_unlock;
    }

    /* data stage */
    ret = usb_bulk_msg(g_udev, usb_rcvbulkpipe(g_udev, ep),(void *)kmalloc_buf, len,
            &actual_length, AML_USB_CONTROL_MSG_TIMEOUT);
    if (ret) {
        BTE("amlbt_intf_usb_read_sram_by_ep cmd Failed to usb_bulk_msg, ret %d, ep: %d, addr: 0x%x, len: %d\n", ret, ep, addr, len);
        goto err_kfree;
    }
    if (actual_length > len)
    {
        BTE("%s:%d, error! actual_length %#x\n", __func__, __LINE__, actual_length);
        goto err_kfree;
    }
    memcpy(pdata, kmalloc_buf, actual_length);
    kfree(kmalloc_buf);

    USB_END_LOCK();
    return ret;
err_kfree:
    kfree(kmalloc_buf);
err_unlock:
    USB_END_LOCK();
    return ret;
}

int amlbt_intf_usb_read_sram(unsigned char *buf,unsigned char *sram_addr, unsigned int len, unsigned int ep)
{
    int ret = 0;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    if (p_bt->pm_res.dr_state & BT_DRV_STATE_SUSPEND)
    {
        BTE("%d:suspend state %d\n", __LINE__, p_bt->pm_res.dr_state);
        return -2;
    }

    if (len == 0)
    {
        BTE("EP-%d read len err!\n", ep);
        return -1;
    }

    ret = amlbt_intf_usb_check();

    if (ret != 0)
    {
        BTE("%s:%d, error!\n", __func__, __LINE__);
        return -1;
    }

    switch (ep) {
        case USB_EP2:
        case USB_EP4:
            ret = amlbt_intf_usb_read_sram_by_ep(buf, (unsigned int)(unsigned long)sram_addr, len, ep);
            if (ret != 0)
            {
                return ret;
            }
            break;
        default:
            BTE("EP-%d unsupported!\n", ep);
            break;
    }
    return ret;
}
#if 0
/*
type 0x04: 1 byte
head[evt code, length]: 2 bytes
payload
*/
static int amlbt_intf_usb_event_to_skb_process(amlbt_t *p_bt, unsigned char *evt_buf, unsigned int len)
{
    struct sk_buff *skb = NULL;
    unsigned int read_len = 0;
    unsigned int i = 0;
    unsigned char *p = evt_buf;
    int ret = 0;
    unsigned int push_len = 0;

    //BTI("process evt %d\n", len);

    while (i < len)
    {
        read_len = 3 + p[2];
        push_len = read_len;
        read_len = (read_len + 3) & ~3;
        if (i + read_len > len)
        {
            BTE("%s: Event exceeds buffer [%u,%u,%u], %u > %u\n", __func__, i, push_len, read_len, i+read_len, len);
            DUMP_BUF("evt_buf", evt_buf, len);
            DUMP_BUF("p", p, len - (p - evt_buf));
            return -EMSGSIZE;
        }
        /* coverity[var_assign:SUPPRESS] */
        /* coverity[alloc_fn:SUPPRESS] */
        skb = alloc_skb(read_len, GFP_KERNEL);
        if (!skb)
        {
            BTE("%s:%d alloc_skb Failed\n", __func__, __LINE__);
            ret = -ENOMEM;
            goto error;
        }
        //BTI("push evt %d[%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", read_len,
        //    p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
        /* coverity[noescape:SUPPRESS] */
        if (skb_tailroom(skb) < read_len) {
            BTE("%s skb_tailroom(skb) failed!\n", __func__);
            ret = -ENOSPC;
            goto error;
        }
        /* coverity[noescape:SUPPRESS] */
        if (skb_put_data(skb, p, push_len) == NULL)
        {
            BTE("%s: skb_put_data failed\n", __func__);
            ret = -EFAULT;
            goto error;
        }
        /* coverity[noescape:SUPPRESS] */
        skb_queue_tail(&p_bt->bt_res.bt_rx_queue, skb);
        p += read_len;
        i += read_len;
    }
    return 0;
error:
    if (skb) {
    /* coverity[noescape:SUPPRESS] */
        kfree_skb(skb);
    }
    /* coverity[leaked_storage:SUPPRESS] */
    return ret;
}

/*
type 0x02: 4 byte
head[handle 2bytes, length 2bytes]: 4 bytes
payload
*/
static int amlbt_intf_usb_data_to_skb_process(amlbt_t *p_bt, unsigned char *data_buf, unsigned int len)
{
    struct sk_buff *skb = NULL;
    unsigned int read_len = 0;
    unsigned int pkt_len = 0;
    unsigned int i = 0;
    unsigned char *p = data_buf;
    int ret = 0;

    //BTI("process data %d\n", len);

    while (i < len)
    {
        if (i + 8 > len)
        {
            BTE("%s: Not enough data for packet header [i=%u, len=%u]\n", \
                __func__, i, len);
            return -EMSGSIZE;
        }
        read_len = 5 + ((p[7] << 8) | (p[6]));
        //read_len = (read_len + 3) & ~3;
        pkt_len = 8 + ((p[7] << 8) | (p[6]));
        pkt_len = (pkt_len + 3) & ~3;
        if (read_len < 5)
        {
            BTE("%s: Invalid read_len: %u (should be >= 5)\n", \
                __func__, read_len);
            return -EINVAL;
        }
        if (i + pkt_len > len)
        {
            BTE("%s: Data exceeds buffer [%u,%u,%u], %u > %u\n", __func__, i, pkt_len, read_len, i+pkt_len, len);
            DUMP_BUF("data_buf", data_buf, len);
            DUMP_BUF("p", p, len - (p - data_buf));
            return -EMSGSIZE;
        }
        /* coverity[var_assign:SUPPRESS] */
        /* coverity[alloc_fn:SUPPRESS] */
        /* coverity[tainted_data:SUPPRESS] */
        skb = alloc_skb(read_len, GFP_KERNEL);
        if (!skb)
        {
            BTE("%s:%d alloc_skb Failed\n", __func__, __LINE__);
            ret = -ENOMEM;
            goto error;
        }
        //BTI("push data %d,%d[%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", pkt_len, read_len,
        //    p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
        /* coverity[noescape:SUPPRESS] */
        if (skb_tailroom(skb) < read_len) {
            BTE("%s skb_tailroom(skb) failed!\n", __func__);
            ret = -ENOSPC;
            goto error;
        }
        /* coverity[noescape:SUPPRESS] */
        if (skb_put_data(skb, p, 1) == NULL) //The type field is 4-byte aligned, but only one byte is actually used.
        {
            BTE("%s: skb_put_data failed\n", __func__);
            ret = -EFAULT;
            goto error;
        }
        p += 4;       //The type field is 4-byte aligned, but only one byte is actually used.
        /* coverity[noescape:SUPPRESS] */
        /* coverity[overflow_const:SUPPRESS] */
        if (skb_put_data(skb, p, read_len-1) == NULL)
        {
            BTE("%s: skb_put_data failed\n", __func__);
            ret = -EFAULT;
            goto error;
        }
        /* coverity[noescape:SUPPRESS] */
        skb_queue_tail(&p_bt->bt_res.bt_rx_queue, skb);
        p -= 4;
        p += pkt_len;
        i += pkt_len;
    }

    return 0;
error:
    if (skb) {
    /* coverity[noescape:SUPPRESS] */
        kfree_skb(skb);
    }
    /* coverity[leaked_storage:SUPPRESS] */
    return ret;
}

/*
type 0x10: 1 byte
head[MHDL(zigbee 0xF5, thread 0xFA), MID, length 2bytes]: 4 bytes
payload + 2 bytes crc
*/
static int amlbt_intf_usb_15p4_to_skb_process(amlbt_t *p_bt, unsigned char *data_buf, unsigned int len)
{
    struct sk_buff *skb = NULL;
    unsigned int read_len = 0;
    unsigned int pkt_len = 0;
    unsigned int i = 0;
    unsigned char *p = data_buf;
    int ret = 0;

    //BTI("process iot %d\n", len);

    while (i < len)
    {
      read_len = 7 + ((p[4] << 8) | (p[3]));
      //read_len = (read_len + 3) & ~3;
      pkt_len = 7 + ((p[4] << 8) | (p[3]));
      pkt_len = (pkt_len + 3) & ~3;
      if (i + pkt_len > len)
      {
        BTE("%s: 15p4 exceeds buffer [%u,%u,%u], %u > %u\n", __func__, i, pkt_len, read_len, i+pkt_len, len);
        DUMP_BUF("data_buf", data_buf, len);
        DUMP_BUF("p", p, len - (p - data_buf));
        return -EMSGSIZE;
      }
      /* coverity[var_assign:SUPPRESS] */
      /* coverity[alloc_fn:SUPPRESS] */
      /* coverity[tainted_data:SUPPRESS] */
      skb = alloc_skb(read_len, GFP_KERNEL);
      if (!skb)
      {
        BTE("%s:%d alloc_skb Failed\n", __func__, __LINE__);
        ret = -ENOMEM;
        goto error;
      }
      //BTI("push data %d[%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", read_len,
      //	  p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
      /* coverity[noescape:SUPPRESS] */
      if (skb_tailroom(skb) < read_len) {
        BTE("%s skb_tailroom(skb) failed!\n", __func__);
        ret = -ENOSPC;
        goto error;
      }
      /* coverity[noescape:SUPPRESS] */
      if (skb_put_data(skb, p, read_len) == NULL)
      {
        BTE("%s: skb_put_data failed\n", __func__);
        ret = -EFAULT;
        goto error;
      }
      if ((p[1] == HCI_15P4_ZIGBEE_PKT || p[1] == HCI_15P4_LOG_PKT) && p_bt->zigbee_res.zigbee_start)
      {
          /* coverity[noescape:SUPPRESS] */
          skb_queue_tail(&p_bt->zigbee_res.zigbee_rx_queue, skb);
      }
      else if ((p[1] == HCI_15P4_THREAD_PKT || p[1] == HCI_15P4_LOG_PKT) && p_bt->thread_res.thread_start)
      {
          /* coverity[noescape:SUPPRESS] */
          skb_queue_tail(&p_bt->thread_res.thread_rx_queue, skb);
      }
      else
      {
        BTE("%s: Unknown packet type  %#x\n", __func__, p[1]);
        kfree_skb(skb);
        return -EPROTO;
      }
      p += pkt_len;
      i += pkt_len;
    }

    return 0;
error:
    if (skb) {
      /* coverity[noescape:SUPPRESS] */
      kfree_skb(skb);
    }
    /* coverity[leaked_storage:SUPPRESS] */
    return ret;

}
#endif

/*
type 0x04: 1 byte
head[evt code, length]: 2 bytes
payload
*/
static int amlbt_intf_usb_event_to_skb_process(amlbt_t *p_bt, unsigned char *event_buff, unsigned int len)
{
    unsigned int offset = 0;
    struct sk_buff *event_skb = NULL;
    struct sk_buff *skb = NULL;
    unsigned int event_len = 0;
    unsigned int total_pkt_len = 0;
    unsigned int aligned_len = 0;
    int ret = 0;

    //BTI("%s event_size %u\n", __func__, len);

    while (offset + 3 <= len) {
        unsigned char pkt_type   = event_buff[offset];
        unsigned char event_code = event_buff[offset + 1];
        unsigned char param_len  = event_buff[offset + 2];

        if (param_len > HCI_MAX_EVENT_SIZE) {
            BTE("Param_len %u too large at offset %u\n", param_len, offset);
            ret = -EINVAL;
            goto error;
        }

        event_len     = 2 + param_len;
        total_pkt_len = 1 + event_len;
        aligned_len   = (total_pkt_len + 3) & ~3;

        if (offset + aligned_len > len) {
            BTE("Incomplete event (0x%02x) at offset %u: need %u, remain %u\n",
                event_code, offset, aligned_len, len - offset);
            ret = -EINVAL;
            goto error;
        }

        event_skb = alloc_skb(event_len, GFP_KERNEL);
        if (!event_skb) {
            BTE("alloc_skb failed at offset %u\n", offset);
            ret = -ENOMEM;
            goto error;
        }

        hci_skb_pkt_type(event_skb) = pkt_type;
        //BTI("%s event_type %u event_len %u\n", __func__, pkt_type, event_len);

        // copy EventCode + ParamLen + Payload
        if (!skb_put_data(event_skb, &event_buff[offset + 1], event_len)) {
            BTE("skb_put_data failed at offset %u\n", offset);
            kfree_skb(event_skb);
            ret = -EFAULT;
            goto error;
        }

        // queue skb
        skb_queue_tail(&p_bt->bt_res.bt_rx_queue, event_skb);
        event_skb = NULL;
        offset += aligned_len;
    }
    return 0;

error:
    if (event_skb)
        kfree_skb(event_skb);
    return ret;
}

/*
type 0x02: 4 byte
head[handle 2bytes, length 2bytes]: 4 bytes
payload
*/
static int amlbt_intf_usb_data_to_skb_process(amlbt_t *p_bt, unsigned char *data_buf, unsigned int len)
{
    unsigned int offset = 0;
    struct sk_buff *acl_skb = NULL;
    struct sk_buff *skb = NULL;
    unsigned int payload_len = 0;
    unsigned int total_pkt_len = 0;
    unsigned int aligned_len = 0;
    int ret = 0;

    //BTI("%s: acl_size = %u\n", __func__, len);

    while (offset + 8 <= len) {

        payload_len = ((data_buf[offset + 7] << 8) | data_buf[offset + 6]);
        if (payload_len > HCI_MAX_DATA_SIZE) {
            BTE("Invalid payload_len %u at offset %u\n", payload_len, offset);
            ret = -EINVAL;
            goto error;
        }

        total_pkt_len = 8 + payload_len;
        aligned_len   = (total_pkt_len + 3) & ~3;

        if (offset + aligned_len > len) {
            BTE("Incomplete ACL packet: offset=%u needed=%u remaining=%u\n",
                offset, aligned_len, len - offset);
            ret = -EINVAL;
            goto error;
        }

        acl_skb = alloc_skb(4 + payload_len, GFP_KERNEL);
        if (!acl_skb) {
            BTE("alloc_skb failed at offset %u\n", offset);
            ret = -ENOMEM;
            goto error;
        }

        hci_skb_pkt_type(acl_skb) = data_buf[offset];

        if (!skb_put_data(acl_skb, &data_buf[offset + 4], 4 + payload_len)) {
            BTE("skb_put_data failed at offset %u\n", offset);
            kfree_skb(acl_skb);
            ret = -EFAULT;
            goto error;
        }

        skb_queue_tail(&p_bt->bt_res.bt_rx_queue, acl_skb);
        acl_skb = NULL;
        offset += aligned_len;
    }
    return 0;

error:
    if (acl_skb)
        kfree_skb(acl_skb);
    return ret;
}

/*
type 0x10: 1 byte
head[MHDL(zigbee 0xF5, thread 0xFA), MID, length 2bytes]: 4 bytes
payload + 2 bytes crc
*/
static int amlbt_intf_usb_15p4_to_skb_process(amlbt_t *p_bt, unsigned char *data_buf, unsigned int len)
{
    struct sk_buff *skb = NULL;
    unsigned int read_len = 0;
    unsigned int pkt_len = 0;
    unsigned int i = 0;
    unsigned char *p = data_buf;
    int ret = 0;

    //BTI("%s: 15p4_size = %u\n", __func__, len);

    while (i < len) {

      read_len = 7 + ((p[4] << 8) | (p[3]));
      pkt_len = 7 + ((p[4] << 8) | (p[3]));
      pkt_len = (pkt_len + 3) & ~3;

      if (i + pkt_len > len) {
        BTE("%s: 15p4 exceeds buffer [%u,%u,%u], %u > %u\n", __func__, i, pkt_len, read_len, i+pkt_len, len);
        DUMP_BUF("data_buf", data_buf, len);
        DUMP_BUF("p", p, len - (p - data_buf));
        return -EMSGSIZE;
      }

      skb = alloc_skb(read_len, GFP_KERNEL);
      if (!skb) {
        BTE("%s:%d alloc_skb Failed\n", __func__, __LINE__);
        ret = -ENOMEM;
        goto error;
      }

      //BTI("push data %d[%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", read_len,
         //  p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);

      if (skb_put_data(skb, p, read_len) == NULL) {
        BTE("%s: skb_put_data failed\n", __func__);
        ret = -EFAULT;
        goto error;
      }

      if ((p[1] == HCI_15P4_ZIGBEE_PKT || p[1] == HCI_15P4_LOG_PKT) && p_bt->zigbee_res.zigbee_start) {
          skb_queue_tail(&p_bt->zigbee_res.zigbee_rx_queue, skb);
      } else if ((p[1] == HCI_15P4_THREAD_PKT || p[1] == HCI_15P4_LOG_PKT) && p_bt->thread_res.thread_start) {
          skb_queue_tail(&p_bt->thread_res.thread_rx_queue, skb);
      } else {
        BTE("%s: Unknown packet type  %#x\n", __func__, p[1]);
        kfree_skb(skb);
        return -EPROTO;
      }

      skb= NULL;
      p += pkt_len;
      i += pkt_len;
    }

    return 0;
error:
    if (skb) {
      kfree_skb(skb);
    }
    return ret;
}

static int amlbt_intf_usb_fw_data_process(amlbt_t *p_bt)
{
    unsigned char *p_buf = p_bt->usb_res.usb_rx_buf;
    amlbt_common_gdsl_fifo_t read_fifo = {0};
    unsigned int reg = 0;
    unsigned int type_size = 0;
    unsigned int evt_size = 0;
    unsigned int data_size = 0;
    unsigned int _15p4_size = 0;
    unsigned char read_reg[16] = {0};
    static unsigned char type_buff[USB_RX_TYPE_FIFO_LEN+4] = {0};
    static unsigned char fw_read_buff[USB_RX_Q_LEN*4] = {0};
    static unsigned char fw_data_buff[USB_RX_Q_LEN*4] = {0};
    amlbt_res_usb_linux_t *info = &p_bt->usb_res_linux;

    int ret = 0;

    //check fw gpio
    if (p_bt->pm_res.irq_handle)
    {
        amlbt_intf_wakeup_key_process(p_bt);
    }

    p_bt->usb_res.fw_type_fifo->w = (unsigned char *)(unsigned long)((p_buf[35]<<24)|(p_buf[34]<<16)|(p_buf[33]<<8)|p_buf[32]);
    if (p_bt->usb_res.fw_type_fifo->w == p_bt->usb_res.fw_type_fifo->r) // no data
    {
        return 0;
    }
    memset(type_buff, 0, sizeof(type_buff));
    p_bt->usb_res.fw_evt_fifo->w = (unsigned char *)(unsigned long)((p_buf[39]<<24)|(p_buf[38]<<16)|(p_buf[37]<<8)|p_buf[36]);
    p_bt->usb_res.fw_data_fifo->w = (unsigned char *)(unsigned long)((p_buf[19]<<24)|(p_buf[18]<<16)|(p_buf[17]<<8)|p_buf[16]);
    p_bt->diag_res.sink_mode = ((p_buf[23]<<24)|(p_buf[22]<<16)|(p_buf[21]<<8)|p_buf[20]);
    p_bt->usb_res._15p4_rx_fifo->w =(unsigned char *)(unsigned long)((p_buf[31]<<24)|(p_buf[30]<<16)|(p_buf[29]<<8)|p_buf[28]);

    BTD("dp1 type:w %#lx, r %#lx\n", (unsigned long)p_bt->usb_res.fw_type_fifo->w, (unsigned long)p_bt->usb_res.fw_type_fifo->r);
    BTD("dp1 evt:w %#lx, r %#lx\n", (unsigned long)p_bt->usb_res.fw_evt_fifo->w, (unsigned long)p_bt->usb_res.fw_evt_fifo->r);
    BTD("dp1 data:w %#lx, r %#lx\n", (unsigned long)p_bt->usb_res.fw_data_fifo->w, (unsigned long)p_bt->usb_res.fw_data_fifo->r);
    if (((unsigned long)p_bt->usb_res.fw_type_fifo->w > p_bt->fw_res.rx_type_len) ||
                ((unsigned long)p_bt->usb_res.fw_evt_fifo->w > p_bt->fw_res.evt_len) ||
                    ((unsigned long)p_bt->usb_res.fw_data_fifo->w > p_bt->fw_res.rx_q_len))
    {
        amlbt_intf_fw_info();
        BTF("%s %d fw_type_fifo->w:%#x fw_type_fifo->r:%#x", __func__, __LINE__,
            p_bt->usb_res.fw_type_fifo->w, p_bt->usb_res.fw_type_fifo->r);
        BTF("%s %d fw_evt_fifo->w:%#x fw_evt_fifo->r:%#x", __func__, __LINE__,
            p_bt->usb_res.fw_evt_fifo->w, p_bt->usb_res.fw_evt_fifo->r);
        BTF("%s %d fw_data_fifo->w:%#x fw_data_fifo->r:%#x", __func__, __LINE__,
            p_bt->usb_res.fw_data_fifo->w, p_bt->usb_res.fw_data_fifo->r);
        ret = -2;
        goto err_exit;
    }
    //copy type fifo
    read_fifo.base_addr = &p_buf[p_bt->fw_res.rx_type_addr - p_bt->fw_res.poll_addr];
    read_fifo.r = p_bt->usb_res.fw_type_fifo->r;
    read_fifo.w = p_bt->usb_res.fw_type_fifo->w;
    read_fifo.size = p_bt->fw_res.rx_type_len;

    type_size = amlbt_common_gdsl_fifo_get_data(&read_fifo, type_buff, p_bt->fw_res.rx_type_len);
    if (type_buff[0] != HCI_ACLDATA_PKT && type_buff[0] != HCI_EVENT_PKT &&
        type_buff[0] != HCI_15P4_PKT && type_buff[0] != HCI_ISO_PKT)
    {
        BTE("%s:%d type error!\n", __func__, __LINE__);
        BTE("fw_type_fifo->w:%#x fw_type_fifo->r:%#x", p_bt->usb_res.fw_type_fifo->w, p_bt->usb_res.fw_type_fifo->r);
        BTE("fw_evt_fifo->w:%#x fw_evt_fifo->r:%#x", p_bt->usb_res.fw_evt_fifo->w, p_bt->usb_res.fw_evt_fifo->r);
        BTE("fw_data_fifo->w:%#x fw_data_fifo->r:%#x", p_bt->usb_res.fw_data_fifo->w, p_bt->usb_res.fw_data_fifo->r);
        ret = -2;
        goto err_exit;
    }

    BTP("type fifo:w %#lx, r %#lx\n", (unsigned long)p_bt->usb_res.fw_type_fifo->w, (unsigned long)p_bt->usb_res.fw_type_fifo->r);
    BTP("[%#x,%#x,%#x,%#x]\n", type_buff[0], type_buff[4], type_buff[8], type_buff[12]);
    p_bt->usb_res.fw_type_fifo->r = read_fifo.r;

    reg = (((unsigned int)(unsigned long)read_fifo.r) & 0xff);
    read_reg[0] = (reg & 0xff);
    read_reg[1] = ((reg >> 8) & 0xff);
    read_reg[2] = ((reg >> 16) & 0xff);
    read_reg[3] = ((reg >> 24) & 0xff);

    //copy event fifo
    if (p_bt->usb_res.fw_evt_fifo->w != p_bt->usb_res.fw_evt_fifo->r)
    {
        read_fifo.base_addr = &p_buf[p_bt->fw_res.evt_addr - p_bt->fw_res.poll_addr];
        read_fifo.r = p_bt->usb_res.fw_evt_fifo->r;
        read_fifo.w = p_bt->usb_res.fw_evt_fifo->w;
        read_fifo.size = p_bt->fw_res.evt_len;
        evt_size = amlbt_common_gdsl_fifo_get_data(&read_fifo, fw_read_buff, p_bt->fw_res.evt_len);
    }
    BTP("evt fifo:w %#lx, r %#lx\n", (unsigned long)p_bt->usb_res.fw_evt_fifo->w, (unsigned long)p_bt->usb_res.fw_evt_fifo->r);
    if (evt_size)
    {
        if (evt_size > p_bt->fw_res.evt_len)
        {
            BTE("evt_size %#x \n", evt_size);
            ret = -2;
            goto err_exit;
        }
        p_bt->usb_res.fw_evt_fifo->r = read_fifo.r;
        /* coverity[overrun-buffer-val:SUPPRESS] */
        /* coverity[tainted_data:SUPPRESS] */
        if (0 != amlbt_intf_usb_event_to_skb_process(p_bt, fw_read_buff, evt_size))
        {
            ret = -2;
            goto err_exit;
        }
    }
    reg = (((unsigned int)(unsigned long)p_bt->usb_res.fw_evt_fifo->r) & 0x1fff);
    read_reg[4] = (reg & 0xff);
    read_reg[5] = ((reg >> 8) & 0xff);
    read_reg[6] = ((reg >> 16) & 0xff);
    read_reg[7] = ((reg >> 24) & 0xff);

    //copy data fifo
    if (p_bt->usb_res.fw_data_fifo->w != p_bt->usb_res.fw_data_fifo->r)
    {
        ret = amlbt_intf_read_sram(&fw_data_buff[0], p_bt->fw_res.rx_q_addr, p_bt->fw_res.rx_q_len);
        if (ret == -1)
        {
            BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
            goto err_exit;
        }
        read_fifo.base_addr = fw_data_buff;
        read_fifo.r = p_bt->usb_res.fw_data_fifo->r;
        read_fifo.w = p_bt->usb_res.fw_data_fifo->w;
        read_fifo.size = p_bt->fw_res.rx_q_len;
        data_size = amlbt_common_gdsl_fifo_get_data(&read_fifo, fw_read_buff, p_bt->fw_res.rx_q_len);
    }
    BTP("data fifo:w %#lx, r %#lx\n", (unsigned long)p_bt->usb_res.fw_data_fifo->w, (unsigned long)p_bt->usb_res.fw_data_fifo->r);

    if (data_size)
    {
        if (data_size > p_bt->fw_res.rx_q_len)
        {
            BTE("data_size %#x \n", data_size);
            ret = -2;
            goto err_exit;
        }
        p_bt->usb_res.fw_data_fifo->r = read_fifo.r;
        /* coverity[overrun-buffer-val:SUPPRESS] */
        if (0 != amlbt_intf_usb_data_to_skb_process(p_bt, fw_read_buff, data_size))
        {
            ret = -2;
            goto err_exit;
        }
    }

    reg = (((unsigned int)(unsigned long)p_bt->usb_res.fw_data_fifo->r) & 0x1fff);
    read_reg[12] = (reg & 0xff);
    read_reg[13] = ((reg >> 8) & 0xff);
    read_reg[14] = ((reg >> 16) & 0xff);
    read_reg[15] = ((reg >> 24) & 0xff);

    if (p_bt->diag_res.chip_family_id >= CHIP_W2L)
    {
        //copy 15.4 fifo
        if (p_bt->usb_res._15p4_rx_fifo->w != p_bt->usb_res._15p4_rx_fifo->r)
        {
            memset(fw_read_buff, 0, sizeof(fw_read_buff));
            ret = amlbt_intf_read_sram(fw_read_buff, p_bt->fw_res._15p4_rx_addr, p_bt->fw_res._15p4_rx_len);
            if (ret == -1)
            {
                BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
                goto err_exit;
            }

            read_fifo.base_addr = fw_read_buff;
            read_fifo.r = p_bt->usb_res._15p4_rx_fifo->r;
            read_fifo.w = p_bt->usb_res._15p4_rx_fifo->w;
            read_fifo.size = p_bt->fw_res._15p4_rx_len;
            _15p4_size = amlbt_common_gdsl_fifo_get_data(&read_fifo, &fw_read_buff[p_bt->fw_res._15p4_rx_len], p_bt->fw_res._15p4_rx_len);
            BTP("15.4 get size %d\n", _15p4_size);
            BTP("15.4 rx:w %#lx, r %#lx\n", (unsigned long)p_bt->usb_res._15p4_rx_fifo->w, (unsigned long)p_bt->usb_res._15p4_rx_fifo->r);
            if (_15p4_size)
            {
                if (_15p4_size > p_bt->fw_res._15p4_rx_len)
                {
                    BTE("_15p4_size %#x \n", _15p4_size);
                    ret = -2;
                    goto err_exit;
                }
                p_bt->usb_res._15p4_rx_fifo->r = read_fifo.r;
                /* coverity[overrun-buffer-val:SUPPRESS] */
                amlbt_intf_usb_15p4_to_skb_process(p_bt, &fw_read_buff[p_bt->fw_res._15p4_rx_len], _15p4_size);
            }
        }

        reg = (((unsigned int)(unsigned long)p_bt->usb_res._15p4_rx_fifo->r) & 0x7ff);
        read_reg[8] = (reg & 0xff);
        read_reg[9] = ((reg >> 8) & 0xff);
        read_reg[10] = ((reg >> 16) & 0xff);
        read_reg[11] = ((reg >> 24) & 0xff);
    }
    ret = amlbt_intf_write_sram(&read_reg[0], p_bt->fw_res.rx_type_r, sizeof(read_reg));
    if (ret == -1)
    {
        BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
        goto err_exit;
    }

    if (test_bit(HDEV_RUNNING, &info->hdev_flags) && !skb_queue_empty(&p_bt->bt_res.bt_rx_queue))
    {
        //wake_up_interruptible(&p_bt->bt_res.bt_wait_queue);
        //queue_work(info->workqueue, &info->receive_work.work);
        queue_delayed_work(info->workqueue, &info->receive_work, 0);
    }
    if (!skb_queue_empty(&p_bt->zigbee_res.zigbee_rx_queue))
    {
        wake_up_interruptible(&p_bt->zigbee_res.zigbee_wait_queue);
    }
    if (!skb_queue_empty(&p_bt->thread_res.thread_rx_queue))
    {
        wake_up_interruptible(&p_bt->thread_res.thread_wait_queue);
    }

    return 0;
err_exit:
    return ret;
}

static int amlbt_intf_usb_driver_state_process(amlbt_t *p_bt)
{
    int ret = BT_DRV_NONE;

    if ((p_bt->pm_res.dr_state & BT_DRV_STATE_SUSPEND_ENTRY) || \
           (p_bt->pm_res.dr_state & BT_DRV_STATE_SUSPEND))
    {
        ret = BT_DRV_SUSPEND;
        goto exit;
    }
    if (p_bt->pm_res.dr_state & BT_DRV_STATE_RESUME)
    {
        ret = BT_DRV_RESUME;
        goto exit;
    }
    if (p_bt->pm_res.dr_state & BT_DRV_STATE_WAIT_RECOVERY)
    {
        ret = BT_DRV_WAIT_RECOVERY;
        goto exit;
    }

    if (p_bt->pm_res.dr_state & BT_DRV_STATE_RECOVERY)
    {
        ret = BT_DRV_CLOSED;
    }
exit:
    return ret;
}


void amlbt_intf_usb_rx_work(struct work_struct *work)
{
    int actual_length = 0;
    int ret = 0;
    unsigned char *data;

    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    if (p_bt->pm_res.dr_state == 0)
    {
        mutex_lock(&p_bt->diag_res.bt_debug_mutex);
        ret = amlbt_intf_read_sram(p_bt->usb_res.usb_rx_buf, p_bt->fw_res.poll_addr, p_bt->fw_res.poll_len);
        if (ret != 0)
        {
            BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
            p_bt->usb_res.usb_rx_len = 0;
            mutex_unlock(&p_bt->diag_res.bt_debug_mutex);
            return ;
        }
        p_bt->usb_res.usb_rx_len = p_bt->fw_res.poll_len;
        mutex_unlock(&p_bt->diag_res.bt_debug_mutex);
    }
    else
    {
        BTE("%s:%d Failed : %d\n", __func__, __LINE__, p_bt->pm_res.dr_state);
        p_bt->usb_res.usb_rx_len = 0;
    }

    ret = amlbt_intf_usb_driver_state_process(p_bt);

    if (ret == BT_DRV_NONE)
    {
        data = p_bt->usb_res.usb_rx_buf;
        actual_length = p_bt->usb_res.usb_rx_len;
        if (actual_length == p_bt->fw_res.poll_len)
        {
            mutex_lock(&p_bt->diag_res.bt_debug_mutex);
            ret = amlbt_intf_usb_fw_data_process(p_bt);
            mutex_unlock(&p_bt->diag_res.bt_debug_mutex);
            if (ret == -2)
            {
                BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
                if (!(p_bt->pm_res.dr_state & BT_DRV_STATE_WAIT_RECOVERY))
                {
                    p_bt->pm_res.dr_state = BT_DRV_STATE_WAIT_RECOVERY;
                    amlbt_intf_queue_work(p_bt->excp_res.exception_work_wq, &p_bt->excp_res.exception_work);
                }
                return ;
            }
        }
        else
        {
            BTE("%s:%d usb rx data length not match!!, %d\n", __func__, __LINE__, actual_length);
        }
        p_bt->usb_res.usb_rx_len = 0;
    }
    else if (ret == BT_DRV_CLOSED)
    {
        BTW("%s:%d BT_DRV_CLOSED!\n", __func__, __LINE__);
    }
    else if (ret == BT_DRV_SUSPEND || ret == BT_DRV_RESUME || ret == BT_DRV_WAIT_RECOVERY)
    {
        BTA("%s:%d BT_DRV_SUSPEND or BT_DRV_RESUME or BT_DRV_WAIT_RECOVERY!\n", __func__, __LINE__);
    }
}

void amlbt_intf_usb_exception_func(struct work_struct *work)
{
    //int ret = 0;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    BTF("%s \n", __func__);
    amlbt_intf_exception_func(p_bt);
}

int amlbt_intf_usb_send_hci_cmd(amlbt_t *p_bt, unsigned char *data, unsigned int len)
{
    int ret = 0;
    unsigned int val = 0;
    amlbt_common_gdsl_fifo_t t_fifo;
    BTA("%s, len %d \n", __func__, len);

    if (p_bt->usb_res.tx_cmd_fifo == NULL)
    {
        BTE("%s: p_bt->usb_res.tx_cmd_fifo NULL!!!!\n", __func__);
        goto err_exit;
    }
    t_fifo = *p_bt->usb_res.tx_cmd_fifo;

    if (len) {
        //BTF("hci_cmd:%#x,%#x,%#x,%#x",data[0],data[1],data[2],data[3]);
        BTD("hci_cmd:%#x,%#x",data[0],data[1]);
    }

    len = ((len + 3) & 0xFFFFFFFC);//Keep 4 bytes aligned
    BTA("%s, Actual length %d \n", __func__, len);
    //step 1: Update the command FIFO read pointer
    ret = amlbt_intf_read_word(p_bt->fw_res.cmd_r, &val);
    BTA("cmd r %#x\n", val);
    if (ret != 0)
    {
        BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
        goto err_exit;
    }
    t_fifo.r = (unsigned char *)(unsigned long)val;
    //step 3: Write HCI commands to WiFi SRAM
    ret = amlbt_common_gdsl_write_data(&t_fifo, data, len);
    //step 4: Update the write pointer and write to WiFi SRAM
    if (ret < 0)
    {
        BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
        if (ret == -2)  //fifo full
        {
            if (!(p_bt->pm_res.dr_state & BT_DRV_STATE_WAIT_RECOVERY))
            {
                amlbt_intf_drv_state_set(BT_DRV_STATE_RECOVERY);
            }
        }
        goto err_exit;
    }

    BTA("before write:r:%#lx, w:%#lx\n", (unsigned long)p_bt->usb_res.tx_cmd_fifo->r, (unsigned long)p_bt->usb_res.tx_cmd_fifo->w);

    ret = amlbt_intf_write_word(p_bt->fw_res.cmd_w, (unsigned long)t_fifo.w & 0xfff);
    if (ret != 0)
    {
        BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
        goto err_exit;
    }
    *p_bt->usb_res.tx_cmd_fifo = t_fifo;
    BTP("len %#x:w %#lx, r %#lx\n", len, (unsigned long)p_bt->usb_res.tx_cmd_fifo->w,
        (unsigned long)p_bt->usb_res.tx_cmd_fifo->r);
    //amlbt_debug_get_cmd((unsigned int)(unsigned long)p_bt->usb_res.tx_cmd_fifo->w,
    //    (unsigned int)(unsigned long)p_bt->usb_res.tx_cmd_fifo->r, data);
    p_bt->diag_res.cmd_cnt++;
    amlbt_intf_diag_add(p_bt, HCI_COMMAND_PKT, (unsigned int)(unsigned long)p_bt->usb_res.tx_cmd_fifo->w,
        (unsigned int)(unsigned long)p_bt->usb_res.tx_cmd_fifo->r, data, 0, p_bt->diag_res.cmd_cnt);
    return ret;
err_exit:
    return ret;
}

static unsigned int amlbt_intf_usb_get_tx_prio(amlbt_common_gdsl_tx_q_t *p_fifo, unsigned int acl_handle)
{
    unsigned int prio = 0;
    unsigned int i = 0;
    unsigned int find = 0;

    for (i = 0; i < USB_TX_Q_NUM; i++)
    {
        if (p_fifo[i].tx_q_dev_index == acl_handle/* && p_fifo[i].tx_q_status == GDSL_TX_Q_USED*/)
        {
            if (p_fifo[i].tx_q_prio >= prio)
            {
                prio = p_fifo[i].tx_q_prio;
                find = 1;
            }
        }
    }

    if (!find)
    {
        prio = USB_TX_Q_MAX_PRIO;
    }

    return prio;
}


int amlbt_intf_usb_send_hci_data(amlbt_t *p_bt, unsigned char *data, unsigned int len)
{
    int ret = 0;
    unsigned int i = 0;
    unsigned int used_cnt = 0;
    unsigned int acl_handle = (((data[1] << 8) | data[0]) & 0xfff);
    unsigned int prio = 0;
    unsigned int tx_q_prio[USB_TX_Q_NUM] = {0};
    unsigned int tx_q_index[USB_TX_Q_NUM] = {0};
    unsigned int tx_q_status[USB_TX_Q_NUM] = {0};
    unsigned int tx_buff[USB_TX_Q_NUM * 4] = {0};//prio, index, status
    amlbt_common_gdsl_tx_q_t ro_tx_q = *p_bt->usb_res.tx_q;
    BTA("%s, len:%d\n", __func__, len);

    ret = amlbt_intf_read_sram((unsigned char *)tx_buff, p_bt->usb_res.tx_q[0].tx_q_prio_addr, sizeof(tx_buff));
    if (ret != 0)
    {
        BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
        USB_END_LOCK();
        goto err_exit;
    }
    for (i = 0; i < USB_TX_Q_NUM; i++)
    {
        tx_q_prio[i] = tx_buff[i*4];
        tx_q_index[i]  = tx_buff[i*4+1];
        tx_q_status[i]   = tx_buff[i*4+2];
    }
    BTA("P %#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", tx_q_prio[0],tx_q_prio[1],tx_q_prio[2],tx_q_prio[3],
        tx_q_prio[4],tx_q_prio[5],tx_q_prio[6],tx_q_prio[7]);
    BTA("A %#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", tx_q_index[0],tx_q_index[1],tx_q_index[2],tx_q_index[3],
        tx_q_index[4],tx_q_index[5],tx_q_index[6],tx_q_index[7]);
    BTA("S %#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", tx_q_status[0],tx_q_status[1],tx_q_status[2],tx_q_status[3],
        tx_q_status[4],tx_q_status[5],tx_q_status[6],tx_q_status[7]);
    for (i = 0; i < USB_TX_Q_NUM; i++)
    {
        if (tx_q_status[i] == GDSL_TX_Q_COMPLETE)
        {
            tx_q_index[i] = 0;
            tx_q_status[i] = GDSL_TX_Q_UNUSED;
            tx_q_prio[i] = USB_TX_Q_MAX_PRIO;
        }
        if (tx_q_status[i] == GDSL_TX_Q_USED)
        {
            used_cnt++;
        }
    }

    for (i = 0; i < USB_TX_Q_NUM; i++)
    {
        p_bt->usb_res.tx_q[i].tx_q_dev_index = tx_q_index[i];
        p_bt->usb_res.tx_q[i].tx_q_status = tx_q_status[i];
        p_bt->usb_res.tx_q[i].tx_q_prio = tx_q_prio[i];
    }

    for (i = 0; i < USB_TX_Q_NUM; i++)
    {
        if (p_bt->usb_res.tx_q[i].tx_q_status == GDSL_TX_Q_UNUSED)
        {
            break;
        }
    }

    if (i == USB_TX_Q_NUM)
    {
        BTE("%s: hci data space invalid!!!!\n", __func__);
        for (i = 0; i < USB_TX_Q_NUM; i++)
        {
            BTI("[%#x,%#x,%#x]", (unsigned int)p_bt->usb_res.tx_q[i].tx_q_prio,
                    (unsigned int)p_bt->usb_res.tx_q[i].tx_q_dev_index,
                    (unsigned int)p_bt->usb_res.tx_q[i].tx_q_status);
            BTI("{%#x,%#x,%#x}", tx_q_prio[i], tx_q_index[i],tx_q_status[i]);
        }
        return -1;
    }
    ro_tx_q = *p_bt->usb_res.tx_q;

    prio = amlbt_intf_usb_get_tx_prio(p_bt->usb_res.tx_q, acl_handle);

    p_bt->usb_res.tx_q[i].tx_q_prio = (++prio & USB_TX_Q_MAX_PRIO);
    p_bt->usb_res.tx_q[i].tx_q_dev_index = acl_handle;
    p_bt->usb_res.tx_q[i].tx_q_status = GDSL_TX_Q_USED;


    len = (len + 3) & ~3;
    ret = amlbt_intf_write_sram(data, p_bt->usb_res.tx_q[i].tx_q_addr, len);
    if (ret != 0)
    {
        BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
        USB_END_LOCK();
        *p_bt->usb_res.tx_q = ro_tx_q;
        goto err_exit;
    }

    tx_buff[0] = p_bt->usb_res.tx_q[i].tx_q_prio;
    tx_buff[1] = p_bt->usb_res.tx_q[i].tx_q_dev_index;
    tx_buff[2] = p_bt->usb_res.tx_q[i].tx_q_status;
    tx_buff[3] = 0;

    BTA("TX:%d,%d,%#x,%#x\n", i, len, acl_handle, p_bt->usb_res.tx_q[i].tx_q_prio);
    ret = amlbt_intf_write_sram((unsigned char *)&tx_buff[0], p_bt->usb_res.tx_q[i].tx_q_prio_addr, sizeof(unsigned int)*4);
    if (ret != 0)
    {
        BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
        *p_bt->usb_res.tx_q = ro_tx_q;
        goto err_exit;
    }
    p_bt->diag_res.acl_cnt++;
    amlbt_intf_diag_add(p_bt, HCI_ACLDATA_PKT, i, used_cnt, data, 0, p_bt->diag_res.acl_cnt);

    BTA("%s, Actual length:%d\n", __func__, len);
    return 0;
err_exit:
    return ret;
}

int amlbt_intf_usb_send_15p4_data(amlbt_t *p_bt,unsigned char *data, unsigned int len)
{
    int ret = 0;
    unsigned int val = 0;
    amlbt_common_gdsl_fifo_t p_fifo;
    BTP("%s, len %d \n", __func__, len);

    if (p_bt->usb_res._15p4_tx_fifo == NULL)
    {
        BTE("%s: p_bt->usb_res._15p4_tx_fifo NULL!!!!\n", __func__);
        return -1;
    }
    p_fifo = *p_bt->usb_res._15p4_tx_fifo;

    len = ((len + 3) & 0xFFFFFFFC);//Keep 4 bytes aligned

    BTA("%s, Actual length %d \n", __func__, len);
    //step 1: Update the tx FIFO read pointer
    ret = amlbt_intf_read_word(p_bt->fw_res._15p4_tx_r, &val);
    if (ret != 0)
    {
        BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
        goto err_exit;
    }
    p_fifo.r = (unsigned char *)(unsigned long)val;
    BTP("15p4 tx r %#x\n", (unsigned long)p_fifo.r);
    BTP("15p4 tx w %#x\n", (unsigned long)p_fifo.w);
    //step 2: Check the command FIFO space

    //step 3: Write HCI commands to WiFi SRAM
    ret = amlbt_common_gdsl_write_data(&p_fifo, data, len);
    if (ret < 0)
    {
        BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
        if (ret == -2)  //fifo full
        {
            if (!(p_bt->pm_res.dr_state & BT_DRV_STATE_WAIT_RECOVERY))
            {
                amlbt_intf_drv_state_set(BT_DRV_STATE_RECOVERY);
            }
        }
        goto err_exit;
    }
    //step 4: Update the write pointer and write to WiFi SRAM
    //BTI("15p4 before write:r:%#lx, w:%#lx\n", (unsigned long)p_bt->usb_res._15p4_tx_fifo->r, (unsigned long)p_bt->usb_res._15p4_tx_fifo->w);
    ret = amlbt_intf_write_word(p_bt->fw_res._15p4_tx_w, (unsigned long)p_fifo.w & 0x7ff);
    if (ret != 0)
    {
        BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
        goto err_exit;
    }
    *p_bt->usb_res._15p4_tx_fifo = p_fifo;
    BTP("15p4 len %#x:w %#lx, r %#lx\n", len, (unsigned long)p_bt->usb_res._15p4_tx_fifo->w, (unsigned long)p_bt->usb_res._15p4_tx_fifo->r);
    return ret;
err_exit:
    return ret;
}

void amlbt_intf_usb_write_work(amlbt_t *p_bt)
{
    struct sk_buff *skb;
    unsigned char *p;
    int ret = 0;
    unsigned int len = 0;

restart:
    //BTI("amlbt_w2ls_uart_write_work \n");

    while ((skb = skb_dequeue(&p_bt->common_res.tx_queue)))
    {
        p = skb->data;
        skb_pull(skb, 1);
        if (*p == HCI_COMMAND_PKT)
        {
            p = skb->data;
            len = 3 + p[2];
            //BTI("hci cmd %d, %d:[%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]", skb->len, len,
            //    p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
            ret = amlbt_intf_usb_send_hci_cmd(p_bt, skb->data, len);
        }
        else if (*p == HCI_ACLDATA_PKT || *p == HCI_ISO_PKT)
        {
            p = skb->data;
            //BTI("hci data %d, %d:[%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", skb->len, len,
            //        p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
            ret = amlbt_intf_usb_send_hci_data(p_bt, skb->data, skb->len);
        }
        else if (*p == HCI_15P4_PKT)
        {
            p = skb->data;
            //BTI("hci iot %d, %d:[%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", skb->len, len,
            //        p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
            ret = amlbt_intf_usb_send_15p4_data(p_bt, skb->data, skb->len);
        }
        else
        {
            BTE("type error!\n");
            BTE("raw %d, %d:[%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", skb->len, len,
                    p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
        }
        //BTI("amlbt_w2ls_uart_dequeue skb->len %d \n", skb->len);
        if (skb)
        {
            kfree_skb(skb);
            skb = NULL;
        }
        //BTI("amlbt_w2ls_uart_write_work complete! \n");
        if (ret != 0)
        {
            BTF("%s:%d Failed : %d\n", __func__, __LINE__, ret);
        }
#if defined(CONFIG_AML_BT_CHIP_W1D)
        //w1d/usb interrupt set bt fw get data
        amlbt_intf_write_word(REG_INTERRUPT_BT_SET, 0x1);
#endif
    }

    if (!skb_queue_empty(&p_bt->common_res.tx_queue))
    {
        goto restart;
    }
}
#if defined(CONFIG_AML_BT_CHIP_W1D)
int amlbt_intf_usb_shutdown(amlbt_t *p_bt)
{
    int ret = 0;
    unsigned int cmd_r = 0;
    unsigned int cmd_w = 0;
    unsigned int len = 0;
    uint8_t cmd[] = {SW_SHUTDOWN_CMD & 0xff, (SW_SHUTDOWN_CMD >> 8) & 0xff, 0x00, 0x00};

    amlbt_intf_flush_workqueue(p_bt->common_res.write_work_wq, &p_bt->common_res.write_work);

    len = ((sizeof(cmd) + 3) & 0xFFFFFFFC);//Keep 4 bytes aligned

    //step 1: Update the command FIFO read/write pointer
    ret = amlbt_intf_read_word(W1D_USB_CMD_Q_R_POINT, &cmd_r);
    if (ret != 0)
    {
        BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
        goto err_exit;
    }
    ret = amlbt_intf_read_word(W1D_USB_CMD_Q_R_POINT, &cmd_w);
    if (ret != 0)
    {
        BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
        goto err_exit;
    }
    //step 2: write data to fw
    ret = amlbt_intf_write_sram(cmd, W1D_USB_CMD_Q_ADDR+cmd_w, len);
    if (ret != 0)
    {
        BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
        goto err_exit;
    }

    //step 3: Update the write pointer
    cmd_w += len;
    ret = amlbt_intf_write_word(W1D_USB_CMD_Q_W_POINT, cmd_w & 0xfff);
    if (ret != 0)
    {
        BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
        goto err_exit;
    }

    //w1d/usb interrupt set bt fw get data
    amlbt_intf_write_word(REG_INTERRUPT_BT_SET, 0x1);

    BTI("%s write len:%d w:%#x r:%#x\n", __func__, len, cmd_w, cmd_r);
    BTI("%s write cmd[%#x,%#x,%#x,%#x]", __func__, cmd[0], cmd[1], cmd[2], cmd[3]);
err_exit:
    return ret;
}
#endif

//linux bt driver for bluez stack
static int amlbt_hci_report_hardware_error(amlbt_res_usb_linux_t *info)
{
    struct sk_buff *skb;
    unsigned char hw_error_evt[4] = {0x04, 0x10, 0x01, 0x00};

    BTI("%s\n", __func__);

    skb = bt_skb_alloc(4, GFP_ATOMIC);
    if (!skb) {
        BTE("Failed to allocate SKB for hardware error\n");
        return -ENOMEM;
    }

    skb_put_data(skb, hw_error_evt, 4);
    hci_recv_frame(info->hdev, skb);
    info->hdev->stat.byte_rx += 4;
    BTI("hw_error_evt: %#x|%#x|%#x|%#x\n",
        hw_error_evt[0], hw_error_evt[1], hw_error_evt[2], hw_error_evt[3]);
    return 0;
}

static void amlbt_hci_receive(struct work_struct *work)
{
    struct sk_buff *skb = NULL;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    //struct amlbt_res_usb_linux_t *info = &p_bt->usb_linux_res;
    amlbt_res_usb_linux_t *info = container_of(to_delayed_work(work), amlbt_res_usb_linux_t, receive_work);
    bool is_empty;
    unsigned int len;

    if (info != &p_bt->usb_res_linux) {
        BTE("info does not belong to p_bt\n");
        return;
    }

    if (!info || !info->hdev) {
        BTE("Invalid info or hdev\n");
        return;
    }

    if (p_bt->pm_res.dr_state & BT_DRV_STATE_RECOVERY) {
        amlbt_hci_report_hardware_error(info);
        return;
    }

    if (!test_bit(HDEV_RUNNING, &info->hdev_flags)) {
        BTI("hdev not running return\n");
        return;
    }

    while ((skb = skb_dequeue(&p_bt->bt_res.bt_rx_queue)) != NULL) {
        len = skb->len;
        hci_recv_frame(info->hdev, skb);
        info->hdev->stat.byte_rx += len;
    }

    //is_empty = skb_queue_empty(&p_bt->bt_res.bt_rx_queue);
    //queue_delayed_work(info->workqueue, &info->receive_work, is_empty ? msecs_to_jiffies(10) : 0);
}

static int amlbt_hci_write(struct sk_buff *skb)
{
    unsigned int actual = 0;
    unsigned int pkt_type = 0;
    unsigned int count = 0;
    //unsigned char *p;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    amlbt_res_usb_linux_t *info = &p_bt->usb_res_linux;

    BTA("%s skb %p\n", __func__,skb);

    //memset(info->bluez_buf, 0, HCI_MAX_FRAME_SIZE);
    //remove pkt type get hci raw data
    count = skb->len - 1;
    memcpy(info->bluez_buf, &skb->data[1], count);

    //p = skb->data;
    //skb_pull(skb, 1);

    pkt_type = hci_skb_pkt_type(skb);
    BTA("get hci raw data:w_type:%d\n",pkt_type);

    switch (pkt_type) {
    case HCI_COMMAND_PKT:
        actual = amlbt_intf_usb_send_hci_cmd(p_bt, info->bluez_buf, count);//keep 4 bytes aligned len = len + 3;
        break;
    case HCI_ACLDATA_PKT:
        actual = amlbt_intf_usb_send_hci_data(p_bt, info->bluez_buf, count);
        break;
    case HCI_ISO_PKT:
        actual = amlbt_intf_usb_send_hci_data(p_bt, info->bluez_buf, count);
        break;
    case HCI_SCODATA_PKT:
        break;
    }

    actual = skb->len;
    return actual;
}

static void amlbt_hci_write_wakeup(amlbt_res_usb_linux_t *info)
{
    BTA("%s\n", __func__);

    if (!info || !info->hdev) {
        BTE("%s: invalid info or hdev", __func__);
        return;
    }

    if (test_and_set_bit(XMIT_SENDING, &info->tx_state)) {
        set_bit(XMIT_WAKEUP, &info->tx_state);
        BTF("%s: already sending, set wakeup flag", __func__);
        return;
    }

    do {
        struct sk_buff *skb = NULL;
        int len = 0;

        clear_bit(XMIT_WAKEUP, &info->tx_state);

        skb = skb_dequeue(&info->txq);
        if (!skb) {
            BTA("%s: no skb in txq, exit loop", __func__);
            break;
        }

        BTA("%s: send skb %p, len %d", __func__, skb, skb->len);
        len = amlbt_hci_write(skb);
        if (len > 0 && len == skb->len) {
            info->hdev->stat.byte_tx += len;
            kfree_skb(skb);
            BTA("%s: skb %p sent completely, kfree", __func__, skb);
        } else if (len > 0 && len < skb->len) {
            skb_pull(skb, len);
            info->hdev->stat.byte_tx += len;
            skb_queue_head(&info->txq, skb);
            BTA("%s: skb %p partially sent (len=%d), requeue", __func__, skb, len);
            set_bit(XMIT_WAKEUP, &info->tx_state);
        } else {
            BTE("%s: skb %p send failed (len=%d)", __func__, skb, len);
            kfree_skb(skb);
        }
    } while (test_bit(XMIT_WAKEUP, &info->tx_state));

    clear_bit(XMIT_SENDING, &info->tx_state);
    BTA("%s: exit, clear XMIT_SENDING", __func__);
}

static int amlbt_hci_send_frame(struct hci_dev *hdev, struct sk_buff *skb)
{
    amlbt_res_usb_linux_t *info = hci_get_drvdata(hdev);

    BTA("%s \n", __func__);

    switch (hci_skb_pkt_type(skb)) {
    case HCI_COMMAND_PKT:
        hdev->stat.cmd_tx++;
        break;
    case HCI_ACLDATA_PKT:
        hdev->stat.acl_tx++;
        break;
    case HCI_SCODATA_PKT:
        hdev->stat.sco_tx++;
        break;
    }

    /*Preoend skb with frame type*/
    memcpy(skb_push(skb,1), &hci_skb_pkt_type(skb), 1);
    skb_queue_tail(&(info->txq), skb);
    amlbt_hci_write_wakeup(info);

    return 0;
}

static int amlbt_hci_flush(struct hci_dev *hdev)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    amlbt_res_usb_linux_t *info = hci_get_drvdata(hdev);

    BTI("%s\n", __func__);

    if (!info) {
        BTE("info is NULL, skip flush\n");
        return 0;
    }

    //clean tx queue
    skb_queue_purge(&(info->txq));
    cancel_delayed_work_sync(&info->receive_work);
    return 0;
}

static int amlbt_hci_open(struct hci_dev *hdev)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    amlbt_res_usb_linux_t *info = hci_get_drvdata(hdev);

    BTI("%s\n", __func__);

    //clean tx queue
    skb_queue_purge(&(info->txq));
    skb_queue_purge(&p_bt->bt_res.bt_rx_queue);

    set_bit(HDEV_RUNNING, &info->hdev_flags);

    //queue_work(info->workqueue, &info->receive_work);
    queue_delayed_work(info->workqueue, &info->receive_work, 0);
    BTI("%s: start schedule bluez receive work\n", __func__);

    return 0;
}

static int amlbt_hci_close(struct hci_dev *hdev)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    amlbt_res_usb_linux_t *info = hci_get_drvdata(hdev);

    BTI("%s\n", __func__);

    clear_bit(HDEV_RUNNING, &info->hdev_flags);
    amlbt_hci_flush(hdev);
    return 0;
}

int amlbt_hci_register_dev(amlbt_t *p_bt)
{
    //amlbt_t *p_bt = amlbt_intf_get_p_bt();
    amlbt_res_usb_linux_t *info = &p_bt->usb_res_linux;

    BTI("%s\n", __func__);

    if (info->reg_flag) {
        BTE("%s:hci_dev already registered, skip\n", __func__);
        return 0;
    }

    if (!info->hdev) {
        BTE("%s:hci_dev is NULL, can't register\n", __func__);
        return -EINVAL;
    }

    if (hci_register_dev(info->hdev) < 0) {
        BTE("%s:Failed to register hci_dev\n", __func__);
        hci_free_dev(info->hdev);
        info->hdev = NULL;
        return -1;
    }

    info->reg_flag = 1;
    BTI("success register hdev reg_flag=%d\n", info->reg_flag);
    return 0;
}

int amlbt_hci_dev_init(struct platform_device *dev)
{
    int err = 0;
    struct hci_dev *hdev;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    amlbt_res_usb_linux_t *info = &p_bt->usb_res_linux;

     BTI("%s\n", __func__);

    spin_lock_init(&(info->lock));
    skb_queue_head_init(&(info->txq));

    info->rx_state = RECV_WAIT_PACKET_TYPE;
    info->rx_count = 0;
    info->rx_skb = NULL;
    info->reg_flag = 0;

    //info->workqueue = create_singlethread_workqueue("bluetooth_wq");
    info->workqueue = alloc_workqueue("bluetooth_wq", WQ_MEM_RECLAIM, 1);
    if (!info->workqueue) {
        BTE("%s:Failed to create workqueue\n", __func__);
        kfree(info);
        return -1;
    }

    //INIT_WORK(&info->receive_work, amlbt_hci_receive);
    INIT_DELAYED_WORK(&info->receive_work, amlbt_hci_receive);

    // allocate hci dev
    hdev = hci_alloc_dev();
    if (!hdev) {
        BTE("%s:Failed to allocate hci_dev\n", __func__);
        destroy_workqueue(info->workqueue);
        return -1;
    }

    info->bluez_buf = kzalloc(HCI_MAX_FRAME_SIZE, GFP_DMA | GFP_KERNEL);
    if (!info->bluez_buf) {
        BTE("%s: bluez_buf kzalloc failed!\n", __func__);
        hci_free_dev(hdev);
        destroy_workqueue(info->workqueue);
        return -ENOMEM;
    }

    info->hdev = hdev;
    hdev->bus = 0;
    hci_set_drvdata(hdev, info);
    SET_HCIDEV_DEV(hdev, &dev->dev);

    hdev->open  = amlbt_hci_open;
    hdev->close = amlbt_hci_close;
    hdev->flush = amlbt_hci_flush;
    hdev->send  = amlbt_hci_send_frame;

    info->pdev = dev;//get dev
    platform_set_drvdata(dev, info);//set info point to dev point

    BTI("%s success\n", __func__);
    return err;
}

void amlbt_hci_dev_deinit(void)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    amlbt_res_usb_linux_t *info = &p_bt->usb_res_linux;

    BTI("%s\n", __func__);

    if (!info) {
        BTE("info is NULL, nothing to deinit\n");
        return;
    }

    cancel_delayed_work_sync(&info->receive_work);
    skb_queue_purge(&(info->txq));

    if (info->workqueue) {
        destroy_workqueue(info->workqueue);
        info->workqueue = NULL;
    }

    if (info->reg_flag) {
        hci_unregister_dev(info->hdev);
        info->reg_flag = 0;
        BTI("%s: unregistered hdev\n", __func__);
    }

    if (info->hdev) {
        hci_free_dev(info->hdev);
        info->hdev = NULL;
    }

    if (info->bluez_buf) {
        kfree(info->bluez_buf);
        info->bluez_buf = NULL;
    }

    platform_set_drvdata(info->pdev, NULL);
}

