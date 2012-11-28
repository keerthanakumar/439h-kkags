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
<<<<<<< HEAD
=======
#define RX_PKT_SIZE 2048	
		char buf[RX_PKT_SIZE];
		int r;
		int perm  = PTE_U | PTE_P | PTE_W;
		int len ;//= RX_PKT_SIZE - 1;
		int i;
		while(1) {
			while((r = sys_net_receive(buf)) < 0) {
				sys_yield();
			}
			len = r;
			while((r = sys_page_alloc(0, &nsipcbuf, perm))<0);
			nsipcbuf.pkt.jp_len = len;
			memmove(nsipcbuf.pkt.jp_data, buf, len);

			while((r = sys_ipc_try_send(ns_envid, NSREQ_INPUT, &nsipcbuf.pkt, perm)) < 0);
			
			sys_page_unmap(0, &nsipcbuf);
		}

>>>>>>> lab6
}
