/*
* Copyright (c) 202X Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*/

#ifndef __INTF_PCIE_H__
#define __INTF_PCIE_H__

#define AML_ADDR_CPU      0
#define AML_ADDR_AON      1
#define AML_ADDR_MAC      2

struct aml_plat_pci {
    struct usb_device *usb_dev;
    struct auc_hif_ops *hif_ops;

    struct device *dev;
    struct aml_hif_sdio_ops *hif_sdio_ops;

    struct pci_dev *pci_dev;

    bool enabled;

    int (*enable)(void *aml_hw);
    int (*disable)(void *aml_hw);
    void (*deinit)(struct aml_plat_pci *aml_plat_pci);
    u8* (*get_address)(struct aml_plat_pci *aml_plat_pci, int addr_name,
                       unsigned int offset);
    void (*ack_irq)(struct aml_plat_pci *aml_plat_pci);
    int (*get_config_reg)(struct aml_plat_pci *aml_plat_pci, const u32 **list);

    u8 priv[0] __aligned(sizeof(void *));
};

#ifdef CONFIG_AML_BT_CHIP_W2
extern unsigned int g_aml_device_id;
extern struct aml_plat_pci *g_aml_plat_pci;
extern struct aml_pm_type g_wifi_pm;
extern uint32_t aml_pci_read_for_bt(int base, u32 offset);
extern void aml_pci_write_for_bt(u32 val, int base, u32 offset);
extern int aml_pci_insmod(void);
extern void aml_pci_rmmod(void);
extern unsigned char g_pci_driver_insmoded;
#endif

void amlbt_intf_pcie_register(void);
void amlbt_intf_pcie_unregister(void);
void amlbt_intf_pcie_write_word(unsigned int addr, unsigned int data);
unsigned int amlbt_intf_pcie_read_word(unsigned int addr);


#endif

