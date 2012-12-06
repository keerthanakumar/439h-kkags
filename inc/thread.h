#ifndef JOS_INC_THREAD_H
#define JOS_INC_THREAD_H

#include <inc/types.h>
#include <inc/trap.h>
#include <inc/memlayout.h>

typedef int32_t threadid_t;

#define LOG2NTHREAD		2
#define NTHREAD			(1 << LOG2NTHREAD)
#define THREADX(threadid)		((threadid) & (NTHREAD - 1))

// Values of env_status in struct Env
enum {
	THREAD_FREE = 0,
	THREAD_DYING,
	THREAD_RUNNABLE,
	THREAD_RUNNING,
	THREAD_NOT_RUNNABLE
};


enum UserProgramIds {
	USER_HELLO = 0,
	NUM_ID_USER_PROGRAMS
};
// Special environment types
enum ThreadType {
	THREAD_TYPE_USER = 0,
	THREAD_TYPE_IDLE,
	THREAD_TYPE_FS,		// File system server
};

struct Thread {
	struct Trapframe thread_tf;	// Saved registers
	struct Thread *thread_link;		// Next free Thread
	threadid_t thread_id;			// Unique thread identifier
	//threadid_t thread_parent_id;		// thread_id of this thread's parent
	enum ThreadType thread_type;		// Indicates special system threads
	unsigned thread_status;		// Status of the thread
	uint32_t thread_runs;		// Number of times thread has run
	int thread_cpunum;			// The CPU that the thread is running on

	// Address space
	pde_t *thread_pgdir;		// Kernel virtual address of page dir

	// Exception handling
	void *thread_pgfault_upcall;	// Page fault upcall entry point
};


#endif // !JOS_INC_THREAD_H
