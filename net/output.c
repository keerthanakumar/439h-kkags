#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	int r;

	while(1){
		cprintf("net/output.c: about to call sys_ipc_recv\n");
		r = sys_ipc_recv(&nsipcbuf);
		cprintf("net/output.c: sys_ipc_recv complete, data = %c, leng = %d\n", *nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len);
		if((thisenv->env_ipc_from != ns_envid) || (thisenv->env_ipc_value != NSREQ_OUTPUT)){
			continue;
		}
//		r = sys_net_send("bla", 11);
		cprintf("net/output.c: about to call sys_net_send, nsipcbuf.pkt.jp_data = %c, nsipcbuf.pkt.jp_len = %d\n", *nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len);
		while((r= sys_net_send(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len)) !=0){
			cprintf("net/output.c: sys_net_send failed\n");
		}
		cprintf("net/output.c: sys_net_send success, jp_data = %c, jp_len = %d\n", *nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len);
	}
}
