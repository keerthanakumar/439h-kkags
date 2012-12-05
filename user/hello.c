// hello, world
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	cprintf("hello, world\n");
	sys_tcreate(thisenv->env_id, USER_HELLO);
	cprintf("i am environment %08x\n", thisenv->env_id);
}
