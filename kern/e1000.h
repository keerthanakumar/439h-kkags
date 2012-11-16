#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#include <kern/pci.h>

#define E1000_VENDORID	0x8086
#define E1000_DEVICEID	0x100e
#define E1000_MMIO_ADDR	KSTACKTOP

#define E1000_STATUS 0x00008/4

//we divided all these by 4 and Kk does know why
#define E1000_TDBAL    0x03800/4  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    0x03804/4  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    0x03808/4  /* TX Descriptor Length - RW */
#define E1000_TDH      0x03810/4  /* TX Descriptor Head - RW */
#define E1000_TDT      0x03818/4  /* TX Descriptor Tail - RW */

#define E1000_TX_DESC		64
#define TX_PKT_SIZE		1518
#define E1000_TXD_STATUS_DD	0X1

#define E1000_TCTL     0x00400  /* TX Control - RW */
#define E1000_TCTL_EN     0x00000002    /* enable tx */
#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
#define E1000_TCTL_CT     0x00000ff0    /* collision threshold */
#define E1000_TCTL_COLD   0x003ff000    /* collision distance */
#define E1000_TIPG     0x00410  /* TX Inter-packet gap -RW */

volatile uint32_t* e1000;
int e1000_attach(struct pci_func *pcifunc);

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
#endif	// JOS_KERN_E1000_H
