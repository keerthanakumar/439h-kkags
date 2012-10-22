// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/pmap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display stack backtrace", mon_backtrace },
	{ "alloc_page", "Manually allocate a page. Give a numerical argument to allocate multiple pages.", mon_alloc_page },
	{ "alloc_and_clear_page", "Manually allocate a cleared page. Give a numerical argument to allocate multiple pages.", mon_alloc_and_clear_page },
	{ "page_status", "Check status of one more pages (allocated or not)", mon_page_status },
	{ "free_page", "Manually free one or more pages", mon_free_page },
	{ "print_page", "Print details of one or more pages", mon_print_page },
	{ "num_free_pages", "Print number of free pages", mon_num_free_pages },
	{ "c", "Continue execution of running user program", mon_c },
	{ "si", "Execute next instruction of running user program", mon_si},
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

//CHANGE
int
mon_si(int argc, char **argv, struct Trapframe *tf) {
	tf->tf_eflags = tf->tf_eflags | FL_TF;
	return -1;
}

int
mon_c(int argc, char **argv, struct Trapframe *tf) {
	return -1;
}

int
mon_alloc_page(int argc, char **argv, struct Trapframe *tf) {
	int num_pages = 1;
	if (argc > 1) {
		num_pages = strtol(argv[1], 0, 10);
	}
	struct Page* pp;
	int i;
	for (i = 0; i < num_pages; i++) {
		if ((pp = page_alloc(0))) {
			pp->pp_ref++;
			cprintf("\t%p\n", pp);
		}
		else {
			cprintf("\tNo more pages left to alloc\n");
		}
	}
	return 1;
}

int
mon_alloc_and_clear_page(int argc, char **argv, struct Trapframe *tf) {
	int num_pages = 1;
	if (argc > 1) {
		num_pages = strtol(argv[1], 0, 10);
	}
	struct Page* pp;
	int i;
	for (i = 0; i < num_pages; i++) {
		if ((pp = page_alloc(ALLOC_ZERO))) {
			cprintf("\t%p\n", pp);
		}
		else {
			cprintf("\tNo more pages left to alloc\n");
		}
	}
	return 1;
}


int
mon_page_status(int argc, char **argv, struct Trapframe *tf) {
	if (argc <= 1) {
		cprintf("\tneeds argument\n");
		return 0;
	}
	int i;
	for(i = 1; i < argc; i++) {
		struct Page* pp = (struct Page*)strtol(argv[i], 0, 16);
		if (pp->pp_ref > 0) {
			cprintf("\t%p is allocated\n", pp);
		}
		else {
			cprintf("\t%p is not allocated\n", pp);
		}	
	}
	return 1;
}

int
mon_free_page(int argc, char **argv, struct Trapframe *tf) {
	if (argc <= 1) {
		cprintf("\tneeds argument\n");
		return 0;
	}
	int i;
	for (i = 1; i < argc; i++) {
		struct Page* pp = (struct Page*)strtol(argv[i], 0, 16);
		pp->pp_ref = 0;
		page_free(pp);
	}
	return 1;
}

int
mon_print_page(int argc, char **argv, struct Trapframe *tf) {
	if (argc <= 1) {
		cprintf("\tneeds argument\n");
		return 0;
	}
	int i;
	for (i = 1; i < argc; i++) {
		struct Page* pp = (struct Page*)strtol(argv[i], 0, 16);
		void* va;
		physaddr_t pa;
		int pp_ref;
		cprintf("\tpage %p:\n", pp);
		if ((pp_ref = pp->pp_ref)) {
			cprintf("\t\tpp_ref = %d\n", pp_ref);
		}
		else {
			cprintf("\t\tpp->pp_ref is null\n");
		}
		if ((va = page2kva(pp))) {
			cprintf("\t\tva = %p\n", va);
		}
		else {
			cprintf("\t\tva is null\n");
		}
		if ((pa = page2pa(pp))) {
			cprintf("\t\tpa = %p\n", pa);
		}
		else {
			cprintf("\t\tpa is null\n");
		}
	}

	return 1;
}

int
mon_num_free_pages(int argc, char **argv, struct Trapframe *tf) {
	int n = num_free_pages();
	cprintf("\t%d free pages\n", n);
	return 1;
}
//ENDCHANGE

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	struct Eipdebuginfo info;
	int* ebp = (int*)read_ebp();
	while (ebp != 0) {
		int eip = *(ebp + 1);
		if (debuginfo_eip(eip, &info) == -1)
			cprintf("debuginfo_eip didn't work\n");
		int arg1 = *(ebp + 2);
		int arg2 = *(ebp + 3);
		int arg3 = *(ebp + 4);
		int arg4 = *(ebp + 5);
		int arg5 = *(ebp + 6);
		cprintf("ebp %x  ", ebp);
		cprintf("eip %x  ", eip);
		cprintf("args %08x %08x %08x %08x %08x\n", arg1, arg2, arg3, arg4, arg5);
		cprintf("       %s:%d: %.*s+%d\n", info.eip_file, info.eip_line/ 4, info.eip_fn_namelen, info.eip_fn_name, (eip - info.eip_fn_addr) / 4);
		ebp = (int*)*ebp;
	}
	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL) {
		print_trapframe(tf);	
		//cprintf("lol: %p\n", tf->tf_eip);
	}

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
