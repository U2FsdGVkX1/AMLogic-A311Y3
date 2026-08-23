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
#include <linux/interrupt.h>
#include <linux/pm_wakeup.h>
#include <linux/pm_wakeirq.h>
#include <linux/amlogic/pm.h>
#include <linux/completion.h>
#include <linux/mmc/sdio_func.h>
#include <linux/amlogic/wifi_dt.h>

#include <linux/input.h>
#include <linux/jiffies.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/version.h>
#include <linux/tty.h>
#include <linux/skbuff.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
#include <linux/unaligned/packed_struct.h>
#else
#include <asm/unaligned.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
#include <linux/amlogic/aml_sd.h>   /* for sdio_reinit() */
#else
void sdio_reinit(void);
#endif

#include "common.h"
#include "intf.h"
#include "intf_sdio.h"
#include "intf_uart.h"
#include "rc_list.h"
#include "debug_dev.h"
#include "driver.h"

static int amlbt_intf_sdio_check(void)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    if (g_hif_sdio_ops.bt_hi_write_word == NULL || g_hif_sdio_ops.bt_hi_read_word == NULL || \
        g_hif_sdio_ops.hi_random_ram_read == NULL || g_hif_sdio_ops.hi_random_ram_write == NULL)
    {
        BTE("g_hif_sdio_ops interface NULL");
        return -1;
    }

    if (bus_state_detect.bus_err || bus_state_detect.bus_reset_ongoing || \
            bus_state_detect.is_recy_ongoing || p_bt->excp_res.notify_recy)
    {
        if (!(p_bt->pm_res.dr_state & BT_DRV_STATE_WAIT_RECOVERY))
        {
            amlbt_intf_drv_state_set(BT_DRV_STATE_WAIT_RECOVERY);
        }
        BTE("bus_err %d reset %d recy %d notify %d", bus_state_detect.bus_err, bus_state_detect.bus_reset_ongoing,
                    bus_state_detect.is_recy_ongoing, p_bt->excp_res.notify_recy);
        return -1;
    }
    return 0;
}

void amlbt_intf_sdio_write_word(unsigned int addr, unsigned int data)
{
    int ret;

    ret = amlbt_intf_sdio_check();
    if (ret != 0)
    {
        BTE("%s:%d, error!\n", __func__, __LINE__);
        return ;
    }

    g_hif_sdio_ops.bt_hi_write_word(addr, data);
}

unsigned int amlbt_intf_sdio_read_word(unsigned int addr)
{
    unsigned int value = 0;
    int ret;

    ret = amlbt_intf_sdio_check();
    if (ret != 0)
    {
        BTE("%s:%d, error!\n", __func__, __LINE__);
        return 0;
    }

    value = g_hif_sdio_ops.bt_hi_read_word(addr);
    return value;
}

void amlbt_intf_sdio_read_sram(unsigned char* buf, unsigned char* addr, unsigned int len)
{
    int ret;

    ret = amlbt_intf_sdio_check();
    if (ret != 0)
    {
        BTE("%s:%d, error!\n", __func__, __LINE__);
        return ;
    }

    g_hif_sdio_ops.hi_random_ram_read(buf, addr, len);
}

void amlbt_intf_sdio_write_sram(unsigned char* buf, unsigned char* addr, unsigned int len)
{
    int ret;

    ret = amlbt_intf_sdio_check();
    if (ret != 0)
    {
        BTE("%s:%d, error!\n", __func__, __LINE__);
        return ;
    }

    g_hif_sdio_ops.hi_random_ram_write(buf, addr, len);
}

void amlbt_intf_sdio_register(void)
{
#if defined(CONFIG_AML_BT_CHIP_W2L)
    unsigned int alive = atomic_read(&g_wifi_pm.wifi_enable);

    if (alive)
    {
        BTI("wifi alive %#x\n", alive);
    }
    else
    {
      BTI("wifi not alive, g_sdio_driver_insmoded:%d\n", g_sdio_driver_insmoded);
      if (!g_sdio_driver_insmoded)
      {
          extern_wifi_set_enable(1);
          sdio_reinit();
          aml_sdio_init();
          g_sdio_driver_insmoded = 1;
      }
    }
#elif defined(CONFIG_AML_BT_CHIP_W1D)
    unsigned int alive = g_sdio_wifi_bt_alive & BIT(1);

    if (alive)
    {
        BTI("wifi alive\n");
    }
    else if (!g_sdio_driver_insmoded)
    {
        BTI("wifi not alive\n");
        aml_sdio_init();
        usleep_range(150000, 150000);
    }
#endif
}

void amlbt_intf_sdio_unregister(void)
{
#if defined(CONFIG_AML_BT_CHIP_W2L)
    unsigned int alive = atomic_read(&g_wifi_pm.wifi_enable);

    if (alive)
    {
        BTI("wifi alive %#x\n", alive);
    }
    else
    {
        BTI("wifi not alive, g_sdio_driver_insmoded:%d\n", g_sdio_driver_insmoded);
        if (g_sdio_driver_insmoded)
        {
            aml_bus_state_detect_deinit();
            BTI("remove wifi sdio device\n");
            aml_sdio_exit();
            extern_wifi_set_enable(0);
            g_sdio_driver_insmoded = 0;
        }
    }
#elif defined(CONFIG_AML_BT_CHIP_W1D)
    unsigned int alive = g_sdio_wifi_bt_alive & BIT(1);

    if (alive)
    {
        BTI("wifi alive\n");
    }
    else if (g_sdio_driver_insmoded)
    {
        BTI("wifi not alive\n");
        //BTI("aml_bus_state_detect_deinit\n");
        //aml_bus_state_detect_deinit();
        //amlbt_aon_addr_bit_set(RG_AON_A56, 31); // shutdown bit
        BTI("remove wifi sdio device\n");
        aml_sdio_exit();
    }
#endif
}

