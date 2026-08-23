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
#include <linux/pci.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>

#include "common.h"
#include "intf.h"
#include "intf_sdio.h"
#include "intf_uart.h"
#include "intf_usb.h"
#include "intf_pcie.h"
#include "debug_dev.h"
#include "driver.h"
#include "rc_list.h"
#include "chip.h"

#define AML_BT_CONFIG_NAME          "aml/aml_bt.conf"

static amlbt_t bt_drv = {0};

amlbt_t * amlbt_intf_get_p_bt(void)
{
    return &bt_drv;
}

unsigned int amlbt_intf_rw_get(void)
{
    return amlbt_intf_get_p_bt()->rw_intf;
}

unsigned int amlbt_intf_bt_get(void)
{
    return amlbt_intf_get_p_bt()->bt_intf;
}

int amlbt_intf_write_word(unsigned int addr, unsigned int data)
{
    int ret = -1;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    if (amlbt_intf_rw_get() == INTF_SDIO)
    {
        amlbt_intf_sdio_write_word(addr, data);
        ret = 0;
    }
    else if (amlbt_intf_rw_get() == INTF_PCIE)
    {
        amlbt_intf_pcie_write_word(addr, data);
        ret = 0;
    }
    else if (amlbt_intf_rw_get() == INTF_USB)
    {
        if (p_bt->diag_res.chip_family_id == CHIP_W1D)
        {
            ret = amlbt_intf_usb_write_word(addr, data, USB_EP4);
        }
        else
        {
            ret = amlbt_intf_usb_write_word(addr, data, USB_EP2);
        }
    }
    else if (amlbt_intf_rw_get() == INTF_UART)
    {
        if (p_bt->uart_res.sw_op)
        {
            ret = amlbt_intf_uart_sw_write_word(addr, data);
        }
        else    //uart hare option use sdio/pcie for now
        {
            if (p_bt->diag_res.chip_intf_id == CHIP_INTF_SDIO)   //sdio
            {
                amlbt_intf_sdio_write_word(addr, data);
                ret = 0;
            }
            else if (p_bt->diag_res.chip_intf_id == CHIP_INTF_PCIE2 || p_bt->diag_res.chip_intf_id == CHIP_INTF_PCIE3)  //pcie
            {
                amlbt_intf_pcie_write_word(addr, data);
                ret = 0;
            }
        }
    }
    else
    {
        BTE("%s:%d interface error!, amlbt_if_type:%#x\n", __func__, __LINE__, amlbt_if_type);
    }
    return ret;
}

int amlbt_intf_read_word(unsigned int addr, unsigned int *data)
{
    int ret = -1;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    if (data == NULL)
    {
        BTE("%s:%d param error! data == NULL\n", __func__, __LINE__);
        return ret;
    }

    if (amlbt_intf_rw_get() == INTF_SDIO)
    {
        *data = amlbt_intf_sdio_read_word(addr);
        ret = 0;
    }
    else if (amlbt_intf_rw_get() == INTF_PCIE)
    {
        *data = amlbt_intf_pcie_read_word(addr);
        ret = 0;
    }
    else if (amlbt_intf_rw_get() == INTF_USB)
    {
        if (p_bt->diag_res.chip_family_id == CHIP_W1D)
        {
            ret = amlbt_intf_usb_read_word(addr, USB_EP4, data);
        }
        else
        {
            ret = amlbt_intf_usb_read_word(addr, USB_EP2, data);
        }
    }
    else if (amlbt_intf_rw_get() == INTF_UART)
    {
        //uart read pc need use sdio/usb/pcie
        //uart hare option use sdio/pcie for now
        if (addr == REG_FW_PC || !p_bt->uart_res.sw_op)
        {
            if (p_bt->diag_res.chip_intf_id == CHIP_INTF_SDIO)   //sdio
            {
                *data = amlbt_intf_sdio_read_word(addr);
                ret = 0;
            }
            else if (p_bt->diag_res.chip_intf_id == CHIP_INTF_PCIE2 || p_bt->diag_res.chip_intf_id == CHIP_INTF_PCIE3)  //pcie
            {
                *data = amlbt_intf_pcie_read_word(addr);
                ret = 0;
            }
        }
        else if (p_bt->uart_res.sw_op)
        {
            ret = amlbt_intf_uart_sw_read_word(addr, data);
        }
    }
    else
    {
        BTE("%s:%d interface error!, amlbt_if_type:%#x\n", __func__, __LINE__, amlbt_if_type);
    }
    return ret;
}

int amlbt_intf_read_sram(unsigned char* buf, unsigned int addr, unsigned int len)
{
    int ret = -1;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    if (buf == NULL)
    {
        BTE("%s:%d param error! data == NULL\n", __func__, __LINE__);
        return ret;
    }

    if (len == 0)
    {
        BTE("%s:%d len error! len == 0\n", __func__, __LINE__);
        return ret;
    }

    if (amlbt_intf_rw_get() == INTF_SDIO)
    {
        amlbt_intf_sdio_read_sram(buf, (unsigned char *)(unsigned long)addr, len);
        ret = 0;
    }
    else if (amlbt_intf_rw_get() == INTF_USB)
    {
        if (p_bt->diag_res.chip_family_id == CHIP_W1D)
        {
            ret = amlbt_intf_usb_read_sram(buf, (unsigned char *)(unsigned long)addr, len, USB_EP4);
        }
        else
        {
            ret = amlbt_intf_usb_read_sram(buf, (unsigned char *)(unsigned long)addr, len, USB_EP2);
        }
    }
    else if (amlbt_intf_rw_get() == INTF_UART)
    {
        if (p_bt->uart_res.sw_op)
        {
            ret = amlbt_intf_uart_sw_read_sram(buf, addr, len);
        }
        else    //hardware read sram is not support in uart RTL, use sdio
        {
            if (p_bt->diag_res.chip_intf_id == CHIP_INTF_SDIO)   //sdio
            {
                amlbt_intf_sdio_read_sram(buf, (unsigned char *)(unsigned long)addr, len);
                ret = 0;
            }
        }
    }
    else
    {
        BTE("%s:%d interface error!, amlbt_if_type:%#x\n", __func__, __LINE__, amlbt_if_type);
    }
    return ret;
}

int amlbt_intf_write_sram(unsigned char* buf, unsigned int addr, unsigned int len)
{
    int ret = -1;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    if (buf == NULL)
    {
        BTE("%s:%d param error! data == NULL\n", __func__, __LINE__);
        return ret;
    }

    if (len == 0)
    {
        BTE("%s:%d len error! len == 0\n", __func__, __LINE__);
        return ret;
    }

    if (amlbt_intf_rw_get() == INTF_SDIO)
    {
        amlbt_intf_sdio_write_sram(buf, (unsigned char *)(unsigned long)addr, len);
        ret = 0;
    }
    else if (amlbt_intf_rw_get() == INTF_USB)
    {
        if (p_bt->diag_res.chip_family_id == CHIP_W1D)
        {
            ret = amlbt_intf_usb_write_sram(buf, (unsigned char *)(unsigned long)addr, len, USB_EP4);
        }
        else
        {
            ret = amlbt_intf_usb_write_sram(buf, (unsigned char *)(unsigned long)addr, len, USB_EP2);
        }
    }
    else if (amlbt_intf_rw_get() == INTF_UART)
    {
        if (p_bt->uart_res.sw_op)
        {
            ret = amlbt_intf_uart_sw_write_sram(buf, addr, len);
        }
        else    //hardware write sram is not used, currently use sdio/pcie
        {
            if (p_bt->diag_res.chip_intf_id == CHIP_INTF_SDIO)   //sdio
            {
                amlbt_intf_sdio_write_sram(buf, (unsigned char *)(unsigned long)addr, len);
                ret = 0;
            }
        }
    }
    else
    {
        BTE("%s:%d interface error!, amlbt_if_type:%#x\n", __func__, __LINE__, amlbt_if_type);
    }
    return ret;
}

int amlbt_intf_register(void)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    BTI("%s \n", __func__);

    if (p_bt->diag_res.chip_family_id == CHIP_W2L ||
        (p_bt->diag_res.chip_family_id == CHIP_W2 && p_bt->diag_res.chip_rev_id >= Rev_C) ||
            p_bt->diag_res.chip_family_id == CHIP_W1D)
    {
        return amlbt_intf_uart_register();
    }
    return 0;
}

void amlbt_intf_version(void)
{
    BTI("%s, version:%#x", __func__, AML_BT_DRIVER_VERSION);

#ifdef CONFIG_AML_BT_USB_HOTPLUG
    BTI("CONFIG_AML_BT_USB_HOTPLUG enabled.");
#endif

    if (amlbt_intf_rw_get() == INTF_SDIO)
    {
        BTI("%s, INTF_SDIO", __func__);
    }
    else if (amlbt_intf_rw_get() == INTF_USB)
    {
        BTI("%s, INTF_USB", __func__);
    }
    else if (amlbt_intf_rw_get() == INTF_UART)
    {
        BTI("%s, INTF_UART", __func__);
    }
    else if (amlbt_intf_rw_get() == INTF_PCIE)
    {
        BTI("%s, INTF_PCIE", __func__);
    }
    else
    {
        BTE("%s:%d interface error!, amlbt_if_type:%#x\n", __func__, __LINE__, amlbt_if_type);
    }
}

void amlbt_intf_fw_info(void)
{
    unsigned int value = 0;
    int ret = 0;

    ret = amlbt_intf_read_word(REG_PMU_POWER_CFG, &value);
    BTI("BT PMU %#x:%#x \n", REG_PMU_POWER_CFG, value);
    if (ret != 0)
    {
        return ;
    }
    usleep_range(10000, 10000);
    ret = amlbt_intf_read_word(REG_FW_PC, &value);
    value = (value >> 6);
    BTI("pc1 %#x:%#x\n", REG_FW_PC, value);
    if (ret != 0)
    {
        return ;
    }
    usleep_range(10000, 10000);
    ret = amlbt_intf_read_word(REG_FW_PC, &value);
    value = (value >> 6);
    BTI("pc2 %#x:%#x\n", REG_FW_PC, value);
    if (ret != 0)
    {
        return ;
    }
    usleep_range(10000, 10000);
    ret = amlbt_intf_read_word(REG_FW_PC, &value);
    value = (value >> 6);
    BTI("pc3 %#x:%#x\n", REG_FW_PC, value);
    if (ret != 0)
    {
        return ;
    }
}

static void amlbt_intf_fifo_deinit(amlbt_common_gdsl_fifo_t **p_fifo, amlbt_t *p_bt, unsigned int r_point_addr, unsigned int w_point_addr)
{
    if (!(p_bt->pm_res.dr_state & BT_DRV_STATE_RECOVERY))
    {
        amlbt_intf_write_word(r_point_addr, 0);
        amlbt_intf_write_word(w_point_addr, 0);
    }
    if (*p_fifo != NULL)
    {
        amlbt_common_gdsl_fifo_deinit(*p_fifo);
        *p_fifo = NULL;
    }
}

static amlbt_common_gdsl_fifo_t *amlbt_intf_fifo_init(amlbt_common_gdsl_fifo_t **p_fifo, unsigned int len, unsigned char *base_addr,
    unsigned int r_point_addr, unsigned int w_point_addr)
{
    int ret = 0;
    unsigned int w = 0;
    unsigned int r = 0;

    if (*p_fifo == NULL)
    {
        *p_fifo = amlbt_common_gdsl_fifo_init(len, base_addr);
        if (*p_fifo != NULL)
        {
            amlbt_intf_write_word(r_point_addr, (unsigned int)(unsigned long)(*p_fifo)->r);
            amlbt_intf_write_word(w_point_addr, (unsigned int)(unsigned long)(*p_fifo)->w);
            ret = amlbt_intf_read_word(r_point_addr, &r);
            BTI("%s, %d, %#x r:%#lx", __func__, ret, r_point_addr, r);
            ret = amlbt_intf_read_word(w_point_addr, &w);
            BTI("%s, %d, %#x w:%#lx", __func__, ret, w_point_addr, w);
        }
    }
    return *p_fifo;
}

void amlbt_intf_wakeup_key_process(amlbt_t *p_bt)
{
    unsigned int key;

    if (0 == amlbt_intf_read_word(RG_AON_A17, &key))
    {
        if (key & BIT(5))   //bit 5 power key, bit 6 netfix
        {
            key &= ~BIT(5);
            amlbt_intf_write_word(RG_AON_A17, key);
            input_event(p_bt->pm_res.input_dev, EV_KEY, KEY_POWER, 1);
            input_sync(p_bt->pm_res.input_dev);
            input_event(p_bt->pm_res.input_dev, EV_KEY, KEY_POWER, 0);
            input_sync(p_bt->pm_res.input_dev);
            p_bt->pm_res.irq_handle = 0;
            BTI("%s input power key\n", __func__);
        }
        else if (key & BIT(6))   //bit 5 power key, bit 6 netfix
        {
            key &= ~BIT(6);
            amlbt_intf_write_word(RG_AON_A17, key);
            input_event(p_bt->pm_res.input_dev, EV_KEY, KEY_NETFLIX, 1);
            input_sync(p_bt->pm_res.input_dev);
            input_event(p_bt->pm_res.input_dev, EV_KEY, KEY_NETFLIX, 0);
            input_sync(p_bt->pm_res.input_dev);
            p_bt->pm_res.irq_handle = 0;
            BTI("%s input netfix key\n", __func__);
        }
        else
        {
            BTE("%s:%d unknown key:%#x\n", __func__, __LINE__, key);
        }
    }
    else
    {
        BTE("%s:%d Failed to read key register!\n", __func__, __LINE__);
    }
}

static void amlbt_intf_wake_func(struct work_struct *work)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    amlbt_intf_wakeup_key_process(p_bt);
}

static void amlbt_intf_wake_fw_gpio(amlbt_t *p_bt)
{
    BTI("%s %d\n", __func__, p_bt->uart_res.wake_gpio);
    if (p_bt->uart_res.wake_gpio >= 0)
    {
        gpio_direction_output(p_bt->uart_res.wake_gpio, 1);
        mdelay(1);   //1ms
        gpio_direction_output(p_bt->uart_res.wake_gpio, 0);
        mdelay(10);  //wait firmware wake
        BTI("%s end\n", __func__);
    }
}

static int amlbt_intf_parse_int_value(char *start, const char *key, int *value, unsigned char *str, amlbt_t *p_bt)
{
    size_t key_len = strlen(key);
    size_t manflen = 0;
    int i = 0;
    int j = 0;
    char *ptr = NULL;
    char sub_str[3] = {0};
    if (strncmp(start, key, key_len) == 0 && start[key_len] == '=')
    {
        ptr = (start + key_len + 1);
        if (strcmp(key, "W1UManfData") == 0)
        {
            manflen = strlen(ptr);
            BTI("PTR %s len %d", ptr, manflen);
            for (; i < manflen; i+=2,j++)
            {
                sub_str[0] = ptr[i];
                sub_str[1] = ptr[i+1];
                str[j] = simple_strtoul(sub_str, NULL, 16);
                if (*(ptr+i+2) == ' ')
                {
                    i += 1;
                }
            }
            p_bt->conf_res.manfdata_len = j;
        }
        else
        {
            *value = simple_strtol(ptr, NULL, 10);
        }
        return 1;
    }
    return 0;
}

static irqreturn_t amlbt_intf_gpio_irq_handler(int irq, void *dev_id)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    p_bt->diag_res.fw_interrupt_cnt++;

    if (amlbt_intf_rw_get() == INTF_UNKNOWN)
    {
        BTE("%s:%d interface error!, amlbt_if_type:%#x\n", __func__, __LINE__, amlbt_if_type);
        return IRQ_HANDLED;
    }

    if (amlbt_intf_rw_get() == INTF_USB)
    {
        if (p_bt->pm_res.dr_state == 0)
        {
            amlbt_intf_queue_work(p_bt->usb_res.rx_work_wq, &p_bt->usb_res.rx_work);
        }
    }
    else
    {
        if (p_bt->pm_res.irq_handle)
        {
            amlbt_intf_queue_work(p_bt->pm_res.wake_work_wq, &p_bt->pm_res.wake_work);
        }
    }

    return IRQ_HANDLED;
}

static int amlbt_intf_register_interrupt_gpio(amlbt_t *p_bt)
{
    struct device_node *node;
    int gpio_num;
    int irq;
    int ret;

    BTI("%s, irq:%d \n", __func__, p_bt->pm_res.irq);

    if (p_bt->pm_res.irq != -1)
    {
        BTE("Irq has already registered!\n");
        return 0;
    }

    node = of_find_node_by_name(NULL, "aml_bt");
    if (!node)
    {
        BTE("Failed to find device node\n");
        return -ENODEV;
    }
    BTI("%s find node success!\n", __func__);

    gpio_num = of_get_named_gpio(node, "btwakeup-gpios", 0);
    if (gpio_num < 0)
    {
        BTE("Failed to get GPIO, error code: %d\n", gpio_num);
        return gpio_num;
    }
    BTI("%s find gpio %d success!\n", __func__, gpio_num);

    ret = gpio_request(gpio_num, "btwakeup_gpio");
    if (ret) {
        BTE("Failed to request gpio %d: %d\n", gpio_num, ret);
        return ret;
    }

    ret = gpio_direction_input(gpio_num);
    if (ret) {
        BTE("Failed to set gpio %d as input: %d\n", gpio_num, ret);
        gpio_free(gpio_num);
        return ret;
    }
    p_bt->pm_res.gpio_num = gpio_num;

    BTI("%s gpio request and direction success!\n", __func__);

    irq = gpio_to_irq(gpio_num);
    if (irq < 0)
    {
        BTE("Failed to get IRQ for GPIO %d\n", gpio_num);
        gpio_free(gpio_num);
        return irq;
    }
    BTI("%s find irq %d success!\n", __func__, irq);

    ret = request_irq(irq, amlbt_intf_gpio_irq_handler, IRQF_TRIGGER_FALLING, "usb_gpio_irq", &p_bt->drv_res.dev_device[0]);
    if (ret)
    {
        BTE("Failed to request IRQ %d: %d\n", irq, ret);
        gpio_free(gpio_num);
        return ret;
    }

    BTI("%s request irq %d success!\n", __func__, irq);
    p_bt->pm_res.irq = irq;
    return 0;
}

static void amlbt_intf_unregister_interrupt_gpio(amlbt_t *p_bt)
{
    if (p_bt->pm_res.irq > 0)
    {
        free_irq(p_bt->pm_res.irq, &p_bt->drv_res.dev_device[0]);
        p_bt->pm_res.irq = -1;
    }

    if (p_bt->pm_res.gpio_num > 0)
    {
        gpio_free(p_bt->pm_res.gpio_num);
        p_bt->pm_res.gpio_num = -1;
    }
}

static int amlbt_intf_register_wakeup_gpio(amlbt_t *p_bt)
{
    struct device_node *node;
    int gpio_num;
    int ret;

    BTI("%s %d\n", __func__, p_bt->uart_res.wake_gpio);

    node = of_find_node_by_name(NULL, "aml_bt");
    if (!node)
    {
        BTE("Failed to find device node\n");
        return -1;
    }
    //BTI("%s find node success!\n", __func__);
    gpio_num = of_get_named_gpio(node, "hostwake-gpios", 0);
    if (gpio_num < 0)
    {
        BTE("Failed to get GPIO\n");
        return -1;
    }
    BTI("%s get wakeup gpio %d success!\n", __func__, gpio_num);
    /*ret = gpio_request(gpio_num, "amlbt_host_wake_gpio");
    if (ret < 0)
    {
        BTE("Failed to get GPIO\n");
        return -1;
    }*/
    p_bt->uart_res.wake_gpio = gpio_num;
    gpio_direction_output(p_bt->uart_res.wake_gpio, 0);
    return 0;
}

static void amlbt_intf_unregister_wakeup_gpio(amlbt_t *p_bt)
{
    BTI("%s %d\n", __func__, p_bt->uart_res.wake_gpio);

    if (p_bt->uart_res.wake_gpio >= 0)
    {
        gpio_free(p_bt->uart_res.wake_gpio);
        p_bt->uart_res.wake_gpio = -1;
    }
}

static int amlbt_intf_load_conf(amlbt_t *p_bt)
{
    int ret = 0;
    const struct firmware *fw_entry = NULL;
    char *data;
    size_t len, pos = 0;

    BTI("Firmware load:%s\n", AML_BT_CONFIG_NAME);
    ret = request_firmware(&fw_entry, AML_BT_CONFIG_NAME, p_bt->drv_res.dev_device[0]);
    if (ret)
    {
        BTE("%s:%d Failed to load config file: %d\n", __func__, __LINE__, ret);
        return -EINVAL;
    }

    if (!fw_entry || !fw_entry->data)
    {
        BTE("Failed to load conf or data is empty\n");
        release_firmware(fw_entry);
        return -EINVAL;
    }

    data = (char *)fw_entry->data;
    len = fw_entry->size;

    // Manual parsing loop
    while (pos < len)
    {
        char *line_start = data + pos;
        char *line_end = strchr(line_start, '\n');  // Find end of line
        if (!line_end)
        {
            line_end = data + len;  // If no newline, this is the last line
        }

        *line_end = '\0';  // Null-terminate the current line

        // Parse known keys
        if (amlbt_intf_parse_int_value(line_start, "BtAntenna", &p_bt->conf_res.antenna, NULL, p_bt))
        {
            BTI("Parsed BtAntenna: %d\n", p_bt->conf_res.antenna);
        }
        else if (amlbt_intf_parse_int_value(line_start, "FirmwareMode", &p_bt->conf_res.fw_mode, NULL, p_bt))
        {
            BTI("Parsed FirmwareMode: %d\n", p_bt->conf_res.fw_mode);
        }
        else if (amlbt_intf_parse_int_value(line_start, "BtSink", &p_bt->conf_res.bt_sink, NULL, p_bt)) {
            BTI("Parsed BtSink: %d\n", p_bt->conf_res.bt_sink);
        }
        else if (amlbt_intf_parse_int_value(line_start, "ChangePinMux", &p_bt->conf_res.pin_mux, NULL, p_bt))
        {
            BTI("Parsed ChangePinMux: %d\n", p_bt->conf_res.pin_mux);
        }
        else if (amlbt_intf_parse_int_value(line_start, "BrDigitGain", &p_bt->conf_res.br_digit_gain, NULL, p_bt))
        {
            BTI("Parsed BrDigitGain: %d\n", p_bt->conf_res.br_digit_gain);
        }
        else if (amlbt_intf_parse_int_value(line_start, "EdrDigitGain", &p_bt->conf_res.edr_digit_gain, NULL, p_bt))
        {
            BTI("Parsed EdrDigitGain: %d\n", p_bt->conf_res.edr_digit_gain);
        }
        else if (amlbt_intf_parse_int_value(line_start, "Btfwlog", &p_bt->conf_res.fw_log, NULL, p_bt))
        {
            BTI("Parsed Btfwlog: %d\n", p_bt->conf_res.fw_log);
        }
        else if (amlbt_intf_parse_int_value(line_start, "Btlog", &p_bt->conf_res.driver_log, NULL, p_bt))
        {
            BTI("Parsed Btlog: %d\n", p_bt->conf_res.driver_log);
        }
        else if (amlbt_intf_parse_int_value(line_start, "Btfactory", &p_bt->conf_res.factory, NULL, p_bt))
        {
            BTI("Parsed Btfactory: %d\n", p_bt->conf_res.factory);
        }
        else if (amlbt_intf_parse_int_value(line_start, "Manfcnt", &p_bt->conf_res.manf_cnt, NULL, p_bt))
        {
            BTI("Parsed Manfcnt: %d\n", p_bt->conf_res.manf_cnt);
        }
        else if (amlbt_intf_parse_int_value(line_start, "W1UManfData", NULL, p_bt->conf_res.manf_data, p_bt))
        {
            BTI("Parsed W1UManfData len: %d\n", p_bt->conf_res.manfdata_len);
        }
        else if (amlbt_intf_parse_int_value(line_start, "Btsystem", &p_bt->conf_res.system, NULL, p_bt))
        {
            BTI("Parsed Btsystem: %d\n", p_bt->conf_res.system);
        }
        else if (amlbt_intf_parse_int_value(line_start, "BtIsolation", &p_bt->conf_res.isolation, NULL, p_bt))
        {
            BTI("Parsed isolatvalue: %d\n", p_bt->conf_res.isolation);
        }

        // Move to the next line
        pos = (line_end - data) + 1;
    }

    release_firmware(fw_entry);
    return 0;
}

static int amlbt_intf_download_firmware_check(amlbt_t *p_bt, unsigned int addr, unsigned int download_len, const unsigned char *data_buf)
{
    unsigned int offset = 0;
    int ret = -1;
    unsigned int retry = 0;
    unsigned int remain_len;
    unsigned int data;
    unsigned int check_data = 0;
    uint8_t *check_buf = kzalloc(p_bt->fw_res.download_size, GFP_DMA|GFP_ATOMIC);

    if (check_buf == NULL)
    {
        BTF("%s:%d check_buf alloc failed!!\n", __func__, __LINE__);
        return -1;
    }
    memset(check_buf, 0, p_bt->fw_res.download_size);
    remain_len = download_len;

    while (offset < download_len)
    {
        if (remain_len < p_bt->fw_res.download_size)
        {
            BTD("offset %#x, addr %#x\n", offset, addr);
            if (p_bt->diag_res.chip_intf_id == CHIP_INTF_PCIE2 || p_bt->diag_res.chip_intf_id == CHIP_INTF_PCIE3)
            {
                data = ((data_buf[offset+3]<<24)|(data_buf[offset+2]<<16)
                    |(data_buf[offset+1]<<8)|data_buf[offset]);
                ret = amlbt_intf_write_word(addr, data);
                if (ret != 0)
                {
                    goto error;
                }
                ret = amlbt_intf_read_word(addr, &check_data);
                if (ret != 0)
                {
                    goto error;
                }
                *(unsigned int *)check_buf = check_data;
            }
            else
            {
                ret = amlbt_intf_write_sram((unsigned char *)&data_buf[offset], addr, remain_len);
                if (ret != 0)
                {
                    goto error;
                }
                ret = amlbt_intf_read_sram(check_buf, addr, remain_len);
                if (ret != 0)
                {
                    goto error;
                }
            }
            if (memcmp(check_buf, &data_buf[offset], remain_len))
            {
                BTE("Firmware check2 error! addr %#x offset %#x, retry:%d\n", addr, offset, retry);
                if (retry < MAX_DOWNLOAD_RETRY)
                {
                    retry++;
                    continue;
                }
                ret = -1;
                goto error;
            }
            offset += remain_len;
            addr += remain_len;
            BTD("offset %#x, write_len %#x\n", offset, remain_len);
        }
        else
        {
            BTD("offset %#x, write_len %#x, addr %#x\n", offset, p_bt->fw_res.download_size, addr);
            if (p_bt->diag_res.chip_intf_id == CHIP_INTF_PCIE2 || p_bt->diag_res.chip_intf_id == CHIP_INTF_PCIE3)
            {
                data = ((data_buf[offset+3]<<24)|(data_buf[offset+2]<<16)
                    |(data_buf[offset+1]<<8)|data_buf[offset]);
                ret = amlbt_intf_write_word(addr, data);
                if (ret != 0)
                {
                    goto error;
                }
                ret = amlbt_intf_read_word(addr, &check_data);
                if (ret != 0)
                {
                    goto error;
                }
                *(unsigned int *)check_buf = check_data;
            }
            else
            {
                ret = amlbt_intf_write_sram((unsigned char *)&data_buf[offset], addr, p_bt->fw_res.download_size);
                if (ret != 0)
                {
                    goto error;
                }
                ret = amlbt_intf_read_sram(check_buf, addr, p_bt->fw_res.download_size);
                if (ret != 0)
                {
                    goto error;
                }
            }

            if (memcmp(check_buf, &data_buf[offset], p_bt->fw_res.download_size))
            {
                BTE("Firmware check error! addr %#x, offset %#x, retry:%d\n", addr, offset, retry);
                if (retry < MAX_DOWNLOAD_RETRY)
                {
                    retry++;
                    continue;
                }
                ret = -1;
                goto error;
            }
            offset += p_bt->fw_res.download_size;
            remain_len -= p_bt->fw_res.download_size;
            addr += p_bt->fw_res.download_size;
        }
        BTD("remain_len %#x\n", remain_len);
    }
error:
    kfree(check_buf);
    return ret;
}

static int amlbt_intf_download_firmware(amlbt_t *p_bt)
{
    unsigned int iccm_base_addr = 0;
    unsigned int dccm_base_addr = 0;
    unsigned int add_base_addr = 0;
    int ret = 0;

    if (amlbt_intf_rw_get() != INTF_USB)
    {
        BTI("%s: intf not USB skip firmware download\n", __func__);
        return 0;
    }

    if (p_bt->diag_res.chip_family_id == CHIP_W1D)
    {
        iccm_base_addr = p_bt->fw_res.iccm_ram_base;
        dccm_base_addr = p_bt->fw_res.dccm_ram_base;
        add_base_addr = p_bt->fw_res.add_ram_base;
    }
    else
    {
        iccm_base_addr = p_bt->fw_res.iccm_ahb_base + p_bt->fw_res.rom_size;
        dccm_base_addr = p_bt->fw_res.dccm_ahb_base;
    }
    //to do download bt fw
    BTI("download addr iccm_base_addr %#x, dccm_base_addr %#x, add_base_addr %#x,", iccm_base_addr, dccm_base_addr, add_base_addr);
    BTI("download size  iccm size %#x, dccm size %#x, add size %#x\n", p_bt->fw_res.iccm_size, p_bt->fw_res.dccm_size, p_bt->fw_res.add_size);

    BTI("iccm start [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n",
        p_bt->fw_res.iccm_buf[0], p_bt->fw_res.iccm_buf[1], p_bt->fw_res.iccm_buf[2], p_bt->fw_res.iccm_buf[3],
        p_bt->fw_res.iccm_buf[4], p_bt->fw_res.iccm_buf[5], p_bt->fw_res.iccm_buf[6], p_bt->fw_res.iccm_buf[7]);
    BTI("dccm start [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n",
        p_bt->fw_res.dccm_buf[0], p_bt->fw_res.dccm_buf[1], p_bt->fw_res.dccm_buf[2], p_bt->fw_res.dccm_buf[3],
        p_bt->fw_res.dccm_buf[4], p_bt->fw_res.dccm_buf[5], p_bt->fw_res.dccm_buf[6], p_bt->fw_res.dccm_buf[7]);
    //download iccm
    ret = amlbt_intf_download_firmware_check(p_bt, iccm_base_addr, p_bt->fw_res.iccm_size, p_bt->fw_res.iccm_buf);
    if (ret != 0)
    {
        BTI("Firmware iccm check failed\n");
        ret = -1;
        goto error;
    }
    BTI("Firmware iccm check pass\n");
    //download dccm
    ret = amlbt_intf_download_firmware_check(p_bt, dccm_base_addr, p_bt->fw_res.dccm_size, p_bt->fw_res.dccm_buf);
    if (ret != 0)
    {
        BTI("Firmware dccm check failed\n");
        ret = -1;
        goto error;
    }
    BTI("Firmware dccm check pass\n");
    //download add
    if (p_bt->diag_res.chip_family_id == CHIP_W1D)
    {
        ret = amlbt_intf_download_firmware_check(p_bt, add_base_addr, p_bt->fw_res.add_size, p_bt->fw_res.add_buf);
        if (ret != 0)
        {
            BTI("Firmware add check failed\n");
            ret = -1;
            goto error;
        }
        BTI("Firmware add check pass\n");
    }
error:
    return ret;
}

static int amlbt_intf_load_firmware(amlbt_t *p_bt)
{
    int ret = 0;
    unsigned int reg = 0;
    const struct firmware *fw_entry = NULL;
    unsigned int rom_size;
    unsigned int iccm_size;
    unsigned int dccm_size;
    unsigned int add_size;
    unsigned int intf = amlbt_intf_rw_get();
    const char *firmware_bin = amlbt_chip_get_firmware_name(p_bt);
    const char *firmware_ft_bin = amlbt_chip_get_firmware_test_name(p_bt);

    if (firmware_bin == NULL)
    {
        BTE("%s:%d firmware bin error!\n", __func__, __LINE__);
        return -1;
    }

    if (firmware_ft_bin == NULL)
    {
        BTE("%s:%d firmware ft bin error!\n", __func__, __LINE__);
        return -1;
    }

    if (intf == INTF_UNKNOWN)
    {
        BTE("%s:%d interface error!\n", __func__, __LINE__);
        return -1;
    }

    if (!amlbt_ft_mode)
    {
        BTI("Firmware load:%s\n", firmware_bin);
        ret = request_firmware(&fw_entry, firmware_bin, p_bt->drv_res.dev_device[0]);
    }
    else
    {
        BTI("Firmware load:%s\n", firmware_ft_bin);
        ret = request_firmware(&fw_entry, firmware_ft_bin, p_bt->drv_res.dev_device[0]);
    }
    if (ret)
    {
        BTE("%s:%d Failed to load firmware: %d\n", __func__, __LINE__, ret);
        return ret;
    }
    BTI("Firmware [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n",
        fw_entry->data[0], fw_entry->data[1], fw_entry->data[2], fw_entry->data[3],
        fw_entry->data[4], fw_entry->data[5], fw_entry->data[6], fw_entry->data[7]);
    rom_size = ((fw_entry->data[3]<<24)|(fw_entry->data[2]<<16)|(fw_entry->data[1]<<8)|(fw_entry->data[0]));
    iccm_size = ((fw_entry->data[7]<<24)|(fw_entry->data[6]<<16)|(fw_entry->data[5]<<8)|(fw_entry->data[4]));
    dccm_size = ((fw_entry->data[11]<<24)|(fw_entry->data[10]<<16)|(fw_entry->data[9]<<8)|(fw_entry->data[8]));
    add_size = ((fw_entry->data[15]<<24)|(fw_entry->data[14]<<16)|(fw_entry->data[13]<<8)|(fw_entry->data[12]));

    BTI("Firmware get, rom_size: %#x, iccm_size: %#x, dccm_size:%#x, add_size:%#x\n", \
                                                    rom_size, iccm_size, dccm_size, add_size);
    if (p_bt->diag_res.chip_family_id == CHIP_W1D)
    {
        p_bt->fw_res.iccm_buf = &fw_entry->data[p_bt->fw_res.rom_size + 16];
        p_bt->fw_res.dccm_buf = &fw_entry->data[iccm_size + p_bt->fw_res.rom_size + 16];
        p_bt->fw_res.add_buf = &fw_entry->data[dccm_size + iccm_size + p_bt->fw_res.rom_size + 16];
    }
    else
    {
        p_bt->fw_res.iccm_buf = &fw_entry->data[p_bt->fw_res.rom_size + 8];  //iccm data
        p_bt->fw_res.dccm_buf = &fw_entry->data[rom_size + 8];  //rom_size = rom + iccm
    }

    //clear bbreset, because bt_en is always remain high level in usb mode
    //amlbt_intf_reg_bit_clr(RG_BT_PMU_A12, 6);
    //amlbt_intf_reg_bit_clr(RG_BT_PMU_A12, 7);
#if defined(CONFIG_AML_BT_CHIP_W2L) // || defined(CONFIG_AML_BT_CHIP_W1D)
    amlbt_intf_read_word(RG_BT_PMU_A30, &reg);
    BTI("pmu power control fsm: %#x\n", reg);
    amlbt_intf_read_word(REG_PMU_POWER_CFG, &reg);
    BTI("pmu power cfg: %#x\n", reg);
#endif
    ret = amlbt_intf_download_firmware(p_bt);
    release_firmware(fw_entry);
    if (ret != 0)
    {
        BTE("Download firmware failed!!\n");
        return ret;
    }
#if defined(CONFIG_AML_BT_CHIP_W2L) // || defined(CONFIG_AML_BT_CHIP_W1D)
    amlbt_intf_read_word(REG_DF_A194, &reg);
    reg &= 0xfffffffc;
    reg |= (p_bt->conf_res.isolation & 0x3);
    amlbt_intf_write_word(REG_DF_A194, reg);

    amlbt_intf_read_word(REG_PMU_POWER_CFG, &reg);
    reg &= 0xedffffff;
    reg |= ((p_bt->conf_res.antenna << BIT_RF_NUM)|(p_bt->conf_res.bt_sink << BT_SINK_MODE));
    amlbt_intf_write_word(REG_PMU_POWER_CFG, reg);

    amlbt_intf_read_word(RG_AON_A53, &reg);
    reg &= 0xffcf0000;
    reg |= ((p_bt->conf_res.pin_mux << 20) | (p_bt->conf_res.factory << 21));
    reg |= (((p_bt->conf_res.edr_digit_gain & 0xff) << 8) | (p_bt->conf_res.br_digit_gain & 0xff));
    amlbt_intf_write_word(RG_AON_A53, reg);

    amlbt_intf_read_word(RG_AON_A56, &reg);
    reg &= 0xfffffffc;
    reg |= (p_bt->conf_res.fw_mode & 0x3);
    amlbt_intf_write_word(RG_AON_A56, reg);

    if (amlbt_intf_rw_get() != INTF_USB) {
        amlbt_intf_read_word(RG_AON_A59, &reg);
        reg &= 0xfffffffc;
        reg |= (p_bt->conf_res.fw_log & 0x3);
        amlbt_intf_write_word(RG_AON_A59, reg);
    }

    if (amlbt_intf_rw_get() == INTF_USB) {
        amlbt_intf_write_word(REG_DEV_RESET, 0); //start cpu
        BTI("usb module dev reset in driver\n");
    }
#else
    //w1d fw init configure unified use of AON61
    amlbt_intf_read_word(RG_AON_A61, &reg);
    reg &= 0xffffffc0;
    reg |= p_bt->conf_res.fw_mode;
    reg |= (p_bt->conf_res.antenna << 2);
    reg |= (p_bt->conf_res.fw_log << 4);
    amlbt_intf_write_word(RG_AON_A61, reg);

   if (amlbt_intf_rw_get() == INTF_USB) {
       amlbt_intf_write_word(REG_DEV_RESET, 0x302c); //start cpu
       BTI("usb module dev reset in driver\n");
   }
#endif
    BTI("start bt cpu ok!\n");
    if (amlbt_intf_rw_get() == INTF_UART)
    {
        p_bt->uart_res.sw_op = 1;
    }

    usleep_range(50000, 50000);
    amlbt_intf_read_word(REG_FW_PC, &reg);
    BTI("pc1:%#x\n", reg);
    usleep_range(10000, 10000);
    amlbt_intf_read_word(REG_FW_PC, &reg);
    BTI("pc2:%#x\n", reg);
    usleep_range(10000, 10000);
    amlbt_intf_read_word(REG_FW_PC, &reg);
    BTI("pc3:%#x\n", reg);
#if defined(CONFIG_AML_BT_CHIP_W1D)
    amlbt_intf_read_word(REG_UART_ENABLE, &reg);
    BTI("uart enable:%#x\n", reg);
#endif

    p_bt->fw_res.iccm_buf = NULL;
    p_bt->fw_res.dccm_buf = NULL;
    p_bt->fw_res.add_buf = NULL;

    return 0;
}

static void amlbt_write_fwlog_mode(amlbt_t *p_bt)
{
    unsigned int reg = 0;

    amlbt_intf_read_word(RG_AON_A59, &reg);
    reg &= 0xfffffffc;
    reg |= (p_bt->conf_res.fw_log & 0x3);
    amlbt_intf_write_word(RG_AON_A59, reg);
    BTI("[%s] fwlog:%d, reg:0x%x\n", __func__, p_bt->conf_res.fw_log, reg);
}

static unsigned int amlbt_intf_fw_pmu_sleep_get(void)
{
    unsigned int reg_value = 0;

    amlbt_intf_read_word(RG_BT_PMU_A15, &reg_value);
    BTI("%s PMU FSM %#x\n", __func__, (reg_value & 0xF));

    if (((reg_value & 0xF) == PMU_SLEEP_MODE) || ((reg_value & 0xF) == PMU_ACT_SLEEP))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static int amlbt_intf_wake_fw(void)
{
    unsigned int reg_value = 0;
    int ret = 0;

    ret = amlbt_intf_read_word(RG_BT_PMU_A16, &reg_value);
    if (ret != 0)
    {
        goto err_exit;
    }
    reg_value &= ~BIT(0);
    reg_value |= BIT(1);
    ret = amlbt_intf_write_word(RG_BT_PMU_A16, reg_value);
    if (ret != 0)
    {
        goto err_exit;
    }
    ret = amlbt_intf_read_word(RG_BT_PMU_A16, &reg_value);
    if (ret != 0)
    {
        goto err_exit;
    }

    BTI("%s RG_BT_PMU_A16 %#x\n", __func__, reg_value);
    return ret;
err_exit:
    return ret;
}

static int amlbt_intf_send_vendor(u16 opcode, const uint8_t *cmd, size_t cmd_length)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    int ret = 0;
    size_t total_written = 0;
    struct sk_buff *skb = NULL;

    skb = alloc_skb(cmd_length, GFP_KERNEL);
    if (!skb) {
        BTE("%s %d alloc_skb failed! \n", __func__, __LINE__);
        return -ENOMEM;
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
    if (p_bt->common_res.write_work_wq != NULL)
    {
        skb_queue_tail(&p_bt->common_res.tx_queue, skb);
        queue_work(p_bt->common_res.write_work_wq, &p_bt->common_res.write_work);
    }
    else
    {
        BTE("%s: bt open failed! write_work_wq is deinit\n", __func__);
        kfree_skb(skb);
    }

    BTI("send hci:%#x, len:%d, total_written:%zu, ret:%d\n", opcode, cmd_length, total_written, ret);

error:
    return ret;
}

unsigned int amlbt_intf_vendor_write_rclist(unsigned char *data, unsigned char cnt, unsigned char length)
{
    uint8_t cmd[258] = {0x01, SW_WRITE_RCLIST & 0xff, (SW_WRITE_RCLIST >> 8) & 0xff};

    cmd[3] = length + 1;
    cmd[4] = cnt;

    memcpy(&cmd[5], data, length);
    BTI("%s %d [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", __func__, length + 4 + 1,
        cmd[0],cmd[1],cmd[2],cmd[3],cmd[4],cmd[5],cmd[6],cmd[7],cmd[8],cmd[9],cmd[10],cmd[11]);
    amlbt_intf_send_vendor(SW_WRITE_RCLIST, cmd, length + 4 + 1);

    return 0;
}

static void amlbt_intf_vendor_write_later_resume(amlbt_t *p_bt)
{
    int ret = 0;
    uint8_t cmd[] = {0x01, SW_STR_CMD & 0xff, (SW_STR_CMD >> 8) & 0xff, 0x01, 0x00};

    amlbt_intf_flush_workqueue(p_bt->common_res.write_work_wq, &p_bt->common_res.write_work);

    BTI("%s %d [%#x,%#x,%#x,%#x,%#x]\n", __func__, sizeof(cmd), cmd[0],cmd[1],cmd[2],cmd[3],cmd[4]);
    amlbt_intf_send_vendor(SW_STR_CMD, cmd, sizeof(cmd));
}

static void amlbt_intf_vendor_write_suspend(amlbt_t *p_bt)
{
    int ret = 0;
    uint8_t cmd[] = {0x01, SW_STR_CMD & 0xff, (SW_STR_CMD >> 8) & 0xff, 0x01, 0x02};

    BTI("%s %d [%#x,%#x,%#x,%#x,%#x]\n", __func__, sizeof(cmd), cmd[0],cmd[1],cmd[2],cmd[3],cmd[4]);
    ret = amlbt_intf_send_vendor(SW_STR_CMD, cmd, sizeof(cmd));
    if (ret == 0)
    {
        amlbt_intf_flush_workqueue(p_bt->common_res.write_work_wq, &p_bt->common_res.write_work);
    }
}

static int amlbt_intf_suspend_fw(amlbt_t *p_bt)
{
    int ret = 0;

    if (p_bt->pm_res.dr_state != 0)
    {
        BTE("%s:%d failed, dr_state:%#x \n", __func__, __LINE__, p_bt->pm_res.dr_state);
        return -1;
    }

    //set suspend bit
    ret = amlbt_common_reg_bit_set(RG_AON_A24, 26);
    if (ret != 0)
    {
        goto err_exit;
    }
    //allow fw sleep
    ret = amlbt_common_reg_bit_clr(RG_AON_A24, 25);
    if (ret != 0)
    {
        goto err_exit;
    }
    return ret;
 err_exit:
    return ret;
}

int amlbt_intf_suspend(amlbt_t *p_bt)
{
    int ret = 0;

    BTI("%s start, dr_state:%#x\n", __func__, p_bt->pm_res.dr_state);
    BTI("[%#x,%#x,%#x,%#x]\n", p_bt->bt_res.bt_start, p_bt->zigbee_res.zigbee_start,
            p_bt->thread_res.thread_start, p_bt->pm_res.dr_state);
    if (!(p_bt->pm_res.dr_state & BT_DRV_STATE_RECOVERY))
    {
        if (p_bt->bt_res.bt_start || p_bt->zigbee_res.zigbee_start || p_bt->thread_res.thread_start)
        {
            amlbt_intf_flush_workqueue(p_bt->common_res.write_work_wq, &p_bt->common_res.write_work);
            if (amlbt_intf_rw_get() == INTF_USB)
            {
                amlbt_intf_unregister_interrupt_gpio(p_bt);
                amlbt_intf_flush_workqueue(p_bt->usb_res.rx_work_wq, &p_bt->usb_res.rx_work);
            }
            ret = amlbt_write_rclist_to_firmware();
            if (ret != 0)
            {
                BTE("%s:%d write rclist failed \n", __func__, __LINE__);
                return -EBUSY;
            }
            if (amlbt_intf_rw_get() == INTF_UART || p_bt->diag_res.chip_family_id == CHIP_W1D)
            {
                amlbt_intf_vendor_write_suspend(p_bt);
                if (amlbt_intf_rw_get() == INTF_UART)
                {
                    amlbt_intf_unregister_wakeup_gpio(p_bt);
                }
                amlbt_intf_drv_state_set(BT_DRV_STATE_SUSPEND);
            }
            else
            {
                if (atomic_read(&g_wifi_pm.bus_suspend_cnt) == 0)
                {
                    ret = amlbt_intf_suspend_fw(p_bt);
                    if (ret != 0)
                    {
                        BTE("%s:%d entery lowpower failed \n", __func__, __LINE__);
                        return -EBUSY;
                    }
                    amlbt_intf_drv_state_set(BT_DRV_STATE_SUSPEND);
                }
                else
                {
                    BTF("%s failed bus_suspend_cnt %#x\n", __func__, atomic_read(&g_wifi_pm.bus_suspend_cnt));
                }
            }
            p_bt->pm_res.irq_handle = 0;
        }
    }
    BTI("%s end\n", __func__);
    return 0;
}

static int amlbt_intf_resume_fw(amlbt_t *p_bt)
{
    int wait_cnt = 0;
    int retry_cnt = 0;
    int ret;
    unsigned int reg_value = 0;
    unsigned int key_value = 0;
    unsigned int wake_value = 0;

    //forbid fw sleep
    ret = amlbt_common_reg_bit_set(RG_AON_A24, 25);
    if (ret != 0)
    {
        goto error;
    }
    usleep_range(1000, 1000);
    // wake bt fw
wake_retry:
    if (amlbt_intf_fw_pmu_sleep_get() == TRUE)
    {
        usleep_range(1000, 1000);
        ret = amlbt_intf_wake_fw();
        if (ret != 0)
        {
            goto error;
        }
    }
    wait_cnt = 0;
    //fw will clear bit after wake done
    do
    {
        ret = amlbt_intf_read_word(RG_AON_A17, &reg_value);
        if (ret == -1)
        {
            goto error;
        }
        key_value = (reg_value >> 5) & 0x3;
        wake_value = (reg_value >> 29) & 0x1;
        BTI("reg value: %#x, key value:%#x, wake value:%#x\n", reg_value, key_value, wake_value);
        usleep_range(10000, 10000);
        if (wait_cnt++ > 5)//wait 50ms
        {
            BTE("%s wake fw failed\n", __func__);
            if (retry_cnt++ < 3)
                goto wake_retry;
            break;
        }
    } while (wake_value);
#ifdef  CONFIG_AMLOGIC_GX_SUSPEND
    if ((get_resume_method() == BT_WAKEUP))
    {
        if (!key_value)
        {
            BTE("%s bt wake no write key value\n", __func__);
        }
    }
#endif
    return ret;
error:
    return ret;
}

int amlbt_intf_resume(amlbt_t *p_bt)
{
    int ret = 0;

    BTI("%s start, dr_state:%#x\n", __func__, p_bt->pm_res.dr_state);
    BTI("[%#x,%#x,%#x,%#x]\n", p_bt->bt_res.bt_start, p_bt->zigbee_res.zigbee_start,
            p_bt->thread_res.thread_start, p_bt->pm_res.dr_state);
    BTI("g_wifi_pm.bus_suspend_cnt:%#x\n", atomic_read(&g_wifi_pm.bus_suspend_cnt));
    BTI("g_wifi_pm.drv_suspend_cnt:%#x\n", atomic_read(&g_wifi_pm.drv_suspend_cnt));
    BTI("bus_state_detect.usb_disconnect:%#x\n", bus_state_detect.usb_disconnect);
    BTI("WAKE REASON %d\n", get_resume_method());

    if (p_bt->bt_res.bt_start || p_bt->zigbee_res.zigbee_start || p_bt->thread_res.thread_start)
    {
        amlbt_intf_drv_state_set(BT_DRV_STATE_RESUME);
        amlbt_intf_drv_state_clr(BT_DRV_STATE_SUSPEND);
        if (amlbt_intf_rw_get() == INTF_UART || p_bt->diag_res.chip_family_id == CHIP_W1D)
        {
            if (amlbt_intf_rw_get() == INTF_UART)
            {
                amlbt_intf_register_wakeup_gpio(p_bt);
                amlbt_intf_wake_fw_gpio(p_bt);
            }
#ifdef CONFIG_AMLOGIC_GX_SUSPEND
            BTI("get_resume_method %d, %d\n", get_resume_method(), BT_WAKEUP);
            if (get_resume_method() != BT_WAKEUP &&
                    get_resume_method() != REMOTE_CUS_WAKEUP &&
                        get_resume_method() != REMOTE_WAKEUP)
            {
                p_bt->pm_res.irq_handle = 1;
            }
#endif
            amlbt_intf_drv_state_clr(BT_DRV_STATE_RESUME);
            if (amlbt_intf_rw_get() == INTF_USB && p_bt->pm_res.irq == -1)
            {
                amlbt_intf_register_interrupt_gpio(p_bt);
            }
        }
        else
        {
            if (p_bt->excp_res.notify_trig)
            {
                BTF("%s module recovery skip resume: trig %#x recy %#x\n", __func__, \
                                p_bt->excp_res.notify_trig, p_bt->excp_res.notify_recy);
                return 0;
            }
            if ((atomic_read(&g_wifi_pm.bus_suspend_cnt) != 0) || (atomic_read(&g_wifi_pm.drv_suspend_cnt) != 0)
                || (bus_state_detect.usb_disconnect != 0))
            {
                BTF("%s failed, start resume work!\n", __func__);
                amlbt_intf_queue_work(p_bt->pm_res.resume_wq, &p_bt->pm_res.resume_work);
            }
            else if (!(p_bt->pm_res.dr_state & BT_DRV_STATE_RECOVERY))
            {
#ifdef  CONFIG_AMLOGIC_GX_SUSPEND
                BTI("get_resume_method %d, %d\n", get_resume_method(), BT_WAKEUP);
                if (((get_resume_method() != REMOTE_WAKEUP) && (get_resume_method() != BT_WAKEUP))
                                    && (get_resume_method() != REMOTE_CUS_WAKEUP))
                {
                    p_bt->pm_res.irq_handle = 1;
                }
#endif
                ret = amlbt_intf_resume_fw(p_bt);
                if (ret == -1)
                {
                    BTE("%s:%d resume fw failed! \n", __func__, __LINE__);
                    ret = 0; //SWPL-222378 use-after-free
                }
                amlbt_intf_drv_state_clr(BT_DRV_STATE_RESUME);
                if (amlbt_intf_rw_get() == INTF_USB && p_bt->pm_res.irq == -1)
                {
                    amlbt_intf_register_interrupt_gpio(p_bt);
                }
                //amlbt_clear_rclist_from_firmware();
            }
        }

        BTI("%s end\n", __func__);
    }
    return ret;
}

void amlbt_intf_shutdown(amlbt_t *p_bt)
{
    BTI("%s, %d \n", __func__, p_bt->common_res.shutdown_value);
    amlbt_write_rclist_to_firmware();
}

void amlbt_intf_remove(amlbt_t *p_bt)
{
    BTI("%s \n", __func__);
    if (amlbt_intf_rw_get() == INTF_SDIO)
    {
        amlbt_intf_sdio_unregister();
    }
}

static void amlbt_intf_resume_work(struct work_struct *work)
{
    int ret = 0;
    int wait_cnt = 0;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    BTI("%s %#x,%#x\n", __func__, p_bt->bt_res.bt_start, p_bt->pm_res.dr_state);
    BTI("g_wifi_pm.bus_suspend_cnt:%#x\n", atomic_read(&g_wifi_pm.bus_suspend_cnt));
    BTI("g_wifi_pm.drv_suspend_cnt:%#x\n", atomic_read(&g_wifi_pm.drv_suspend_cnt));
    BTI("bus_state_detect.usb_disconnect:%#x\n", bus_state_detect.usb_disconnect);
    BTI("WAKE REASON %d\n", get_resume_method());

    while ((atomic_read(&g_wifi_pm.bus_suspend_cnt) != 0) || (atomic_read(&g_wifi_pm.drv_suspend_cnt) != 0)
        || (bus_state_detect.usb_disconnect != 0))
    {
        usleep_range(10000, 10000);
        wait_cnt++;
        if (wait_cnt > 100)
        {
            BTF("wifi resume failed!!!!, %#x,%#x,%#x\n", atomic_read(&g_wifi_pm.bus_suspend_cnt),
                atomic_read(&g_wifi_pm.drv_suspend_cnt), bus_state_detect.usb_disconnect);
            break ;
        }
    }

    amlbt_intf_drv_state_set(BT_DRV_STATE_RESUME);
    amlbt_intf_drv_state_clr(BT_DRV_STATE_SUSPEND);
#ifdef  CONFIG_AMLOGIC_GX_SUSPEND
    if (((get_resume_method() != REMOTE_WAKEUP) && (get_resume_method() != BT_WAKEUP))
                        && (get_resume_method() != REMOTE_CUS_WAKEUP))
    {
        p_bt->pm_res.irq_handle = 1;
    }
#endif
    ret = amlbt_intf_resume_fw(p_bt);
    if (ret == -1)
    {
        BTE("%s:%d resume fw failed! \n", __func__, __LINE__);
    }
    amlbt_intf_drv_state_clr(BT_DRV_STATE_RESUME);
    if (amlbt_intf_rw_get() == INTF_USB && p_bt->pm_res.irq == -1)
    {
        amlbt_intf_register_interrupt_gpio(p_bt);
    }
    amlbt_intf_queue_work(p_bt->common_res.write_work_wq, &p_bt->common_res.write_work);
    BTI("%s end\n", __func__);
}

static void amlbt_intf_early_suspend(struct early_suspend *h)
{
    BTI("%s \n", __func__);
}

static void amlbt_intf_later_resume(struct early_suspend *h)
{
    int wait_cnt = 0;

    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    BTI("%s start, dr_state:%#x\n", __func__, p_bt->pm_res.dr_state);
    BTI("[%#x,%#x,%#x,%#x]\n", p_bt->bt_res.bt_start, p_bt->zigbee_res.zigbee_start,
            p_bt->thread_res.thread_start, p_bt->pm_res.dr_state);

    p_bt->pm_res.irq_handle = 0;
    if (p_bt->bt_res.bt_start || p_bt->zigbee_res.zigbee_start || p_bt->thread_res.thread_start)
    {
        if (amlbt_intf_rw_get() == INTF_UART || p_bt->diag_res.chip_family_id == CHIP_W1D)
        {
            amlbt_intf_vendor_write_later_resume(p_bt);
        }
        else
        {
            if (!(p_bt->pm_res.dr_state & BT_DRV_STATE_RECOVERY))
            {
                while (p_bt->pm_res.dr_state & BT_DRV_STATE_RESUME)
                {
                    usleep_range(10000, 10000); //wait resume 1s
                    wait_cnt++;
                    if (wait_cnt > 100)
                    {
                        BTW("%s:%d amlbt_lateresume timeout!\n", __func__, __LINE__);
                        break;
                    }
                }
                amlbt_common_reg_bit_clr(RG_AON_A24, 26);
            }
        }
    }

    BTI("%s end\n", __func__);
}

void amlbt_intf_register_early_suspend(amlbt_t *p_bt, struct platform_device *dev)
{
    BTI("%s \n", __func__);

    p_bt->pm_res.early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
    p_bt->pm_res.early_suspend.suspend = amlbt_intf_early_suspend;
    p_bt->pm_res.early_suspend.resume = amlbt_intf_later_resume;
    p_bt->pm_res.early_suspend.param = dev;
    register_early_suspend(&p_bt->pm_res.early_suspend);
}

void amlbt_intf_unregister_early_suspend(amlbt_t *p_bt, struct platform_device *dev)
{
    BTI("%s \n", __func__);
    unregister_early_suspend(&p_bt->pm_res.early_suspend);
}

static void amlbt_intf_write_work(struct work_struct *work)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    if (atomic_read(&g_wifi_pm.drv_suspend_cnt) != 0)
    {
        BTE("wifi drv_suspend_cnt not ready!, %#x\n", atomic_read(&g_wifi_pm.drv_suspend_cnt));
        return;
    }

    if (p_bt->pm_res.dr_state != 0)
    {
        BTE("amlbt_write_work pm_res.dr_state:%#x\n", p_bt->pm_res.dr_state);
        return ;
    }

    if (amlbt_intf_bt_get() == BT_INTF_DRIVER_USB)
    {
        amlbt_intf_usb_write_work(p_bt);
    }
    else if (amlbt_intf_bt_get() == BT_INTF_DRIVER_TTY)
    {
        amlbt_intf_uart_write_work(p_bt);
    }
}

static void amlbt_intf_res_deinit(amlbt_t *p_bt)
{
    unsigned int st_reg = 0;
    BTI("%s \n", __func__);

    if (p_bt->bt_res.hw_error_skb)
    {
        kfree_skb(p_bt->bt_res.hw_error_skb);
        p_bt->bt_res.hw_error_skb = NULL;
    }
    if (p_bt->zigbee_res.hw_error_skb)
    {
        kfree_skb(p_bt->zigbee_res.hw_error_skb);
        p_bt->zigbee_res.hw_error_skb = NULL;
    }
    if (p_bt->thread_res.hw_error_skb)
    {
        kfree_skb(p_bt->thread_res.hw_error_skb);
        p_bt->thread_res.hw_error_skb = NULL;
    }

    if (p_bt->pm_res.wake_work_wq != NULL)
    {
        amlbt_intf_flush_workqueue(p_bt->pm_res.wake_work_wq, &p_bt->pm_res.wake_work);
        destroy_workqueue(p_bt->pm_res.wake_work_wq);
        p_bt->pm_res.wake_work_wq = NULL;
    }

    if (p_bt->common_res.write_work_wq != NULL)
    {
        amlbt_intf_flush_workqueue(p_bt->common_res.write_work_wq, &p_bt->common_res.write_work);
        destroy_workqueue(p_bt->common_res.write_work_wq);
        p_bt->common_res.write_work_wq = NULL;
    }

    if (p_bt->usb_res.rx_work_wq != NULL)
    {
        amlbt_intf_flush_workqueue(p_bt->usb_res.rx_work_wq, &p_bt->usb_res.rx_work);
        destroy_workqueue(p_bt->usb_res.rx_work_wq);
        p_bt->usb_res.rx_work_wq = NULL;
    }

    if (p_bt->pm_res.resume_wq != NULL)
    {
        amlbt_intf_flush_workqueue(p_bt->pm_res.resume_wq, &p_bt->pm_res.resume_work);
        destroy_workqueue(p_bt->pm_res.resume_wq);
        p_bt->pm_res.resume_wq = NULL;
    }

    if (p_bt->excp_res.exception_work_wq != NULL)
    {
        amlbt_intf_flush_workqueue(p_bt->excp_res.exception_work_wq, &p_bt->excp_res.exception_work);
        destroy_workqueue(p_bt->excp_res.exception_work_wq);
        p_bt->excp_res.exception_work_wq = NULL;
    }

    if (p_bt->usb_res.usb_rx_buf != NULL)
    {
        kfree(p_bt->usb_res.usb_rx_buf);
        p_bt->usb_res.usb_rx_buf = NULL;
    }

    if (amlbt_intf_rw_get() == INTF_USB)
    {
        if (!(p_bt->pm_res.dr_state & BT_DRV_STATE_RECOVERY))
        {
            amlbt_intf_read_word(p_bt->fw_res.driver_fw_status_reg, &st_reg);
            st_reg |= SRAM_FD_INIT_FLAG;
            amlbt_intf_write_word(p_bt->fw_res.driver_fw_status_reg, st_reg);
        }
        amlbt_intf_fifo_deinit(&p_bt->usb_res.fw_type_fifo, p_bt, p_bt->fw_res.rx_type_r, p_bt->fw_res.rx_type_w);
        amlbt_intf_fifo_deinit(&p_bt->usb_res.fw_evt_fifo, p_bt, p_bt->fw_res.evt_r, p_bt->fw_res.evt_w);
        amlbt_intf_fifo_deinit(&p_bt->usb_res.fw_data_fifo, p_bt, p_bt->fw_res.rx_q_r, p_bt->fw_res.rx_q_w);
        amlbt_intf_fifo_deinit(&p_bt->usb_res._15p4_tx_fifo, p_bt, p_bt->fw_res._15p4_tx_r, p_bt->fw_res._15p4_tx_w);
        amlbt_intf_fifo_deinit(&p_bt->usb_res._15p4_rx_fifo, p_bt, p_bt->fw_res._15p4_rx_r, p_bt->fw_res._15p4_rx_w);
        amlbt_intf_fifo_deinit(&p_bt->usb_res.tx_cmd_fifo, p_bt, p_bt->fw_res.cmd_r, p_bt->fw_res.cmd_w);

        if (!(p_bt->pm_res.dr_state & BT_DRV_STATE_RECOVERY))
        {
            st_reg &= ~(SRAM_FD_INIT_FLAG);
            amlbt_intf_write_word(p_bt->fw_res.driver_fw_status_reg, st_reg);
        }
    }
    skb_queue_purge(&p_bt->common_res.tx_queue);
    skb_queue_purge(&p_bt->bt_res.bt_rx_queue);
    skb_queue_purge(&p_bt->zigbee_res.zigbee_rx_queue);
    skb_queue_purge(&p_bt->thread_res.thread_rx_queue);
    skb_queue_purge(&p_bt->diag_res.diag_queue);
    p_bt->pm_res.dr_state = 0;
    if (amlbt_intf_rw_get() == INTF_UART)
    {
        p_bt->uart_res.sw_op = 0;
    }
    BTI("%s finished \n", __func__);
}

static int amlbt_intf_res_init(amlbt_t *p_bt)
{
    unsigned int i = 0;
    int ret;
    unsigned int st_reg = 0;
    unsigned int tx_info[USB_TX_Q_NUM * 4] = {0};
    unsigned char hw_error_evt[4] = {0x04, 0x10, 0x01, 0x00};
    unsigned char zigbee_hw_error[8] = {0x10, 0xf5, 0x42, 0x01, 0x00, 0x00, 0x00, 0x00};
    unsigned char thread_hw_error[8] = {0x10, 0xfa, 0x42, 0x01, 0x00, 0x00, 0x00, 0x00};

    BTI("%s \n", __func__);

    init_completion(&p_bt->excp_res.notify_comp);
    p_bt->excp_res.notify_trig = 0;

    p_bt->conf_res.antenna = 2;
    p_bt->conf_res.fw_mode = 1;
    p_bt->conf_res.bt_sink = 0;
    p_bt->conf_res.pin_mux = 0;
    p_bt->conf_res.br_digit_gain = 66;
    p_bt->conf_res.edr_digit_gain = 98;
    p_bt->conf_res.fw_log = 0;
    p_bt->conf_res.driver_log = 3;
    p_bt->conf_res.factory = 0;
    p_bt->conf_res.system = 1;
    p_bt->conf_res.manf_cnt = 0;

    p_bt->pm_res.irq = -1;
    p_bt->pm_res.irq_handle = 0;
    p_bt->pm_res.dr_state = 0;

    p_bt->uart_res.wake_gpio = -1;
    p_bt->uart_res.sw_op = 0;
    INIT_LIST_HEAD(&p_bt->uart_res.hci_pending_list);

    p_bt->diag_res.sink_mode = 0;
    p_bt->common_res.recovery_value= 0;
    p_bt->common_res.shutdown_value = 0;
    p_bt->usb_res.usb_irq_task_quit = 0;

    p_bt->diag_res.flush_skb = 0;
    p_bt->excp_res.notify_recy = 0;

    p_bt->diag_res.fw_interrupt_cnt = 0;
    p_bt->diag_res.current_time = 0;
    p_bt->diag_res.acl_cnt = 0;
    p_bt->diag_res.cmd_cnt = 0;

    memset(&p_bt->uart_res.rx, 0, sizeof(p_bt->uart_res.rx));
    INIT_WORK(&p_bt->pm_res.resume_work, amlbt_intf_resume_work);
    INIT_WORK(&p_bt->pm_res.wake_work, amlbt_intf_wake_func);
    INIT_WORK(&p_bt->usb_res.rx_work, amlbt_intf_usb_rx_work);
    INIT_WORK(&p_bt->common_res.write_work, amlbt_intf_write_work);
    INIT_WORK(&p_bt->excp_res.exception_work, amlbt_intf_uart_exception_func);

    skb_queue_head_init(&p_bt->diag_res.diag_queue);

    init_waitqueue_head(&p_bt->bt_res.bt_wait_queue);
    skb_queue_head_init(&p_bt->bt_res.bt_rx_queue);
    init_waitqueue_head(&p_bt->zigbee_res.zigbee_wait_queue);
    skb_queue_head_init(&p_bt->zigbee_res.zigbee_rx_queue);
    init_waitqueue_head(&p_bt->thread_res.thread_wait_queue);
    skb_queue_head_init(&p_bt->thread_res.thread_rx_queue);

    skb_queue_head_init(&p_bt->common_res.tx_queue);

    mutex_init(&p_bt->diag_res.bt_debug_mutex);
    hrtimer_init(&p_bt->usb_res.poll_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    p_bt->common_res.res_init = 1;
    p_bt->pm_res.resume_wq = create_singlethread_workqueue("resume_wq");
    if (!p_bt->pm_res.resume_wq)
    {
        BTE("%s:%d resume_wq create Failed\n", __func__, __LINE__);
        return -1;
    }
    p_bt->usb_res.rx_work_wq = create_singlethread_workqueue("rx_work_wq");
    if (!p_bt->usb_res.rx_work_wq)
    {
        BTE("%s:%d rx_work_wq create Failed\n", __func__, __LINE__);
        return -1;
    }
    p_bt->common_res.write_work_wq = create_singlethread_workqueue("write_work_wq");
    if (!p_bt->common_res.write_work_wq)
    {
        BTE("%s:%d write_work_wq create Failed\n", __func__, __LINE__);
        return -1;
    }
    p_bt->pm_res.wake_work_wq = create_singlethread_workqueue("wake_work_wq");
    if (!p_bt->pm_res.wake_work_wq)
    {
        BTE("%s:%d wake_work_wq create Failed\n", __func__, __LINE__);
        return -1;
    }
    p_bt->excp_res.exception_work_wq = create_singlethread_workqueue("exception_work_wq");
    if (!p_bt->excp_res.exception_work_wq)
    {
        BTE("%s:%d exception_work_wq create Failed\n", __func__, __LINE__);
        return -1;
    }
    /* coverity[var_assign:SUPPRESS] */
    /* coverity[alloc_fn:SUPPRESS] */
    p_bt->bt_res.hw_error_skb = alloc_skb(sizeof(hw_error_evt), GFP_KERNEL);
    if (!p_bt->bt_res.hw_error_skb)
    {
        BTE("%s:%d alloc_bt_skb Failed\n", __func__, __LINE__);
        goto error;
    }
    /* coverity[noescape:SUPPRESS] */
    if (skb_put_data(p_bt->bt_res.hw_error_skb, hw_error_evt, sizeof(hw_error_evt)) == NULL)
    {
        BTE("%s: skb_put_data failed\n", __func__);
        goto error;
    }
    /* coverity[var_assign:SUPPRESS] */
    /* coverity[alloc_fn:SUPPRESS] */
    p_bt->zigbee_res.hw_error_skb = alloc_skb(sizeof(zigbee_hw_error), GFP_KERNEL);
    if (!p_bt->zigbee_res.hw_error_skb)
    {
        BTE("%s:%d alloc_zigbee_skb Failed\n", __func__, __LINE__);
        goto error;
    }
    /* coverity[noescape:SUPPRESS] */
    if (skb_put_data(p_bt->zigbee_res.hw_error_skb, zigbee_hw_error, sizeof(zigbee_hw_error)) == NULL)
    {
        BTE("%s: skb_put_data failed\n", __func__);
        goto error;
    }
    /* coverity[var_assign:SUPPRESS] */
    /* coverity[alloc_fn:SUPPRESS] */
    p_bt->thread_res.hw_error_skb = alloc_skb(sizeof(thread_hw_error), GFP_KERNEL);
    if (!p_bt->thread_res.hw_error_skb)
    {
        BTE("%s:%d alloc_thread_skb Failed\n", __func__, __LINE__);
        goto error;
    }
    /* coverity[noescape:SUPPRESS] */
    if (skb_put_data(p_bt->thread_res.hw_error_skb, thread_hw_error, sizeof(thread_hw_error)) == NULL)
    {
        BTE("%s: skb_put_data failed\n", __func__);
        goto error;
    }
    if (amlbt_intf_rw_get() == INTF_USB)
    {
        amlbt_intf_read_word(p_bt->fw_res.driver_fw_status_reg, &st_reg);
        st_reg |= SRAM_FD_INIT_FLAG;
        amlbt_intf_write_word(p_bt->fw_res.driver_fw_status_reg, st_reg);

        p_bt->usb_res.usb_rx_buf = kzalloc(p_bt->fw_res.poll_len, GFP_DMA|GFP_ATOMIC);
        if (!p_bt->usb_res.usb_rx_buf)
        {
            BTE("%s:%d usb_rx_buf failed!\n", __func__, __LINE__);
            goto error;
        }
        BTI("usb_rx_buf:%#x\n", (unsigned long)p_bt->usb_res.usb_rx_buf);
        //fw type fifo init
        if (NULL == amlbt_intf_fifo_init(&p_bt->usb_res.fw_type_fifo, p_bt->fw_res.rx_type_len,
            (unsigned char *)p_bt->fw_res.rx_type_addr, p_bt->fw_res.rx_type_r, p_bt->fw_res.rx_type_w))
        {
            BTE("%s:%d fw type fifo init failed!\n", __func__, __LINE__);
            goto error;
        }
        //fw event fifo init
        if (NULL == amlbt_intf_fifo_init(&p_bt->usb_res.fw_evt_fifo, p_bt->fw_res.evt_len,
            (unsigned char *)p_bt->fw_res.evt_addr, p_bt->fw_res.evt_r, p_bt->fw_res.evt_w))
        {
            BTE("%s:%d fw event fifo init failed!\n", __func__, __LINE__);
            goto error;
        }
        //fw data fifo init
        if (NULL == amlbt_intf_fifo_init(&p_bt->usb_res.fw_data_fifo, p_bt->fw_res.rx_q_len,
            (unsigned char *)p_bt->fw_res.rx_q_addr, p_bt->fw_res.rx_q_r, p_bt->fw_res.rx_q_w))
        {
            BTE("%s:%d fw data fifo init failed!\n", __func__, __LINE__);
            goto error;
        }

        //tx hci cmd fifo init
        if (NULL == amlbt_intf_fifo_init(&p_bt->usb_res.tx_cmd_fifo, p_bt->fw_res.cmd_len,
                (unsigned char *)p_bt->fw_res.cmd_addr, p_bt->fw_res.cmd_r, p_bt->fw_res.cmd_w))
        {
            BTE("%s:%d tx hci cmd fifo init failed!\n", __func__, __LINE__);
            goto error;
        }

        for (i = 0; i < USB_TX_Q_NUM; i++)
        {
            p_bt->usb_res.tx_q[i].tx_q_addr = (p_bt->fw_res.tx_q_addr + i * USB_TX_Q_LEN);

            p_bt->usb_res.tx_q[i].tx_q_prio_addr = (p_bt->fw_res.tx_q_prio_addr + i * 16);
            p_bt->usb_res.tx_q[i].tx_q_dev_index_addr = (p_bt->usb_res.tx_q[i].tx_q_prio_addr + 4);
            p_bt->usb_res.tx_q[i].tx_q_status_addr = (p_bt->usb_res.tx_q[i].tx_q_dev_index_addr + 4);

            p_bt->usb_res.tx_q[i].tx_q_dev_index = 0;
            p_bt->usb_res.tx_q[i].tx_q_prio = USB_TX_Q_MAX_PRIO;
            p_bt->usb_res.tx_q[i].tx_q_status = GDSL_TX_Q_UNUSED;
            tx_info[i*4] = p_bt->usb_res.tx_q[i].tx_q_prio;
            tx_info[i*4+1] = p_bt->usb_res.tx_q[i].tx_q_dev_index;
            tx_info[i*4+2] = p_bt->usb_res.tx_q[i].tx_q_status;
            BTP("tx_addr:%#x,%#x,%#x\n", p_bt->usb_res.tx_q[i].tx_q_prio_addr,
                p_bt->usb_res.tx_q[i].tx_q_dev_index_addr,
                p_bt->usb_res.tx_q[i].tx_q_status_addr);
        }
        ret = amlbt_intf_write_sram((unsigned char *)tx_info, p_bt->fw_res.tx_q_prio_addr, sizeof(tx_info));
        if (ret != 0)
        {
            goto error;
        }
        //15.4 fifo init
        if (NULL == amlbt_intf_fifo_init(&p_bt->usb_res._15p4_tx_fifo, p_bt->fw_res._15p4_tx_len,
            (unsigned char *)p_bt->fw_res._15p4_tx_addr, p_bt->fw_res._15p4_tx_r, p_bt->fw_res._15p4_tx_w))
        {
            BTE("%s:%d fw 15.4 tx fifo init failed!\n", __func__, __LINE__);
            goto error;
        }
        BTI("15p4 init tx r %#x\n", (unsigned long)p_bt->usb_res._15p4_tx_fifo->r);
        BTI("15p4 init tx w %#x\n", (unsigned long)p_bt->usb_res._15p4_tx_fifo->w);
        if (NULL == amlbt_intf_fifo_init(&p_bt->usb_res._15p4_rx_fifo, p_bt->fw_res._15p4_rx_len,
            (unsigned char *)p_bt->fw_res._15p4_rx_addr, p_bt->fw_res._15p4_rx_r, p_bt->fw_res._15p4_rx_w))
        {
            BTE("%s:%d fw 15.4 rx fifo init failed!\n", __func__, __LINE__);
            goto error;
        }

        st_reg &= ~(SRAM_FD_INIT_FLAG);
        amlbt_intf_write_word(p_bt->fw_res.driver_fw_status_reg, st_reg);
    }
    return 0;
error:
    amlbt_intf_res_deinit(p_bt);
    return -1;
}

#if 0
int amlbt_sw_reset(void)
{
    int ret = 0;
    ret = amlbt_intf_write_word(REG_DEV_RESET, ((BIT_PHY|BIT_MAC|BIT_CPU)<<16)|(BIT_PHY|BIT_MAC|BIT_CPU));
    if (ret != 0)
    {
        BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
        goto error;
    }
    usleep_range(1000, 1000);
    ret = amlbt_intf_write_word(REG_DEV_RESET, ((BIT_CPU)<<16)|(BIT_CPU));
    if (ret != 0)
    {
        BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
        goto error;
    }
    return ret;
error:
    return ret;
}
#endif

#if defined(CONFIG_AML_BT_CHIP_W2L)
static int amlbt_intf_powersave_clear(void)
{
    // set bt open flag
    amlbt_common_reg_bit_set(RG_AON_A24, 24);

    // clear shutdown bit
    amlbt_common_reg_bit_clr(RG_AON_A16, 28);

    // clear suspend bit
    amlbt_common_reg_bit_clr(RG_AON_A24, 26);

    return 0;
}
#endif

void amlbt_intf_drv_state_set(unsigned int bit)
{
    unsigned int reg_value = 0;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    reg_value = p_bt->pm_res.dr_state;
    BTI("amlbt_intf_drv_state_set %#x: %#x\n", reg_value, bit);
    reg_value |= bit;
    BTI("amlbt_intf_drv_state_set end %#x: %#x", reg_value, bit);
    p_bt->pm_res.dr_state = reg_value;
    if (bit == BT_DRV_STATE_RECOVERY)
    {
        if (!p_bt->common_res.recovery_value)
        {
            p_bt->common_res.recovery_value = BT_DRV_STATE_RECOVERY;
        }
    }
}

void amlbt_intf_drv_state_clr(unsigned int bit)
{
    unsigned int reg_value = 0;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    reg_value = p_bt->pm_res.dr_state;
    BTD("amlbt_intf_drv_state_clr %#x: %#x\n", reg_value, bit);
    reg_value &= ~bit;
    BTD("amlbt_intf_drv_state_clr end %#x: %#x", reg_value, bit);
    p_bt->pm_res.dr_state = reg_value;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
int amlbt_intf_bind_bus(amlbt_t *p_bt, struct device *consumer, struct device *supplier, u32 flags)
{
    if (p_bt->pm_res.link == NULL)
    {
        p_bt->pm_res.link = device_link_add(consumer, supplier, flags);
        if (p_bt->pm_res.link == NULL)
        {
            BTE("Failed to create device link");
            return -ENOMEM;
        }
        else
        {
            BTI("Success to create device link");
        }
    }
    else
    {
        BTI("p_bt->pm_res.link is ready %#x \n", p_bt->pm_res.link);
    }
    return 0;
}

static struct device_link *amlbt_intf_find_device_link(struct device *consumer, struct device *supplier)
{
    struct device_link *link;

    list_for_each_entry(link, &consumer->links.suppliers, c_node)
    {
        if (link->supplier == supplier)
        {
            return link;
        }
    }

    return NULL;
}

void amlbt_intf_unbind_bus(amlbt_t *p_bt, struct device *consumer, struct device *supplier)
{
    struct device_link *link = NULL;

    if (p_bt->pm_res.link != NULL)
    {
        if (device_is_registered(supplier))
        {
            link = amlbt_intf_find_device_link(consumer, supplier);
            BTI("amlbt_intf_find_device_link : %#x", (unsigned long)link);
        }
        if (link != NULL && link == p_bt->pm_res.link)
        {
            device_link_del(p_bt->pm_res.link);
            BTI("Success to del device link");
        }
        p_bt->pm_res.link = NULL;
    }
}
#endif


static unsigned int amlbt_intf_coex_is_running(amlbt_t *p_bt)
{
    BTI("%s, %#x, %#x, %#x\n", __func__, p_bt->bt_res.bt_start, p_bt->zigbee_res.zigbee_start, p_bt->thread_res.thread_start);

    if (p_bt->bt_res.bt_start || p_bt->zigbee_res.zigbee_start || p_bt->thread_res.thread_start)
    {
        return 1;
    }

    return 0;
}

static int amlbt_intf_coex_fops_open(struct inode *inode, struct file *file)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    BTI("%s \n", __func__);

    file->private_data = p_bt;
    return nonseekable_open(inode, file);
}

static int amlbt_intf_coex_fops_close(struct inode *inode, struct file *file)
{
    BTI("%s \n", __func__);

    return 0;
}

/*
RG_AON_A15
bit[30]: bt_en manual control, 1 is manual mode
bit[31]: bt_en manual value.
*/
static void amlbt_intf_power_on(void)
{
    BTI("%s \n", __func__);
#if defined(CONFIG_AML_BT_CHIP_W1D)
    //w1d bt en enable defect, revC rtl fix
    amlbt_common_reg_bit_set(RG_AON_A33, 1);
#endif
    amlbt_common_reg_bit_set(RG_AON_A15, 30); //set bt_en manual control
    amlbt_common_reg_bit_clr(RG_AON_A15, 31); //set bt_en = 0
    usleep_range(10000, 10000);
    amlbt_common_reg_bit_set(RG_AON_A15, 31); //set bt_en = 1
    usleep_range(100000, 100000);
    amlbt_common_reg_bit_clr(RG_AON_A15, 30); //BT Firmware needs Bit 30 and 31 to determine if WiFi is doing calibration
    amlbt_common_reg_bit_clr(RG_AON_A15, 31); //BT Firmware needs Bit 30 and 31 to determine if WiFi is doing calibration

    BTI("%s end\n", __func__);
}

static void amlbt_intf_power_off(void)
{
    BTI("%s \n", __func__);

    amlbt_common_reg_bit_set(RG_AON_A15, 30); //set bt_en manual control
    amlbt_common_reg_bit_clr(RG_AON_A15, 31); //set bt_en = 0
    //usleep_range(100000, 100000);
    BTI("%s end\n", __func__);
}

static int amlbt_intf_ops_open(amlbt_t *p_bt)
{
    int ret = 0;
    if (amlbt_intf_rw_get() == INTF_USB)
    {
        amlbt_intf_power_on();
    }

    if (amlbt_intf_rw_get() == INTF_SDIO)
    {
        amlbt_intf_sdio_register();
    }

    if (amlbt_intf_res_init(p_bt) != 0)
    {
        BTI("amlbt_intf_res_init failed!\n");
        return -1;
    }
    //The USB interface cannot control BT_EN, so the CPU needs to be stopped before downloading the firmware.
    //if (amlbt_intf_get() == INTF_USB)
    //{
    //    ret = amlbt_sw_reset();
    //    if (ret != 0)
    //    {
    //        goto err_exit;
    //    }
    //}
#if defined(CONFIG_AML_BT_CHIP_W2L)
    ret = amlbt_intf_powersave_clear();
    if (ret != 0)
    {
        return -1;
    }
#endif
    amlbt_intf_load_conf(p_bt);
    ret = amlbt_intf_load_firmware(p_bt);
    if (ret != 0)
    {
        BTI("amlbt_intf_load_firmware failed!\n");
        return -1;
    }
    amlbt_wakeup_unlock();
    ret = amlbt_intf_register_interrupt_gpio(p_bt);
    if (ret != 0)
    {
        BTI("irq register failed!\n");
        //return -1;
    }
    if (amlbt_intf_rw_get() == INTF_UART)
    {
        ret = amlbt_intf_register_wakeup_gpio(p_bt);
        if (ret != 0)
        {
            BTI("wakeup register failed!\n");
            //return -1;
        }
    }
    return 0;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
int amlbt_intf_ops_bind_bus(amlbt_t *p_bt)
{
    int ret = 0;
    if (amlbt_intf_rw_get() == INTF_USB)
    {
        if (g_udev != NULL)
        {
            ret = amlbt_intf_bind_bus(p_bt, &amlbt_device.dev, &g_udev->dev, DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS);
            if (ret != 0)
            {
                return -1;
            }
        }
        else
        {
            BTE("g_udev is NULL");
            return -1;
        }
    }
    else if (amlbt_intf_rw_get() == INTF_SDIO || amlbt_intf_rw_get() == INTF_UART)
    {
        if (aml_priv_to_func(7) != NULL)
        {
            ret = amlbt_intf_bind_bus(p_bt, &amlbt_device.dev, &aml_priv_to_func(7)->dev, DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS);
            if (ret != 0)
            {
                return -1;
            }
        }
        else
        {
            BTE("aml_priv_to_func(7) is NULL");
            return -1;
        }
    }
    else if (amlbt_intf_rw_get() == INTF_PCIE)
    {
#ifdef CONFIG_AML_BT_CHIP_W2
        if (g_aml_plat_pci != NULL && g_aml_plat_pci->pci_dev != NULL)
        {
            ret = amlbt_intf_bind_bus(p_bt, &amlbt_device.dev, &g_aml_plat_pci->pci_dev->dev, DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS);
            if (ret != 0)
            {
                return -1;
            }
        }
        else
        {
            BTE("g_aml_plat_pci or g_aml_plat_pci->pci_dev is NULL");
            return -1;
        }
#else
        BTE("%s error, not w2 !\n", __func__);
#endif
    }
    return 0;
}

void amlbt_intf_ops_unbind_bus(amlbt_t *p_bt)
{
    if (amlbt_intf_rw_get() == INTF_USB)
    {
        if (g_udev != NULL)
        {
            amlbt_intf_unbind_bus(p_bt, &amlbt_device.dev, &g_udev->dev);
        }
        else
        {
            BTE("g_udev is NULL");
        }
    }
    else if (amlbt_intf_rw_get() == INTF_SDIO || amlbt_intf_rw_get() == INTF_UART)
    {
        if (aml_priv_to_func(7) != NULL)
        {
            amlbt_intf_unbind_bus(p_bt, &amlbt_device.dev, &aml_priv_to_func(7)->dev);
        }
        else
        {
            BTE("aml_priv_to_func(7) is NULL");
        }
    }
    else if (amlbt_intf_rw_get() == INTF_PCIE)
    {
#ifdef CONFIG_AML_BT_CHIP_W2
        if (g_aml_plat_pci != NULL && g_aml_plat_pci->pci_dev != NULL)
        {
            amlbt_intf_unbind_bus(p_bt, &amlbt_device.dev, &g_aml_plat_pci->pci_dev->dev);
        }
        else
        {
            BTE("g_aml_plat_pci or g_aml_plat_pci->pci_dev is NULL");
        }
#else
        BTE("%s error, not w2 !\n", __func__);
#endif
    }
}
#endif

void amlbt_intf_flush_workqueue(struct workqueue_struct *wq, struct work_struct *work)
{
    if (wq != NULL && work != NULL)
    {
        flush_workqueue(wq);
    }
    else
    {
        BTE("%s:%d Failed workqueue is NULL:\n", __func__, __LINE__);
    }
}

void amlbt_intf_queue_work(struct workqueue_struct *wq , struct work_struct *work)
{
    if (wq == NULL)
    {
        BTE("%s:%d Failed workqueue is NULL:\n", __func__, __LINE__);
    }
    else
    {
        queue_work(wq, work);
    }
}

void amlbt_intf_ops_close(amlbt_t *p_bt)
{
    amlbt_intf_fw_info();
    if (amlbt_intf_rw_get() == INTF_USB)
    {
        p_bt->usb_res.usb_irq_task_quit = 1;
    }
    amlbt_intf_unregister_interrupt_gpio(p_bt);
    if (amlbt_intf_rw_get() == INTF_UART)
    {
        amlbt_intf_unregister_wakeup_gpio(p_bt);
    }
#if defined(CONFIG_AML_BT_CHIP_W2L) // || defined(CONFIG_AML_BT_CHIP_W1D)
    //bug fix, WIRELESS-10963, Solve the problem that fw cannot run after downloading
    amlbt_common_reg_bit_clr(RG_AON_A24, 26);//wake up firmware, make sure firmware running
#endif
    amlbt_write_rclist_to_firmware(); //shutdown write rc list advance
    amlbt_intf_res_deinit(p_bt);
    if (!p_bt->common_res.shutdown_value)
    {
        if (amlbt_intf_rw_get() == INTF_USB)
        {
            amlbt_intf_power_off();
        }
        //atomic_set(&g_wifi_pm.bt_enable, 0);
    }
    else
    {
#if defined(CONFIG_AML_BT_CHIP_W1D)
        amlbt_common_reg_bit_set(RG_AON_A94, 26);//set shutdown reg bit
        amlbt_intf_write_word(REG_INTERRUPT_SW_SET, 0x1); // wake bt interrupt
#endif
    }
}

static int amlbt_intf_fops_open(struct inode *inode, struct file *file, enum fops_mode_t mode)
{
    int ret = 0;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    unsigned char fops_start = 0;

    amlbt_intf_version();
    BTI("%s, %d, fops_mode:%d\n", __func__, amlbt_ft_mode, mode);

    file->private_data = p_bt;

    if (amlbt_intf_rw_get() == INTF_USB)
    {
        ret = amlbt_intf_usb_check();
        if (ret != 0)
        {
            goto err_exit;
        }
    }

    if (mode == INTF_FOPS_BT)
    {
        fops_start = p_bt->bt_res.bt_start;
    }
    else if (mode == INTF_FOPS_ZIGBEE)
    {
        fops_start = p_bt->zigbee_res.zigbee_start;
    }
    else if (mode == INTF_FOPS_THREAD)
    {
        fops_start = p_bt->thread_res.thread_start;
    }

    if (amlbt_ft_mode && fops_start)
    {
        BTI("%s FT MODE", __func__);
        return nonseekable_open(inode, file);
    }
    else
    {
        if (!amlbt_intf_coex_is_running(p_bt))
        {
            if (amlbt_intf_ops_open(p_bt) != 0)
            {
                goto err_buf;
            }
        }
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
        if (amlbt_intf_ops_bind_bus(p_bt) != 0)
        {
            goto err_buf;
        }
#endif
        if (mode == INTF_FOPS_BT)
        {
            p_bt->bt_res.bt_start = 1;
        }
        else if (mode == INTF_FOPS_ZIGBEE)
        {
            p_bt->zigbee_res.zigbee_start = 1;
        }
        else if (mode == INTF_FOPS_THREAD)
        {
            p_bt->thread_res.thread_start = 1;
        }
        //atomic_set(&g_wifi_pm.bt_enable, 1);
        return nonseekable_open(inode, file);
err_buf:
        amlbt_intf_unregister_interrupt_gpio(p_bt);
        amlbt_intf_res_deinit(p_bt);
err_exit:
        return nonseekable_open(inode, file);
    }
}

static int amlbt_intf_fops_close(struct inode *inode, struct file *file, enum fops_mode_t mode)
{
    amlbt_t *p_bt = (amlbt_t *)file->private_data;
    unsigned char start_status_1 = 0;
    unsigned char start_status_2 = 0;
    unsigned long timeout = msecs_to_jiffies(15000);

    amlbt_intf_version();
    BTI("%s, %d, fops_mode:%d\n", __func__, amlbt_ft_mode, mode);

    if (!amlbt_ft_mode)
    {
        amlbt_intf_diag_rx_remain_print(p_bt);
        if (p_bt->excp_res.notify_trig)
        {
            if (!completion_done(&p_bt->excp_res.notify_comp))
            {
                BTI("Waiting for exception task to finish...\n");
                if (!wait_for_completion_timeout(&p_bt->excp_res.notify_comp, timeout))
                {
                    BTE("Exception task timeout after %d ms\n", timeout);
                }
            }
        }

        if (mode == INTF_FOPS_BT)
        {
            start_status_1 = p_bt->zigbee_res.zigbee_start;
            start_status_2 = p_bt->thread_res.thread_start;
        }
        else if (mode == INTF_FOPS_ZIGBEE)
        {
            start_status_1 = p_bt->bt_res.bt_start;
            start_status_2 = p_bt->thread_res.thread_start;
        }
        else if (mode == INTF_FOPS_THREAD)
        {
            start_status_1 = p_bt->bt_res.bt_start;
            start_status_2 = p_bt->zigbee_res.zigbee_start;
        }
        BTI("start_status_1:%d, start_status_2:%d\n", start_status_1, start_status_2);
        if (!start_status_1 && !start_status_2)
        {
#if LINUX_VERSION_CODE > KERNEL_VERSION(4, 10, 0)
            amlbt_intf_ops_unbind_bus(p_bt);
#endif
            amlbt_intf_ops_close(p_bt);
        }

        if (mode == INTF_FOPS_BT)
        {
            p_bt->bt_res.bt_start = 0;
            BTI("bt closed\n");
        }
        else if (mode == INTF_FOPS_ZIGBEE)
        {
            p_bt->zigbee_res.zigbee_start = 0;
            BTI("zigbee closed\n");
        }
        else if (mode == INTF_FOPS_THREAD)
        {
            p_bt->thread_res.thread_start = 0;
            BTI("thread closed\n");
        }
    }

    return 0;
}

static unsigned int amlbt_intf_fops_poll(struct file *file, poll_table *wait, enum fops_mode_t mode)
{
    int mask = 0;
    amlbt_t *p_bt = (amlbt_t *)file->private_data;

    if ((mode == INTF_FOPS_BT) && !p_bt->bt_res.bt_start)
    {
        goto exit;
    }
    else if ((mode == INTF_FOPS_ZIGBEE) && !p_bt->zigbee_res.zigbee_start)
    {
        goto exit;
    }
    else if ((mode == INTF_FOPS_THREAD) && !p_bt->thread_res.thread_start)
    {
        goto exit;
    }

    if (mode == INTF_FOPS_BT)
    {
        poll_wait(file, &p_bt->bt_res.bt_wait_queue, wait);
    }
    else if (mode == INTF_FOPS_ZIGBEE)
    {
        poll_wait(file, &p_bt->zigbee_res.zigbee_wait_queue, wait);
    }
    else if (mode == INTF_FOPS_THREAD)
    {
        poll_wait(file, &p_bt->thread_res.thread_wait_queue, wait);
    }

    if (p_bt->pm_res.dr_state & BT_DRV_STATE_RECOVERY)
    {
        if (mode == INTF_FOPS_BT && p_bt->bt_res.hw_error_skb)
        {
            mask |= POLLIN | POLLRDNORM;
        }
        else if (mode == INTF_FOPS_ZIGBEE && p_bt->zigbee_res.hw_error_skb)
        {
            mask |= POLLIN | POLLRDNORM;
        }
        else if (mode == INTF_FOPS_THREAD && p_bt->thread_res.hw_error_skb)
        {
            mask |= POLLIN | POLLRDNORM;
        }
        goto exit;
    }

    if (amlbt_intf_rw_get() == INTF_USB)
    {
        if ((g_udev == NULL) || bus_state_detect.bus_err || bus_state_detect.bus_reset_ongoing)
        {
            BTF("%s:%d usb error!, %#x,%#x\n", __func__, __LINE__, bus_state_detect.bus_err, bus_state_detect.bus_reset_ongoing);
            goto exit;
        }
    }

    if (mode == INTF_FOPS_BT)
    {
        if (p_bt->bt_res.current_skb)
        {
            mask |= POLLIN | POLLRDNORM;
        }
        else if (skb_queue_len(&p_bt->bt_res.bt_rx_queue) > 0)
        {
            p_bt->bt_res.current_skb = skb_dequeue(&p_bt->bt_res.bt_rx_queue);
            mask |= POLLIN | POLLRDNORM;
            p_bt->diag_res.flush_skb = 1;
        }
    }
    else if (mode == INTF_FOPS_ZIGBEE)
    {
        if (p_bt->zigbee_res.current_skb)
        {
            mask |= POLLIN | POLLRDNORM;
        }
        else if (skb_queue_len(&p_bt->zigbee_res.zigbee_rx_queue) > 0)
        {
            p_bt->zigbee_res.current_skb = skb_dequeue(&p_bt->zigbee_res.zigbee_rx_queue);
            mask |= POLLIN | POLLRDNORM;
            p_bt->diag_res.flush_skb = 1;
        }
    }
    else if (mode == INTF_FOPS_THREAD)
    {
        if (p_bt->thread_res.current_skb)
        {
            mask |= POLLIN | POLLRDNORM;
        }
        else if (skb_queue_len(&p_bt->thread_res.thread_rx_queue) > 0)
        {
            p_bt->thread_res.current_skb = skb_dequeue(&p_bt->thread_res.thread_rx_queue);
            mask |= POLLIN | POLLRDNORM;
            p_bt->diag_res.flush_skb = 1;
        }
    }
exit:
    return mask;
}

static ssize_t amlbt_intf_fops_write(struct file *file_p, const char __user *buf_p, size_t count, loff_t *pos_p, enum fops_mode_t mode)
{
    int i = 0;
    static unsigned char bt_type = 0;
    static unsigned char thread_type = 0;
    static unsigned char zigbee_type = 0;
    amlbt_t *p_bt = (amlbt_t *)file_p->private_data;
    struct sk_buff *skb = NULL;
    unsigned char *p;
    int ret = 0;
    unsigned char fops_start = 0;

    BTD("%s, count:%ld\n", __func__, count);

    if (mode == INTF_FOPS_BT)
    {
        fops_start = p_bt->bt_res.bt_start;
    }
    else if (mode == INTF_FOPS_ZIGBEE)
    {
        fops_start = p_bt->zigbee_res.zigbee_start;
    }
    else if (mode == INTF_FOPS_THREAD)
    {
        fops_start = p_bt->thread_res.thread_start;
    }

    if (!fops_start)
    {
        BTE("%s:%d fops_start == 0, fops_mode:%d!\n", __func__, __LINE__, mode);
        return -EFAULT;
    }

    if (amlbt_intf_bt_get() == BT_INTF_DRIVER_TTY && p_bt->uart_res_linux.hu == NULL)
    {
        BTE("%s:%d p_bt->uart_res.hu == 0!\n", __func__, __LINE__);
        return -EFAULT;
    }

    if (count > HCI_MAX_FRAME_SIZE) {
        BTE("%s:%d count > HCI_MAX_FRAME_SIZE %d, %d!\n",
            __func__, __LINE__, count, HCI_MAX_FRAME_SIZE);
        return -EINVAL;
    }

    if (count == 1)
    {
        if (mode == INTF_FOPS_BT)
        {
            if (get_user(bt_type, buf_p))
            {
              BTE("%s %d get_user failed! %#x \n", __func__, __LINE__, bt_type);
              return -EFAULT;
            }
        }
        else if (mode == INTF_FOPS_ZIGBEE)
        {
            if (get_user(zigbee_type, buf_p))
            {
              BTE("%s %d get_user failed! %#x \n", __func__, __LINE__, zigbee_type);
              return -EFAULT;
            }
        }
        else if (mode == INTF_FOPS_THREAD)
        {
            if (get_user(thread_type, buf_p))
            {
              BTE("%s %d get_user failed! %#x \n", __func__, __LINE__, thread_type);
              return -EFAULT;
            }
        }

        return count;
    }

    /* coverity[var_assign:SUPPRESS] */
    /* coverity[alloc_fn:SUPPRESS] */
    skb = alloc_skb(count + 1, GFP_KERNEL);
    if (!skb) {
        BTE("%s %d alloc_skb failed! \n", __func__, __LINE__);
        return -ENOMEM;
    }

    /* coverity[noescape:SUPPRESS] */
    if (skb_tailroom(skb) < count + 1) {
        BTE("%s skb_tailroom(skb) failed!\n", __func__);
        ret = -ENOSPC;
        goto error;
    }

    if (mode == INTF_FOPS_BT)
    {
        /* coverity[noescape:SUPPRESS] */
        *(unsigned char *)skb_put(skb, 1) = bt_type;
    }
    else if (mode == INTF_FOPS_ZIGBEE)
    {
        /* coverity[noescape:SUPPRESS] */
        *(unsigned char *)skb_put(skb, 1) = zigbee_type;
        BTD("%s zigbee type %#x\n", __func__, zigbee_type);
    }
    else if (mode == INTF_FOPS_THREAD)
    {
        /* coverity[noescape:SUPPRESS] */
        *(unsigned char *)skb_put(skb, 1) = thread_type;
        BTD("%s thread type %#x\n", __func__, thread_type);
    }
    /* coverity[noescape:SUPPRESS] */
    if (copy_from_user(skb_put(skb, count), buf_p, count)) {
        BTE("%s: Failed to get data from user space\n", __func__);
        ret = -EFAULT;
        goto error;
    }
    BTD("%s:buffer:[%#x,%#x,%#x,%#x]\n", __func__, skb->data[0], skb->data[1], skb->data[2], skb->data[3]);
    BTD("%s:buffer:[%#x,%#x,%#x,%#x]\n", __func__, skb->data[4], skb->data[5], skb->data[6], skb->data[7]);

    if (mode == INTF_FOPS_BT)
    {
        p = &skb->data[1];
        if (bt_type == HCI_COMMAND_PKT) {
            if (p[0] == 0x05 && p[1] == 0x14) {  // read rssi
                amlbt_intf_diag_rssi_print(p_bt);
            }
            if (p[0] == 0x1a && p[1] == 0xfc) {
                for (; i < sizeof(p_bt->diag_res.mac_addr); i++) {
                    p_bt->diag_res.mac_addr[i] = p[i+3];
                }
            }
        }
    }
    skb_queue_tail(&p_bt->common_res.tx_queue, skb);
    amlbt_intf_queue_work(p_bt->common_res.write_work_wq, &p_bt->common_res.write_work);

    return count;

error:
    if (skb) {
    /* coverity[noescape:SUPPRESS] */
        kfree_skb(skb);
    }
    /* coverity[leaked_storage:SUPPRESS] */
    return ret;
}

static ssize_t amlbt_intf_fops_read(struct file *file_p, char __user *buf_p, size_t count, loff_t *pos_p, enum fops_mode_t mode)
{
    amlbt_t *p_bt = (amlbt_t *)file_p->private_data;
    struct sk_buff *skb = NULL;
    unsigned int length = 0;
    unsigned char *p;
    static int pt = 0;
    unsigned char pkt_type;
    unsigned char fops_start = 0;
    struct sk_buff *hw_err_skb = NULL;

    if (mode == INTF_FOPS_BT)
    {
        fops_start = p_bt->bt_res.bt_start;
        hw_err_skb = p_bt->bt_res.hw_error_skb;
    }
    else if (mode == INTF_FOPS_ZIGBEE)
    {
        fops_start = p_bt->zigbee_res.zigbee_start;
        hw_err_skb = p_bt->zigbee_res.hw_error_skb;
    }
    else if (mode == INTF_FOPS_THREAD)
    {
        fops_start = p_bt->thread_res.thread_start;
        hw_err_skb = p_bt->thread_res.hw_error_skb;
    }

    if (!fops_start)
    {
        BTE("%s:%d bt_start == 0, fops_mode:%d!\n", __func__, __LINE__, fops_start);
        return 0;
    }

    if (p_bt->pm_res.dr_state & BT_DRV_STATE_RECOVERY)
    {
        BTE("%s:%d BT_DRV_STATE_RECOVERY, fops_mode:%d\n", __func__, __LINE__, fops_start);
        skb = hw_err_skb;
        if ((mode == INTF_FOPS_BT) && p_bt->bt_res.current_skb)
        {
            kfree_skb(p_bt->bt_res.current_skb);
            p_bt->bt_res.current_skb = NULL;
        }
        else if ((mode == INTF_FOPS_ZIGBEE) && p_bt->zigbee_res.current_skb)
        {
            kfree_skb(p_bt->zigbee_res.current_skb);
            p_bt->zigbee_res.current_skb = NULL;
        }
        else if ((mode == INTF_FOPS_THREAD) && p_bt->thread_res.current_skb)
        {
            kfree_skb(p_bt->thread_res.current_skb);
            p_bt->thread_res.current_skb = NULL;
        }
        if (skb == NULL)
        {
            BTE("%s:%d hw_err_skb == 0!\n", __func__, __LINE__);
            return 0;
        }
    }
    else
    {
        if (mode == INTF_FOPS_BT)
        {
            skb = p_bt->bt_res.current_skb;
        }
        else if (mode == INTF_FOPS_ZIGBEE)
        {
            skb = p_bt->zigbee_res.current_skb;
        }
        else if (mode == INTF_FOPS_THREAD)
        {
            skb = p_bt->thread_res.current_skb;
        }
    }

    if (skb == NULL)
    {
        BTW("%s:%d skb == 0!\n", __func__, __LINE__);
        if ((mode == INTF_FOPS_BT) && skb_queue_len(&p_bt->bt_res.bt_rx_queue) > 0)
        {
            p_bt->bt_res.current_skb = skb_dequeue(&p_bt->bt_res.bt_rx_queue);
            p_bt->diag_res.flush_skb = 1;
            skb = p_bt->bt_res.current_skb;
        }
        else if ((mode == INTF_FOPS_ZIGBEE) && skb_queue_len(&p_bt->zigbee_res.zigbee_rx_queue) > 0)
        {
            p_bt->zigbee_res.current_skb = skb_dequeue(&p_bt->zigbee_res.zigbee_rx_queue);
            p_bt->diag_res.flush_skb = 1;
            skb = p_bt->zigbee_res.current_skb;
        }
        else if ((mode == INTF_FOPS_THREAD) && skb_queue_len(&p_bt->thread_res.thread_rx_queue) > 0)
        {
            p_bt->thread_res.current_skb = skb_dequeue(&p_bt->thread_res.thread_rx_queue);
            p_bt->diag_res.flush_skb = 1;
            skb = p_bt->thread_res.current_skb;
        }
        else
        {
            BTE("%s:%d rx_queue == 0, fops_mode:%d!\n", __func__, __LINE__, mode);
            return 0;
        }
    }

    if (skb == NULL)
    {
        BTE("%s:%d current_skb == 0,fops_mode:%d!\n", __func__, __LINE__, mode);
        return 0;
    }

    if ((mode == INTF_FOPS_BT) && p_bt->diag_res.flush_skb)
    {
        p_bt->diag_res.flush_skb = 0;
        p = skb->data;
        //if (p[0] == HCI_ACLDATA_PKT)
        //{
        //    pt = 1;
        //}
        if (p[0] == HCI_EVENT_PKT)
        {
            if (0 == amlbt_intf_diag_filter_event(skb->data))
            {
                if (amlbt_intf_bt_get() == BT_INTF_DRIVER_USB)
                {
                    amlbt_intf_diag_add(p_bt, HCI_EVENT_PKT, (unsigned int)p_bt->usb_res.fw_evt_fifo->w,
                        (unsigned int)p_bt->usb_res.fw_evt_fifo->r, skb->data, p_bt->diag_res.fw_log_cnt, 0);
                }
                else if (amlbt_intf_bt_get() == BT_INTF_DRIVER_TTY)
                {
                    amlbt_intf_diag_add(p_bt, HCI_EVENT_PKT, 0, 0, skb->data, p_bt->diag_res.fw_log_cnt, 0);
                }
                p_bt->diag_res.fw_log_cnt = 0;
            }
            else if (p[1] == HCI_FWLOG_PKT)  //fw log
            {
                p_bt->diag_res.fw_log_cnt++;
            }
        }
    }

    if (count > skb->len)
    {
        length = skb->len;
    }
    else
    {
        length = count;
    }

    if (pt)
    {
        p = skb->data;
        //BTI("read %d,%d, [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", length, skb->len, p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
    }

    if (count == 1 && amlbt_intf_rw_get() == INTF_USB)
    {
        pkt_type = hci_skb_pkt_type(skb);
        if (copy_to_user(buf_p, &pkt_type, 1)) {
            BTE("%s, copy_to_user \n", __func__);
            return -EFAULT;
        }
        BTI("%s, read packet type %d\n", __func__);
        return 1;
    }

    BTD("read event %d,%d\n", length, skb->len);
    BTD("read event[%#x,%#x,%#x,%#x]\n", skb->data[0],skb->data[1],skb->data[2],skb->data[3]);
    BTD("read event[%#x,%#x,%#x,%#x]\n", skb->data[4],skb->data[5],skb->data[6],skb->data[7]);
    if (copy_to_user(buf_p, skb->data, length))
    {
        BTE("%s, copy_to_user error \n", __func__);
        return -EFAULT;
    }
    skb_pull(skb, length);

    if (skb->len == 0)
    {
        if (mode == INTF_FOPS_BT)
        {
            if (skb == p_bt->bt_res.current_skb)
            {
                kfree_skb(p_bt->bt_res.current_skb);
                p_bt->bt_res.current_skb = NULL;
            }
            else if (skb == p_bt->bt_res.hw_error_skb)
            {
                kfree_skb(p_bt->bt_res.hw_error_skb);
                p_bt->bt_res.hw_error_skb = NULL;
            }
            pt = 0;
        }
        else if (mode == INTF_FOPS_ZIGBEE)
        {
            if (skb == p_bt->zigbee_res.current_skb)
            {
                kfree_skb(p_bt->zigbee_res.current_skb);
                p_bt->zigbee_res.current_skb = NULL;
            }
            else if (skb == p_bt->zigbee_res.hw_error_skb)
            {
                kfree_skb(p_bt->zigbee_res.hw_error_skb);
                p_bt->zigbee_res.hw_error_skb = NULL;
            }
            pt = 0;
        }
        else if (mode == INTF_FOPS_THREAD)
        {
            if (skb == p_bt->thread_res.current_skb)
            {
                kfree_skb(p_bt->thread_res.current_skb);
                p_bt->thread_res.current_skb = NULL;
            }
            else if (skb == p_bt->thread_res.hw_error_skb)
            {
                kfree_skb(p_bt->thread_res.hw_error_skb);
                p_bt->thread_res.hw_error_skb = NULL;
            }
            pt = 0;
        }
    }
    return length;
}

static unsigned int amlbt_intf_bt_fops_poll(struct file *file, poll_table *wait)
{
    return amlbt_intf_fops_poll(file, wait, INTF_FOPS_BT);
}

static int amlbt_intf_bt_fops_open(struct inode *inode, struct file *file)
{
    return amlbt_intf_fops_open(inode, file, INTF_FOPS_BT);
}

static int amlbt_intf_bt_fops_close(struct inode *inode, struct file *file)
{
    BTI("%s\n", __func__);
    return 0;
    //return amlbt_intf_fops_close(inode, file, INTF_FOPS_BT);
}

static ssize_t amlbt_intf_bt_fops_write(struct file *file_p, const char __user *buf_p, size_t count, loff_t *pos_p)
{
    return amlbt_intf_fops_write(file_p, buf_p, count, pos_p, INTF_FOPS_BT);
}

static ssize_t amlbt_intf_bt_fops_read(struct file *file_p, char __user *buf_p, size_t count, loff_t *pos_p)
{
    return amlbt_intf_fops_read(file_p, buf_p, count, pos_p, INTF_FOPS_BT);
}

static int amlbt_intf_zigbee_fops_open(struct inode *inode, struct file *file)
{
    return amlbt_intf_fops_open(inode, file, INTF_FOPS_ZIGBEE);
}

static int amlbt_intf_zigbee_fops_close(struct inode *inode, struct file *file)
{
    return amlbt_intf_fops_close(inode, file, INTF_FOPS_ZIGBEE);
}

static unsigned int amlbt_intf_zigbee_fops_poll(struct file *file, poll_table *wait)
{
    return amlbt_intf_fops_poll(file, wait, INTF_FOPS_ZIGBEE);
}

static ssize_t amlbt_intf_zigbee_fops_write(struct file *file_p, const char __user *buf_p, size_t count, loff_t *pos_p)
{
    return amlbt_intf_fops_write(file_p, buf_p, count, pos_p, INTF_FOPS_ZIGBEE);
}

static ssize_t amlbt_intf_zigbee_fops_read(struct file *file_p, char __user *buf_p, size_t count, loff_t *pos_p)
{
    return amlbt_intf_fops_read(file_p, buf_p, count, pos_p, INTF_FOPS_ZIGBEE);
}

static int amlbt_intf_thread_fops_open(struct inode *inode, struct file *file)
{
    return amlbt_intf_fops_open(inode, file, INTF_FOPS_THREAD);
}

static int amlbt_intf_thread_fops_close(struct inode *inode, struct file *file)
{
    return amlbt_intf_fops_close(inode, file, INTF_FOPS_THREAD);
}

static unsigned int amlbt_intf_thread_fops_poll(struct file *file, poll_table *wait)
{
    return amlbt_intf_fops_poll(file, wait, INTF_FOPS_THREAD);
}

static ssize_t amlbt_intf_thread_fops_write(struct file *file_p, const char __user *buf_p, size_t count, loff_t *pos_p)
{
    return amlbt_intf_fops_write(file_p, buf_p, count, pos_p, INTF_FOPS_THREAD);
}

static ssize_t amlbt_intf_thread_fops_read(struct file *file_p, char __user *buf_p, size_t count, loff_t *pos_p)
{
    return amlbt_intf_fops_read(file_p, buf_p, count, pos_p, INTF_FOPS_THREAD);
}

static int amlbt_intf_diag_fops_open(struct inode *inode, struct file *file)
{
    return nonseekable_open(inode, file);
}

static int amlbt_intf_diag_fops_close(struct inode *inode, struct file *file)
{
    return 0;
}

static void amlbt_intf_diag_usage(void)
{
    BTI("------------driver diag cmd usage--------------\n");
    BTI("read version:       echo \"get_version\" > /dev/aml_bt_debug\n");
    BTI("read reg:           echo \"get_reg <addr>\" > /dev/aml_bt_debug\n");
    BTI("write reg:          echo \"set_reg <addr> <val>\" > /dev/aml_bt_debug\n");
    BTI("read driver info:   echo \"driver_info\" > /dev/aml_bt_debug\n");
    BTI("data capture open:  echo \"start_capture <bus> <start_bank> <end_bank>\" > /dev/aml_bt_debug\n");
    BTI("data capture close: echo \"close_capture\" > /dev/aml_bt_debug\n");
    BTI("read interrupt cnt:  echo \"interrupt_cnt\" > /dev/aml_bt_debug\n");
}

static void amlbt_intf_diag_interrupt_cnt(amlbt_t *p_bt)
{
    unsigned long elapsed_ms = 0;
    unsigned long elapsed_jiffies = 0;

    if (p_bt->diag_res.current_time == 0)
    {
        p_bt->diag_res.current_time = jiffies;
    }
    else
    {
        if (time_after(jiffies, p_bt->diag_res.current_time))
        {
            elapsed_jiffies = jiffies - p_bt->diag_res.current_time;
            elapsed_ms = jiffies_to_msecs(elapsed_jiffies);
            p_bt->diag_res.current_time = jiffies;
        }
    }
    BTI("fw interrupt cnt:%d time interval:%d(ms)\n", p_bt->diag_res.fw_interrupt_cnt, elapsed_ms);
    p_bt->diag_res.fw_interrupt_cnt = 0;
}

static int amlbt_intf_diag_start_capture(amlbt_t *p_bt, unsigned int addr, unsigned long s, unsigned long e)
{
    int ret = 0;
    unsigned int len = 0;
    uint8_t cmd[258] = {0x01, SW_CAPTURE_CMD & 0xff, (SW_CAPTURE_CMD >> 8) & 0xff};
    //para len
    len = 1 + 4 + 1 + 1;
    cmd[3] = len;
    //capture open 1
    cmd[4] = 0x1;
    //addr
    cmd[5] = addr & 0xff;
    cmd[6] = (addr >> 8) & 0xff;
    cmd[7] = (addr >> 16) & 0xff;
    cmd[8] = (addr >> 24) & 0xff;
    //start bank
    cmd[9] = s & 0xff;
    //end bank
    cmd[10] = e & 0xff;

    BTI("%s %d [%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x]\n", __func__, len + 4,
        cmd[0],cmd[1],cmd[2],cmd[3],cmd[4],cmd[5],cmd[6],cmd[7],cmd[8],cmd[9],cmd[10]);
    ret = amlbt_intf_send_vendor(SW_CAPTURE_CMD, cmd, len + 4);

    return ret;
}

static int amlbt_intf_diag_end_capture(amlbt_t *p_bt)
{
    int ret = 0;
    uint8_t cmd[5] = {0x01, SW_CAPTURE_CMD & 0xff, (SW_CAPTURE_CMD >> 8) & 0xff, 0x01, 0x0}; //close 0

    BTI("%s %d [%#x,%#x,%#x,%#x,%#x]\n", __func__, sizeof(cmd),
        cmd[0],cmd[1],cmd[2],cmd[3],cmd[4]);
    ret = amlbt_intf_send_vendor(SW_CAPTURE_CMD, cmd, sizeof(cmd));

    return ret;
}

static ssize_t amlbt_intf_diag_fops_write(struct file *file_p, const char __user *buf_p, size_t count, loff_t *pos_p)
{
    char *kbuf;
    ssize_t ret = count;
    unsigned long addr = 0, val = 0;
    unsigned long start_bank = 0;
    unsigned long end_bank = 0;
    int result;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    if (count == 0)
    {
        return 0;
    }

    if (count > MAX_DIAG_CMD_LENGTH - 1)
        count = MAX_DIAG_CMD_LENGTH - 1;

    kbuf = kzalloc(MAX_DIAG_CMD_LENGTH, GFP_KERNEL);
    if (!kbuf)
    {
        BTE("%s:%d kzalloc error!\n", __func__, __LINE__);
        return -ENOMEM;
    }

    if (copy_from_user(kbuf, buf_p, count))
    {
        kfree(kbuf);
        BTE("%s:%d copy_from_user error!\n", __func__, __LINE__);
        return -EFAULT;
    }

    kbuf[count] = '\0';

    if (kbuf[count - 1] == '\n')
        kbuf[count - 1] = '\0';

    if (!strncmp(kbuf, "help", 4))
    {
        amlbt_intf_diag_usage();
    }
    else if (!strncmp(kbuf, "get_version", 11))
    {
        amlbt_chip_info_print(p_bt);
    }
    else if (!strncmp(kbuf, "get_reg", 7))
    {
        if (sscanf(kbuf + 7, "%lx", &addr) == 1)
        {
            result = amlbt_intf_read_word(addr, (unsigned int *)&val);
            if (result == 0)
            {
                BTI("get_reg: [0x%lx] = 0x%lx\n", addr, val);
            }
            else
            {
                BTE("get_reg error!\n");
            }
        }
        else
        {
            BTE("get_reg usage: get_reg <addr>\n");
        }
    }
    else if (!strncmp(kbuf, "set_reg", 7))
    {
        if (sscanf(kbuf + 7, "%lx %lx", &addr, &val) == 2)
        {
            result = amlbt_intf_write_word(addr, (unsigned int)val);
            if (result == 0)
            {
                BTI("set_reg: [0x%lx] = 0x%lx\n", addr, val);
            }
            else
            {
                BTE("set_reg error!\n");
            }
        }
        else
        {
            BTE("set_reg usage: set_reg <addr> <val>\n");
        }
    }
    else if (!strncmp(kbuf, "driver_info", 11))
    {
        amlbt_intf_diag_rssi_print(p_bt);
    }
    else if (!strncmp(kbuf, "start_capture", 13))
    {
        if (sscanf(kbuf + 13, "%lx %lx %lx", &addr, &start_bank, &end_bank) == 3)
        {
            result = amlbt_intf_diag_start_capture(p_bt, addr, start_bank, end_bank);
            if (result == 0)
            {
                BTI("start_capture: addr:0x%lx start_bank:0x%lx end_bank:0x%lx\n", addr, start_bank, end_bank);
            }
            else
            {
                BTE("start_capture error!\n");
            }
        }
    }
    else if (!strncmp(kbuf, "close_capture", 13))
    {
        amlbt_intf_diag_end_capture(p_bt);
    }
    else if (!strncmp(kbuf, "interrupt_cnt", 13))
    {
        amlbt_intf_diag_interrupt_cnt(p_bt);
    }
    else
    {
        BTI("amlbt: unknown command [%s]\n", kbuf);
        amlbt_intf_diag_usage();
    }

    kfree(kbuf);
    return ret;
}

static ssize_t amlbt_intf_diag_fops_read(struct file *file_p, char __user *buf_p, size_t count, loff_t *pos_p)
{
    return 0;
}

static long amlbt_intf_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
    unsigned char coex_running = 0;
    amlbt_t *p_bt = (amlbt_t *)filp->private_data;
    unsigned int version = AML_BT_DRIVER_VERSION;
    unsigned long max_diag = MAX_DIAG_SKB_QUEUED;
    struct sk_buff *skb;
    struct amlbt_diag_entry remain_entry;
    struct amlbt_diag_remain_buf __user *rbuf;
    struct amlbt_diag_remain_buf r_hdr;
    unsigned char *p;
    unsigned int copied = 0;
//    unsigned char fw_log[516];
    unsigned int cid_invalid = 0;
    unsigned int reg_value = 0;
    //int opt_val = 0;
    int ret;
    switch (cmd)
    {
        case IOCTL_GET_BT_RECOVERY:
        {
            if (copy_to_user((unsigned char __user *)arg, &p_bt->common_res.recovery_value, sizeof(unsigned char)) != 0)
            {
                BTE("IOCTL_GET_BT_RECOVERY copy error\n");
                return -EFAULT;
            }
            BTI("IOCTL_GET_BT_RECOVERY %#x\n", p_bt->common_res.recovery_value);
        }
        break;
        case IOCTL_GET_DEVICE_PID:
        {
            if (copy_to_user((unsigned char __user *)arg, &g_chip_function_ctrl, sizeof(unsigned char)) != 0)
            {
                BTE("IOCTL_GET_DEVICE_PID copy error\n");
                return -EFAULT;
            }
            BTI("IOCTL_GET_DEVICE_PID %#x\n", g_chip_function_ctrl);
        }
        break;
        case IOCTL_SET_BT_SHUTDOWN:
        {
            /*if (copy_from_user(&p_bt->common_res.shutdown_value, (unsigned char __user *)arg, sizeof(unsigned long)) != 0)
            {
                BTE("IOCTL_SET_BT_SHUTDOWN copy error\n");
                return -EFAULT;
            }*/
            p_bt->common_res.shutdown_value = 1;
            BTI("IOCTL_SET_BT_SHUTDOWN %#x\n", p_bt->common_res.shutdown_value);
        }
        break;
        case IOCTL_GET_COEX_STATUS:
        {
            coex_running = ((p_bt->thread_res.thread_start << 2) | (p_bt->zigbee_res.zigbee_start << 1) | p_bt->bt_res.bt_start);
            if (copy_to_user((unsigned char __user *)arg, &coex_running, sizeof(unsigned char)) != 0)
            {
                BTE("IOCTL_GET_COEX_STATUS copy error\n");
                return -EFAULT;
            }
            BTI("IOCTL_GET_COEX_STATUS %#x\n", coex_running);
        }
        break;
        case IOCTL_REGISTER_SDIO:
        {
            amlbt_intf_sdio_register();
            BTI("IOCTL_REGISTER_SDIO \n");
        }
        break;
        case IOCTL_UNREGISTER_SDIO:
        {
            amlbt_intf_sdio_unregister();
            BTI("IOCTL_UNREGISTER_SDIO \n");
        }
        break;
        case IOCTL_GET_SDIO_PROBE_STATUS:
        {
            if (copy_to_user((unsigned char __user *)arg, &g_sdio_driver_insmoded, sizeof(unsigned char)) != 0)
            {
                BTE("IOCTL_GET_SDIO_PROBE_STATUS copy error\n");
                return -EFAULT;
            }
            BTI("IOCTL_GET_SDIO_PROBE_STATUS: %d\n", g_sdio_driver_insmoded);
        }
        break;
        case IOCTL_GET_DEVICE_CID:
        {
#ifdef CONFIG_AML_BT_CHIP_W2
            if (copy_to_user((unsigned int __user *)arg, &g_aml_device_id, sizeof(unsigned int)) != 0)
            {
                BTE("IOCTL_GET_DEVICE_PID copy error\n");
                return -EFAULT;
            }
            BTI("IOCTL_GET_DEVICE_PID %#x\n", g_aml_device_id);
#else
            if (copy_to_user((unsigned int __user *)arg, &cid_invalid, sizeof(unsigned int)) != 0)
            {
                BTE("IOCTL_GET_DEVICE_PID cid_invalid copy error\n");
                return -EFAULT;
            }
            BTI("IOCTL_GET_DEVICE_PID cid_invalid %#x\n", cid_invalid);
#endif
        }
        break;
        case IOCTL_GET_DRIVER_VERSION:
        {
            if (copy_to_user((unsigned int __user *)arg, &version, sizeof(unsigned int)) != 0)
            {
                BTE("IOCTL_GET_DRIVER_VERSION copy error\n");
                return -EFAULT;
            }
            BTI("IOCTL_GET_DRIVER_VERSION: %#x\n", version);
        }
        break;
        case IOCTL_GET_DIAG_COUNT:
        {
            max_diag = skb_queue_len(&p_bt->diag_res.diag_queue);
            if (copy_to_user((unsigned char __user *)arg, &max_diag, sizeof(unsigned long)) != 0)
            {
                BTE("IOCTL_GET_DIAG_COUNT copy error\n");
                return -EFAULT;
            }
            BTI("IOCTL_GET_DIAG_COUNT: %d\n", max_diag);
        }
        break;
        case IOCTL_GET_DIAG_BUFF:
        {
            struct amlbt_diag_buf __user *ubuf = (struct amlbt_diag_buf __user *)arg;
            struct {
                unsigned int count;
                unsigned int max;
            } hdr;
            unsigned int copied = 0;
            struct sk_buff *skb;
            struct amlbt_diag_entry *entry;

            BTI("IOCTL_GET_DIAG_BUFF offsetof(entries) = %d\n", offsetof(struct amlbt_diag_buf, entries));

            if (copy_from_user(&hdr, ubuf, sizeof(hdr))) {
                BTE("IOCTL_GET_DIAG_BUFF: copy_from_user(hdr) failed\n");
                return -EFAULT;
            }
            BTI("IOCTL_GET_DIAG_BUFF hdr.max:%d, skb_queue_len:%d\n", hdr.max, skb_queue_len(&p_bt->diag_res.diag_queue));

            if (hdr.max == 0 || hdr.max > MAX_DIAG_SKB_QUEUED) {
                BTE("IOCTL_GET_DIAG_BUFF: hdr.max invalid: %u\n", hdr.max);
                return -EINVAL;
            }
            /* coverity[var_deref_op:SUPPRESS] */
            skb_queue_walk(&p_bt->diag_res.diag_queue, skb) {
                //BTI("copied:%d\n", copied);
                if (copied >= hdr.max)
                    break;
                if (unlikely(!skb))
                {
                    BTE("IOCTL_GET_DIAG_BUFF: unexpected NULL skb in queue\n");
                    continue;
                }
                if (unlikely(skb->len < sizeof(struct amlbt_diag_entry)))
                {
                    BTE("IOCTL_GET_DIAG_BUFF: invalid skb length: %u\n", skb->len);
                    continue;
                }
                entry = (struct amlbt_diag_entry *)skb->data;

                if (copy_to_user(&ubuf->entries[copied], entry, sizeof(*entry))) {
                    BTE("IOCTL_GET_DIAG_BUFF: copy_to_user(entry) failed at idx %u\n", copied);
                    return -EFAULT;
                }
                copied++;
            }
            BTI("copied finished:%d\n", copied);
            if (put_user(copied, &ubuf->count)) {
                BTE("IOCTL_GET_DIAG_BUFF: put_user(count) failed\n");
                return -EFAULT;
            }

            BTI("IOCTL_GET_DIAG_BUFF: returned %u entries\n", copied);
        }
        break;

        case IOCTL_GET_DIAG_REMAIN_COUNT:
        {
            max_diag = skb_queue_len(&p_bt->bt_res.bt_rx_queue);
            if (copy_to_user((unsigned char __user *)arg, &max_diag, sizeof(unsigned long)) != 0)
            {
                BTE("IOCTL_GET_DIAG_REMAIN_COUNT copy error\n");
                return -EFAULT;
            }
            BTI("IOCTL_GET_DIAG_REMAIN_COUNT: %#x\n", max_diag);
        }
        break;

        case IOCTL_GET_DIAG_REMAIN_BUFF:
        {
            rbuf = (void __user *)arg;
            if (copy_from_user(&r_hdr, rbuf, sizeof(r_hdr))) {
                BTE("IOCTL_GET_DIAG_REMAIN_BUFF: copy_from_user(r_hdr) failed\n");
                return -EFAULT;
            }

            if (r_hdr.max == 0 || r_hdr.max > MAX_DIAG_SKB_QUEUED) {
                BTE("IOCTL_GET_DIAG_REMAIN_BUFF: r_hdr.max invalid: %u\n", (unsigned int)r_hdr.max);
                return -EINVAL;
            }
            copied = 0;
            /* coverity[var_deref_op:SUPPRESS] */
            skb_queue_walk(&p_bt->bt_res.bt_rx_queue, skb) {
                if (copied >= r_hdr.max)
                    break;
                if (unlikely(!skb))
                {
                    BTE("IOCTL_GET_DIAG_BUFF: unexpected NULL skb in queue\n");
                    continue;
                }
                if (unlikely(skb->len < 1 + sizeof(remain_entry.info)))
                {
                    BTE("IOCTL_GET_DIAG_BUFF: invalid skb length: %u\n", skb->len);
                    continue;
                }
                p = skb->data;
                memset(&remain_entry, 0, sizeof(remain_entry));
                remain_entry.opcode = p[0];
                remain_entry.info[0] = p[0];
                memcpy(&remain_entry.info[1], &p[1], sizeof(remain_entry.info)-1);
                if (copy_to_user(&rbuf->entries[copied], &remain_entry, sizeof(remain_entry))) {
                    BTE("IOCTL_GET_DIAG_REMAIN_BUFF: copy_to_user(entry) failed at idx %u\n", copied);
                    return -EFAULT;
                }
                copied++;
            }
            if (put_user(copied, &rbuf->count)) {
                BTE("IOCTL_GET_DIAG_REMAIN_BUFF: put_user(count) failed\n");
                return -EFAULT;
            }
            BTI("IOCTL_GET_DIAG_REMAIN_BUFF: returned %u entries\n", copied);
        }
        break;
        case IOCTL_SET_BT_EN_ENABLE:
        {
            //w1d bt en enable defect, revC rtl fix
            amlbt_intf_read_word(RG_AON_A33, &reg_value);
            BTI("start %#x: %#x\n", RG_AON_A33, reg_value);
            reg_value |= BIT(1);
            amlbt_intf_write_word(RG_AON_A33, reg_value);
            amlbt_intf_read_word(RG_AON_A33, &reg_value);
            BTI("end %#x: %#x\n", RG_AON_A33, reg_value);
            BTI("IOCTL_SET_BT_EN_ENABLE \n");
        }
        break;
        case IOCTL_REGISTER_HCI0:
        {
            /*if (copy_from_user(&opt_val, (int __user *)arg, sizeof(int))) {
                BTE("IOCTL_REGISTER_HCI0: copy_from_user failed\n");
                return -EFAULT;
            }
            BTI("Received IOCTL_REGISTER_HCI0, param: %d\n", opt_val);*/

            //after vendor cmd fc22 open fwlog
            amlbt_write_fwlog_mode(p_bt);

            ret = amlbt_hci_register_dev(p_bt);
            if (ret < 0) {
                BTE("Failed to register HCI device: %d\n", ret);
                return ret;
            }
        }
        break;
        default:
        {
            BTE("DEFAULT case! cmd=0x%08x not matched\n", cmd);
            return -ENOTTY;
        }
    }
    return 0;
}

#ifdef CONFIG_COMPAT
static long amlbt_intf_compat_ioctl(struct file* filp, unsigned int cmd, unsigned long arg)
{
    return amlbt_intf_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif

int amlbt_intf_input_device_init(struct platform_device *pdev)
{
    int err;
    amlbt_t *p_bt = amlbt_intf_get_p_bt();

    BTI("%s \n", __func__);

    p_bt->pm_res.input_dev = input_allocate_device();
    if (!p_bt->pm_res.input_dev)
    {
        BTF("input_allocate_device failed:");
        return -EINVAL;
    }
    set_bit(EV_KEY,  p_bt->pm_res.input_dev->evbit);
    set_bit(KEY_POWER, p_bt->pm_res.input_dev->keybit);
    set_bit(KEY_NETFLIX, p_bt->pm_res.input_dev->keybit);

    p_bt->pm_res.input_dev->name = INPUT_NAME;
    p_bt->pm_res.input_dev->phys = INPUT_PHYS;
    p_bt->pm_res.input_dev->dev.parent = &pdev->dev;
    p_bt->pm_res.input_dev->id.bustype = BUS_ISA;
    p_bt->pm_res.input_dev->id.vendor = 0x0001;
    p_bt->pm_res.input_dev->id.product = 0x0001;
    p_bt->pm_res.input_dev->id.version = 0x0100;
    p_bt->pm_res.input_dev->rep[REP_DELAY] = 0xffffffff;
    p_bt->pm_res.input_dev->rep[REP_PERIOD] = 0xffffffff;
    p_bt->pm_res.input_dev->keycodesize = sizeof(unsigned short);
    p_bt->pm_res.input_dev->keycodemax = 0x1ff;
    err = input_register_device(p_bt->pm_res.input_dev);
    if (err < 0)
    {
        pr_err("input_register_device failed: %d\n", err);
        input_free_device(p_bt->pm_res.input_dev);
        return -EINVAL;
    }

    return err;
}

void amlbt_intf_exception_func(amlbt_t *p_bt)
{
    BTF("Coex driver detect exception! [%#x,%#x,%#x]\n",
        p_bt->bt_res.bt_start, p_bt->zigbee_res.zigbee_start, p_bt->thread_res.thread_start);

    amlbt_intf_drv_state_set(BT_DRV_STATE_RECOVERY);
    amlbt_intf_drv_state_clr(BT_DRV_STATE_WAIT_RECOVERY);
    if (p_bt->bt_res.bt_start)
    {
        amlbt_wakeup_lock();
        reinit_completion(&p_bt->excp_res.notify_comp);
        p_bt->excp_res.notify_trig = 1;
        skb_queue_purge(&p_bt->bt_res.bt_rx_queue);
        wake_up_interruptible(&p_bt->bt_res.bt_wait_queue);
    }
    if (p_bt->zigbee_res.zigbee_start)
    {
        skb_queue_purge(&p_bt->zigbee_res.zigbee_rx_queue);
        wake_up_interruptible(&p_bt->zigbee_res.zigbee_wait_queue);
    }
    if (p_bt->thread_res.thread_start)
    {
        skb_queue_purge(&p_bt->thread_res.thread_rx_queue);
        wake_up_interruptible(&p_bt->thread_res.thread_wait_queue);
    }

    BTF("Coex driver exception finish!\n");
    if (p_bt->bt_res.bt_start)
    {
        complete(&p_bt->excp_res.notify_comp);
    }
    return ;
}


static int amlbt_intf_event_handler(struct notifier_block *nb, unsigned long event, void *data)
{
    amlbt_t *p_bt = amlbt_intf_get_p_bt();
    BTF("%s, event:%lu \n", __func__, event);

    switch (event)
    {
        case MODULE_RESTART:
        {
            p_bt->excp_res.notify_recy = MODULE_RESTART;
            amlbt_intf_exception_func(p_bt);
        }
        break;

        case MODULE_POWER_OFF:
        {
            p_bt->excp_res.notify_trig = MODULE_POWER_OFF;
            amlbt_intf_drv_state_clr(BT_DRV_STATE_SUSPEND);
            amlbt_intf_flush_workqueue(p_bt->common_res.write_work_wq, &p_bt->common_res.write_work);
            amlbt_intf_flush_workqueue(p_bt->usb_res.rx_work_wq, &p_bt->usb_res.rx_work);
            amlbt_intf_unregister_interrupt_gpio(p_bt);
            if (amlbt_intf_rw_get() == INTF_USB)
            {
                amlbt_intf_power_off();
            }
            p_bt->excp_res.notify_recy = MODULE_POWER_OFF;
            //atomic_set(&g_wifi_pm.bt_enable, 0);
        }
        break;

        default:
            break;
    }

    return NOTIFY_OK;
}

struct notifier_block bt_nb = {
    .notifier_call = amlbt_intf_event_handler,
};

const struct file_operations amlbt_intf_bt_fops =
{
    .open       = amlbt_intf_bt_fops_open,
    .release    = amlbt_intf_bt_fops_close,
    .write      = amlbt_intf_bt_fops_write,
    .read      = amlbt_intf_bt_fops_read,
    .unlocked_ioctl = amlbt_intf_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = amlbt_intf_compat_ioctl,
#endif
    .poll       = amlbt_intf_bt_fops_poll,
    .fasync     = NULL
};

const struct file_operations amlbt_intf_zigbee_fops =
{
    .open       = amlbt_intf_zigbee_fops_open,
    .release    = amlbt_intf_zigbee_fops_close,
    .write      = amlbt_intf_zigbee_fops_write,
    .read      = amlbt_intf_zigbee_fops_read,
    .unlocked_ioctl = amlbt_intf_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = amlbt_intf_compat_ioctl,
#endif
    .poll       = amlbt_intf_zigbee_fops_poll,
    .fasync     = NULL
};

const struct file_operations amlbt_intf_thread_fops =
{
    .open       = amlbt_intf_thread_fops_open,
    .release    = amlbt_intf_thread_fops_close,
    .write      = amlbt_intf_thread_fops_write,
    .read      = amlbt_intf_thread_fops_read,
    .unlocked_ioctl = amlbt_intf_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = amlbt_intf_compat_ioctl,
#endif
    .poll       = amlbt_intf_thread_fops_poll,
    .fasync     = NULL
};

const struct file_operations amlbt_intf_coex_fops =
{
    .open       = amlbt_intf_coex_fops_open,
    .release    = amlbt_intf_coex_fops_close,
    .write      = NULL,
    .read      = NULL,
    .unlocked_ioctl = amlbt_intf_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = amlbt_intf_compat_ioctl,
#endif
    .poll       = NULL,
    .fasync     = NULL
};

const struct file_operations amlbt_intf_diag_fops =
{
    .open       = amlbt_intf_diag_fops_open,
    .release    = amlbt_intf_diag_fops_close,
    .write      = amlbt_intf_diag_fops_write,
    .read      = amlbt_intf_diag_fops_read,
    .unlocked_ioctl = amlbt_intf_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = amlbt_intf_compat_ioctl,
#endif
    .poll       = NULL,
    .fasync     = NULL
};

int amlbt_intf_diag_filter_event(unsigned char *evt_buf)
{
    if (evt_buf[1] == HCI_ADVIND_PKT && evt_buf[3] == 0x02)   //adv report
    {
        return 1;
    }
    if (evt_buf[1] == HCI_ADVIND_PKT && evt_buf[3] == 0x0d)   //extend adv report
    {
        return 1;
    }
    /*if (evt_buf[1] == 0x13 && evt_buf[3] == 0x1)   //number of complete
    {
        return 1;
    }*/
    if (evt_buf[1] == 0x05 && evt_buf[3] == 0x00) //disconnect complete
    {
        return 1;
    }
    if (evt_buf[1] == HCI_FWLOG_PKT) //fw log
    {
        return 1;
    }
    if (evt_buf[1] == 0x02) //inquiry result
    {
        return 1;
    }
    if (evt_buf[1] == 0x22) //inquiry result with rssi
    {
        return 1;
    }
    if (evt_buf[1] == 0x2f) //enhanced inquiry result
    {
        return 1;
    }
    //if (evt_buf[1] == 0xf && evt_buf[3] == 0x0)   //event status
    //{
    //    return 1;
    ///}

    return 0;
}

void amlbt_intf_diag_add(amlbt_t *p_bt, u8 type,
    u32 w, u32 r, u8 *data, u32 fw_log_cnt, u32 data_cnt)
{
    struct sk_buff *skb = NULL, *old_skb = NULL;
    struct timespec64 ts = {0};
    struct tm tm = {0};
    struct amlbt_diag_entry entry;
    u32 ms;

    skb = alloc_skb(sizeof(entry), GFP_KERNEL);
    if (!skb) {
        BTE("%s:%d alloc_skb Failed\n", __func__, __LINE__);
        return;
    }

    memset(&entry, 0, sizeof(entry));

    entry.type = type;
    entry.w = w;
    entry.r = r;
    /* coverity[uninit_use_in_call:SUPPRESS] */
    /* coverity[uninit_use:SUPPRESS] */
    ktime_get_real_ts64(&ts);
    time64_to_tm(ts.tv_sec, 0, &tm);
    entry.mon  = tm.tm_mon + 1;
    entry.day  = tm.tm_mday;
    entry.hour = tm.tm_hour;
    entry.min  = tm.tm_min;
    entry.sec  = tm.tm_sec;

    ms = ts.tv_nsec / 1000000;
    entry.ms = (u16)ms;

    entry.opcode = type;
    if (amlbt_intf_bt_get() == BT_INTF_DRIVER_USB && type == HCI_COMMAND_PKT)
    {
        entry.info[0] = type;
        memcpy(&entry.info[1], data, sizeof(entry.info)-1);
    }
    else
    {
        memcpy(entry.info, data, sizeof(entry.info));
    }
    entry.fw_log_cnt = fw_log_cnt;
    entry.hci_cmd_cnt = data_cnt;

    /* coverity[noescape:SUPPRESS] */
    if (!skb_put_data(skb, &entry, sizeof(entry))) {
        BTE("%s: skb_put_data failed\n", __func__);
        kfree_skb(skb);
        return;
    }

    if (skb_queue_len(&p_bt->diag_res.diag_queue) >= MAX_DIAG_SKB_QUEUED) {
        old_skb = skb_dequeue(&p_bt->diag_res.diag_queue);
        if (old_skb)
            kfree_skb(old_skb);
    }
    /* coverity[leaked_storage:SUPPRESS] */
    skb_queue_tail(&p_bt->diag_res.diag_queue, skb);
}

void amlbt_intf_diag_rssi_print(amlbt_t *p_bt)
{
    int ret;
    unsigned int i = 0;
    unsigned int tx_q_prio[USB_TX_Q_NUM] = {0};
    unsigned int tx_q_index[USB_TX_Q_NUM] = {0};
    unsigned int tx_q_status[USB_TX_Q_NUM] = {0};
    unsigned int tx_buff[USB_TX_Q_NUM * 4] = {0};//prio, index, status

    BTW("%s: cnt: %d\n", __func__, skb_queue_len(&p_bt->bt_res.bt_rx_queue));

    if (amlbt_intf_bt_get() == BT_INTF_DRIVER_USB)
    {
        BTW("fw_type_fifo size:%d,used:%d,remain:%d\n", p_bt->usb_res.fw_type_fifo->size,
            amlbt_common_gdsl_fifo_used(p_bt->usb_res.fw_type_fifo), amlbt_common_gdsl_fifo_remain(p_bt->usb_res.fw_type_fifo));
        BTW("fw_type_fifo->w:%#x fw_type_fifo->r:%#x", p_bt->usb_res.fw_type_fifo->w, p_bt->usb_res.fw_type_fifo->r);
        BTW("fw_evt_fifo size:%d,used:%d,remain:%d\n", p_bt->usb_res.fw_evt_fifo->size,
            amlbt_common_gdsl_fifo_used(p_bt->usb_res.fw_evt_fifo), amlbt_common_gdsl_fifo_remain(p_bt->usb_res.fw_evt_fifo));
        BTW("fw_evt_fifo->w:%#x fw_evt_fifo->r:%#x", p_bt->usb_res.fw_evt_fifo->w, p_bt->usb_res.fw_evt_fifo->r);
        BTW("fw_data_fifo size:%d,used:%d,remain:%d\n", p_bt->usb_res.fw_data_fifo->size,
            amlbt_common_gdsl_fifo_used(p_bt->usb_res.fw_data_fifo), amlbt_common_gdsl_fifo_remain(p_bt->usb_res.fw_data_fifo));
        BTW("w_data_fifo->w:%#x fw_data_fifo->r:%#x", p_bt->usb_res.fw_data_fifo->w, p_bt->usb_res.fw_data_fifo->r);
        ret = amlbt_intf_read_sram((unsigned char *)tx_buff, p_bt->usb_res.tx_q[0].tx_q_prio_addr, sizeof(tx_buff));
        if (ret != 0)
        {
            BTE("%s:%d Failed : %d\n", __func__, __LINE__, ret);
            return;
        }
        for (i = 0; i < USB_TX_Q_NUM; i++)
        {
            tx_q_prio[i] = tx_buff[i*4];
            tx_q_index[i]  = tx_buff[i*4+1];
            tx_q_status[i]   = tx_buff[i*4+2];
        }
        BTW("Tx prio %#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", tx_q_prio[0],tx_q_prio[1],tx_q_prio[2],tx_q_prio[3],
            tx_q_prio[4],tx_q_prio[5],tx_q_prio[6],tx_q_prio[7]);
        BTW("Tx handle %#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", tx_q_index[0],tx_q_index[1],tx_q_index[2],tx_q_index[3],
            tx_q_index[4],tx_q_index[5],tx_q_index[6],tx_q_index[7]);
        BTW("Tx status %#x,%#x,%#x,%#x,%#x,%#x,%#x,%#x\n", tx_q_status[0],tx_q_status[1],tx_q_status[2],tx_q_status[3],
            tx_q_status[4],tx_q_status[5],tx_q_status[6],tx_q_status[7]);
    }
}

void amlbt_intf_diag_skb_print(amlbt_t *p_bt)
{
    struct sk_buff *skb;
    struct amlbt_diag_entry *entry;

    BTI("%s: cnt: %d\n", __func__, skb_queue_len(&p_bt->diag_res.diag_queue));

    while ((skb = skb_dequeue(&p_bt->diag_res.diag_queue))) {
        entry = (struct amlbt_diag_entry *)skb->data;

        BTI("%s: info:[%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x], w:0x%04x, r:0x%04x,"
            "date:%02u-%02u %02u:%02u:%02u.%03u, f_cnt:%u\n",
            __func__,
            (unsigned int)entry->type,
            (unsigned int)entry->info[0],
            (unsigned int)entry->info[1],
            (unsigned int)entry->info[2],
            (unsigned int)entry->info[3],
            (unsigned int)entry->info[4],
            (unsigned int)entry->info[5],
            (unsigned int)entry->info[6],
            (unsigned int)entry->w,
            (unsigned int)entry->r,
            (unsigned int)entry->mon,
            (unsigned int)entry->day,
            (unsigned int)entry->hour,
            (unsigned int)entry->min,
            (unsigned int)entry->sec,
            (unsigned int)entry->ms,
            (unsigned int)entry->fw_log_cnt);

        kfree_skb(skb);
    }
}

void amlbt_intf_diag_rx_remain_print(amlbt_t *p_bt)
{
    struct sk_buff *skb;
    unsigned char *p;
    unsigned int fwlog_cnt = 0;
    unsigned int advind_cnt = 0;

    BTI("%s: cnt: %d\n", __func__, skb_queue_len(&p_bt->bt_res.bt_rx_queue));

    while ((skb = skb_dequeue(&p_bt->bt_res.bt_rx_queue))) {
        p = skb->data;
        if (p[0] == HCI_EVENT_PKT && p[1] == HCI_FWLOG_PKT)
        {
            fwlog_cnt++;
        }
        else if (p[0] == HCI_EVENT_PKT && p[1] == HCI_ADVIND_PKT)
        {
            advind_cnt++;
        }
        else
        {
            BTI("%s: info:[%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x]\n",
                __func__,
                (unsigned int)p[0],
                (unsigned int)p[1],
                (unsigned int)p[2],
                (unsigned int)p[3],
                (unsigned int)p[4],
                (unsigned int)p[5],
                (unsigned int)p[6],
                (unsigned int)p[7]);
        }
        kfree_skb(skb);
    }
    BTI("%s: rx_remain fwlog_cnt %d advind_cnt %d\n", __func__, fwlog_cnt, advind_cnt);
    BTI("%s: fw_interrupt_cnt %d\n", __func__, p_bt->diag_res.fw_interrupt_cnt);
}

int amlbt_intf_create_device(amlbt_t *p_bt)
{
    int ret = 0;
    int i = 0, j = 0;
    int cdevErr = 0;
    dev_t dev = 0;
    char *device_names[AMLBT_MAX_COEX_DEVICES] =
        { AML_BT_NOTE, AML_ZIGBEE_NOTE, AML_THREAD_NOTE, AML_COEX_NOTE, AML_BT_DIAG_NOTE };
    char *device_node_name = NULL;
    BTI("%s \n", __func__);

    if (amlbt_intf_rw_get() == INTF_UNKNOWN)
    {
        BTE("%s:%d interface error!, amlbt_if_type:%#x\n", __func__, __LINE__, amlbt_if_type);
        return -1;
    }

    if (amlbt_intf_rw_get() == INTF_USB)
    {
        device_names[0] = AML_BT_USB_NOTE;
        device_node_name = AML_BT_USB_NOTE;
    }
    else if (amlbt_intf_rw_get() == INTF_SDIO || amlbt_intf_rw_get() == INTF_PCIE || amlbt_intf_rw_get() == INTF_UART)
    {
        device_names[0] = AML_BT_NOTE;
        device_node_name = AML_BT_NOTE;
    }

    ret = alloc_chrdev_region(&dev, 0, AMLBT_MAX_COEX_DEVICES, device_node_name);
    if (ret)
    {
        BTE("fail to allocate chrdev\n");
        return ret;
    }

    p_bt->drv_res.dev_major = MAJOR(dev);
    BTI("major number:%d\n", p_bt->drv_res.dev_major);

    i = 0;
    //bt node
    cdev_init(&p_bt->drv_res.dev_cdev[i], &amlbt_intf_bt_fops);
    p_bt->drv_res.dev_cdev[i].owner = THIS_MODULE;

    cdevErr = cdev_add(&p_bt->drv_res.dev_cdev[i], MKDEV(p_bt->drv_res.dev_major, i), 1);
    if (cdevErr)
    {
        goto error_cdev;
    }

    i++;
    //zigbee node
    cdev_init(&p_bt->drv_res.dev_cdev[i], &amlbt_intf_zigbee_fops);
    p_bt->drv_res.dev_cdev[i].owner = THIS_MODULE;

    ret = cdev_add(&p_bt->drv_res.dev_cdev[i], MKDEV(p_bt->drv_res.dev_major, i), 1);
    if (ret)
    {
        BTE("cdev_add failed for minor %d\n", i);
        goto error_cdev;
    }

    i++;
    //thread node
    cdev_init(&p_bt->drv_res.dev_cdev[i], &amlbt_intf_thread_fops);
    p_bt->drv_res.dev_cdev[i].owner = THIS_MODULE;

    ret = cdev_add(&p_bt->drv_res.dev_cdev[i], MKDEV(p_bt->drv_res.dev_major, i), 1);
    if (ret)
    {
        BTE("cdev_add failed for minor %d\n", i);
        goto error_cdev;
    }

    i++;
    //coex node
    cdev_init(&p_bt->drv_res.dev_cdev[i], &amlbt_intf_coex_fops);
    p_bt->drv_res.dev_cdev[i].owner = THIS_MODULE;

    ret = cdev_add(&p_bt->drv_res.dev_cdev[i], MKDEV(p_bt->drv_res.dev_major, i), 1);
    if (ret)
    {
        BTE("cdev_add failed for minor %d\n", i);
        goto error_cdev;
    }

    i++;
    //diag node
    cdev_init(&p_bt->drv_res.dev_cdev[i], &amlbt_intf_diag_fops);
    p_bt->drv_res.dev_cdev[i].owner = THIS_MODULE;

    ret = cdev_add(&p_bt->drv_res.dev_cdev[i], MKDEV(p_bt->drv_res.dev_major, i), 1);
    if (ret)
    {
        BTE("cdev_add failed for minor %d\n", i);
        goto error_cdev;
    }

    BTI("driver(major %d) installed.\n", p_bt->drv_res.dev_major);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
    p_bt->drv_res.dev_class = class_create(THIS_MODULE, device_node_name);
#else
    p_bt->drv_res.dev_class = class_create(device_node_name);
#endif

    if (IS_ERR(p_bt->drv_res.dev_class))
    {
        BTE("class create fail, error code(%ld)\n", PTR_ERR(p_bt->drv_res.dev_class));
        goto error_class;
    }

    for (i = 0; i < AMLBT_MAX_COEX_DEVICES; i++)
    {
        p_bt->drv_res.dev_device[i] = device_create(p_bt->drv_res.dev_class, NULL, MKDEV(p_bt->drv_res.dev_major, i), NULL, device_names[i]);
        if (IS_ERR(p_bt->drv_res.dev_device[i]))
        {
            BTE("device create fail for %s, error code(%ld)\n", device_names[i], PTR_ERR(p_bt->drv_res.dev_device[i]));
            goto error_device;
        }
    }

    BTI("Devices created success!\n");

    return 0;

error_device:
    j = i;
    while (--j >= 0)
    {
        device_destroy(p_bt->drv_res.dev_class, MKDEV(p_bt->drv_res.dev_major, j));
    }
    class_destroy(p_bt->drv_res.dev_class);

error_class:
    j = i;
error_cdev:
    while (--j >= 0)
    {
        cdev_del(&p_bt->drv_res.dev_cdev[j]);
    }
    unregister_chrdev_region(dev, AMLBT_MAX_COEX_DEVICES);

    return -1;
}

int amlbt_intf_destroy_device(amlbt_t *p_bt)
{
    dev_t dev;
    int i;

    BTI("%s start\n", __func__);

    for (i = 0; i < AMLBT_MAX_COEX_DEVICES; i++)
    {
        dev = MKDEV(p_bt->drv_res.dev_major, i);
        if (p_bt->drv_res.dev_device[i])
        {
            device_destroy(p_bt->drv_res.dev_class, dev);
            p_bt->drv_res.dev_device[i] = NULL;
        }
    }

    if (p_bt->drv_res.dev_class)
    {
        class_destroy(p_bt->drv_res.dev_class);
        p_bt->drv_res.dev_class = NULL;
    }

    for (i = 0; i < AMLBT_MAX_COEX_DEVICES; i++)
    {
        cdev_del(&p_bt->drv_res.dev_cdev[i]);
    }

    unregister_chrdev_region(MKDEV(p_bt->drv_res.dev_major, 0), AMLBT_MAX_COEX_DEVICES);

    BTI("%s end\n", __func__);
    return 0;
}

