//#include <inc/env.h> //MAYBE?
#include <inc/error.h>
#include <inc/assert.h>
#include <inc/string.h>

#include <kern/thread.h>

struct Thread* thrds = NULL;
static struct Thread* thread_free_list;

void thread_init(void) {
	int i;
	thread_free_list = NULL;
	
	for (i = NTHREAD-1; i >= 0; i--) {
 		thrds[i].thread_id = 0;
		thrds[i].thread_status = THREAD_FREE;
		thrds[i].thread_runs = 0;
		thrds[i].thread_link = thread_free_list;
		thrds[i].thread_pgdir = NULL;
		thread_free_list = &thrds[i];
	}
}

static int thread_setup_vm(struct Thread* t) {
	//KK HELP
	return -1;
}


int thread_alloc(struct Thread** newthread_store/*, threadid_t parent_id*/) { //the commented part is for matching what's in kern/env.c
	int r;
	struct Thread* t;
	if (!(t = thread_free_list))
		return -E_NO_FREE_THREAD;

	if ((r = thread_setup_vm(t)) < 0) {
		return r;
	}

	t->thread_id = t - thrds;
	cprintf("env/thread.c: thread_alloc(): new thread's id = %d\n", t->thread_id);

	t->thread_type = THREAD_TYPE_USER;
	t->thread_status = THREAD_RUNNABLE;
	t->thread_runs = 0;

	memset(&t->thread_tf, 0, sizeof(t->thread_tf)); //ASKING FOR TROUBLE KK HELP

	/****************************************
	 * now there's all this stuff and we have
	 * no idea how to do it. KK HELP
	 *e->env_tf.tf_ds = GD_UD | 3;
	 *e->env_tf.tf_es = GD_UD | 3;
	 *e->env_tf.tf_ss = GD_UD | 3;
	 *e->env_tf.tf_esp = USTACKTOP;
	 *e->env_tf.tf_cs = GD_UT | 3;

	 * Enable interrupts while in user mode.
	 * LAB 4: Your code here.
	 *e->env_tf.tf_eflags |= FL_IF;
	 ***************************************/

	// Clear the page fault handler until user installs one.
	t->thread_pgfault_upcall = 0;
	
	// commit the allocation
	thread_free_list = t->thread_link;
	*newthread_store = t;
	cprintf("env/thread.c: thread_alloc(): new thread %08x\n", t->thread_id);
	return 0;

}

static void region_alloc(struct Thread* t, void* sa, size_t len) {
	//KK HELP
}

static void load_icode(struct Thread* t, uint8_t* binary, size_t size) {
	//KK HELP
}

void thread_create(uint8_t* binary, size_t size, enum ThreadType type) {
	struct Thread* t;
	if (thread_alloc(&t/*, 0*/) == 0) {
		load_icode(t, binary, size);
		t->thread_type = type;
	}
	else {
		panic("thread_create: unable to env_alloc()");
	}
}

void thread_free(struct Thread* t) {
//copied from kern/env.c: env_free. KK HELP
//	pte_t *pt;
//	uint32_t pdeno, pteno;
//	physaddr_t pa;
//
//	// If freeing the current environment, switch to kern_pgdir
//	// before freeing the page directory, just in case the page
//	// gets reused.

// KK says, "We should make VADDR and then
// Have lcr3 shift to VADDR(env_pgdir)
// 
 

//
//	// Note the environment's demise.
//	// cprintf("[%08x] free env %08x\n", curenv ? curenv->env_id : 0, e->env_id);
//
//	// Flush all mapped pages in the user portion of the address space
//	static_assert(UTOP % PTSIZE == 0);
//	for (pdeno = 0; pdeno < PDX(UTOP); pdeno++) {
//
//		// only look at mapped page tables
//		if (!(e->env_pgdir[pdeno] & PTE_P))
//			continue;
//
//		// find the pa and va of the page table
//		pa = PTE_ADDR(e->env_pgdir[pdeno]);
//		pt = (pte_t*) KADDR(pa);
//
//		// unmap all PTEs in this page table
//		for (pteno = 0; pteno <= PTX(~0); pteno++) {
//			if (pt[pteno] & PTE_P)
//				page_remove(e->env_pgdir, PGADDR(pdeno, pteno, 0));
//		}
//
//		// free the page table itself
//		e->env_pgdir[pdeno] = 0;
//		page_decref(pa2page(pa));
//	}
//
//	// free the page directory
//	pa = PADDR(e->env_pgdir);
//	e->env_pgdir = 0;
//	page_decref(pa2page(pa));
//
//	// return the environment to the free list
	t->thread_status = THREAD_FREE;
	t->thread_link = thread_free_list;
	thread_free_list = t;
}

void thread_destroy(struct Thread* t) {
//copied from kern/env.c: env_destroy(). KK HELP
//	if (e->env_status == ENV_RUNNING && curenv != e) {
//		e->env_status = ENV_DYING;
//		return;
//	}
//	env_free(e);
//
//	if (curenv == e) {
//		curenv = NULL;
//		sched_yield();
//	}
}

//copied from kern/env.c: KK HELP
//void thread_pop_tf(struct Trapframe *tf) {
//	// Record the CPU we are running on for user-space debugging
//	curenv->env_cpunum = cpunum();
//
//	__asm __volatile("movl %0,%%esp\n"
//		"\tpopal\n"
//		"\tpopl %%es\n"
//		"\tpopl %%ds\n"
//		"\taddl $0x8,%%esp\n" /* skip tf_trapno and tf_errcode */
//		"\tiret"
//		: : "g" (tf) : "memory");
//	panic("iret failed");  /* mostly to placate the compiler */
//}

void thread_run (struct Thread* t) {
//copied from kern/env.c: env_run(). KK HELP
//	if (curenv == NULL || curenv->env_id != e->env_id) {
//		if (curenv && curenv->env_status == ENV_RUNNING)				curenv->env_status = ENV_RUNNABLE;
//
//		curenv = e;
//		e->env_cpunum = cpunum();
//		e->env_status = ENV_RUNNING;
//		e->env_runs++;
//		lcr3(PADDR(e->env_pgdir));
//	}

//	unlock_kernel();
//
//	env_pop_tf(&(e->env_tf));
}
