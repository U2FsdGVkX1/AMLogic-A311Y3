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
#include <linux/compat.h>
#include <linux/io.h>
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
#include <linux/pm_wakeup.h>
#include <linux/amlogic/pm.h>
#include <linux/ctype.h>
#include <linux/version.h>

#include "common.h"
#include "intf.h"

unsigned int g_dbg_level = LOG_LEVEL_INFO;
unsigned int amlbt_if_type = AMLBT_TRANS_UNKNOWN;
unsigned int polling_time = 8000;
unsigned int amlbt_ft_mode = 0;

int amlbt_common_reg_bit_set(unsigned int addr, unsigned int bit)
{
    int ret = 0;
    unsigned int reg_value = 0;

    ret = amlbt_intf_read_word(addr, &reg_value);
    if (ret != 0)
    {
        goto err_exit;
    }
    BTI("%#x: %#x\n", addr, reg_value);
    reg_value |= BIT(bit);
    ret = amlbt_intf_write_word(addr, reg_value);
    if (ret != 0)
    {
        goto err_exit;
    }
    ret = amlbt_intf_read_word(addr, &reg_value);
    if (ret != 0)
    {
        goto err_exit;
    }
    BTI("%#x: %#x", addr, reg_value);
    return ret;
err_exit:
    return ret;
}

int amlbt_common_reg_bit_clr(unsigned int addr, unsigned int bit)
{
    unsigned int reg_value = 0;
    int ret = 0;

    ret = amlbt_intf_read_word(addr, &reg_value);
    if (ret != 0)
    {
       goto err_exit;
    }
    BTI("%#x: %#x\n", addr, reg_value);
    reg_value &= ~BIT(bit);
    ret = amlbt_intf_write_word(addr, reg_value);
    if (ret != 0)
    {
       goto err_exit;
    }
    ret = amlbt_intf_read_word(addr, &reg_value);
    if (ret != 0)
    {
       goto err_exit;
    }
    BTI("%#x: %#x", addr, reg_value);

    return 0;
err_exit:
    return ret;
}

int amlbt_common_reg_bit_get(unsigned int addr, unsigned int bit)
{
    unsigned int reg_value = 0;
    int bit_value = 0;
    int ret;

    ret = amlbt_intf_read_word(addr, &reg_value);
    if (ret == -1)
    {
        return ret;
    }
    bit_value = (reg_value >> bit) & 0x1;
    BTI("get %#x bit%#d: %#x\n", addr, bit, bit_value);

    return bit_value;
}


amlbt_common_gdsl_fifo_t *amlbt_common_gdsl_fifo_init(unsigned int len, unsigned char *base_addr)
{
    amlbt_common_gdsl_fifo_t *p_fifo = (amlbt_common_gdsl_fifo_t *)kzalloc(sizeof(amlbt_common_gdsl_fifo_t), GFP_DMA|GFP_ATOMIC);
    BTA("%s \n", __func__);
    if (p_fifo == NULL)
    {
        BTE("amlbt_common_gdsl_fifo_init alloc error!\n");
        return NULL;
    }
    else
    {
        memset(p_fifo, 0, sizeof(amlbt_common_gdsl_fifo_t));
        p_fifo->w = 0;
        p_fifo->r = 0;
        p_fifo->base_addr = base_addr;
        p_fifo->size = len;
    }
    return p_fifo;
}

void amlbt_common_gdsl_fifo_deinit(amlbt_common_gdsl_fifo_t *p_fifo)
{
    if (p_fifo == NULL)
    {
        return ;
    }

    kfree(p_fifo);
}

unsigned int amlbt_common_gdsl_fifo_used(amlbt_common_gdsl_fifo_t *p_fifo)
{
    if (p_fifo->r <= p_fifo->w)
        return (p_fifo->w - p_fifo->r);

    return (p_fifo->size + p_fifo->w - p_fifo->r);
}

unsigned int amlbt_common_gdsl_fifo_remain(amlbt_common_gdsl_fifo_t *p_fifo)
{
    unsigned int used = amlbt_common_gdsl_fifo_used(p_fifo);

    return p_fifo->size - used - 4;
}


unsigned int amlbt_common_gdsl_fifo_copy_data(amlbt_common_gdsl_fifo_t *p_fifo, unsigned char *buff, unsigned int len)
{
    unsigned int i = 0;
    unsigned long offset = (unsigned long)p_fifo->w;

    BTA("copy d %d, %#x, %#x\n", len, p_fifo->base_addr, p_fifo->w);

    if (amlbt_common_gdsl_fifo_remain(p_fifo) < len)
    {
        BTE("amlbt_common_gdsl_fifo_copy_data no space!!\n");
        BTE("fifo->base_addr %#x, fifo->size %#x\n", (unsigned long)p_fifo->base_addr, p_fifo->size);
        BTE("fifo->w %#x, fifo->r %#x\n", (unsigned long)p_fifo->w, (unsigned long)p_fifo->r);
        BTE("remain %#x, len %#x\n", amlbt_common_gdsl_fifo_remain(p_fifo), len);
        printk(KERN_CONT "fifodata:[");
        for (; i < p_fifo->size; i++)
        {
            printk(KERN_CONT "%#x,", p_fifo->base_addr[i]);
        }
        printk(KERN_CONT "]");
        return -1;
    }

    if (len < (p_fifo->size - offset))
    {
        memcpy((unsigned char *)((unsigned long)p_fifo->w + (unsigned long)p_fifo->base_addr), buff, len);
        p_fifo->w = (unsigned char *)(((unsigned long)p_fifo->w + len) % p_fifo->size);
    }
    else
    {
        memcpy((unsigned char *)((unsigned long)p_fifo->w + (unsigned long)p_fifo->base_addr), buff, p_fifo->size - offset);
        memcpy((unsigned char *)((unsigned long)p_fifo->base_addr), &buff[p_fifo->size - offset], len - (p_fifo->size - offset));
        p_fifo->w = (unsigned char *)((len - (p_fifo->size - offset)) % p_fifo->size);
    }

    BTA("actual len %#x \n", i);

    return len;
}

unsigned int amlbt_common_gdsl_fifo_calc_r(amlbt_common_gdsl_fifo_t *p_fifo, unsigned char *buff, unsigned int len)
{
    unsigned int used = amlbt_common_gdsl_fifo_used(p_fifo);
    unsigned int i = 0;
    unsigned int get_len = (len >= used ? used : len);
    unsigned long offset = (unsigned long)p_fifo->r;

    BTA("get d %d, %#x, %#x\n", get_len, p_fifo->base_addr, p_fifo->w);
    if (used == 0)
    {
        return 0;
    }

    if (get_len < (p_fifo->size - offset))
    {
        p_fifo->r = (unsigned char *)(((unsigned long)p_fifo->r + get_len) % p_fifo->size);
    }
    else
    {
        p_fifo->r = (unsigned char *)((get_len - (p_fifo->size - offset)) % p_fifo->size);
    }

    BTA("actual len %#x \n", i);

    return get_len;
}

unsigned int amlbt_common_gdsl_fifo_update_r(amlbt_common_gdsl_fifo_t *p_fifo, unsigned int len)
{
    unsigned int offset = 0;
    unsigned int read_len = 0;
    unsigned char *p_end = 0;

    //printk("%s p_fifo->w %#x, p_fifo->r %#x\n", __func__, (unsigned long)p_fifo->w, (unsigned long)p_fifo->r);
    //printk("%s len %d\n", __func__, len);

    if (p_fifo->w == p_fifo->r)
    {
        printk("%s no data!!!\n", __func__);
        return 0;
    }

    if (p_fifo->w > p_fifo->r)
    {
        read_len = (unsigned int)(p_fifo->w - p_fifo->r);
        if (len <= read_len)
        {
            read_len = len;
        }
        //printk("%s read len A %d\n", __func__, read_len);
        p_fifo->r += read_len;
    }
    else
    {
        p_end = (p_fifo->base_addr + p_fifo->size);
        BTA("%s w %#x, r %#x\n", __func__, (unsigned long)p_fifo->w, (unsigned long)p_fifo->r);
        BTA("%s read p_end %#x\n", __func__, (unsigned long)p_end);
        offset = (unsigned int)(p_end - p_fifo->r);
        read_len = offset;
        if (len < offset)
        {
            p_fifo->r += len;
            read_len = len;
            BTA("%s 111 len %#x \n", __func__, len);
        }
        else
        {
            p_fifo->r = p_fifo->base_addr;
            read_len += (len - offset);
            p_fifo->r += (len - offset);
            BTA("%s 222 len %#x \n", __func__, len);
        }
        //printk("%s read len B %#x \n", __func__, read_len);
    }

    //printk("%s actual len %#x \n", __func__, read_len);

    return read_len;
}

unsigned int amlbt_common_gdsl_fifo_get_data(amlbt_common_gdsl_fifo_t *p_fifo, unsigned char *buff, unsigned int len)
{
    unsigned int used = amlbt_common_gdsl_fifo_used(p_fifo);
    unsigned int i = 0;
    unsigned int get_len = (len >= used ? used : len);
    unsigned long offset = (unsigned long)p_fifo->r;

    BTD("get d %d, %#x, %#x %#x\n", get_len, p_fifo->base_addr, p_fifo->w, p_fifo->r);
    if (used == 0)
    {
        return 0;
    }

    if (get_len < (p_fifo->size - offset))
    {
        memcpy(buff, (unsigned char *)((unsigned long)p_fifo->r + (unsigned long)p_fifo->base_addr), get_len);
        p_fifo->r = (unsigned char *)(((unsigned long)p_fifo->r + get_len) % p_fifo->size);
    }
    else
    {
        memcpy(buff, (unsigned char *)((unsigned long)p_fifo->r + (unsigned long)p_fifo->base_addr), (p_fifo->size - offset));
        memcpy(&buff[p_fifo->size - offset], (unsigned char *)((unsigned long)p_fifo->base_addr), (get_len - (p_fifo->size - offset)));
        p_fifo->r = (unsigned char *)((get_len - (p_fifo->size - offset)) % p_fifo->size);
    }

    BTA("actual len %s %#x \n", __func__, i);

    return get_len;
}

int amlbt_common_gdsl_write_data(amlbt_common_gdsl_fifo_t *p_fifo, unsigned char *data, int len)
{
    int ret = 0;
    unsigned long offset = (unsigned long)p_fifo->w;

    BTA("%s len:%d\n", __func__, len);

    len = ((len + 3) & 0xFFFFFFFC);
    if (amlbt_common_gdsl_fifo_remain(p_fifo) < len)
    {
        BTE("write data no space!!\n");
        return -2;
    }

    if (len < (p_fifo->size - offset))
    {
        ret = amlbt_intf_write_sram(data, (unsigned int)((unsigned long)p_fifo->w + (unsigned long)p_fifo->base_addr), len);
        if (ret != 0)
        {
            BTE("%s:%d Failed : %d len %d\n", __func__, __LINE__, ret, len);
            return -1;
        }
        p_fifo->w = (unsigned char *)(((unsigned long)p_fifo->w + len) % p_fifo->size);
    }
    else
    {
        ret = amlbt_intf_write_sram(data, (unsigned int)((unsigned long)p_fifo->w + (unsigned long)p_fifo->base_addr),
            p_fifo->size - offset);
        if (ret != 0)
        {
            BTE("%s:%d Failed : %d len %d\n", __func__, __LINE__, ret, len);
            return -1;
        }
        if ((len - (p_fifo->size - offset)) != 0)
        {
            ret = amlbt_intf_write_sram(&data[p_fifo->size - offset],
                (unsigned int)(unsigned long)p_fifo->base_addr, (len - (p_fifo->size - offset)));
            if (ret != 0)
            {
                BTE("%s:%d Failed : %d len %d\n", __func__, __LINE__, ret, len);
                return -1;
            }
        }
        p_fifo->w = (unsigned char *)((len - (p_fifo->size - offset)) % p_fifo->size);
    }

    return len;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0)

#ifndef __poll_t_defined
    typedef unsigned int __poll_t;
#define __poll_t_defined
#endif

void *skb_put_data(struct sk_buff *skb, const void *data, unsigned int len)
{
    void *tmp = skb_put(skb, len);
    if (tmp)
    {
        memcpy(tmp, data, len);
    }
    return tmp;
}

#endif

