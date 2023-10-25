#ifndef __COROUTINE_H__
#define __COROUTINE_H__

enum {
  COROUTINE_DEAD = 0,
  COROUTINE_SUSPENDE,
  COROUTINE_RUNNING,
  COROUTINE_READY,
};

typedef struct scheduler_t scheduler_t;

typedef void (*coroutine_func)(scheduler_t *sched, void *arg);

/* new scheduler_t */
scheduler_t * scheduler_new(int stack_size, int capacity);

/* safe close scheduler_t */
void scheduler_close(scheduler_t *sched);

/* new coroutine */
int coroutine_new(scheduler_t *sched, coroutine_func func, void *arg);

/* resume coroutine by id */
void coroutine_resume(scheduler_t *sched, int id);

/* yield current running coroutine */
void coroutine_yield(scheduler_t *sched);

/* get coroutine state by id */
int coroutine_state(scheduler_t *sched, int id);

/* get current running coroutine id */
int coroutine_current(scheduler_t *sched);

/* get number of coroutines */
int coroutine_count(scheduler_t *sched);

/* destroy coroutine by id */
void coroutine_destroy(scheduler_t *sched, int id);

#endif /* __COROUTINE_H__ */
