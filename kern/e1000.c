#include <inc/string.h>
#include <kern/pmap.h>
#include <kern/e1000.h>
#include <inc/assert.h>
#include <inc/error.h>

// LAB 6: Your driver code here
struct tx_desc tx_bufs[E1000_TX_DESC];
struct tx_pkt tx_pkt_bufs[E1000_TX_DESC];

int
e1000_attach(struct pci_func *pcifunc) {
	pci_func_enable(pcifunc);
	e1000 = (void*)mmio_map_region(pcifunc->reg_base[0], pcifunc->reg_size[0]);
//	boot_map_region(kern_pgdir, E1000_MMIO_ADDR,
//		pcifunc->reg_size[0], pcifunc->reg_base[0],
//		PTE_W | PTE_PCD | PTE_PWT);
//	e1000 = (void*)E1000_MMIO_ADDR;
	assert(e1000[E1000_STATUS] == 0X80080783);

	memset(tx_bufs, 0,
		sizeof(struct tx_desc) * E1000_TX_DESC);
	memset(tx_pkt_bufs, 0,
		sizeof(struct tx_pkt) * E1000_TX_DESC);
	int i;
	for (i = 0; i < E1000_TX_DESC; i++) {
		tx_bufs[i].addr = PADDR(tx_pkt_bufs[i].buf);
//		cprintf("init: tx_bufs[%d].addr = %p\n", i, tx_bufs[i].addr);
//		cprintf("\tPADDR(tx_pkt_bufs[%d].buf) = %p\n", i, PADDR(tx_pkt_bufs[i].buf));
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
//	e1000[E1000_TCTL] &= ~E1000_TCTL_CT;
	e1000[E1000_TCTL] |= 0x10 << 4;
//	e1000[E1000_TCTL] &= ~E1000_TCTL_COLD;
	e1000[E1000_TCTL] |= 0x40 << 12;

	//register bit descriptors, part deux
	e1000[E1000_TIPG] = (0x6 << 20)|(0x8 << 10)|(0xa);

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
