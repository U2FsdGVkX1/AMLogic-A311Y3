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
#include <linux/mmc/sdio_func.h>
#include <linux/firmware.h>
#include <linux/jiffies.h>
#include <linux/wait.h>
#include <linux/input.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/version.h>
#include <linux/timekeeping.h>
#include <linux/rtc.h>

#include "common.h"
#include "intf.h"
#include "intf_sdio.h"
#include "intf_uart.h"
#include "intf_usb.h"
#include "debug_dev.h"
#include "driver.h"
#include "rc_list.h"
#include "chip.h"

static unsigned char *chip_family[] = {"unknown", "w1", "w1u", "w2", "w2l", "w3", "w1d"};
static unsigned char *chip_rev[] = {"Rev_A","Rev_B","Rev_C","Rev_D","Rev_E","Rev_F","Rev_G","Rev_H"};
static unsigned char *chip_intf[] = {"sdio", "usb_2.0", "pcie_2.0", "usb_3.0", "pcie_4.0"};

static void amlbt_chip_info_init(amlbt_t *p_bt)
{
    if (p_bt->diag_res.chip_family_id == CHIP_W2) //w2
    {
        p_bt->fw_res.iccm_size = W2_ICCM_SIZE;
        p_bt->fw_res.dccm_size = W2_DCCM_SIZE;
        p_bt->fw_res.rom_size = W2_ROM_SIZE;
        p_bt->fw_res.iccm_ahb_base = W2_ICCM_AHB_BASE_ADDR;
        p_bt->fw_res.dccm_ahb_base = W2_DCCM_AHB_BASE_ADDR;
        p_bt->fw_res.iccm_ram_base = W2_ICCM_RAM_BASE_ADDR;    //uart download used
        p_bt->fw_res.dccm_ram_base = W2_DCCM_RAM_BASE_ADDR;    //uart download used
        if (p_bt->diag_res.chip_intf_id == CHIP_INTF_PCIE2 || p_bt->diag_res.chip_intf_id == CHIP_INTF_PCIE3)
        {
            p_bt->fw_res.download_size = W2_PCIE_DOWNLOAD_SIZE;
        }
        else
        {
            p_bt->fw_res.download_size = W2_DOWNLOAD_SIZE;
        }
        p_bt->fw_res.poll_len = W2_USB_POLL_LEN;
        p_bt->fw_res.poll_addr = W2_USB_MEM4_ADDR;
        p_bt->fw_res.rx_q_addr = W2_USB_MEM1_ADDR;
        p_bt->fw_res.rx_q_len = W2_USB_RX_Q_LEN;
        p_bt->fw_res.rx_q_r = W2_USB_RX_Q_R_POINT;
        p_bt->fw_res.rx_q_w = W2_USB_RX_Q_W_POINT;
        p_bt->fw_res.rx_type_addr = W2_USB_RX_TYPE_Q_ADDR;
        p_bt->fw_res.rx_type_len = USB_RX_TYPE_FIFO_LEN;
        p_bt->fw_res.rx_type_r = W2_USB_RX_TYPE_Q_R_POINT;
        p_bt->fw_res.rx_type_w = W2_USB_RX_TYPE_Q_W_POINT;
        p_bt->fw_res.evt_addr = W2_USB_EVT_Q_ADDR;
        p_bt->fw_res.evt_len = W2_USB_EVT_FIFO_LEN;
        p_bt->fw_res.evt_r = W2_USB_EVT_Q_R_POINT;
        p_bt->fw_res.evt_w = W2_USB_EVT_Q_W_POINT;
        p_bt->fw_res.cmd_addr = W2_USB_CMD_Q_ADDR;
        p_bt->fw_res.cmd_len = W2_USB_CMD_FIFO_LEN;
        p_bt->fw_res.cmd_r = W2_USB_CMD_Q_R_POINT;
        p_bt->fw_res.cmd_w = W2_USB_CMD_Q_W_POINT;
        p_bt->fw_res.tx_q_addr = W2L_USB_MEM2_ADDR;
        p_bt->fw_res.tx_q_prio_addr = W2L_USB_TX_Q_PRIO_ADDR;
        p_bt->fw_res.driver_fw_status_reg = W2L_USB_DRIVER_FW_STATUS;
    }
    else if (p_bt->diag_res.chip_family_id == CHIP_W2L) //w2l
    {
        p_bt->fw_res.iccm_size = W2L_ICCM_SIZE;
        p_bt->fw_res.dccm_size = W2L_DCCM_SIZE;
        p_bt->fw_res.rom_size = W2L_ROM_SIZE;
        p_bt->fw_res.iccm_ahb_base = W2L_ICCM_AHB_BASE_ADDR;
        p_bt->fw_res.dccm_ahb_base = W2L_DCCM_AHB_BASE_ADDR;
        p_bt->fw_res.iccm_ram_base = W2L_ICCM_RAM_BASE_ADDR;    //uart download used
        p_bt->fw_res.dccm_ram_base = W2L_DCCM_RAM_BASE_ADDR;    //uart download used
        p_bt->fw_res.download_size = W2L_DOWNLOAD_SIZE;
        p_bt->fw_res.poll_len = W2L_USB_POLL_LEN;
        p_bt->fw_res.poll_addr = W2L_USB_MEM4_ADDR;
        p_bt->fw_res.rx_q_addr = W2L_USB_MEM1_ADDR;
        p_bt->fw_res.rx_q_len = W2L_USB_RX_Q_LEN;
        p_bt->fw_res.rx_q_r = W2L_USB_RX_Q_R_POINT;
        p_bt->fw_res.rx_q_w = W2L_USB_RX_Q_W_POINT;
        p_bt->fw_res.rx_type_addr = W2L_USB_RX_TYPE_Q_ADDR;
        p_bt->fw_res.rx_type_len = USB_RX_TYPE_FIFO_LEN;
        p_bt->fw_res.rx_type_r = W2L_USB_RX_TYPE_Q_R_POINT;
        p_bt->fw_res.rx_type_w = W2L_USB_RX_TYPE_Q_W_POINT;
        p_bt->fw_res.evt_addr = W2L_USB_EVT_Q_ADDR;
        p_bt->fw_res.evt_len = W2L_USB_EVT_FIFO_LEN;
        p_bt->fw_res.evt_r = W2L_USB_EVT_Q_R_POINT;
        p_bt->fw_res.evt_w = W2L_USB_EVT_Q_W_POINT;
        p_bt->fw_res.cmd_addr = W2L_USB_CMD_Q_ADDR;
        p_bt->fw_res.cmd_len = W2L_USB_CMD_FIFO_LEN;
        p_bt->fw_res.cmd_r = W2L_USB_CMD_Q_R_POINT;
        p_bt->fw_res.cmd_w = W2L_USB_CMD_Q_W_POINT;
        p_bt->fw_res._15p4_rx_addr = W2L_USB_15P4_RX_Q_ADDR;
        p_bt->fw_res._15p4_rx_len = W2L_USB_15P4_RX_FIFO_LEN;
        p_bt->fw_res._15p4_rx_r = W2L_USB_15P4_RX_Q_R_POINT;
        p_bt->fw_res._15p4_rx_w = W2L_USB_15P4_RX_Q_W_POINT;
        p_bt->fw_res._15p4_tx_addr = W2L_USB_15P4_TX_Q_ADDR;
        p_bt->fw_res._15p4_tx_len = W2L_USB_15P4_TX_FIFO_LEN;
        p_bt->fw_res._15p4_tx_r = W2L_USB_15P4_TX_Q_R_POINT;
        p_bt->fw_res._15p4_tx_w = W2L_USB_15P4_TX_Q_W_POINT;
        p_bt->fw_res.tx_q_addr = W2L_USB_MEM2_ADDR;
        p_bt->fw_res.tx_q_prio_addr = W2L_USB_TX_Q_PRIO_ADDR;
        p_bt->fw_res.driver_fw_status_reg = W2L_USB_DRIVER_FW_STATUS;
    }
    else if (p_bt->diag_res.chip_family_id == CHIP_W1D) //W1D
    {
        p_bt->fw_res.rom_size = W1D_ROM_SIZE;
        p_bt->fw_res.iccm_size = W1D_ICCM_SIZE;
        p_bt->fw_res.dccm_size = W1D_DCCM_SIZE;
        p_bt->fw_res.add_size = W1D_ADD_SIZE;
        p_bt->fw_res.iccm_ram_base = W1D_ICCM_RAM_BASE_ADDR;
        p_bt->fw_res.dccm_ram_base = W1D_DCCM_RAM_BASE_ADDR;
        p_bt->fw_res.add_ram_base = W1D_ADD_RAM_BASE_ADDR;
        p_bt->fw_res.download_size = W1D_DOWNLOAD_SIZE;
        p_bt->fw_res.poll_len = W1D_USB_POLL_LEN;
        p_bt->fw_res.poll_addr = W1D_USB_MEM4_ADDR;
        p_bt->fw_res.rx_q_addr = W1D_USB_MEM1_ADDR;
        p_bt->fw_res.rx_q_len = W1D_USB_RX_Q_LEN;
        p_bt->fw_res.rx_q_r = W1D_USB_RX_Q_R_POINT;
        p_bt->fw_res.rx_q_w = W1D_USB_RX_Q_W_POINT;
        p_bt->fw_res.rx_type_addr = W1D_USB_RX_TYPE_Q_ADDR;
        p_bt->fw_res.rx_type_len = USB_RX_TYPE_FIFO_LEN;
        p_bt->fw_res.rx_type_r = W1D_USB_RX_TYPE_Q_R_POINT;
        p_bt->fw_res.rx_type_w = W1D_USB_RX_TYPE_Q_W_POINT;
        p_bt->fw_res.evt_addr = W1D_USB_EVT_Q_ADDR;
        p_bt->fw_res.evt_len = W1D_USB_EVT_FIFO_LEN;
        p_bt->fw_res.evt_r = W1D_USB_EVT_Q_R_POINT;
        p_bt->fw_res.evt_w = W1D_USB_EVT_Q_W_POINT;
        p_bt->fw_res.cmd_addr = W1D_USB_CMD_Q_ADDR;
        p_bt->fw_res.cmd_len = W1D_USB_CMD_FIFO_LEN;
        p_bt->fw_res.cmd_r = W1D_USB_CMD_Q_R_POINT;
        p_bt->fw_res.cmd_w = W1D_USB_CMD_Q_W_POINT;
        p_bt->fw_res._15p4_rx_addr = W1D_USB_15P4_RX_Q_ADDR;
        p_bt->fw_res._15p4_rx_len = W1D_USB_15P4_RX_FIFO_LEN;
        p_bt->fw_res._15p4_rx_r = W1D_USB_15P4_RX_Q_R_POINT;
        p_bt->fw_res._15p4_rx_w = W1D_USB_15P4_RX_Q_W_POINT;
        p_bt->fw_res._15p4_tx_addr = W1D_USB_15P4_TX_Q_ADDR;
        p_bt->fw_res._15p4_tx_len = W1D_USB_15P4_TX_FIFO_LEN;
        p_bt->fw_res._15p4_tx_r = W1D_USB_15P4_TX_Q_R_POINT;
        p_bt->fw_res._15p4_tx_w = W1D_USB_15P4_TX_Q_W_POINT;
        p_bt->fw_res.tx_q_addr = W1D_USB_MEM2_ADDR;
        p_bt->fw_res.tx_q_prio_addr = W1D_USB_TX_Q_PRIO_ADDR;
        p_bt->fw_res.driver_fw_status_reg = W1D_USB_DRIVER_FW_STATUS;
    }
}

int amlbt_chip_id_init(void)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    p_bt->bt_intf = BT_INTF_UNKNOWN;
    p_bt->rw_intf = INTF_UNKNOWN;
    p_bt->diag_res.chip_family_id = INTF_CHIP_FAMILY_ID(amlbt_if_type);
    p_bt->diag_res.chip_rev_id = INTF_CHIP_FAMILY_REV_ID(amlbt_if_type);
    //unsigned int chip_wireless_config = INTF_WIRELESS_CONFIG(amlbt_if_type);
    p_bt->diag_res.chip_intf_id = INTF_CHIP_INTF_ID(amlbt_if_type);

    BTI("chip_family_id:%d\n", p_bt->diag_res.chip_family_id);
    BTI("chip_rev_id:%d\n", p_bt->diag_res.chip_rev_id);
    //BTI("chip_wireless_config:%d\n", chip_wireless_config);
    BTI("chip_intf_id:%d\n", p_bt->diag_res.chip_intf_id);

    if (p_bt->diag_res.chip_family_id >= sizeof(chip_family)/sizeof(chip_family[0]))
    {
        BTE("chip_family_id invalid!\n");
        return -1;
    }
    if (p_bt->diag_res.chip_rev_id >= sizeof(chip_rev)/sizeof(chip_rev[0]))
    {
        BTE("chip_rev_id invalid!\n");
        return -1;
    }
    if (p_bt->diag_res.chip_intf_id >= sizeof(chip_intf)/sizeof(chip_intf[0]))
    {
        BTE("chip_intf_id invalid!\n");
        return -1;
    }

    if (p_bt->diag_res.chip_intf_id == CHIP_INTF_USB2 || p_bt->diag_res.chip_intf_id == CHIP_INTF_USB3)   //usb
    {
        p_bt->rw_intf = INTF_USB;
        p_bt->bt_intf = BT_INTF_DRIVER_USB;
    }
    else if (p_bt->diag_res.chip_intf_id == CHIP_INTF_SDIO)  //sdio
    {
        if (p_bt->diag_res.chip_family_id == CHIP_W2) //w2
        {
            if (p_bt->diag_res.chip_rev_id >= CHIP_REVC) //revC or later
            {
                p_bt->rw_intf = INTF_UART;
                p_bt->bt_intf = BT_INTF_DRIVER_TTY;
            }
            else
            {
                p_bt->rw_intf = INTF_SDIO;
                p_bt->bt_intf = BT_INTF_KERNEL_TTY;
            }
        }
        else if (p_bt->diag_res.chip_family_id == CHIP_W2L)    //w2l
        {
            if (p_bt->diag_res.chip_rev_id >= CHIP_REVC) //revC or later
            {
                p_bt->rw_intf = INTF_UART;
            }
            else
            {
                p_bt->rw_intf = INTF_SDIO;
            }
            p_bt->bt_intf = BT_INTF_DRIVER_TTY;
        }
        else if (p_bt->diag_res.chip_family_id == CHIP_W1D)    //w1d
        {
            p_bt->rw_intf = INTF_UART;
            p_bt->bt_intf = BT_INTF_DRIVER_TTY;
        }
    }
    else if (p_bt->diag_res.chip_intf_id == CHIP_INTF_PCIE2 || p_bt->diag_res.chip_intf_id == CHIP_INTF_PCIE3)  //pcie
    {
        if (p_bt->diag_res.chip_family_id == CHIP_W2) //w2
        {
            if (p_bt->diag_res.chip_rev_id >= CHIP_REVC) //revC or later
            {
                p_bt->rw_intf = INTF_UART;
                p_bt->bt_intf = BT_INTF_DRIVER_TTY;
            }
            else
            {
                p_bt->rw_intf = INTF_PCIE;
                p_bt->bt_intf = BT_INTF_KERNEL_TTY;
            }
        }
    }

    BTI("chip info:%s-%s-%s, bt_intf:%d, rw_intf:%d\n",
        chip_family[p_bt->diag_res.chip_family_id], chip_intf[p_bt->diag_res.chip_intf_id],
        chip_rev[p_bt->diag_res.chip_rev_id], p_bt->bt_intf, p_bt->rw_intf);
    if (p_bt->bt_intf == BT_INTF_UNKNOWN || p_bt->rw_intf == INTF_UNKNOWN)
    {
        BTE("%s:%d interface error!, amlbt_if_type:%#x\n", __func__, __LINE__, amlbt_if_type);
        return -1;
    }
    amlbt_chip_info_init(p_bt);
    return 0;
}

void amlbt_chip_info_print(amlbt_t *p_bt)
{
    BTI("bt driver version :%#x\n", AML_BT_DRIVER_VERSION);
    BTI("bt driver version info:%s\n",  VERSION_INFO);
    BTI("chip info:%s-%s-%s, bt_intf:%d, rw_intf:%d\n",
        chip_family[p_bt->diag_res.chip_family_id], chip_intf[p_bt->diag_res.chip_intf_id],
        chip_rev[p_bt->diag_res.chip_rev_id], p_bt->bt_intf, p_bt->rw_intf);
}

const char* amlbt_chip_get_firmware_name(amlbt_t *p_bt)
{
    if (p_bt->diag_res.chip_intf_id == CHIP_INTF_SDIO ||
        p_bt->diag_res.chip_intf_id == CHIP_INTF_PCIE2 ||
        p_bt->diag_res.chip_intf_id == CHIP_INTF_PCIE3)
    {
        if (p_bt->diag_res.chip_family_id == CHIP_W2)
        {
            if (p_bt->diag_res.chip_rev_id == CHIP_REVA || p_bt->diag_res.chip_rev_id == CHIP_REVB)
            {
                return "aml/w2_bt_fw_uart.bin";
            }
            else if (p_bt->diag_res.chip_rev_id == CHIP_REVC)
            {
                return "aml/w2_bt_revC_fw_uart.bin";
            }
        }
        else if (p_bt->diag_res.chip_family_id == CHIP_W2L)
        {
            return "aml/w2l_bt_15p4_fw_uart.bin";
        }
        else if (p_bt->diag_res.chip_family_id == CHIP_W1D)
        {
            return "aml/w1d_bt_15p4_fw_uart.bin";
        }
    }
    else if (p_bt->diag_res.chip_intf_id == CHIP_INTF_USB2 || p_bt->diag_res.chip_intf_id == CHIP_INTF_USB3)
    {
        if (p_bt->diag_res.chip_family_id == CHIP_W2)
        {
            if (p_bt->diag_res.chip_rev_id == CHIP_REVA || p_bt->diag_res.chip_rev_id == CHIP_REVB)
            {
                return "aml/w2_bt_fw_usb.bin";
            }
            else if (p_bt->diag_res.chip_rev_id == CHIP_REVC)
            {
                return "aml/w2_bt_revC_fw_usb.bin";
            }
        }
        else if (p_bt->diag_res.chip_family_id == CHIP_W2L)
        {
            return "aml/w2l_bt_15p4_fw_usb.bin";
        }
        else if (p_bt->diag_res.chip_family_id == CHIP_W1D)
        {
            return "aml/w1d_bt_15p4_fw_usb.bin";
        }
    }
    BTF("%s:%d interface error!, amlbt_if_type:%#x\n", __func__, __LINE__, amlbt_if_type);
    BTF("chip_family_id:%d\n", p_bt->diag_res.chip_family_id);
    BTF("chip_rev_id:%d\n", p_bt->diag_res.chip_rev_id);
    BTF("chip_intf_id:%d\n", p_bt->diag_res.chip_intf_id);
    return NULL;
}

const char* amlbt_chip_get_firmware_test_name(amlbt_t *p_bt)
{
    if (p_bt->diag_res.chip_intf_id == CHIP_INTF_SDIO ||
        p_bt->diag_res.chip_intf_id == CHIP_INTF_PCIE2 ||
        p_bt->diag_res.chip_intf_id == CHIP_INTF_PCIE3)
    {
        if (p_bt->diag_res.chip_family_id == CHIP_W2)
        {
            if (p_bt->diag_res.chip_rev_id == CHIP_REVA || p_bt->diag_res.chip_rev_id == CHIP_REVB)
            {
                return "aml/w2_bt_fw_uart_test.bin";
            }
            else if (p_bt->diag_res.chip_rev_id == CHIP_REVC)
            {
                return "aml/w2_bt_revC_fw_uart_test.bin";
            }
        }
        else if (p_bt->diag_res.chip_family_id == CHIP_W2L)
        {
            return "aml/w2l_bt_15p4_fw_uart_test.bin";
        }
        else if (p_bt->diag_res.chip_family_id == CHIP_W1D)
        {
            return "aml/w1d_bt_15p4_fw_uart_test.bin";
        }
    }
    else if (p_bt->diag_res.chip_intf_id == CHIP_INTF_USB2 || p_bt->diag_res.chip_intf_id == CHIP_INTF_USB3)
    {
        if (p_bt->diag_res.chip_family_id == CHIP_W2)
        {
            if (p_bt->diag_res.chip_rev_id == CHIP_REVA || p_bt->diag_res.chip_rev_id == CHIP_REVB)
            {
                return "aml/w2_bt_fw_usb_test.bin";
            }
            else if (p_bt->diag_res.chip_rev_id == CHIP_REVC)
            {
                return "aml/w2_bt_revC_fw_usb_test.bin";
            }
        }
        else if (p_bt->diag_res.chip_family_id == CHIP_W2L)
        {
            return "aml/w2l_bt_15p4_fw_usb_test.bin";
        }
        else if (p_bt->diag_res.chip_family_id == CHIP_W1D)
        {
            return "aml/w1d_bt_15p4_fw_usb_test.bin";
        }
    }
    BTF("%s:%d interface error!, amlbt_if_type:%#x\n", __func__, __LINE__, amlbt_if_type);
    BTF("chip_family_id:%d\n", p_bt->diag_res.chip_family_id);
    BTF("chip_rev_id:%d\n", p_bt->diag_res.chip_rev_id);
    BTF("chip_intf_id:%d\n", p_bt->diag_res.chip_intf_id);
    return NULL;
}

