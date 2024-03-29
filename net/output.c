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
		r = sys_ipc_recv(&nsipcbuf);
		if((thisenv->env_ipc_from != ns_envid) && (thisenv->env_ipc_value != NSREQ_OUTPUT)){
			break;

		}
		if((r= sys_net_send(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len)) <0){
			cprintf("net/output.c: sys_net_send failed\n");
		}
	}
}
