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
#include "intf_pcie.h"
#include "rc_list.h"
#include "debug_dev.h"
#include "driver.h"
#include "chip.h"

void amlbt_intf_pcie_write_word(unsigned int addr, unsigned int data)
{
#ifdef CONFIG_AML_BT_CHIP_W2
    int base = AML_ADDR_AON;

    if (g_aml_plat_pci == NULL)
    {
        BTE("%s:g_aml_plat_pci is NULL, aborted\n", __func__);
        return;
    }
    if (g_aml_plat_pci->pci_dev == NULL)
    {
        BTE("%s:g_aml_plat_pci->dev is NULL, aborted\n", __func__);
        return;
    }

    if (addr == REG_FW_PC)
    {
        base = AML_ADDR_CPU;
    }
    else if (addr >= W2_ICCM_AHB_BASE_ADDR && addr < W2_DCCM_AHB_BASE_ADDR + W2_DCCM_SIZE)
    {
        base = AML_ADDR_CPU;
    }
    else if (addr >= FIFO_FW_RC_LIST_ADDR && addr <= MAC_ADDR_LEN*MAX_MAC_LIST)
    {
        base = AML_ADDR_CPU;
    }
    aml_pci_write_for_bt(data, base, addr);
#else
    BTE("%s error, not w2 !\n", __func__);
#endif
}

unsigned int amlbt_intf_pcie_read_word(unsigned int addr)
{
#ifdef CONFIG_AML_BT_CHIP_W2
    int base = AML_ADDR_AON;

    if (g_aml_plat_pci == NULL)
    {
        BTE("%s:g_aml_plat_pci is NULL, aborted\n", __func__);
        return 0xdead;
    }
    if (g_aml_plat_pci->pci_dev == NULL)
    {
        BTE("%s:g_aml_plat_pci->dev is NULL, aborted\n", __func__);
        return 0xdead;
    }

    if (addr == REG_FW_PC)
    {
        base = AML_ADDR_CPU;
    }
    else if (addr >= W2_ICCM_AHB_BASE_ADDR && addr < W2_DCCM_AHB_BASE_ADDR + W2_DCCM_SIZE)
    {
        base = AML_ADDR_CPU;
    }
    else if (addr >= FIFO_FW_RC_LIST_ADDR && addr <= MAC_ADDR_LEN*MAX_MAC_LIST)
    {
        base = AML_ADDR_CPU;
    }
    return aml_pci_read_for_bt(base, addr);
#else
    BTE("%s error, not w2 !\n", __func__);
    return 0xdead;
#endif
}

void amlbt_intf_pcie_register(void)
{
#ifdef CONFIG_AML_BT_CHIP_W2
    unsigned int alive = atomic_read(&g_wifi_pm.wifi_enable);

    if (alive)
    {
        BTI("wifi alive %#x\n", alive);
    }
    else if (!g_pci_driver_insmoded)
    {
        BTI("wifi not alive %#x\n", g_pci_driver_insmoded);
        aml_pci_insmod();
        usleep_range(100000, 100000);
    }
#else
    BTE("%s error, not w2 !\n", __func__);
#endif
}

void amlbt_intf_pcie_unregister(void)
{
#ifdef CONFIG_AML_BT_CHIP_W2
    unsigned int alive = atomic_read(&g_wifi_pm.wifi_enable);

    if (alive)
    {
        BTI("wifi alive %#x\n", alive);
    }
    else if (g_pci_driver_insmoded)
    {
        BTI("wifi not alive %#x\n", g_pci_driver_insmoded);
        aml_pci_rmmod();
    }
#else
    BTE("%s error, not w2 !\n", __func__);
#endif
}

