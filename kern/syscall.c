/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/sched.h>
#include <kern/time.h>
#include <kern/e1000.h>

//CHANGE
#include <env/thread.h>
#include <inc/upids.h>

static int sys_tcreate(int user_program_id) {
	switch(user_program_id) {
		case USER_HELLO:
			cprintf("kern/syscall.c: sys_tcreate(): about to THREAD_CREATE user_hello\n");
			THREAD_CREATE(user_hello, THREAD_TYPE_USER);
			break;
		default:
			panic("kern/syscall.c: sys_tcreate(): unknown program enum");
			break;
	}
	return -1;
}
//ENDCHANGE

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.
	
	// LAB 3: Your code here.
	user_mem_assert(curenv, s, len, PTE_P);

	// Print the string supplied by the user.

	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid) //Don't forget to modify syscall() to dispatch sys_yield()
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	env_destroy(e);
	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
//	-E_NO_MEM on memory exhaustion.
static envid_t
sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.

	// LAB 4: Your code here.
	struct Env *e;
	int r = env_alloc(&e, curenv->env_id);
	if (r < 0) {
		return r; //what about "memory exhaustion?"
	}
	e->env_status = ENV_NOT_RUNNABLE;
	//e->env_pgfault_upcall = curenv->env_pgfault_upcall;
	e->env_tf = curenv->env_tf;
	e->env_tf.tf_regs.reg_eax = 0;
	return e->env_id;
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
	// Hint: Use the 'envid2env' function from kern/env.c to translate an
	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.

	// LAB 4: Your code here.
	//cprintf("\tsys_env_set_status: envid = %d, status = %d, ENV_RUNNABLE = %d\n",
		//envid, status, ENV_RUNNABLE);
	struct Env *e;
	if (status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE)
		return -E_INVAL;
	if (envid2env(envid, &e, 1) < 0)
		return -E_BAD_ENV;
	
	e->env_status = status;
	//cprintf("\t\te->envid = %d, e->env_status = %d\n", e->env_id, e->env_status);
	return 0;
}

// Set envid's trap frame to 'tf'.
// tf is modified to make sure that user environments always run at code
// protection level 3 (CPL 3) with interrupts enabled.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	// LAB 5: Your code here.
	// Remember to check whether the user has supplied us with a good
	// address!
	struct Env* e;
	if (envid2env(envid, &e, 1) < 0) {
		return -E_BAD_ENV;
	}
	if (tf->tf_eip >= UTOP) {
		return -1;
	}
	e->env_tf = *tf;
	e->env_tf.tf_eflags |= FL_IF;
	return 0;
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	// LAB 4: Your code here.
	struct Env *e;
	if (envid2env(envid, &e, 1) < 0)
		return -E_BAD_ENV;
	if(!func)
		return -E_INVAL;
	e->env_pgfault_upcall = func;
	user_mem_assert(e, func, 4, 0);
	return 0;
	
}


// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_SYSCALL in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 4: Your code here.
	//cprintf("SYS_PAGE_ALLOC: envid = %d, va = %p\n", envid, va);
	struct Env *e;
	struct Page *p;
	if (envid2env(envid, &e, 1) < 0)
		return -E_BAD_ENV;
	if ((uint32_t)va >= UTOP || ((uint32_t)va % PGSIZE != 0)) {
		return -E_INVAL;
	}
	if ((perm & PTE_U) == 0 || (perm & PTE_P) == 0) {
		return -E_INVAL;
	}
	if ((perm & ~PTE_SYSCALL) != 0) {
		return -E_INVAL;
	}
	if ((p = page_alloc(ALLOC_ZERO)) == NULL) {
		return -E_NO_MEM;
	}
	if (page_insert(e->env_pgdir, p, va, perm) != 0) {
		cprintf("\tHEY WE'RE GONNA FREE!");
		page_free(p); //was free
		return -E_NO_MEM;
	}
//	user_mem_assert((struct Env*)0xf02915d0, (void*)0xeebffffc, 0, PTE_U);
	return 0;
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
	     envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 4: Your code here.
	//cprintf("sys_page_map: srcenvid = %d, srcva = %p,\n\tdstenvid = %d, dstva = %p,\n\tperm = %d\n", srcenvid, srcva, dstenvid, dstva, perm);
	struct Env *src, *dst;
	struct Page *p;
	pte_t *ps;
	//cprintf("env_alloc in sys_page_map\n");
	if (envid2env(srcenvid, &src, 1) < 0) {
		cprintf("sys_page_map: bad src env");
		return -E_BAD_ENV;
	}
	//cprintf("sys_page_map: envid2env dst = %d\n",
		//envid2env(dstenvid, &dst, 1));
	if (envid2env(dstenvid, &dst, 1) < 0) {
		cprintf("sys_page_map: bad dst env = %d\n", dstenvid);
		return -E_BAD_ENV;
	}
	if ((uint32_t)srcva >= UTOP || (uint32_t)srcva % PGSIZE != 0 || (uint32_t)dstva >= UTOP || (uint32_t)dstva % PGSIZE != 0) {
		cprintf("sys_page_map: bad src or dst va");
		return -E_INVAL;
	}
	if (!(p = page_lookup(src->env_pgdir, srcva, &ps))) {
		cprintf("sys_page_map: page lookup failed");
		return -E_INVAL;
	}
	if ((perm & PTE_U) == 0 || (perm & PTE_P) == 0) {
		cprintf("sys_page_map: bad perms (not u or p)");
		return -E_INVAL;
	}
	if ((perm & ~PTE_SYSCALL) != 0) {
		cprintf("sys_page_map: bad perms for syscall");
		return -E_INVAL;
	}
	if (((perm & PTE_W) != 0) && (!(*ps & PTE_W))) {
		cprintf("sys_page_map: bad perms for write. srcva = %p,  perm & PTE_W = %x, *ps & PTE_W = %x\n", srcva, perm & PTE_W, *ps & PTE_W);
		return -E_INVAL;
	}
	if (page_insert(dst->env_pgdir, p, dstva, perm) < 0) {
		cprintf("sys_page_map: bad page insert");
		return -E_NO_MEM;
	}
	return 0;
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().

	// LAB 4: Your code here.
	struct Env *e;
	if ((uint32_t)va > UTOP || (uint32_t)va % PGSIZE != 0) {
		return -E_BAD_ENV;
	}
	if (envid2env(envid, &e, 1) < 0)
		return -E_BAD_ENV;
	page_remove(e->env_pgdir, va);
	return 0;
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in the
//		current environment's address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	// LAB 4: Your code here.
	struct Env *e;
	struct Page *p;
	pte_t *pte;

	if(envid2env(envid, &e, 0) < 0) {
		cprintf("\tbad env\n");
		return -E_BAD_ENV;
	}
	//cprintf("SEND e->env_id = %x, e->env_ipc_recv = %d\n", e->env_id, e->env_ipc_recving);
	if (e->env_ipc_recving == 0) {
		//cprintf("\tenv doesn't want to receive\n");
		return -E_IPC_NOT_RECV;
	}
	if (e->env_status != ENV_NOT_RUNNABLE) {
		cprintf("\tenv NOT RUNNABLE\n");
		return -E_IPC_NOT_RECV;
		
	}

	if((uint32_t) e->env_ipc_dstva < UTOP && (uint32_t) srcva < UTOP){
		if ((int)srcva % PGSIZE != 0) {
			return -E_INVAL;
		}
		if ((p = page_lookup(curenv->env_pgdir, srcva, &pte)) < 0) {
			return -E_INVAL;
		}
		
		/*if(!p)
			return -E_INVAL;*/
		
		if((perm & PTE_W) && ((*pte & PTE_W) == 0))
			return -E_INVAL;
		
		if((page_insert(e->env_pgdir, p, e->env_ipc_dstva, perm)) < 0)
			return -E_NO_MEM;
		e->env_ipc_perm = perm;
	}
	else {
		//cprintf("\tenv doesn't want a page\n");
	//memcpy(data,rx_pkt_bufs[rdt].buf, len);
	//memcpy(data,rx_pkt_bufs[rdt].buf, len);
		e->env_ipc_perm = 0;
	}
	e->env_ipc_recving = 0;
	//e->env_ipc_dstva = (void*) UTOP;
	e->env_ipc_from = curenv->env_id;
	e->env_ipc_value = value;
	e->env_status = ENV_RUNNABLE;
	//cprintf("sender: done\n");
	//cprintf("sys_ipc_try_send, start: envid = %d, value = %d, srcva = %p, perm = %d\n",
		//envid, value, srcva, perm);
	return 0;
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	// LAB 4: Your code here.
	//cprintf("RECV thisenv = %x\n", curenv->env_id);
	//panic("");
	if(((uint32_t) dstva < UTOP) && ((uint32_t) dstva % PGSIZE != 0)){
		cprintf("sys_ipc_recv: dstva is not page-aligned\n");
		return -E_INVAL;
	}
	curenv->env_ipc_dstva = dstva;
	curenv->env_ipc_recving = 1;
	//panic("");
	curenv->env_status = ENV_NOT_RUNNABLE;
	curenv->env_tf.tf_regs.reg_eax = 0;
	sys_yield();
	

	return 0;

//	panic("sys_ipc_recv not implemented");
//	return 0;
}

// Return the current time.
static int
sys_time_msec(void)
{
	// LAB 6: Your code here.
        return time_msec();
}

static int
sys_net_send(char* data, int len) {
	if ((uintptr_t)data >= UTOP) {
		return -E_INVAL;
	}
	return e1000_transmit(data, len);
}

static int 
sys_net_receive(char* data){//, int* len){
	if((uintptr_t) data >=UTOP)
		return -E_INVAL;
	int r;
	r = e1000_receive(data);//, len);
	//cprintf("kern/syscall.c, sys_net_receive(): data = %c, len = %d\n", *data, *len);
	return r;
}
static int 
sys_get_mac(uint32_t *low, uint32_t *high){
	*low = e1000[E1000_RAL];
	*high = e1000[E1000_RAH] & 0xffff;
	return 0;
}

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.
	int32_t return_value;
	
	switch(syscallno){
		//CHANGE
		case SYS_tcreate:
			return_value = sys_tcreate((int)a1);
			break;
		//ENDCHANGE
		case SYS_cputs:
			return_value = 0;
			sys_cputs((char *) a1, (size_t) a2);
			break;
		case SYS_cgetc:
			return_value = sys_cgetc();
			break;
		case SYS_getenvid:
			return_value = sys_getenvid();
			break;
		case SYS_env_destroy:
			return_value = sys_env_destroy((envid_t) a1);
			break;
		case SYS_yield:
			return_value = 0;
			sys_yield();
			break;
		case SYS_exofork:
			return_value = sys_exofork();
			break;
		case SYS_env_set_status:
			return_value = sys_env_set_status(a1, a2);
			break;
		case SYS_page_alloc:
			return_value = sys_page_alloc(a1, (void*)a2, a3);
			break;
		case SYS_page_map:
			return_value = sys_page_map(a1, (void*)a2, a3, (void*)a4, a5);
			break;
		case SYS_page_unmap:
			return_value = sys_page_unmap(a1, (void*)a2);
			break;
		case SYS_env_set_pgfault_upcall:
			return_value = sys_env_set_pgfault_upcall(a1, (void*)a2);
			break;
		case SYS_ipc_try_send:
			return_value = sys_ipc_try_send(a1,a2,(void*) a3, a4);
			break;
		case SYS_ipc_recv:
			return_value = sys_ipc_recv((void*) a1);
			break;
		case SYS_env_set_trapframe:
			return_value = sys_env_set_trapframe(a1, (struct Trapframe*) a2);
			break;
                case SYS_time_msec:
                        return_value = sys_time_msec();
                        break;
		case SYS_net_send:
			return_value = sys_net_send((char *)a1, (int)a2);
			break;
		case SYS_net_receive:
			return_value = sys_net_receive((char *) a1);
			break;
		case SYS_get_mac:
			return_value = sys_get_mac((uint32_t *) a1, (uint32_t *)a2);
			break;
		default:
			return_value = -E_INVAL;
	}

	return return_value;
}

