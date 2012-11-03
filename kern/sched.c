#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>


// Choose a user environment to run and run it.
void
sched_yield(void)
{
	struct Env *idle;
	int i;
	//cprintf("\nStarting Schedule\n");
	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING) and never choose an
	// idle environment (env_type == ENV_TYPE_IDLE).  If there are
	// no runnable environments, simply drop through to the code
	// below to switch to this CPU's idle environment.

	// LAB 4: Your code here.
	uint32_t envid = curenv ? ENVX(curenv->env_id) : 0;
	uint32_t firsteid = (++envid) % NENV;
	uint32_t nexteid;

	for (i = 0; i < NENV; i++) {
	       nexteid = (firsteid + i) % NENV;
	//cprintf("\n Progress 1 on Schedule\n");
	       if (envs[nexteid].env_type != ENV_TYPE_IDLE && envs[nexteid].env_status == ENV_RUNNABLE) {
				//cprintf("\n Progress in if on Schedule: %d\n", nexteid);
		       env_run(&envs[nexteid]);
				//cprintf("\n Progress completed running on Schedule\n");
		       break;
	       }
	}

	//cprintf("\n Progress 2 on Schedule\n");
	if (envs[envid].env_type != ENV_TYPE_IDLE && envs[envid].env_status == ENV_RUNNING){
		//envs[envid].env_cpunum == cpunum()) {
			env_run(&envs[envid]);
		}


	// For debugging and testing purposes, if there are no
	// runnable environments other than the idle environments,
	// drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if (envs[i].env_type != ENV_TYPE_IDLE &&
		    (envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING))
			break;
	}
	if (i == NENV) {
		cprintf("No more runnable environments!\n");
		while (1)
			monitor(NULL);
	}

	// Run this CPU's idle environment when nothing else is runnable.
	idle = &envs[cpunum()];
	if (!(idle->env_status == ENV_RUNNABLE || idle->env_status == ENV_RUNNING))
		panic("CPU %d: No idle environment!", cpunum());
	env_run(idle);
}
