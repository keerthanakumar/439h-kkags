// User-level IPC library routines

#include <inc/lib.h>

// Receive a value via IPC and return it.
// If 'pg' is nonnull, then any page sent by the sender will be mapped at
//	that address.
// If 'from_env_store' is nonnull, then store the IPC sender's envid in
//	*from_env_store.
// If 'perm_store' is nonnull, then store the IPC sender's page permission
//	in *perm_store (this is nonzero iff a page was successfully
//	transferred to 'pg').
// If the system call fails, then store 0 in *fromenv and *perm (if
//	they're nonnull) and return the error.
// Otherwise, return the value sent by the sender
//
// Hint:
//   Use 'thisenv' to discover the value and who sent it.
//   If 'pg' is null, pass sys_ipc_recv a value that it will understand
//   as meaning "no page".  (Zero is not the right value, since that's
//   a perfectly valid place to map a page.)
int32_t
ipc_recv(envid_t *from_env_store, void *pg, int *perm_store)
{
	// LAB 4: Your code here.
	/*uint32_t value;
	envid_t sender;
	int perm;*/

	if (pg) {
		;
	}
	else {
		pg = (void *)UTOP;
	}
	if (from_env_store) {
		*from_env_store = thisenv->env_id;
	}
	if (perm_store) {
		*perm_store = thisenv->env_ipc_perm;
	}
	if (sys_ipc_recv(pg) < 0) {
		if (from_env_store) *from_env_store = 0;
		if (perm_store) *perm_store = 0;
	}
	return thisenv->env_ipc_value;
	
	/*if(!pg){
		pg = (void *) UTOP;
	}
	value = sys_ipc_recv(pg);
	if(value < 0){
		sender = 0;
		perm = 0;
	}
	else{
		sender = thisenv->env_ipc_from;
		perm = thisenv->env_ipc_perm;
		value = thisenv->env_ipc_value;
	}
	if(from_env_store){
		*from_env_store = sender;
	}
	if(perm_store){
		*perm_store = perm;
	}
	return value;*/
}

// Send 'val' (and 'pg' with 'perm', if 'pg' is nonnull) to 'toenv'.
// This function keeps trying until it succeeds.
// It should panic() on any error other than -E_IPC_NOT_RECV.
//
// Hint:
//   Use sys_yield() to be CPU-friendly.
//   If 'pg' is null, pass sys_ipc_try_send a value that it will understand
//   as meaning "no page".  (Zero is not the right value.)
void
ipc_send(envid_t to_env, uint32_t val, void *pg, int perm)
{
	// LAB 4: Your code here.
	if(!pg){
		pg = (void *) UTOP;
		perm = 0;
	}	
	int k;
	while(1){
		k = sys_ipc_try_send(to_env, val, pg, perm);
		if(!k){
			break;
		}	
		else if(k!= -E_IPC_NOT_RECV){
			//cprintf("The type of ipc error %e\n", k);
			panic("Error in ipc send\n");
		}
		sys_yield();
	}
}

// Find the first environment of the given type.  We'll use this to
// find special environments.
// Returns 0 if no such environment exists.
envid_t
ipc_find_env(enum EnvType type)
{
	int i;
	for (i = 0; i < NENV; i++)
		if (envs[i].env_type == type)
			return envs[i].env_id;
	return 0;
}
