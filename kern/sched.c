#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

#define CUSTOM_SCHED 0

// Choose a user environment to run and run it.
void
sched_yield(void)
{
	int i;
	struct Env *idle;
	if (!CUSTOM_SCHED) {
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
	       if (envs[nexteid].env_type != ENV_TYPE_IDLE && envs[nexteid].env_status == ENV_RUNNABLE) {
		       env_run(&envs[nexteid]);
	       }
	}
	if (curenv && curenv->env_type != ENV_TYPE_IDLE && curenv->env_status == ENV_RUNNING){
			env_run(curenv);
	}

	}

	//our dynamic-priority scheduler (challenge problem)
	else {
		static int numRuns = 0;
		if (numRuns && curenv->env_type != ENV_TYPE_IDLE &&
			curenv->env_status == ENV_RUNNING) {
			numRuns--;
			if (numRuns == 0) {
				if (curenv->env_priority != LOW_PRI)
					curenv->env_priority--;
			}
			env_run(curenv);
		}

		uint32_t envid = curenv ? ENVX(curenv->env_id) : 0;
		uint32_t firsteid = (++envid) % NENV;
		uint32_t nexteid;

		for (i = 0; i < NENV; i++) {
		       nexteid = (firsteid + i) % NENV;
		       if (envs[nexteid].env_type != ENV_TYPE_IDLE &&
				envs[nexteid].env_status == ENV_RUNNABLE) {
				numRuns = (int)envs[nexteid].env_priority;
			       env_run(&envs[nexteid]);
		       }
		}
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
