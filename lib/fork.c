// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>
//cprintf("\t\tsys_page_map: src->env_id = %d, dst->env_id = %d\n", src->env_id, dst->env_id);

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	//cprintf("\tpgfault called\n");
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if ((err & FEC_WR) == 0) {
		panic("faulting access was not a write");
	}
	pte_t pte = vpt[PGNUM((uint32_t)addr)];
	if (!(pte & PTE_COW)) {
		panic("faulting page was not COW");
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.
	void* vatemp = (void*)ROUNDDOWN((uint32_t)addr, PGSIZE);
	if ((r = sys_page_alloc(0, (void*)PFTEMP, PTE_P | PTE_U | PTE_W)) < 0) {
		panic("unable to allocate page %e\n", r);
	}
	memmove((void*)PFTEMP, vatemp, PGSIZE);
	if ((r = sys_page_map(0, PFTEMP, 0, vatemp, PTE_P | PTE_U | PTE_W)) < 0) {
		panic("unable to map page %e\n", r);
	}
	if ((r = sys_page_unmap(0, PFTEMP)) < 0) {
		panic("unable to unmap");
	}
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	//cprintf("\tduppage called, envid = %d, vpt[%d] = %d\n", envid, pn, vpt[pn]);
	int r;

	// LAB 4: Your code here.
	unsigned va = pn << PGSHIFT;
	if (!(vpt[pn] & PTE_P)) {
		return -1;
	}
	if (!((vpt[pn] & PTE_W) || (vpt[pn] & PTE_COW))) {
		if ((r = sys_page_map(0, (void*)va, envid, (void*)va, PGOFF(vpt[pn])) )< 0) {
			panic("duppage0: error in mapping %e\n", r);
		}
	}
	if(pn >=PGNUM(UTOP) || va >=UTOP)
		panic("out of bounds\n");
	if(!(vpt[pn]& PTE_U))
		panic("Not user privilege");
	if (sys_page_map(0, (void*)va, envid, (void*)va, PTE_P | PTE_U | PTE_COW) < 0) {
			panic("duppage1: error in mapping");
	}
	if (sys_page_map(0, (void*)va, 0, (void*)va, PTE_P | PTE_U | PTE_COW) < 0) {
			panic("duppage2: error in mapping");
	}
	if ((vpt[pn] & PTE_W) && (vpt[pn] & PTE_COW)) {
	
	panic("it's writeable and COW");
	}
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	//cprintf("\tfork called\n");
	extern void _pgfault_upcall(void);
	struct Env* e_par;
	envid_t envid_par = sys_getenvid();
	envid_t envid_ch;
	set_pgfault_handler(pgfault);
	if ((envid_ch = sys_exofork()) < 0) {
		panic("unable to fork child process");
	}
	//int lol;
	//lol = sys_env_set_pgfault_upcall(envid_ch, thisenv->env_pgfault_upcall);
	//cprintf("\nI am right now at la.. in fork(): lol = %d\n", lol);
	//cprintf("\tsys_getenvid() = %d\n", sys_getenvid());
	//cprintf("\tenvid_ch = %d\n", envid_ch);
	if (envid_ch == 0) {
		cprintf("envid_ch == 0\n");
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	int i, j;
	int k = 0;
	int pgnum =0;
	for (i = 0; i < PDX(UTOP); i++) {
		//if (vpd[i] != 0) cprintf("vpd[%d] = %d\n", i, vpd[i]);
		if (vpd[i] & PTE_P) {
			for (j = 0; j < NPTENTRIES; j++) {
				pgnum = i * NPTENTRIES + j;
				if (pgnum == PGNUM(UXSTACKTOP - PGSIZE)) {
					k = 1;
					break;
				}
				if (vpt[pgnum] & PTE_P) {
					duppage(envid_ch, pgnum);
				}
			}
		if (k == 1)
			break;
		}
	}
	//cprintf("FORK: done with for loop\n");
	if (sys_page_alloc(envid_ch, (void*)(UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W) < 0) {
		panic("unable to allocate uxstack for child");
	}
	/*if (sys_page_map(envid_ch, (void*)(UXSTACKTOP - PGSIZE),
			 envid_par, (PFTEMP), PTE_P | PTE_U | PTE_W) < 0) {
		return -1;
	}
	memmove((void*)(UXSTACKTOP - PGSIZE), PFTEMP, PGSIZE);
	if (sys_page_unmap(envid_ch, (void*)(UXSTACKTOP - PGSIZE))) {
		return -1;
	}*/
	if (sys_env_set_pgfault_upcall(envid_ch, thisenv->env_pgfault_upcall) < 0) {
		panic("cannot set page fault handler of child");
	}
	if (sys_env_set_status(envid_ch, ENV_RUNNABLE) < 0) {
		return -1;
	}
	return envid_ch;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
