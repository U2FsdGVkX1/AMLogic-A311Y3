/*
* Copyright (c) 202X Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*/

#ifndef __INTF_UART_COEX_H__
#define __INTF_UART_COEX_H__

#define HCI_COMMAND_PKT        0x01
#define HCI_ACLDATA_PKT        0x02
#define HCI_SCODATA_PKT        0x03
#define HCI_EVENT_PKT          0x04
#define HCI_15P4_PKT           0x10
#define HCI_15P4_HDR_SIZE      4

#define HCI_AML_ZIGBEE_TYPE    0xF5
#define HCI_AML_THREAD_TYPE    0xFA

#define H4_RECV_ACL \
    .type = HCI_ACLDATA_PKT, \
    .hlen = HCI_ACL_HDR_SIZE, \
    .loff = 2, \
    .lsize = 2, \
    .maxlen = HCI_MAX_FRAME_SIZE \

#define H4_RECV_SCO \
    .type = HCI_SCODATA_PKT, \
    .hlen = HCI_SCO_HDR_SIZE, \
    .loff = 2, \
    .lsize = 1, \
    .maxlen = HCI_MAX_SCO_SIZE

#define H4_RECV_EVENT \
    .type = HCI_EVENT_PKT, \
    .hlen = HCI_EVENT_HDR_SIZE, \
    .loff = 1, \
    .lsize = 1, \
    .maxlen = HCI_MAX_EVENT_SIZE

#define H4_RECV_ISO \
    .type = HCI_ISODATA_PKT, \
    .hlen = HCI_ISO_HDR_SIZE, \
    .loff = 2, \
    .lsize = 2, \
    .maxlen = HCI_MAX_FRAME_SIZE \

#define H4_RECV_15P4 \
    .type = HCI_15P4_PKT, \
    .hlen = HCI_15P4_HDR_SIZE, \
    .loff = 2, \
    .lsize = 3, \
    .maxlen = HCI_MAX_EVENT_SIZE\

struct h4_recv_pkt {
    u8  type;   /* Packet type */
    u8  hlen;   /* Header length */
    u8  loff;   /* Data length offset in header */
    u8  lsize;  /* Data length field size */
    u16 maxlen; /* Max overall packet length */
    int (*recv)(struct hci_dev *hdev, struct sk_buff *skb);
};

//struct sk_buff *h4_recv_buf(struct hci_dev *hdev, struct sk_buff *skb,
//                const unsigned char *buffer, int count,
//                const struct h4_recv_pkt *pkts, int pkts_count);

int aml_coex_init(void);
void aml_coex_deinit(void);

#endif

