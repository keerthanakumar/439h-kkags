#ifndef JOS_ENV_THREAD_H
#define JOS_ENV_THREAD_H
#include <inc/thread.h>

extern struct Thread *thrds;
void thread_init(void);
static int thread_setup_vm(struct Thread* t);
static void region_alloc(struct Thread* t, void* sa, size_t len);
static void load_icode(struct Thread* t, uint8_t* binary, size_t size);
void thread_create(uint8_t* binary, size_t len, enum EnvType type);
void thread_free(struct Thread* t);
void thread_destroy(struct Thread* t);
void thread_pop_tf(struct Trapframe* tf); //KK HELP
void thread_run (struct Thread* t);
void tcreate(uint8_t* binary, size_t, enum EnvType type);

#define THREAD_PASTE3(x, y, z) x ## y ## z

#define THREAD_CREATE(x, type)						\
	do {								\
		extern uint8_t THREAD_PASTE3(_binary_obj_, x, _start)[],	\
			THREAD_PASTE3(_binary_obj_, x, _size)[];		\
		thread_create(THREAD_PASTE3(_binary_obj_, x, _start),		\
			   (int)THREAD_PASTE3(_binary_obj_, x, _size),	\
			   type);					\
	} while (0)

#endif
