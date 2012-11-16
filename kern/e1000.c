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
		tx_bufs[i].status |= E1000_TXD_STATUS_DD;
	}
	//initializing transmit descriptor base registers
	cprintf("e1000 = %p\n", e1000);
	e1000[E1000_TDBAL] = PADDR(tx_bufs);
	e1000[E1000_TDBAH] = 0;

	//initializing transmit descriptor length
	e1000[E1000_TDLEN] = sizeof(struct tx_desc) * E1000_TX_DESC;

	//now for head & tail
	e1000[E1000_TDH] = 0;
	e1000[E1000_TDT] = 0;

	//now for control registers
	e1000[E1000_TCTL] |= E1000_TCTL_EN;
	e1000[E1000_TCTL] |= E1000_TCTL_PSP;
	e1000[E1000_TCTL] &= ~E1000_TCTL_CT;
	e1000[E1000_TCTL] |= 0x10 << 4;
	e1000[E1000_TCTL] &= ~E1000_TCTL_COLD;
	e1000[E1000_TCTL] |= 0x40 << 12;

	//now for register bit descriptors
	e1000[E1000_TIPG] = 0;
	e1000[E1000_TIPG] |= 0x6 << 20; // IPGR2
	e1000[E1000_TIPG] |= 0x4 << 10; // IPGR1
	e1000[E1000_TIPG] |= 0xa;	// IPGT

	return 0;
}

int 
e1000_transmit (char *data, int len) {
	if (len > TX_PKT_SIZE){
		return -E_PKT_TOO_LONG;
	}
	
	uint32_t tdt = e1000[E1000_TDT];
	
	if (tx_bufs[tdt].status & E1000_TXD_STATUS_DD) {
		memmove(tx_pkt_bufs[tdt].buf, data, len);
		tx_bufs[tdt].length = len;

		//We need to actually dump the data

	}
	return 0;
}


