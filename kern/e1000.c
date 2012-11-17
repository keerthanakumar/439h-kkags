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
	cprintf("kern/e1000.c:e1000_attach: entered\n");
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

	//register bit descriptors, part deux
	e1000[E1000_TIPG] = (7<<20)|(8<<10)|(8<<0);

	return 0;
}


void 
bufDump(const char* prefix, void* data, int len) {
	cprintf("\tkern/e1000.c:bufDump: entered\n");
	char buf[80];
	char* end = buf + sizeof(buf);
	char *out = NULL;
	int i;
	for (i = 0; i < len; i++) {
		if(i%16 == 0)
			out = buf + snprintf(buf, end-buf, "%s%04x ", prefix, i);

		out += snprintf(out, end - out, "%02x", ((uint8_t*) data)[i]);
			
		if (i % 16 == 15 || i == len - 1)
			cprintf("%.*s\n", out - buf, buf);
		if (i % 2 == 1)
			*(out++) = ' ';
		if (i % 16 == 7)
			*(out++) = ' ';
	}
}

int 
e1000_transmit (char *data, int len) {
	cprintf("kern/e1000.c:e1000_transmit: entered\n");
	if (len > TX_PKT_SIZE){
		cprintf("\tERROR: kern/e1000.c:e1000_transmit: the packet is too long\n");
		return -E_PKT_TOO_LONG;
	}
	
	uint32_t tdt = e1000[E1000_TDT];
	
	if (tx_bufs[tdt].status & E1000_TXD_STATUS_DD) {
		cprintf("\tkern/e1000.c:e1000_transmit: packet status passes\n");
		memmove(tx_pkt_bufs[tdt].buf, data, len);
		tx_bufs[tdt].length = len;

		//We need to actually dump the data
		bufDump("buf_prefix:", data, len);
		tx_bufs[tdt].status &= ~E1000_TXD_STATUS_DD;
		tx_bufs[tdt].cmd |= E1000_TXD_CMD_RS;
		tx_bufs[tdt].cmd |= E1000_TXD_CMD_EOP;
		e1000[E1000_TDT] = (tdt + 1) % E1000_TX_DESC;
	}
	else {
		cprintf("\tERROR: kern/e1000.c:e1000_transmit: this state ain't big enough for the two of us\n");
		return -E_TX_FULL;
	}
	cprintf("\tkern/e1000.c:e1000_transmit: returning\n");
	return 0;
}
