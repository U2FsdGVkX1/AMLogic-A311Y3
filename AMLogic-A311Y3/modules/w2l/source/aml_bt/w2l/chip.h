/*
* Copyright (c) 202X Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*/

#ifndef __CHIP_H__
#define __CHIP_H__

#define USB_TX_Q_MAX_PRIO           0xFFFFFFFF
#define USB_TX_Q_NUM                (8)
#define USB_TX_Q_LEN                (1032)
#define USB_RX_Q_LEN                (1032)
#define USB_RX_TYPE_FIFO_LEN        256
#define USB_RC_MANFDATA_LEN         (6*8)

/*-------------------------------W2 USB-----------------------------------------------
Event FIFO,   start:0x00514000, end:0x005151fc, length:4604 bytes
    Rx Type FIFO Read Pointer     : 0x00514000  [0,1,2,3]
    HCI Event FIFO Read Pointer   : 0x00514004  [4,5,6,7]
    dummy                         : 0x00514008  [8,9,10,11]
    rx data fifo r reg            : 0x0051400c  [12,13,14,15]
    rx data fifo w reg            : 0x00514010  [16,17,18,19]
    sink mode status              : 0x00514014  [20,21,22,23]
    dummy                         : 0x00514018  [24,25,26,27]
    dummy                         : 0x0051401c  [28,29,30,31]
    Rx Type FIFO Write Pointer    : 0x00514020  [32,33,34,35]
    HCI Event FIFO Write Pointer  : 0x00514024  [36,37,38,39]
    dummy                         : 0x00514028  [40,41,42,43]
    dummy                         : 0x0051402c  [44,45,46,47]
    dummy                         : 0x00514030  [48,49,50,51]
    dummy                         : 0x00514034  [52,53,54,55]
    dummy                         : 0x00514038  [56,57,58,59]
    Rx Type FIFO                  : 0x0051403c  [256 bytes]
    HCI Event FIFO                : 0x0051413c  [2048 bytes]

Tx Queue,     start:0x00508000, end:0x0050a3fc, length:9212 bytes

Register RAM, start:0x00510000, end:0x00510200, length:512 bytes
    HCI Command FIFO Read Pointer : 0x00510000
    HCI Command FIFO Write Pointer: 0x00510004

    Tx Queue Prio Pointer         : 0x00510018
    Tx Queue Acl Handle Pointer   : 0x0051001c
    Tx Queue Status Pointer       : 0x00510020
    Dummy                         : 0x00510024

    Driver Firmware Status Pointer: 0x005101fc

Rx Queue,     start:0x00500000, end:0x00501000, length:4096 bytes

Command FIFO, start:0x00518000, end:0x00519000, length:4096 bytes

---------------------------------------------------------------------------------*/

#define W2_ICCM_SIZE               0x40000
#define W2_DCCM_SIZE               0x20000
#define W2_ROM_SIZE                256*1024
#define W2_ICCM_AHB_BASE_ADDR      0x00300000
#define W2_DCCM_AHB_BASE_ADDR      0x00400000
#define W2_ICCM_RAM_BASE_ADDR      0x00000000
#define W2_DCCM_RAM_BASE_ADDR      0x00d00000

#define W2_DOWNLOAD_SIZE           4096
#define W2_PCIE_DOWNLOAD_SIZE      4
#define W2_USB_RX_Q_LEN            USB_RX_Q_LEN*4
#define W2_USB_POLL_LEN            2364        //60 bytes register+256 bytes type fifo+2048 bytes event fifo
#define W2_USB_RX_TYPE_FIFO_LEN    256
#define W2_USB_EVT_FIFO_LEN        2048
#define W2_USB_CMD_FIFO_LEN        4096

#define W2_USB_MEM1_ADDR           0x00500000  //w2l usb memory1, length:4096 bytes
#define W2_USB_MEM2_ADDR           0x00508000  //w2l usb memory2, length:9212 bytes
#define W2_USB_MEM3_ADDR           0x00510000  //w2l usb memory3, length:512 bytes
#define W2_USB_CMD_Q_R_POINT       W2_USB_MEM3_ADDR+0x00
#define W2_USB_CMD_Q_W_POINT       W2_USB_MEM3_ADDR+0x04
#define W2_USB_DRIVER_FW_STATUS    W2_USB_MEM3_ADDR+0x1fc

#define W2_USB_MEM4_ADDR           0x00514000  //w2l usb memory4, length:4096 bytes
#define W2_USB_RX_TYPE_Q_R_POINT   W2_USB_MEM4_ADDR+0x00
#define W2_USB_EVT_Q_R_POINT       W2_USB_MEM4_ADDR+0x04

#define W2_USB_RX_Q_R_POINT        W2_USB_MEM4_ADDR+0x0c
#define W2_USB_RX_Q_W_POINT        W2_USB_MEM4_ADDR+0x10

#define W2_USB_RX_TYPE_Q_W_POINT   W2_USB_MEM4_ADDR+0x20
#define W2_USB_EVT_Q_W_POINT       W2_USB_MEM4_ADDR+0x24

#define W2_USB_RX_TYPE_Q_ADDR      W2_USB_MEM4_ADDR+0x3c
#define W2_USB_EVT_Q_ADDR          W2_USB_MEM4_ADDR+0x13c


#define W2_USB_CMD_Q_ADDR          0x00518000  //length:4096 bytes
#define W2_USB_TX_Q_PRIO_ADDR      W2_USB_MEM3_ADDR + 0x18


/*-------------------------------W2L USB-----------------------------------------------
Event FIFO,   start:0x00514000, end:0x005151fc, length:4604 bytes
    Rx Type FIFO Read Pointer     : 0x00514000  [0,1,2,3]
    HCI Event FIFO Read Pointer   : 0x00514004  [4,5,6,7]
    15p4 rx data fifo r           : 0x00514008  [8,9,10,11]
    rx data fifo r reg            : 0x0051400c  [12,13,14,15]
    rx data fifo w reg            : 0x00514010  [16,17,18,19]
    sink mode status              : 0x00514014  [20,21,22,23]
    dummy                         : 0x00514018  [24,25,26,27]
    15p4 rx data fifo w           : 0x0051401c  [28,29,30,31]
    Rx Type FIFO Write Pointer    : 0x00514020  [32,33,34,35]
    HCI Event FIFO Write Pointer  : 0x00514024  [36,37,38,39]
    15p4 data tx fifo r           : 0x00514028  [40,41,42,43]
    15p4 data tx fifo w           : 0x0051402c  [44,45,46,47]
    dummy                         : 0x00514030  [48,49,50,51]
    dummy                         : 0x00514034  [52,53,54,55]
    dummy                         : 0x00514038  [56,57,58,59]
    Rx Type FIFO                  : 0x0051403c  [256 bytes]
    HCI Event FIFO                : 0x0051413c  [2048 bytes]

Tx Queue,     start:0x00508000, end:0x0050a3fc, length:9212 bytes

Register RAM, start:0x00510000, end:0x00510200, length:512 bytes
    HCI Command FIFO Read Pointer : 0x00510000
    HCI Command FIFO Write Pointer: 0x00510004

    Tx Queue Prio Pointer         : 0x00510018
    Tx Queue Acl Handle Pointer   : 0x0051001c
    Tx Queue Status Pointer       : 0x00510020
    Dummy                         : 0x00510024

    Driver Firmware Status Pointer: 0x005101fc

Rx Queue,     start:0x00500000, end:0x00501000, length:4096 bytes

Command FIFO, start:0x00518000, end:0x00519000, length:4096 bytes
---------------------------------------------------------------------------------*/
#define W2L_ICCM_SIZE               0x38000
#define W2L_DCCM_SIZE               0x20000
#define W2L_ROM_SIZE                384*1024
#define W2L_ICCM_AHB_BASE_ADDR      0x00300000
#define W2L_DCCM_AHB_BASE_ADDR      0x00400000
#define W2L_ICCM_RAM_BASE_ADDR      0x00000000
#define W2L_DCCM_RAM_BASE_ADDR      0x00d00000

#define W2L_DOWNLOAD_SIZE           4096
#define W2L_USB_RX_Q_LEN            USB_RX_Q_LEN*4
#define W2L_USB_POLL_LEN            2364        //60 bytes register+256 bytes type fifo+2048 bytes event fifo
#define W2L_USB_EVT_FIFO_LEN        2048
#define W2L_USB_CMD_FIFO_LEN        4096
#define W2L_USB_15P4_RX_FIFO_LEN    2048
#define W2L_USB_15P4_TX_FIFO_LEN    2048

#define W2L_USB_MEM1_ADDR           0x00500000  //w2l usb memory1, length:4096 bytes
#define W2L_USB_MEM2_ADDR           0x00508000  //w2l usb memory2, length:9212 bytes
#define W2L_USB_MEM3_ADDR           0x00510000  //w2l usb memory3, length:512 bytes
#define W2L_USB_CMD_Q_R_POINT       W2L_USB_MEM3_ADDR+0x00
#define W2L_USB_CMD_Q_W_POINT       W2L_USB_MEM3_ADDR+0x04
#define W2L_USB_DRIVER_FW_STATUS    W2L_USB_MEM3_ADDR+0x1fc

#define W2L_USB_MEM4_ADDR           0x00514000  //w2l usb memory4, length:4096 bytes
#define W2L_USB_RX_TYPE_Q_R_POINT   W2L_USB_MEM4_ADDR+0x00
#define W2L_USB_EVT_Q_R_POINT       W2L_USB_MEM4_ADDR+0x04
#define W2L_USB_15P4_RX_Q_R_POINT   W2L_USB_MEM4_ADDR+0x08

#define W2L_USB_RX_Q_R_POINT        W2L_USB_MEM4_ADDR+0x0c
#define W2L_USB_RX_Q_W_POINT        W2L_USB_MEM4_ADDR+0x10
#define W2L_USB_15P4_RX_Q_W_POINT   W2L_USB_MEM4_ADDR+0x1c

#define W2L_USB_RX_TYPE_Q_W_POINT   W2L_USB_MEM4_ADDR+0x20
#define W2L_USB_EVT_Q_W_POINT       W2L_USB_MEM4_ADDR+0x24
#define W2L_USB_15P4_TX_Q_R_POINT   W2L_USB_MEM4_ADDR+0x28
#define W2L_USB_15P4_TX_Q_W_POINT   W2L_USB_MEM4_ADDR+0x2c

#define W2L_USB_RX_TYPE_Q_ADDR      W2L_USB_MEM4_ADDR+0x3c
#define W2L_USB_EVT_Q_ADDR          W2L_USB_MEM4_ADDR+0x13c


#define W2L_USB_CMD_Q_ADDR          0x00518000  //length:4096 bytes
#define W2L_USB_15P4_Q_ADDR         0x00700000  //length:2048 bytes
#define W2L_USB_15P4_RX_Q_ADDR      W2L_USB_15P4_Q_ADDR+0x2000
#define W2L_USB_15P4_TX_Q_ADDR      W2L_USB_15P4_Q_ADDR+0x3000

#define W2L_USB_TX_Q_PRIO_ADDR      W2L_USB_MEM3_ADDR + 0x18

/*-------------------------------W1D USB-----------------------------------------------
Event FIFO,   start:0xf7403000, end:0xf7404000, length:4096 bytes
    Rx Type FIFO Read Pointer     : 0xf7403000  [0,1,2,3]
    HCI Event FIFO Read Pointer   : 0xf7403004  [4,5,6,7]
    15p4 rx data fifo r           : 0xf7403008  [8,9,10,11]
    rx data fifo r reg            : 0xf740300c [12,13,14,15]
    rx data fifo w reg            : 0xf7403010  [16,17,18,19]
    sink mode status              : 0xf7403014  [20,21,22,23]
    dummy                         : 0xf7403018  [24,25,26,27]
    15p4 rx data fifo w           : 0xf740301c  [28,29,30,31]
    Rx Type FIFO Write Pointer    : 0xf7403020  [32,33,34,35]
    HCI Event FIFO Write Pointer  : 0xf7403024  [36,37,38,39]
    15p4 data tx fifo r           : 0xf7403028  [40,41,42,43]
    15p4 data tx fifo w           : 0xf740302c  [44,45,46,47]
    dummy                         : 0xf7403030  [48,49,50,51]
    dummy                         : 0xf7403034  [52,53,54,55]
    dummy                         : 0xf7403038  [56,57,58,59]
    Rx Type FIFO                  : 0xf740303c  [256 bytes]
    HCI Event FIFO                : 0xf740303c  [2048 bytes]

Tx Queue,     start:0xf7400000, end:0xf7402000, length:9212 bytes

Register RAM, start:0xf7407c00, end:0xf7408000, length:1024 bytes
    HCI Command FIFO Read Pointer : 0xf7407c00
    HCI Command FIFO Write Pointer: 0xf7407c04

    Tx Queue Prio Pointer         : 0xf7407c18
    Tx Queue Acl Handle Pointer   : 0xf7407c1c
    Tx Queue Status Pointer       : 0xf7407c20
    Dummy                         : 0xf7407c24

    Driver Firmware Status Pointer: 0xf7407dfc

Rx Queue,     start:0xf7406200, end:0xf7407200, length:4096 bytes

Command FIFO, start:0xf7408000, end:0xf7409000, length:4096 bytes
---------------------------------------------------------------------------------*/
#define W1D_ICCM_SIZE               0x18000
#define W1D_DCCM_SIZE               0x20000
#define W1D_ROM_SIZE                0x80000
#define W1D_ADD_SIZE                0x24000
#define W1D_ROM_RAM_BASE_ADDR       0xf7000000
#define W1D_ICCM_RAM_BASE_ADDR      0xf7080000
#define W1D_DCCM_RAM_BASE_ADDR      0xf7200000
#define W1D_ADD_RAM_BASE_ADDR       0xf7510000

#define W1D_DOWNLOAD_SIZE           4096
#define W1D_USB_RX_Q_LEN            USB_RX_Q_LEN*4
#define W1D_USB_POLL_LEN            2364        //60 bytes register+256 bytes type fifo+2048 bytes event fifo
#define W1D_USB_EVT_FIFO_LEN        2048
#define W1D_USB_CMD_FIFO_LEN        4096
#define W1D_USB_15P4_RX_FIFO_LEN    2048
#define W1D_USB_15P4_TX_FIFO_LEN    2048

#define W1D_USB_MEM1_ADDR           0xf7406200  //W1D usb memory1, length:4096 bytes
#define W1D_USB_MEM2_ADDR           0xf7400000  //W1D usb memory2, length:9212 bytes
#define W1D_USB_MEM3_ADDR           0xf7407c00  //W1D usb memory3, length:1024 bytes
#define W1D_USB_CMD_Q_R_POINT       W1D_USB_MEM3_ADDR+0x00
#define W1D_USB_CMD_Q_W_POINT       W1D_USB_MEM3_ADDR+0x04
#define W1D_USB_DRIVER_FW_STATUS    W1D_USB_MEM3_ADDR+0x1fc

#define W1D_USB_MEM4_ADDR           0xf7403000  //W1D usb memory4, length:4096 bytes
#define W1D_USB_RX_TYPE_Q_R_POINT   W1D_USB_MEM4_ADDR+0x00
#define W1D_USB_EVT_Q_R_POINT       W1D_USB_MEM4_ADDR+0x04
#define W1D_USB_15P4_RX_Q_R_POINT   W1D_USB_MEM4_ADDR+0x08

#define W1D_USB_RX_Q_R_POINT        W1D_USB_MEM4_ADDR+0x0c
#define W1D_USB_RX_Q_W_POINT        W1D_USB_MEM4_ADDR+0x10
#define W1D_USB_15P4_RX_Q_W_POINT   W1D_USB_MEM4_ADDR+0x1c

#define W1D_USB_RX_TYPE_Q_W_POINT   W1D_USB_MEM4_ADDR+0x20
#define W1D_USB_EVT_Q_W_POINT       W1D_USB_MEM4_ADDR+0x24
#define W1D_USB_15P4_TX_Q_R_POINT   W1D_USB_MEM4_ADDR+0x28
#define W1D_USB_15P4_TX_Q_W_POINT   W1D_USB_MEM4_ADDR+0x2c

#define W1D_USB_RX_TYPE_Q_ADDR      W1D_USB_MEM4_ADDR+0x3c
#define W1D_USB_EVT_Q_ADDR          W1D_USB_MEM4_ADDR+0x13c


#define W1D_USB_CMD_Q_ADDR          0xf7408000  //length:4096 bytes
#define W1D_USB_15P4_RX_Q_ADDR      0xf7405400  //length:2048 bytes
#define W1D_USB_15P4_TX_Q_ADDR      0xf7407200  //length:2048 bytes

#define W1D_USB_TX_Q_PRIO_ADDR      W1D_USB_MEM3_ADDR + 0x18

int amlbt_chip_id_init(void);
void amlbt_chip_info_print(amlbt_t *p_bt);
unsigned int amlbt_chip_get_iccm_size(amlbt_t *p_bt);
const char* amlbt_chip_get_firmware_name(amlbt_t *p_bt);
const char* amlbt_chip_get_firmware_test_name(amlbt_t *p_bt);

#endif

