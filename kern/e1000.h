#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

#define E1000_VENDORID	0x8086
#define E1000_DEVICEID	0x100e

#define E1000_STATUS 0x00008/4

#define E1000_EERD     0x00014/4  /* EEPROM Read - RW */

//we divided all these by 4
#define E1000_TDBAL    0x03800/4  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    0x03804/4  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    0x03808/4  /* TX Descriptor Length - RW */
#define E1000_TDH      0x03810/4  /* TX Descriptor Head - RW */
#define E1000_TDT      0x03818/4  /* TX Descriptor Tail - RW */

#define TX_PKT_SIZE		1518

#define E1000_TXD_STATUS_DD	0X1
#define E1000_TXD_CMD_RS	0X8
#define E1000_TXD_CMD_EOP	0X1

#define E1000_TX_DESC           64

#define E1000_RX_DESC		64
#define RX_PKT_SIZE		1518 //i'm not sure

#define E1000_RXD_STATUS_DD	0x1
#define E1000_RXD_STAT_EOP      0x02    /* End of Packet */

#define E1000_TCTL     0x00400/4  /* TX Control - RW */
#define E1000_TCTL_EN     0x00000002    /* enable tx */
#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
#define E1000_TCTL_CT     0x00000ff0    /* collision threshold */
#define E1000_TCTL_COLD   0x003ff000    /* collision distance */
#define E1000_TIPG     0x00410/4  /* TX Inter-packet gap -RW */

#define E1000_RDBAL    0x02800/4  /* RX Descriptor Base Address Low - RW */
#define E1000_RDBAH    0x02804/4  /* RX Descriptor Base Address High - RW */
#define E1000_RDLEN    0x02808/4  /* RX Descriptor Length - RW */
#define E1000_RDH      0x02810/4  /* RX Descriptor Head - RW */
#define E1000_RDT      0x02818/4  /* RX Descriptor Tail - RW */
#define E1000_RAL	0x05400/4 /* Receive addr lo*/
#define E1000_RAH	0x05404/4 /* Receive addr hi*/

#define E1000_RCTL     0x00100/4  /* RX Control - RW */
#define E1000_RCTL_EN             0x00000002    /* enable */
#define E1000_RCTL_LPE            0x00000020    /* long packet enable */

#define E1000_RCTL_LBM_NO         0x00000000    /* no loopback mode */
#define E1000_RCTL_LBM_MAC        0x00000040    /* MAC loopback mode */
#define E1000_RCTL_LBM_SLP        0x00000080    /* serial link loopback mode */
#define E1000_RCTL_LBM_TCVR       0x000000C0    /* tcvr loopback mode */
#define E1000_RCTL_LBM	E1000_RCTL_LBM_MAC //not sure about this, might want to try all four above in debugging

#define E1000_RCTL_MO           0x00003000    /* multicast offset 15:4 */ //chose a specific one, MO_3
#define E1000_RCTL_BAM            0x00008000    /* broadcast enable */
#define E1000_RCTL_SZ         0x00030000    /* rx buffer size 256 */ //assuming BSEX to be 0, and this is SZ_256
#define E1000_RCTL_SECRC          0x04000000    /* Strip Ethernet CRC */

#define E1000_RCTL_RDMTS	0x300		/* descriptor minimum threshold size */ //manually created
#define E1000_MTA		0x5200/4

#define E1000_EERD_START	0x1
#define E1000_EERD_DONE		0x10

volatile uint32_t* e1000;
int e1000_attach(struct pci_func *pcifunc);
int e1000_transmit (char *data, int len);
int e1000_receive (char **data, int* len);

struct tx_desc
{
	uint64_t addr;
	uint16_t length;
	uint8_t cso;
	uint8_t cmd;
	uint8_t status;
	uint8_t css;
	uint16_t special;
};

struct tx_pkt
{
	uint8_t buf[TX_PKT_SIZE];
	
};

struct rx_desc
{	
	uint64_t addr;
	uint16_t length;
	uint8_t checksum;
	uint8_t status;
	uint8_t error;
	uint16_t special;
};

struct rx_pkt
{
	uint8_t buf[RX_PKT_SIZE];
};

#endif	// JOS_KERN_E1000_H
