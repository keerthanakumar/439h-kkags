#include <inc/string.h>
#include <kern/pmap.h>
#include <kern/e1000.h>
#include <inc/assert.h>
#include <inc/error.h>

// LAB 6: Your driver code here
struct tx_desc tx_bufs[E1000_TX_DESC];
struct tx_pkt tx_pkt_bufs[E1000_TX_DESC];
struct rx_desc rx_bufs[E1000_RX_DESC];
struct rx_pkt rx_pkt_bufs[E1000_RX_DESC];

int
e1000_attach(struct pci_func *pcifunc) {
	pci_func_enable(pcifunc);
	e1000 = (void*)mmio_map_region(pcifunc->reg_base[0], pcifunc->reg_size[0]);
	assert(e1000[E1000_STATUS] == 0X80080783);

	memset(tx_bufs, 0,
		sizeof(struct tx_desc) * E1000_TX_DESC);
	memset(tx_pkt_bufs, 0,
		sizeof(struct tx_pkt) * E1000_TX_DESC);
	int i;
	for (i = 0; i < E1000_TX_DESC; i++) {
		tx_bufs[i].addr = PADDR(tx_pkt_bufs[i].buf);
		tx_bufs[i].status |= E1000_TXD_STATUS_DD;
	}
	//initializing transmit descriptor base registers
	e1000[E1000_TDBAL] = PADDR(tx_bufs);
	e1000[E1000_TDBAH] = 0;

	//initializing transmit descriptor length
	e1000[E1000_TDLEN] = sizeof(tx_bufs);

	//now for head & tail
	e1000[E1000_TDH] = 0;
	e1000[E1000_TDT] = 0;

	//now for control registers
	e1000[E1000_TCTL] = 0;
	e1000[E1000_TCTL] |= E1000_TCTL_EN;
	e1000[E1000_TCTL] |= E1000_TCTL_PSP;
	e1000[E1000_TCTL] |= 0x10 << 4;
	e1000[E1000_TCTL] |= 0x40 << 12;

	//register bit descriptors, part deux
	e1000[E1000_TIPG] = (0x6 << 20)|(0x8 << 10)|(0xa);

	for (i = 0; i < E1000_RX_DESC; i++) {
		rx_bufs[i].addr = PADDR(rx_pkt_bufs[i].buf);
		rx_bufs[i].status = 0;
	}

	e1000[E1000_MTA] = 0;

	e1000[E1000_RDH] = 0;
	e1000[E1000_RDT] = 0;
	e1000[E1000_RDBAL] = PADDR(rx_bufs);
	e1000[E1000_RDBAH] = 0;

	e1000[E1000_RDLEN] = sizeof(struct rx_desc) * E1000_RX_DESC;

	//control registers
	e1000[E1000_RCTL] = 0;
	e1000[E1000_RCTL] |= E1000_RCTL_SECRC;
	e1000[E1000_RCTL] |= E1000_RCTL_BAM;
		
//	e1000[E1000_RAL] = 0x12005452;
//	e1000[E1000_RAH] = 0x80005634;
	e1000[E1000_RCTL] |= E1000_RCTL_EN;

//MAC Address initialization
	e1000[E1000_EERD] = 0x0;
	e1000[E1000_EERD] |= E1000_EERD_START;
	while (!(e1000[E1000_EERD] & E1000_EERD_DONE));
	e1000[E1000_RAL] = e1000[E1000_EERD] >> 16;

	e1000[E1000_EERD] = 0x1 << 8;
	e1000[E1000_EERD] |= E1000_EERD_START;
	while (!(e1000[E1000_EERD] & E1000_EERD_DONE));
	e1000[E1000_RAL] |= e1000[E1000_EERD] & 0xffff0000;

	e1000[E1000_EERD] = 0x2 << 8;
	e1000[E1000_EERD] |= E1000_EERD_START;
	while (!(e1000[E1000_EERD] & E1000_EERD_DONE));
	e1000[E1000_RAH] = e1000[E1000_EERD] >> 16;

	e1000[E1000_RAH] |= 0x1 << 31;
	return 0;
}

int 
e1000_transmit (char *data, int len) {
	int numBufs = len % TX_PKT_SIZE == 0 ? len / TX_PKT_SIZE : len / TX_PKT_SIZE + 1;
	int tdt = e1000[E1000_TDT];

	int bufNum;
	int totalBytes = 0;
	for (bufNum = 0; bufNum < numBufs; bufNum++) {
		tdt = e1000[E1000_TDT];

		int byteNum;
		for (byteNum = 0; byteNum < TX_PKT_SIZE; byteNum++) {
			tx_pkt_bufs[tdt].buf[byteNum] = data[totalBytes];
			totalBytes++;
			if (totalBytes >= len) break;
		}
		tx_bufs[tdt].length = len - (bufNum * TX_PKT_SIZE);
		tx_bufs[tdt].cmd |= E1000_TXD_CMD_RS;
		tx_bufs[tdt].status &= ~E1000_TXD_STATUS_DD;
		if (bufNum == numBufs - 1)
			tx_bufs[tdt].cmd |= E1000_TXD_CMD_EOP;
		e1000[E1000_TDT] = (tdt + 1) % E1000_TX_DESC;
	}

	return 0;
}

int
e1000_receive (char*  data){
	int len;
	int rdt = e1000[E1000_RDT];
	int i;
	if (!(rx_bufs[rdt].status & E1000_RXD_STATUS_DD)) {
		return -E_RX_TRYAGAIN;
	}
	len = rx_bufs[rdt].length;
	rx_bufs[rdt].length = 0;
	memcpy(data,rx_pkt_bufs[rdt].buf, len);
	rx_bufs[rdt].status &= ~E1000_RXD_STATUS_DD;
	rx_bufs[rdt].status &= ~E1000_RXD_STAT_EOP;
	e1000[E1000_RDT] = (rdt + 1) % E1000_RX_DESC;

	return len;
}
