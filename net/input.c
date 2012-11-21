#include "ns.h"

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.

//iwejfsivhj
#define RECV_BUF_SIZE 1518	
		char buf[RECV_BUF_SIZE]; //RECEIVER BUFFER SIZE
		int r;
		int perm  = PTE_U | PTE_P | PTE_W;
		int len = RECV_BUF_SIZE - 1;
		int i;
		while(1) {
			cprintf("net/input.c: about to try sys_net_receive\n");
			while((r = sys_net_receive(buf, &len)) < 0) {
				cprintf("net/input.c: tried sys_net_receive, yielding\n");
				sys_yield();
			}
			cprintf("net/input.c: sys_net_receive success, data = %c, len = %d\n", buf[0], len);
			while((r = sys_page_alloc(0, &nsipcbuf, perm))<0);
			cprintf("net/input.c: page_alloc success\n");
			nsipcbuf.pkt.jp_len = len;
			memmove(nsipcbuf.pkt.jp_data, buf, len);

			cprintf("net/input.c: about to call sys_ipc_try_send, nsipcbuf.pkt.jp_data = %c, len = %d\n", nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len);	
			while((r = sys_ipc_try_send(ns_envid, NSREQ_INPUT, &nsipcbuf, perm)) < 0);
			cprintf("net/input.c: sys_ipc_try_send success\n");
			
			sys_page_unmap(0, &nsipcbuf);
		}

}
