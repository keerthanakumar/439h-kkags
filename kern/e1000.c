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

//	memset(tx_bufs, 0,
//		sizeof(struct tx_desc) * E1000_TX_DESC);
//	memset(tx_pkt_bufs, 0,
//		sizeof(struct tx_pkt) * E1000_TX_DESC);
	int i;
	for (i = 0; i < E1000_TX_DESC; i++) {
		tx_bufs[i].addr = PADDR(tx_pkt_bufs[i].buf);
		tx_bufs[i].status |= E1000_TXD_STATUS_DD;
	}
	//initializing transmit descriptor base registers
	e1000[E1000_TDBAL] = PADDR(tx_bufs);
	e1000[E1000_TDBAH] = 0;

	//initializing transmit descriptor length
	e1000[E1000_TDLEN] = sizeof(tx_bufs);//sizeof(struct tx_desc) * E1000_TX_DESC;

	//now for head & tail
	e1000[E1000_TDH] = 0;
	e1000[E1000_TDT] = 0;

	//now for control registers
	e1000[E1000_TCTL] |= E1000_TCTL_PSP;
	e1000[E1000_TCTL] &= ~E1000_TCTL_CT;
	e1000[E1000_TCTL] |= 0x10 << 4;
	e1000[E1000_TCTL] &= ~E1000_TCTL_COLD;
	e1000[E1000_TCTL] |= 0x40 << 12;
	cprintf("TCTL before enable set = %p\n", e1000[E1000_TCTL]);
	e1000[E1000_TCTL] |= E1000_TCTL_EN;
	cprintf("TCTL after init = %p\n", e1000[E1000_TCTL]);

	//register bit descriptors, part deux
	e1000[E1000_TIPG] = 0x702008;
//(7<<20)|(8<<10)|(8<<0);

	return 0;
}

void 
bufDump(const char* prefix, void* data, int len) {
	cprintf("\t\tEntered Buff with data = %p, len = %d\n", data, len);
	char buf[80];
	char* end = buf + sizeof(buf);
	char *out = NULL;
	int i;
	for (i = 0; i < len; i++) {
		if(i%16 == 0)
			out = buf + snprintf(buf, end-buf, "%s%04x ", prefix, i);

		out += snprintf(out, end - out, "%02x", ((uint8_t*) data)[i]);

		if (i % 16 == 15 || i == len - 1);
			//cprintf("%.*s\n", out - buf, buf);
		if (i % 2 == 1)
			*(out++) = ' ';
		if (i % 16 == 7)
			*(out++) = ' ';
	}
}

int 
e1000_transmit (char *data, int len) {
	if (len > TX_PKT_SIZE){
		cprintf("\tERROR: kern/e1000.c:e1000_transmit: the packet is too long\n");
		return -E_PKT_TOO_LONG;
	}
	
	cprintf("Entered e1000_transmit\n");
	uint32_t tdt = e1000[E1000_TDT];
	cprintf("\ttdt = %x, E1000_TDT = %x\n", tdt, E1000_TDT);
	if (tx_bufs[tdt].status & E1000_TXD_STATUS_DD) {
		cprintf("\tData = %s\n", data);
		cprintf("\tLen = %x\n", len);
		int i;
		for (i = 0; i < len; i++) {
			tx_pkt_bufs[tdt].buf[i] = data[i];
		}
		//memmove(tx_pkt_bufs[tdt].buf, data, len);
		tx_bufs[tdt].length = len;
		cprintf("\ttx_pkt_buf[tdt].buf = %s\n", tx_pkt_bufs[tdt].buf);

		//We need to actually dump the data
		//bufDump("e1000:", data, len);
		cprintf("\ttxt_bufs[tdt].status before = %x\n", tx_bufs[tdt].status);
		tx_bufs[tdt].status &= ~E1000_TXD_STATUS_DD;
		cprintf("\ttxt_bufs[tdt].status after = %x\n", tx_bufs[tdt].status);
		cprintf("\tTCTL = %p\n", e1000[E1000_TCTL]);
		tx_bufs[tdt].cmd |= E1000_TXD_CMD_RS;
		cprintf("\tTCTL = %p\n", e1000[E1000_TCTL]);
		tx_bufs[tdt].cmd |= E1000_TXD_CMD_EOP;
		cprintf("tdt before increment: %d\n", e1000[E1000_TDT]);
		e1000[E1000_TDT] = (tdt + 1) % E1000_TX_DESC;
		cprintf("tdt after increment: %d\n", e1000[E1000_TDT]);
		cprintf("\tTCTL = %p\n", e1000[E1000_TCTL]);
		cprintf("\te1000[E1000_TDT] = %x\n", e1000[E1000_TDT]);
		cprintf("\ttx_buf[tdt].addr = %p, cmd = %p, CSO = %p, len = %p, special = %p, CSS = %p, status = %p\n", tx_bufs[tdt].addr, tx_bufs[tdt].cmd, tx_bufs[tdt].cso, tx_bufs[tdt].length, tx_bufs[tdt].special, tx_bufs[tdt].css, tx_bufs[tdt].status);
	}
	else {
		return -E_TX_FULL;
	}
	return 0;
}
