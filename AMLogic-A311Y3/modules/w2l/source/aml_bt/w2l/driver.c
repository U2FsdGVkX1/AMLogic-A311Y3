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
#include <linux/mmc/sdio_func.h>
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

#include "common.h"
#include "intf.h"
#include "intf_sdio.h"
#include "intf_uart.h"
#include "intf_usb.h"
#include "driver.h"
#include "debug_dev.h"
#include "rc_list.h"
#include "chip.h"

static void amlbt_driver_release(struct device *dev)
{
    return;
}

static int amlbt_driver_probe(struct platform_device *dev)
{
    BTI("%s\n", __func__);

    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
static int amlbt_driver_remove(struct platform_device *dev)
#else
static void amlbt_driver_remove(struct platform_device *dev)
#endif
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    BTI("%s \n", __func__);
    amlbt_intf_remove(p_bt);
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
    return 0;
#endif
}

static int amlbt_driver_suspend(struct platform_device *dev, pm_message_t state)
{
    int ret = 0;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    ret = amlbt_intf_suspend(p_bt);
    BTI("%s end, ret=%d\n", __func__, ret);
    return ret;
}

static int amlbt_driver_resume(struct platform_device *dev)
{
    int ret = 0;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    ret = amlbt_intf_resume(p_bt);
    BTI("%s end, ret=%d\n", __func__, ret);
    return ret;
}

static void amlbt_driver_shutdown(struct platform_device *dev)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    BTI("%s \n", __func__);
    amlbt_intf_shutdown(p_bt);
}

struct platform_device amlbt_device =
{
    .name    = "amlbt_dev",
    .id      = -1,
    .dev     = {
        .release = &amlbt_driver_release,
    }
};

struct platform_driver amlbt_driver =
{
    .probe = amlbt_driver_probe,
    .remove = amlbt_driver_remove,
    .suspend = amlbt_driver_suspend,
    .resume = amlbt_driver_resume,
    .shutdown = amlbt_driver_shutdown,

    .driver = {
        .name = "amlbt_dev",
        .owner = THIS_MODULE,
    },
};

static int amlbt_driver_init(void)
{
    int ret = 0;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    struct platform_device *p_device = &amlbt_device;
    struct platform_driver *p_driver = &amlbt_driver;

    BTI("%s \n", __func__);

    ret = amlbt_chip_id_init();
    if (ret)
    {
        BTE("chip id invalid!\n");
        return ret;
    }
    amlbt_intf_version();

    ret = amlbt_intf_register();
    if (ret)
    {
        dev_err(&p_device->dev, "intf register failed!\n");
        return ret;
    }

    ret = platform_driver_register(p_driver);
    if (ret)
    {
        dev_err(&p_device->dev, "platform_driver_register failed!\n");
        return ret;
    }

    ret = platform_device_register(p_device);
    if (ret)
    {
        dev_err(&p_device->dev, "platform_device_register failed!\n");
        platform_driver_unregister(p_driver);
        return ret;
    }

    amlbt_intf_create_device(p_bt);
    amlbt_intf_register_early_suspend(p_bt, p_device);
    amlbt_intf_input_device_init(p_device);
    register_bt_event_notifier(&bt_nb);
    amlbt_rc_list_init(p_bt->drv_res.dev_device[0]);
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
    amlbt_intf_ops_bind_bus(p_bt);
#endif
    if (amlbt_intf_rw_get() == INTF_USB)
    {
        g_cmd_buf = kzalloc(sizeof(*g_cmd_buf), GFP_DMA | GFP_ATOMIC);
        if (!g_cmd_buf)
        {
            BTE("%s:%d g_cmd_buf kzalloc failed!\n", __func__, __LINE__);
            return -EINVAL;
        }
    }
    amlbt_hci_dev_init(p_device);
    amlbt_debug_dev_init();
    return ret;
}

static void amlbt_driver_exit(void)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    struct platform_device *p_device = &amlbt_device;
    struct platform_driver *p_driver = &amlbt_driver;

    BTI("%s, log level:%d \n", __func__, g_dbg_level);

    amlbt_debug_dev_deinit(p_bt);
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
    amlbt_intf_ops_unbind_bus(p_bt);
#endif
    amlbt_rc_list_deinit(p_bt->drv_res.dev_device[0]);
    unregister_bt_event_notifier(&bt_nb);
    input_unregister_device(p_bt->pm_res.input_dev);
    p_bt->pm_res.input_dev = NULL;
    amlbt_intf_unregister_early_suspend(p_bt, p_device);
    amlbt_intf_destroy_device(p_bt);
    amlbt_intf_uart_unregister();
    platform_device_unregister(p_device);
    platform_driver_unregister(p_driver);
    if (amlbt_intf_rw_get() == INTF_USB)
    {
        if (g_cmd_buf != NULL)
        {
            kfree(g_cmd_buf);
            g_cmd_buf = NULL;
        }
    }
    amlbt_hci_dev_deinit();
}

module_param(amlbt_if_type, uint, S_IRUGO);
module_param(polling_time, uint, S_IRUGO|S_IWUSR);
module_param(amlbt_ft_mode, uint, S_IRUGO);

module_init(amlbt_driver_init);
module_exit(amlbt_driver_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(STR(AML_BT_DRIVER_VERSION));
MODULE_DESCRIPTION(VERSION_INFO);
MODULE_VERSION("1.0.0");

