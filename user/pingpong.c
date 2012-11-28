// Ping-pong a counter between two processes.
// Only need to start one of these -- splits into two with fork.

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	envid_t who;

	if ((who = fork()) != 0) {
		// get the ball rolling
		cprintf("send 0 from %x to %x\n", sys_getenvid(), who);
		//send a 0 to who, no page or perm
		ipc_send(who, 0, 0, 0);
	}

	while (1) {
		//receive from who, no pg or perm
		uint32_t i = ipc_recv(&who, 0, 0);
		cprintf("%x got %d from %x\n", sys_getenvid(), i, who);
		if (i == 10)
			return;
		i++;
		//send to who an i, no pg or perm
		ipc_send(who, i, 0, 0);
		if (i == 10)
			return;
	}

}

