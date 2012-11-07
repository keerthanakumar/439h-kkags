// Fork a binary tree of processes and display their structure.

#include <inc/lib.h>

#define DEPTH 3

void forktree(const char *cur);

void
forkchild(const char *cur, char branch)
{
	cprintf("\nforktree.c: forkchild called with cur = %s & branch = %c\n", cur, branch);
	char nxt[DEPTH+1];

	if (strlen(cur) >= DEPTH) {
		cprintf("forktree.c: strlen(cur) >= DEPTH\n");
		return;
	}

	snprintf(nxt, DEPTH+1, "%s%c", cur, branch);
	if (fork() == 0) {
		cprintf("\tabout to forktree\n");
		forktree(nxt);
		exit();
	}
}

void
forktree(const char *cur)
{
	cprintf("%04x: I am '%s'\n", sys_getenvid(), cur);

	forkchild(cur, '0');
	forkchild(cur, '1');
}

void
umain(int argc, char **argv)
{
	forktree("");
}

