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
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0))
#include <linux/panic_notifier.h>
#endif

#include "common.h"
#include "intf.h"
#include "debug_dev.h"
#include "driver.h"

static debug_dev_t debug_dev = {0};

static ssize_t recy_dbg_read(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t recy_dbg_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static struct device_attribute recy_attr_dbg = {
    .attr = { .name = AML_BT_CHAR_RECYDBG_NAME, .mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH },
    .show = recy_dbg_read,
    .store = recy_dbg_write,
};

static ssize_t recy_dbg_read(struct device *dev, struct device_attribute *attr, char *buf)
{
    debug_dev_t *d_bt = &debug_dev;
    return scnprintf(buf, PAGE_SIZE, "%s\n", d_bt->recy_dbg_buf);
}

static ssize_t recy_dbg_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    debug_dev_t *d_bt = &debug_dev;

    BTI("recy_dbg_write count: %zu\n", count);

    if (count > sizeof(d_bt->recy_dbg_buf) - 1)
    {
        count = sizeof(d_bt->recy_dbg_buf) - 1;
    }
    memset(d_bt->recy_dbg_buf, 0, sizeof(d_bt->recy_dbg_buf));
    memcpy(d_bt->recy_dbg_buf, buf, count);
    d_bt->recy_dbg_buf[count] = '\0';

    if (strncmp(buf, "over", 4) == 0 || strncmp(buf, "over\n", 5) == 0)
    {
        BTI("%s bt recovery!\n", __func__);
        p_bt->pm_res.dr_state = BT_DRV_STATE_WAIT_RECOVERY;
        amlbt_intf_queue_work(p_bt->excp_res.exception_work_wq, &p_bt->excp_res.exception_work);
    }

    return count;
}

static int amlbt_recy_dbg_init(void)
{
    int res = 0;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    res = device_create_file(p_bt->drv_res.dev_device[0], &recy_attr_dbg);
    if (res)
    {
        BTE("%s:Failed to create device attribute\n", __func__);
    }
    return res;
}

static ssize_t amlbt_debug_level_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    char temp[10];
    int len = snprintf(temp, sizeof(temp), "%d\n", g_dbg_level);
    return simple_read_from_buffer(buf, count, ppos, temp, len);
}

static ssize_t amlbt_debug_level_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char temp[10];
    int ret = 0, val = 0;

    if (count > sizeof(temp) - 1)
        return -EINVAL;

    if (copy_from_user(temp, buf, count))
        return -EFAULT;

    temp[count] = '\0';

    ret = kstrtoint(temp, 10, &val);
    if (ret)
    {
        pr_err("Invalid input for debug_level\n");
        return ret;
    }

    g_dbg_level = val;
    pr_info("Debug level set to %d\n", g_dbg_level);

    return count;
}

static const struct file_operations debug_level_fops =
{
    .read = amlbt_debug_level_read,
    .write = amlbt_debug_level_write,
};

static int amlbt_debug_level_init(void)
{
    debug_dev_t *d_bt = &debug_dev;

    d_bt->debug_dir = debugfs_create_dir("aml_btz", NULL);
    if (d_bt->debug_dir == NULL)
        return -ENOMEM;

    debugfs_create_file("aml_btz_dbg_lvl", 0644, d_bt->debug_dir, NULL, &debug_level_fops);
    return 0;
}

static void amlbt_debug_level_deinit(void)
{
    debug_dev_t *d_bt = &debug_dev;

    if (d_bt->debug_dir != NULL)
    {
        debugfs_remove_recursive(d_bt->debug_dir);
        d_bt->debug_dir = NULL;
    }
}

static int amlbt_panic_callback(struct notifier_block *nb, unsigned long event, void *arg)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    BTF("kernel panic \n", __func__);

    if (amlbt_intf_rw_get() == INTF_USB)
    {
        BTF("usb_rx_buf:%#x\n", (unsigned long)p_bt->usb_res.usb_rx_buf);
        BTF("dr_state: %#x\n", p_bt->pm_res.dr_state);
        if (p_bt->usb_res.fw_type_fifo != NULL)
        {
            BTF("fw_type_fifo->w:%#x fw_type_fifo->r:%#x\n", p_bt->usb_res.fw_type_fifo->w, p_bt->usb_res.fw_type_fifo->r);
        }
        if (p_bt->usb_res.fw_evt_fifo)
        {
            BTF("fw_evt_fifo->w:%#x fw_evt_fifo->r:%#x\n", p_bt->usb_res.fw_evt_fifo->w, p_bt->usb_res.fw_evt_fifo->r);
        }
        if (p_bt->usb_res.fw_data_fifo)
        {
            BTF("fw_data_fifo->w:%#x fw_data_fifo->r:%#x\n", p_bt->usb_res.fw_data_fifo->w, p_bt->usb_res.fw_data_fifo->r);
        }
    }
    return NOTIFY_DONE;
}

static struct notifier_block aml_panic_notifier = {
    .notifier_call = amlbt_panic_callback
};

int aml_register_panic_notifier(void)
{
    atomic_notifier_chain_register(&panic_notifier_list, &aml_panic_notifier);

    return 0;
}

void aml_unregister_panic_notifier(void)
{
    atomic_notifier_chain_unregister(&panic_notifier_list, &aml_panic_notifier);
}

int amlbt_debug_dev_init(void)
{
    int res = 0;

    aml_register_panic_notifier();

    res = amlbt_debug_level_init();
    if (res != 0)
    {
        BTE("%s:Failed to create debugfs_create_dir\n", __func__);
    }
    res = amlbt_recy_dbg_init();
    if (res)
    {
        BTE("%s:Failed to create recy debug device attribute\n", __func__);
    }
    return res;
}

void amlbt_debug_dev_deinit(amlbt_t *p_bt)
{
    device_remove_file(p_bt->drv_res.dev_device[0], &recy_attr_dbg);
    amlbt_debug_level_deinit();
    aml_unregister_panic_notifier();
}


