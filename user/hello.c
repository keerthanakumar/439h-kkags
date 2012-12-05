// hello, world
#include <inc/lib.h>
//for 'big daddy' functionality
#include <lib/thread.h>

void
umain(int argc, char **argv)
{
	cprintf("hello, world\n");
	thread_create(thisenv->env_id, USER_HELLO, THREAD_TYPE_USER);
	cprintf("i am environment %08x\n", thisenv->env_id);
}
