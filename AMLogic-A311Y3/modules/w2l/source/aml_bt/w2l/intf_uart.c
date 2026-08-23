/*
* Copyright (c) 202X Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/signal.h>
#include <linux/ioctl.h>
#include <linux/skbuff.h>
#include <linux/firmware.h>
#include <linux/serdev.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeup.h>
#include <linux/pm_wakeirq.h>
#include <linux/amlogic/pm.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

//#include "hci_uart.h"
#include "common.h"
#include "intf.h"
#include "intf_uart.h"
#include "intf_uart_coex.h"
#include "rc_list.h"
#include "debug_dev.h"
#include "driver.h"
#include "chip.h"


#define VERSION "2.3"

static const struct hci_uart_proto *hup[HCI_UART_MAX_PROTO];
static int amlbt_hci_uart_tx_wakeup(struct hci_uart *hu);

int amlbt_intf_uart_send_and_wait(u16 opcode,
                                        const uint8_t *cmd, size_t cmd_length,
                                        uint8_t *rsp_buf, size_t *rsp_len)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    int ret = 0;
    size_t total_written = 0;
    struct sk_buff *skb = NULL;
    struct hci_cmd_request *req = kzalloc(sizeof(struct hci_cmd_request), GFP_DMA | GFP_ATOMIC);

    if (req == NULL)
    {
        BTE("hci_cmd_request alloc failed!!\n");
        return -ENOMEM;
    }

    req->opcode = opcode;
    req->cmd_data = cmd;
    req->cmd_length = cmd_length;
    req->rsp_len = 0;
    req->status = 0;

    init_completion(&req->done);

    skb = alloc_skb(cmd_length, GFP_KERNEL);
    if (!skb) {
        BTE("%s %d alloc_skb failed! \n", __func__, __LINE__);
        ret = -ENOMEM;
        goto error;
    }

    /* coverity[noescape:SUPPRESS] */
    if (skb_tailroom(skb) < cmd_length) {
        BTE("%s skb_tailroom(skb) failed!\n", __func__);
        ret = -ENOSPC;
        kfree_skb(skb);
        goto error;
    }
    /* coverity[noescape:SUPPRESS] */
    if (skb_put_data(skb, cmd, cmd_length) == NULL)
    {
        BTE("%s: skb_put_data failed\n", __func__);
        ret = -EFAULT;
        kfree_skb(skb);
        goto error;
    }

    skb_queue_tail(&p_bt->common_res.tx_queue, skb);
    amlbt_intf_queue_work(p_bt->common_res.write_work_wq, &p_bt->common_res.write_work);
    list_add_tail(&req->list, &p_bt->uart_res.hci_pending_list);

    BTI("send hci:%#x, len:%d, total_written:%zu, ret:%d\n", opcode, cmd_length, total_written, ret);

    ret = wait_for_completion_timeout(&req->done, msecs_to_jiffies(1000));
    if (ret == 0)
    {
        BTE("hci cmd wait timeout, opcode:%#x\n", opcode);
        list_del(&req->list);
        kfree(req);
        BTE("trigger bt hw error!\n");
        //schedule_work(&sdio_bt.exception_work);
        return -ETIMEDOUT;
    }

    BTI("hci rsp %d:[%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", req->rsp_len, req->rsp_buf[0],
        req->rsp_buf[1], req->rsp_buf[2], req->rsp_buf[3], req->rsp_buf[4], req->rsp_buf[5], req->rsp_buf[6],
        req->rsp_buf[7], req->rsp_buf[8], req->rsp_buf[9], req->rsp_buf[10]);

    if (rsp_buf && rsp_len)
    {
        BTD("*rsp_len :%d, req->rsp_len: %d\n", *rsp_len, req->rsp_len);
    }

    if (rsp_buf && rsp_len && (*rsp_len >= req->rsp_len) && (req->rsp_len <= sizeof(req->rsp_buf)))
    {
        memcpy(rsp_buf, req->rsp_buf, req->rsp_len);
        *rsp_len = req->rsp_len;
    }
    else
    {
        BTE("memcpy failed! , req->rsp_len:%d", req->rsp_len);
    }

    ret = req->status;
error:
    kfree(req);
    return ret;
}

int amlbt_intf_uart_check_hci_event(struct sk_buff *skb)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    const uint8_t *data = skb->data;
    size_t data_len = skb->len;
    //uint8_t type = data[0];
    uint8_t type = hci_skb_pkt_type(skb);
    uint8_t evt_code = data[0];
    uint16_t opcode = ((data[4] << 8) | data[3]);
    uint8_t plen = data[1];
    struct hci_cmd_request *req, *tmp;
    uint32_t copy_len;

    if (data_len < 3)
    {
        BTE("amlbt_check_hci_event: data too short %d\n", data_len);
        return 1;
    }

    if (type == HCI_EVENT_PKT && evt_code == 0x0f && ((data[5] << 8) | data[4]) == SW_STR_CMD)
    {
        BTE("amlbt_check_hci_event SW_STR_CMD status, drop!\n");
        return 1;
    }
    if (type == HCI_EVENT_PKT && evt_code == 0x0e && opcode == SW_STR_CMD)
    {
        BTE("amlbt_check_hci_event SW_STR_CMD complete, drop!\n");
        return 1;
    }
    if (type == HCI_EVENT_PKT && evt_code == 0x0f && ((data[5] << 8) | data[4]) == SW_SHUTDOWN_CMD)
    {
        BTE("amlbt_check_hci_event SW_SHUTDOWN_CMD status, drop!\n");
        return 1;
    }
    if (type == HCI_EVENT_PKT && evt_code == 0x0e && opcode == SW_SHUTDOWN_CMD)
    {
        BTE("amlbt_check_hci_event SW_SHUTDOWN_CMD complete, drop!\n");
        return 1;
    }

    if (type != HCI_EVENT_PKT || evt_code != 0x0e) //command complete event
    {
        return 0;
    }
    //BTP("amlbt_check_hci_event start\n");
    /* coverity[unreachable:SUPPRESS] */
    list_for_each_entry_safe(req, tmp, &p_bt->uart_res.hci_pending_list, list)
    {
        BTI("amlbt_check_hci_event:%#x, %#x\n", opcode, req->opcode);
        BTI("amlbt_check_hci_event %#x, opcode:%#x\n", (unsigned long)req, req->opcode);
        BTI("evt: %d, %#x [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x] \n", data_len,
                                   data[0], data[1], data[2], data[3], data[4], data[5],
                                   data[6], data[7], data[8], data[9], data[10], data[11]);
        if (req->opcode == opcode)
        {
            list_del(&req->list);
            copy_len = plen + 3;
            if (copy_len > sizeof(req->rsp_buf))
            {
                BTE("amlbt_check_hci_event: copy_len:%d  exceeds rsp_buf_size:%d \n", \
                        copy_len, sizeof(req->rsp_buf));
                return 1;
            }
            memcpy(req->rsp_buf, data, copy_len);
            req->rsp_len = copy_len;
            req->status = 0;
            //if (opcode == TCI_DOWNLOAD_BT_FW)
            //{
            //    kfree(req);
            //    if (skb_queue_len(&p_bt->hu->download_queue) == 0)
            //    {
            //        BTI("Firmware download_queue finished!\n");
            //        complete(&p_bt->done);
            //    }
            //    else
            //    {
            //        amlbt_uart_download();
            //    }
            //}
            //else
            {
                complete(&req->done);
            }
        }
        else
        {
            BTE("opcode not match![%#x,%#x]\n", req->opcode, opcode);
            BTE("evt: %d, %#x [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x] \n", data_len,
                                   data[0], data[1], data[2], data[3], data[4], data[5],
                                   data[6], data[7], data[8], data[9], data[10], data[11]);
            return 0;
        }
        return 1;
    }

    return 0;
}

int amlbt_intf_uart_sw_read_word(unsigned int addr, unsigned int *reg)
{
    int ret;
    uint8_t cmd[] = {0x01, SW_READ_REG & 0xff, (SW_READ_REG >> 8) & 0xff, 0x04, 0x00, 0x00, 0x00, 0x00};
    uint8_t rsp[11] = {0};
    size_t rsp_len = sizeof(rsp);

    cmd[7] = ((addr >> 24) & 0xff);
    cmd[6] = ((addr >> 16) & 0xff);
    cmd[5] = ((addr >> 8) & 0xff);
    cmd[4] = ((addr) & 0xff);

    BTI("%s [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", __func__, cmd[0],cmd[1],cmd[2],cmd[3],cmd[4],cmd[5],cmd[6],cmd[7]);
    ret = amlbt_intf_uart_send_and_wait(SW_READ_REG, cmd, sizeof(cmd), rsp, &rsp_len);
    *reg = (rsp[10] << 24) | (rsp[9] << 16) | (rsp[8] << 8) | rsp[7];
    BTI("%s [%#x, %#x]\n", __func__, addr, reg);
    return ret;
}

int amlbt_intf_uart_sw_write_word(unsigned int addr, unsigned int data)
{
    uint8_t cmd[] = {0x01, SW_WRITE_REG & 0xff, (SW_WRITE_REG >> 8) & 0xff, 0x08,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    BTI("%s [%#x, %#x]\n", __func__, addr, data);
    cmd[7] = ((addr >> 24) & 0xff);
    cmd[6] = ((addr >> 16) & 0xff);
    cmd[5] = ((addr >> 8) & 0xff);
    cmd[4] = ((addr) & 0xff);

    cmd[11] = ((data >> 24) & 0xff);
    cmd[10] = ((data >> 16) & 0xff);
    cmd[9] = ((data >> 8) & 0xff);
    cmd[8] = ((data) & 0xff);
    BTI("%s [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", __func__,
        cmd[0],cmd[1],cmd[2],cmd[3],cmd[4],cmd[5],cmd[6],cmd[7],cmd[8],cmd[9],cmd[10],cmd[11]);
    return amlbt_intf_uart_send_and_wait(SW_WRITE_REG, cmd, sizeof(cmd), NULL, NULL);
}

int amlbt_intf_uart_sw_read_sram(unsigned char* buf, unsigned int addr, unsigned int len)
{
    int ret;
    uint8_t cmd[9] = {0x01, SW_READ_SRAM & 0xff, (SW_READ_SRAM >> 8) & 0xff};
    size_t rsp_len = len+7;//event header 7 bytes

    cmd[3] = 5;
    cmd[7] = ((addr >> 24) & 0xff);
    cmd[6] = ((addr >> 16) & 0xff);
    cmd[5] = ((addr >> 8) & 0xff);
    cmd[4] = ((addr) & 0xff);
    cmd[8] = len;

    BTI("%s [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", __func__, cmd[0],cmd[1],cmd[2],cmd[3],cmd[4],cmd[5],cmd[6],cmd[7]);
    ret = amlbt_intf_uart_send_and_wait(SW_READ_SRAM, cmd, sizeof(cmd), buf, &rsp_len);
    BTI("%s [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", __func__,
        buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],buf[11]);
    return ret;
}

unsigned int amlbt_intf_uart_sw_write_sram(unsigned char *data, unsigned int addr, unsigned char length)
{
    uint8_t cmd[258] = {0x01, SW_WRITE_SRAM & 0xff, (SW_WRITE_SRAM >> 8) & 0xff};

    BTI("%s [%#x, %d]\n", __func__, addr, length);
    cmd[3] = length + 4;
    cmd[7] = ((addr >> 24) & 0xff);
    cmd[6] = ((addr >> 16) & 0xff);
    cmd[5] = ((addr >> 8) & 0xff);
    cmd[4] = ((addr) & 0xff);

    //cmd[11] = ((data >> 24) & 0xff);
    //cmd[10] = ((data >> 16) & 0xff);
    //cmd[9] = ((data >> 8) & 0xff);
    //cmd[8] = ((data) & 0xff);
    memcpy(&cmd[8], data, length);
    BTI("%s %d [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", __func__, length + 4 + 4,
        cmd[0],cmd[1],cmd[2],cmd[3],cmd[4],cmd[5],cmd[6],cmd[7],cmd[8],cmd[9],cmd[10],cmd[11]);
    amlbt_intf_uart_send_and_wait(SW_WRITE_SRAM, cmd, length + 4 + 4, NULL, NULL);

    return 0;
}

int amlbt_intf_uart_hw_write_word(unsigned int addr, unsigned int data)
{
    int ret = 0;
    uint8_t cmd[] = {0x01, TCI_WRITE_REG & 0xff, (TCI_WRITE_REG >> 8) & 0xff, 0x08,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    cmd[7] = ((addr >> 24) & 0xff);
    cmd[6] = ((addr >> 16) & 0xff);
    cmd[5] = ((addr >> 8) & 0xff);
    cmd[4] = ((addr) & 0xff);

    cmd[11] = ((data >> 24) & 0xff);
    cmd[10] = ((data >> 16) & 0xff);
    cmd[9] = ((data >> 8) & 0xff);
    cmd[8] = ((data) & 0xff);
    BTI("%s [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", __func__, cmd[0],cmd[1],cmd[2],cmd[3],cmd[4],cmd[5],cmd[6],cmd[7]);
    ret = amlbt_intf_uart_send_and_wait(TCI_WRITE_REG, cmd, sizeof(cmd), NULL, NULL);
    BTI("%s [%#x, %#x]\n", __func__, addr, data);
    return ret;
}

int amlbt_intf_uart_hw_read_word(unsigned int addr, unsigned int *reg)
{
    int ret;
    uint8_t cmd[] = {0x01, TCI_READ_REG & 0xff, (TCI_READ_REG >> 8) & 0xff, 0x04, 0x00, 0x00, 0x00, 0x00};
    uint8_t rsp[11] = {0};
    size_t rsp_len = sizeof(rsp);

    cmd[7] = ((addr >> 24) & 0xff);
    cmd[6] = ((addr >> 16) & 0xff);
    cmd[5] = ((addr >> 8) & 0xff);
    cmd[4] = ((addr) & 0xff);

    BTI("%s [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", __func__, cmd[0],cmd[1],cmd[2],cmd[3],cmd[4],cmd[5],cmd[6],cmd[7]);
    ret = amlbt_intf_uart_send_and_wait(TCI_READ_REG, cmd, sizeof(cmd), rsp, &rsp_len);
    *reg = (rsp[10] << 24) | (rsp[9] << 16) | (rsp[8] << 8) | rsp[7];
    BTI("%s [%#x, %#x]\n", __func__, addr, reg);
    return ret;
}

int amlbt_intf_uart_hw_write_sram(unsigned int addr, unsigned char len, unsigned char *data)
{
    int ret = 0;
    uint8_t cmd[256] = {0x01, TCI_DOWNLOAD_BT_FW & 0xff, (TCI_DOWNLOAD_BT_FW >> 8) & 0xff, 0x04, 0x00, 0x00, 0x00, 0x00};

    if (len > RW_OPERATION_SIZE)
    {
        len = RW_OPERATION_SIZE;
    }

    cmd[3] = len + 4;

    cmd[7] = ((addr >> 24) & 0xff);
    cmd[6] = ((addr >> 16) & 0xff);
    cmd[5] = ((addr >> 8) & 0xff);
    cmd[4] = ((addr) & 0xff);

    memcpy(&cmd[8], data, len);
    BTI("%s [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", __func__, cmd[0],cmd[1],cmd[2],cmd[3],cmd[4],cmd[5],cmd[6],cmd[7]);
    ret = amlbt_intf_uart_send_and_wait(TCI_DOWNLOAD_BT_FW, cmd, sizeof(cmd), NULL, NULL);
    BTI("%s [%#x, %#x]\n", __func__, addr, data);
    return ret;
}

void amlbt_intf_uart_early_suspend(amlbt_t *p_bt)
{
    int ret = 0;
    uint8_t cmd[] = {0x01, SW_STR_CMD & 0xff, (SW_STR_CMD >> 8) & 0xff, 0x01, 0x01};

    amlbt_intf_flush_workqueue(p_bt->common_res.write_work_wq, &p_bt->common_res.write_work);
    ret = p_bt->uart_res_linux.hu->tty->ops->write(p_bt->uart_res.hu->tty, cmd, sizeof(cmd));
    BTI("%s write len:%d \n", __func__, ret);
}

void amlbt_intf_uart_suspend(amlbt_t *p_bt)
{
    int ret = 0;
    uint8_t cmd[] = {0x01, SW_STR_CMD & 0xff, (SW_STR_CMD >> 8) & 0xff, 0x01, 0x02};

    amlbt_intf_flush_workqueue(p_bt->common_res.write_work_wq, &p_bt->common_res.write_work);
    ret = p_bt->uart_res_linux.hu->tty->ops->write(p_bt->uart_res.hu->tty, cmd, sizeof(cmd));
    BTI("%s write len:%d \n", __func__, ret);
}

void amlbt_intf_uart_resume(amlbt_t *p_bt)
{
    int ret = 0;
    uint8_t cmd[] = {0x01, SW_STR_CMD & 0xff, (SW_STR_CMD >> 8) & 0xff, 0x01, 0x03};

    amlbt_intf_flush_workqueue(p_bt->common_res.write_work_wq, &p_bt->common_res.write_work);
    ret = p_bt->uart_res_linux.hu->tty->ops->write(p_bt->uart_res.hu->tty, cmd, sizeof(cmd));
    BTI("%s write len:%d \n", __func__, ret);
}

void amlbt_intf_uart_later_resume(amlbt_t *p_bt)
{
    int ret = 0;
    uint8_t cmd[] = {0x01, SW_STR_CMD & 0xff, (SW_STR_CMD >> 8) & 0xff, 0x01, 0x00};

    amlbt_intf_flush_workqueue(p_bt->common_res.write_work_wq, &p_bt->common_res.write_work);
    ret = p_bt->uart_res_linux.hu->tty->ops->write(p_bt->uart_res.hu->tty, cmd, sizeof(cmd));
    BTI("%s write len:%d \n", __func__, ret);
}

void amlbt_intf_uart_shutdown(amlbt_t *p_bt)
{
    int ret = 0;
    uint8_t cmd[] = {0x01, SW_SHUTDOWN_CMD & 0xff, (SW_SHUTDOWN_CMD >> 8) & 0xff, 0x00};

    amlbt_intf_flush_workqueue(p_bt->common_res.write_work_wq, &p_bt->common_res.write_work);
    ret = p_bt->uart_res_linux.hu->tty->ops->write(p_bt->uart_res.hu->tty, cmd, sizeof(cmd));
    BTI("%s write len:%d \n", __func__, ret);
}

unsigned int amlbt_intf_uart_sw_write_rclist(unsigned char *data, unsigned char cnt, unsigned char length)
{
    uint8_t cmd[258] = {0x01, SW_WRITE_RCLIST & 0xff, (SW_WRITE_RCLIST >> 8) & 0xff};

    BTI("%s [%#x, %d]\n", __func__, length);
    cmd[3] = length;
    cmd[4] = cnt;

    memcpy(&cmd[5], data, length);
    BTI("%s %d [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", __func__, length + 4 + 1,
        cmd[0],cmd[1],cmd[2],cmd[3],cmd[4],cmd[5],cmd[6],cmd[7],cmd[8],cmd[9],cmd[10],cmd[11]);
    amlbt_intf_uart_send_and_wait(SW_WRITE_RCLIST, cmd, length + 4 + 1, NULL, NULL);

    return 0;
}

int amlbt_intf_uart_sw_read_rclist(unsigned char* buf, unsigned int len)
{
    int ret;
    uint8_t cmd[9] = {0x01, SW_READ_RCLIST & 0xff, (SW_READ_RCLIST >> 8) & 0xff};
    size_t rsp_len = len+6;//event header 4 Bytes+ opcode 2 Bytes

    BTI("%s [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", __func__, cmd[0],cmd[1],cmd[2],cmd[3],cmd[4],cmd[5],cmd[6],cmd[7]);
    ret = amlbt_intf_uart_send_and_wait(SW_READ_RCLIST, cmd, sizeof(cmd), buf, &rsp_len);
    BTI("%s [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", __func__,
        buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],buf[11]);
    return ret;
}

void amlbt_intf_uart_write_work(amlbt_t *p_bt)
{
    amlbt_res_uart_linux_t *uart_res_linux;
    struct hci_uart *hu;

    if (!p_bt) {
        BTE("%s: p_bt is NULL!\n", __func__);
        return;
    }

    uart_res_linux = &p_bt->uart_res_linux;
    hu = uart_res_linux->hu;

    if (hu) {
        amlbt_hci_uart_tx_wakeup(hu);
    }
}

//linux hci_ldisc for bt/15p4
int amlbt_hci_uart_register_proto(const struct hci_uart_proto *p)
{
     BTI("%s", __func__);

    if (p->id >= HCI_UART_MAX_PROTO)
        return -EINVAL;

    if (hup[p->id])
        return -EEXIST;

    hup[p->id] = p;

    BTI("HCI UART protocol %s registered", p->name);

    return 0;
}

int amlbt_hci_uart_unregister_proto(const struct hci_uart_proto *p)
{
    BTI("%s", __func__);

    if (p->id >= HCI_UART_MAX_PROTO)
        return -EINVAL;

    if (!hup[p->id])
        return -EINVAL;

    hup[p->id] = NULL;

    return 0;
}

static const struct hci_uart_proto *amlbt_hci_uart_get_proto(unsigned int id)
{
    if (id >= HCI_UART_MAX_PROTO)
        return NULL;

    return hup[id];
}

static inline void amlbt_hci_uart_tx_complete(struct hci_uart *hu, int pkt_type)
{
    struct hci_dev *hdev = hu->hdev;

    /* Update HCI stat counters */
    switch (pkt_type) {
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
}

static inline struct sk_buff *amlbt_hci_uart_dequeue(struct hci_uart *hu)
{
    struct sk_buff *skb = hu->tx_skb;

    if (!skb) {
        percpu_down_read(&hu->proto_lock);

        if (test_bit(HCI_UART_PROTO_READY, &hu->flags))
            skb = hu->proto->dequeue(hu);

        percpu_up_read(&hu->proto_lock);
    } else {
        hu->tx_skb = NULL;
    }

    return skb;
}

static int amlbt_hci_uart_tx_wakeup(struct hci_uart *hu)
{
    /* This may be called in an IRQ context, so we can't sleep. Therefore
     * we try to acquire the lock only, and if that fails we assume the
     * tty is being closed because that is the only time the write lock is
     * acquired. If, however, at some point in the future the write lock
     * is also acquired in other situations, then this must be revisited.
     */
    if (!percpu_down_read_trylock(&hu->proto_lock))
        return 0;

    if (!test_bit(HCI_UART_PROTO_READY, &hu->flags))
        goto no_schedule;

    set_bit(HCI_UART_TX_WAKEUP, &hu->tx_state);
    if (test_and_set_bit(HCI_UART_SENDING, &hu->tx_state))
        goto no_schedule;

    //BTI("%s", __func__);

    schedule_work(&hu->write_work);

no_schedule:
    percpu_up_read(&hu->proto_lock);

    return 0;
}

static void amlbt_hci_uart_write_work(struct work_struct *work)
{
    struct hci_uart *hu = container_of(work, struct hci_uart, write_work);
    struct tty_struct *tty = hu->tty;
    struct hci_dev *hdev = hu->hdev;
    struct sk_buff *skb;

    /* REVISIT: should we cope with bad skbs or ->write() returning
     * and error value ?
     */

restart:
    clear_bit(HCI_UART_TX_WAKEUP, &hu->tx_state);

    while ((skb = amlbt_hci_uart_dequeue(hu))) {
        int len;

        set_bit(TTY_DO_WRITE_WAKEUP, &tty->flags);
        BTD("%s data: %02x %02x %02x %02x %02x %02x %02x %02x\n", __func__,
            skb->data[0], skb->data[1], skb->data[2], skb->data[3],
            skb->data[4], skb->data[5], skb->data[6], skb->data[7]);
        len = tty->ops->write(tty, skb->data, skb->len);
        hdev->stat.byte_tx += len;

        skb_pull(skb, len);
        if (skb->len) {
            hu->tx_skb = skb;
            break;
        }

        amlbt_hci_uart_tx_complete(hu, hci_skb_pkt_type(skb));
        kfree_skb(skb);
    }

    clear_bit(HCI_UART_SENDING, &hu->tx_state);
    if (test_bit(HCI_UART_TX_WAKEUP, &hu->tx_state))
        goto restart;

    wake_up_bit(&hu->tx_state, HCI_UART_SENDING);
}
#if 0
void hci_uart_init_work(struct work_struct *work)
{
    struct hci_uart *hu = container_of(work, struct hci_uart, init_ready);
    int err;
    struct hci_dev *hdev;

    if (!test_and_clear_bit(HCI_UART_INIT_PENDING, &hu->hdev_flags))
        return;

    err = hci_register_dev(hu->hdev);
    if (err < 0) {
        BTE("Can't register HCI device");
        clear_bit(HCI_UART_PROTO_READY, &hu->flags);
        hu->proto->close(hu);
        hdev = hu->hdev;
        hu->hdev = NULL;
        hci_free_dev(hdev);
        return;
    }

    set_bit(HCI_UART_REGISTERED, &hu->flags);
}

int hci_uart_init_ready(struct hci_uart *hu)
{
    if (!test_bit(HCI_UART_INIT_PENDING, &hu->hdev_flags))
        return -EALREADY;

    schedule_work(&hu->init_ready);

    return 0;
}

int hci_uart_wait_until_sent(struct hci_uart *hu)
{
    return wait_on_bit_timeout(&hu->tx_state, HCI_UART_SENDING,
                   TASK_INTERRUPTIBLE,
                   msecs_to_jiffies(2000));
}
#endif

/* ------- Interface to HCI layer ------ */
/* Reset device */
static int amlbt_hci_uart_flush(struct hci_dev *hdev)
{
    struct hci_uart *hu  = hci_get_drvdata(hdev);
    struct tty_struct *tty = hu->tty;

    BTI("%s hdev %p tty %p", __func__, hdev, tty);

    if (hu->tx_skb) {
        kfree_skb(hu->tx_skb); hu->tx_skb = NULL;
    }

    /* Flush any pending characters in the driver and discipline. */
    tty_ldisc_flush(tty);
    tty_driver_flush_buffer(tty);

    percpu_down_read(&hu->proto_lock);

    if (test_bit(HCI_UART_PROTO_READY, &hu->flags))
        hu->proto->flush(hu);

    percpu_up_read(&hu->proto_lock);

    return 0;
}

/* Initialize device */
static int amlbt_hci_uart_open(struct hci_dev *hdev)
{
    BTI("%s %s %p", __func__, hdev->name, hdev);

    /* Undo clearing this from hci_uart_close() */
    hdev->flush = amlbt_hci_uart_flush;

    return 0;
}

/* Close device */
static int amlbt_hci_uart_close(struct hci_dev *hdev)
{
    BTI("%s hdev %p", __func__, hdev);

    amlbt_hci_uart_flush(hdev);
    hdev->flush = NULL;
    return 0;
}

/* Send frames from HCI layer */
static int amlbt_hci_uart_send_frame(struct hci_dev *hdev, struct sk_buff *skb)
{
    struct hci_uart *hu = hci_get_drvdata(hdev);

    BTD("%s: type %d len %d", hdev->name, hci_skb_pkt_type(skb),
           skb->len);

    percpu_down_read(&hu->proto_lock);

    if (!test_bit(HCI_UART_PROTO_READY, &hu->flags)) {
        percpu_up_read(&hu->proto_lock);
        return -EUNATCH;
    }

    hu->proto->enqueue(hu, skb);
    percpu_up_read(&hu->proto_lock);

    amlbt_hci_uart_tx_wakeup(hu);

    return 0;
}
#if 0
/* Check the underlying device or tty has flow control support */
bool hci_uart_has_flow_control(struct hci_uart *hu)
{
    /* serdev nodes check if the needed operations are present */
    if (hu->serdev)
        return true;

    if (hu->tty->driver->ops->tiocmget && hu->tty->driver->ops->tiocmset)
        return true;

    return false;
}

/* Flow control or un-flow control the device */
void hci_uart_set_flow_control(struct hci_uart *hu, bool enable)
{
    struct tty_struct *tty = hu->tty;
    struct ktermios ktermios;
    int status;
    unsigned int set = 0;
    unsigned int clear = 0;

    if (hu->serdev) {
        serdev_device_set_flow_control(hu->serdev, !enable);
        serdev_device_set_rts(hu->serdev, !enable);
        return;
    }

    if (enable) {
        /* Disable hardware flow control */
        ktermios = tty->termios;
        ktermios.c_cflag &= ~CRTSCTS;
        tty_set_termios(tty, &ktermios);
        BTI("Disabling hardware flow control: %s",
               (tty->termios.c_cflag & CRTSCTS) ? "failed" : "success");

        /* Clear RTS to prevent the device from sending */
        /* Most UARTs need OUT2 to enable interrupts */
        status = tty->driver->ops->tiocmget(tty);
        BTI("Current tiocm 0x%x", status);

        set &= ~(TIOCM_OUT2 | TIOCM_RTS);
        clear = ~set;
        set &= TIOCM_DTR | TIOCM_RTS | TIOCM_OUT1 |
             TIOCM_OUT2 | TIOCM_LOOP;
        clear &= TIOCM_DTR | TIOCM_RTS | TIOCM_OUT1 |
             TIOCM_OUT2 | TIOCM_LOOP;
        status = tty->driver->ops->tiocmset(tty, set, clear);
        BTI("Clearing RTS: %s", status ? "failed" : "success");
    } else {
        /* Set RTS to allow the device to send again */
        status = tty->driver->ops->tiocmget(tty);
        BT_DBG("Current tiocm 0x%x", status);

        set |= (TIOCM_OUT2 | TIOCM_RTS);
        clear = ~set;
        set &= TIOCM_DTR | TIOCM_RTS | TIOCM_OUT1 |
               TIOCM_OUT2 | TIOCM_LOOP;
        clear &= TIOCM_DTR | TIOCM_RTS | TIOCM_OUT1 |
             TIOCM_OUT2 | TIOCM_LOOP;
        status = tty->driver->ops->tiocmset(tty, set, clear);
        BTI("Setting RTS: %s", status ? "failed" : "success");

        /* Re-enable hardware flow control */
        ktermios = tty->termios;
        ktermios.c_cflag |= CRTSCTS;
        tty_set_termios(tty, &ktermios);
        BTI("Enabling hardware flow control: %s",
               !(tty->termios.c_cflag & CRTSCTS) ? "failed" : "success");
    }
}

void hci_uart_set_speeds(struct hci_uart *hu, unsigned int init_speed,
                unsigned int oper_speed)
{
    hu->init_speed = init_speed;
    hu->oper_speed = oper_speed;
}
#endif

static void amlbt_hci_uart_set_baudrate(struct hci_uart *hu, unsigned int speed)
{
    struct tty_struct *tty = hu->tty;
    struct ktermios ktermios;

    ktermios = tty->termios;
    ktermios.c_cflag &= ~CBAUD;
    tty_termios_encode_baud_rate(&ktermios, speed, speed);

    /* tty_set_termios() return not checked as it is always 0 */
    tty_set_termios(tty, &ktermios);

    BTI("%s: New tty speeds: %d/%d", hu->hdev->name,
           tty->termios.c_ispeed, tty->termios.c_ospeed);
}

static int amlbt_hci_uart_setup(struct hci_dev *hdev)
{
    struct hci_uart *hu = hci_get_drvdata(hdev);
    struct hci_rp_read_local_version *ver;
    struct sk_buff *skb;
    unsigned int speed;
    int err;

    /* Init speed if any */
    if (hu->init_speed)
        speed = hu->init_speed;
    else if (hu->proto->init_speed)
        speed = hu->proto->init_speed;
    else
        speed = 0;

    if (speed)
        amlbt_hci_uart_set_baudrate(hu, speed);

    /* Operational speed if any */
    if (hu->oper_speed)
        speed = hu->oper_speed;
    else if (hu->proto->oper_speed)
        speed = hu->proto->oper_speed;
    else
        speed = 0;

    if (hu->proto->set_baudrate && speed) {
        err = hu->proto->set_baudrate(hu, speed);
        if (!err)
            amlbt_hci_uart_set_baudrate(hu, speed);
    }

    if (hu->proto->setup)
        return hu->proto->setup(hu);

    if (!test_bit(HCI_UART_VND_DETECT, &hu->hdev_flags))
        return 0;

    skb = __hci_cmd_sync(hdev, HCI_OP_READ_LOCAL_VERSION, 0, NULL,
                 HCI_INIT_TIMEOUT);
    if (IS_ERR(skb)) {
        BTE("%s: Reading local version information failed (%ld)",
               hdev->name, PTR_ERR(skb));
        return 0;
    }

    if (skb->len != sizeof(*ver)) {
        BTE("%s: Event length mismatch for version information",
               hdev->name);
        goto done;
    }
#if 0
    ver = (struct hci_rp_read_local_version *)skb->data;

    switch (le16_to_cpu(ver->manufacturer)) {
#ifdef CONFIG_BT_HCIUART_INTEL
    case 2:
        hdev->set_bdaddr = btintel_set_bdaddr;
        btintel_check_bdaddr(hdev);
        break;
#endif
#ifdef CONFIG_BT_HCIUART_BCM
    case 15:
        hdev->set_bdaddr = btbcm_set_bdaddr;
        btbcm_check_bdaddr(hdev);
        break;
#endif
    default:
        break;
    }
#endif
done:
    kfree_skb(skb);
    return 0;
}

/* ------ LDISC part ------ */
/* hci_uart_tty_open
 *
 *     Called when line discipline changed to HCI_UART.
 *
 * Arguments:
 *     tty    pointer to tty info structure
 * Return Value:
 *     0 if success, otherwise error code
 */
static int amlbt_hci_uart_tty_open(struct tty_struct *tty)
{
    struct hci_uart *hu;

    BTI("%s tty %p", __func__, tty);

    if (!capable(CAP_NET_ADMIN))
        return -EPERM;

    /* Error if the tty has no write op instead of leaving an exploitable
     * hole
     */
    if (tty->ops->write == NULL)
        return -EOPNOTSUPP;

    hu = kzalloc(sizeof(struct hci_uart), GFP_KERNEL);
    if (!hu) {
        BTE("Can't allocate control structure");
        return -ENFILE;
    }
    if (percpu_init_rwsem(&hu->proto_lock)) {
        BTE("Can't allocate semaphore structure");
        kfree(hu);
        return -ENOMEM;
    }

    tty->disc_data = hu;
    hu->tty = tty;
    tty->receive_room = 65536;

    /* disable alignment support by default */
    hu->alignment = 1;
    hu->padding = 0;

    //INIT_WORK(&hu->init_ready, hci_uart_init_work);
    INIT_WORK(&hu->write_work, amlbt_hci_uart_write_work);

    /* Flush any pending characters in the driver */
    tty_driver_flush_buffer(tty);

    return 0;
}

/* hci_uart_tty_close()
 *
 *    Called when the line discipline is changed to something
 *    else, the tty is closed, or the tty detects a hangup.
 */
static void amlbt_hci_uart_tty_close(struct tty_struct *tty)
{
    struct hci_uart *hu = tty->disc_data;
    struct hci_dev *hdev;

    BTI("%s tty %p", __func__, tty);

    /* Detach from the tty */
    tty->disc_data = NULL;

    if (!hu)
        return;

    hdev = hu->hdev;
    if (hdev)
        amlbt_hci_uart_close(hdev);

    if (test_bit(HCI_UART_PROTO_READY, &hu->flags)) {
        percpu_down_write(&hu->proto_lock);
        clear_bit(HCI_UART_PROTO_READY, &hu->flags);
        percpu_up_write(&hu->proto_lock);

        //cancel_work_sync(&hu->init_ready);
        cancel_work_sync(&hu->write_work);

        if (hdev) {
            if (test_bit(HCI_UART_REGISTERED, &hu->flags))
                hci_unregister_dev(hdev);
            hci_free_dev(hdev);
        }
        hu->proto->close(hu);
    }
    clear_bit(HCI_UART_PROTO_SET, &hu->flags);

    percpu_free_rwsem(&hu->proto_lock);

    kfree(hu);
}

/* hci_uart_tty_wakeup()
 *
 *    Callback for transmit wakeup. Called when low level
 *    device driver can accept more send data.
 *
 * Arguments:        tty    pointer to associated tty instance data
 * Return Value:    None
 */
static void amlbt_hci_uart_tty_wakeup(struct tty_struct *tty)
{
    struct hci_uart *hu = tty->disc_data;

    //BTD("%s tty %p", __func__, tty);

    if (!hu)
        return;

    clear_bit(TTY_DO_WRITE_WAKEUP, &tty->flags);

    if (tty != hu->tty)
        return;

    if (test_bit(HCI_UART_PROTO_READY, &hu->flags))
        amlbt_hci_uart_tx_wakeup(hu);
}

/* hci_uart_tty_receive()
 *
 *     Called by tty low level driver when receive data is
 *     available.
 *
 * Arguments:  tty          pointer to tty instance data
 *             data         pointer to received data
 *             flags        pointer to flags for data
 *             count        count of received data in bytes
 *
 * Return Value:    None
 */
//static void amlbt_hci_uart_tty_receive(struct tty_struct *tty, const unsigned char *data,
//                 const unsigned char *flags, size_t count)
static void amlbt_hci_uart_tty_receive(struct tty_struct *tty, const unsigned char *data,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0) || LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0))
                                       const unsigned char *flags, size_t count)
#else
                                       const char *flags, int count)
#endif
{
    struct hci_uart *hu = tty->disc_data;

    if (!hu || tty != hu->tty)
        return;

    percpu_down_read(&hu->proto_lock);

    if (!test_bit(HCI_UART_PROTO_READY, &hu->flags)) {
        percpu_up_read(&hu->proto_lock);
        return;
    }

    /* It does not need a lock here as it is already protected by a mutex in
     * tty caller
     */
    hu->proto->recv(hu, data, count);
    percpu_up_read(&hu->proto_lock);

    if (hu->hdev)
        hu->hdev->stat.byte_rx += count;

    tty_unthrottle(tty);
}

static int amlbt_hci_uart_register_dev(struct hci_uart *hu)
{
    struct hci_dev *hdev;
    int err;

    BTI("%s", __func__);

    /* Initialize and register HCI device */
    hdev = hci_alloc_dev();
    if (!hdev) {
        BTE("Can't allocate HCI device");
        return -ENOMEM;
    }

    hu->hdev = hdev;

    hdev->bus = HCI_UART;
    hci_set_drvdata(hdev, hu);

    /* Only when vendor specific setup callback is provided, consider
     * the manufacturer information valid. This avoids filling in the
     * value for Ericsson when nothing is specified.
     */
    if (hu->proto->setup)
        hdev->manufacturer = hu->proto->manufacturer;

    hdev->open  = amlbt_hci_uart_open;
    hdev->close = amlbt_hci_uart_close;
    hdev->flush = amlbt_hci_uart_flush;
    hdev->send  = amlbt_hci_uart_send_frame;
    hdev->setup = amlbt_hci_uart_setup;
    SET_HCIDEV_DEV(hdev, hu->tty->dev);

    if (test_bit(HCI_UART_RAW_DEVICE, &hu->hdev_flags))
        set_bit(HCI_QUIRK_RAW_DEVICE, &hdev->quirks);

    if (test_bit(HCI_UART_EXT_CONFIG, &hu->hdev_flags))
        set_bit(HCI_QUIRK_EXTERNAL_CONFIG, &hdev->quirks);

    if (!test_bit(HCI_UART_RESET_ON_INIT, &hu->hdev_flags))
        set_bit(HCI_QUIRK_RESET_ON_CLOSE, &hdev->quirks);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 10, 0)
    if (test_bit(HCI_UART_CREATE_AMP, &hu->hdev_flags))
        hdev->dev_type = HCI_AMP;
    else
        hdev->dev_type = HCI_PRIMARY;
#endif

    /* Only call open() for the protocol after hdev is fully initialized as
     * open() (or a timer/workqueue it starts) may attempt to reference it.
     */
    err = hu->proto->open(hu);
    if (err) {
        hu->hdev = NULL;
        hci_free_dev(hdev);
        return err;
    }

    if (test_bit(HCI_UART_INIT_PENDING, &hu->hdev_flags))
        return 0;

    if (hci_register_dev(hdev) < 0) {
        BTE("Can't register HCI device");
        hu->proto->close(hu);
        hu->hdev = NULL;
        hci_free_dev(hdev);
        return -ENODEV;
    }

    set_bit(HCI_UART_REGISTERED, &hu->flags);

    return 0;
}

static int amlbt_hci_uart_set_proto(struct hci_uart *hu, int id)
{
    const struct hci_uart_proto *p;
    int err;

    p = amlbt_hci_uart_get_proto(id);
    if (!p)
        return -EPROTONOSUPPORT;

    hu->proto = p;

    err = amlbt_hci_uart_register_dev(hu);
    if (err) {
        return err;
    }

    set_bit(HCI_UART_PROTO_READY, &hu->flags);
    return 0;
}

static int amlbt_hci_uart_set_flags(struct hci_uart *hu, unsigned long flags)
{
    unsigned long valid_flags = BIT(HCI_UART_RAW_DEVICE) |
                    BIT(HCI_UART_RESET_ON_INIT) |
                    BIT(HCI_UART_CREATE_AMP) |
                    BIT(HCI_UART_INIT_PENDING) |
                    BIT(HCI_UART_EXT_CONFIG) |
                    BIT(HCI_UART_VND_DETECT);

    if (flags & ~valid_flags)
        return -EINVAL;

    hu->hdev_flags = flags;

    return 0;
}

/* hci_uart_tty_ioctl()
 *
 *    Process IOCTL system call for the tty device.
 *
 * Arguments:
 *
 *    tty        pointer to tty instance data
 *    cmd        IOCTL command code
 *    arg        argument for IOCTL call (cmd dependent)
 *
 * Return Value:    Command dependent
 */
static int amlbt_hci_uart_tty_ioctl(struct tty_struct *tty,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))
                                    struct file *file,
#endif
                                    unsigned int cmd, unsigned long arg)

{
    struct hci_uart *hu = tty->disc_data;
    int err = 0;

    BTI("%s tty %p", __func__, tty);

    /* Verify the status of the device */
    if (!hu)
        return -EBADF;

    switch (cmd) {
    case HCIUARTSETPROTO:
        if (!test_and_set_bit(HCI_UART_PROTO_SET, &hu->flags)) {
            err = amlbt_hci_uart_set_proto(hu, arg);
            if (err)
                clear_bit(HCI_UART_PROTO_SET, &hu->flags);
        } else
            err = -EBUSY;
        break;

    case HCIUARTGETPROTO:
        if (test_bit(HCI_UART_PROTO_SET, &hu->flags) &&
            test_bit(HCI_UART_PROTO_READY, &hu->flags))
            err = hu->proto->id;
        else
            err = -EUNATCH;
        break;

    case HCIUARTGETDEVICE:
        if (test_bit(HCI_UART_REGISTERED, &hu->flags))
            err = hu->hdev->id;
        else
            err = -EUNATCH;
        break;

    case HCIUARTSETFLAGS:
        if (test_bit(HCI_UART_PROTO_SET, &hu->flags))
            err = -EBUSY;
        else
            err = amlbt_hci_uart_set_flags(hu, arg);
        break;

    case HCIUARTGETFLAGS:
        err = hu->hdev_flags;
        break;

    default:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))
        err = n_tty_ioctl_helper(tty, file, cmd, arg);
#else
        err = n_tty_ioctl_helper(tty, cmd, arg);
#endif
        break;
    }

    return err;
}

/*
 * We don't provide read/write/poll interface for user space.
 */
static ssize_t amlbt_hci_uart_tty_read(struct tty_struct *tty, struct file *file,
                 u8 *buf, size_t nr, void **cookie,
                 unsigned long offset)
{
    BTI("%s \n", __func__);
    return 0;
}

static ssize_t amlbt_hci_uart_tty_write(struct tty_struct *tty, struct file *file,
                 const u8 *data, size_t count)
{
    BTI("%s \n", __func__);
    return 0;
}

static struct tty_ldisc_ops hci_uart_ldisc = {
    .owner      = THIS_MODULE,
    .num        = N_AML,
    .name       = "n_aml",
    .open       = amlbt_hci_uart_tty_open,
    .close      = amlbt_hci_uart_tty_close,
    .read       = amlbt_hci_uart_tty_read,
    .write      = amlbt_hci_uart_tty_write,
    .ioctl      = amlbt_hci_uart_tty_ioctl,
    .compat_ioctl   = amlbt_hci_uart_tty_ioctl,
    .receive_buf    = amlbt_hci_uart_tty_receive,
    .write_wakeup   = amlbt_hci_uart_tty_wakeup,
};

int amlbt_intf_uart_register(void)
{
    int err;

    BTI("%s UART driver ver %s", __func__, VERSION);

    /* Register the tty discipline */
    err = tty_register_ldisc(&hci_uart_ldisc);
    if (err) {
        BTE("HCI line discipline registration failed. (%d)", err);
        return err;
    }

    aml_coex_init();
    return 0;
}

void amlbt_intf_uart_unregister(void)
{
    BTI("%s\n", __func__);

    aml_coex_deinit();
    tty_unregister_ldisc(&hci_uart_ldisc);
}

void amlbt_intf_uart_exception_func(struct work_struct *work)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    BTI("%s \n", __func__);
    amlbt_intf_exception_func(p_bt);
}

