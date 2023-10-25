#include "coroutine.h"

#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(__APPLE__) && defined(__MACH__)
#define _XOPEN_SOURCE
#include <sys/ucontext.h>
#undef _XOPEN_SOURCE
#else
#include <ucontext.h>
#endif

typedef struct coroutine_t {
  scheduler_t *sched;
  coroutine_func func;
  void *arg;
  int state;
  int stack_size;
  char *stack;
  ucontext_t ctx;
} coroutine_t;

typedef struct scheduler_t {
  int stack_size;
  int count;
  int current;
  int capacity;
  coroutine_t **coroutines;
  ucontext_t ctx;
} scheduler_t;

static coroutine_t * _co_new(scheduler_t *sched, coroutine_func func, void *arg) {
  coroutine_t *co = (coroutine_t*)malloc(sizeof(coroutine_t));
  co->sched = sched;
  co->func = func;
  co->arg = arg;
  co->state = COROUTINE_READY;
  co->stack_size = sched->stack_size;
  co->stack = (char*)malloc(co->stack_size);
  return co;
}

static void _co_delete(scheduler_t *sched, int id) {
  coroutine_t* co = sched->coroutines[id];
  free(co->stack);
  free(co);
  sched->coroutines[id] = NULL;
}

static void _co_entry(scheduler_t* sched) {
  int id = sched->current;
  coroutine_t* co = sched->coroutines[id];
  co->func(sched, co->arg);
  co->state = COROUTINE_DEAD;
  sched->count--;
  sched->current = -1;
}

scheduler_t * scheduler_new(int stack_size, int capacity) {
  scheduler_t *sched = (scheduler_t*)malloc(sizeof(scheduler_t));
  sched->stack_size = stack_size;
  sched->count = 0;
  sched->current = -1;
  sched->capacity = capacity;
  sched->coroutines = (coroutine_t**)malloc(sizeof(coroutine_t*) * sched->capacity);
  memset(sched->coroutines, 0, sizeof(coroutine_t*) * sched->capacity);
  return sched;
}

void scheduler_close(scheduler_t *sched) {
  for(int id = 0; id < sched->capacity; id++) {
    if(sched->coroutines[id] != NULL) {
      _co_delete(sched, id);
    }
  }
  free(sched->coroutines);
  free(sched);
}

int coroutine_new(scheduler_t *sched, coroutine_func func, void *arg) {
  coroutine_t *co = _co_new(sched, func, arg);

  if(sched->count >= sched->capacity) {
    int id = sched->capacity;
    int new_capacity = sched->capacity * 2;
    sched->coroutines = (coroutine_t**)realloc(sched->coroutines, sizeof(coroutine_t*) * new_capacity);
    memset(sched->coroutines + sched->capacity, 0, sizeof(coroutine_t*) * (new_capacity - sched->capacity));
    sched->capacity = new_capacity;
    sched->coroutines[id] = co;
    sched->count++;
    return id;
  }

  for(int id = 0; id < sched->capacity; id++) {
    if(sched->coroutines[id] == NULL) {
      sched->coroutines[id] = co;
      sched->count++;
      return id;
    } else if(sched->coroutines[id]->state == COROUTINE_DEAD) {
      _co_delete(sched, id);
      sched->coroutines[id] = co;
      sched->count++;
      return id;
    }
  }

  return -1;
}

void coroutine_resume(scheduler_t *sched, int id) {
  assert(sched != NULL);
  assert(sched->current == -1);
  assert(id >= 0 && id < sched->capacity);
  coroutine_t *co = sched->coroutines[id];
  assert(co != NULL);

  if(co->state == COROUTINE_READY) {
    getcontext(&co->ctx);
    co->ctx.uc_stack.ss_sp = co->stack;
    co->ctx.uc_stack.ss_size = co->stack_size;
    co->ctx.uc_stack.ss_flags = 0;
    co->ctx.uc_link = &sched->ctx;
    co->state = COROUTINE_RUNNING;
    sched->current = id;
    makecontext(&co->ctx, (void (*) (void))_co_entry, 1, sched);
    swapcontext(&sched->ctx, &co->ctx);
  } else if(co->state == COROUTINE_SUSPENDE) {
    sched->current = id;
    swapcontext(&sched->ctx, &co->ctx);
  } else {
    // panic: coroutine is running or dead
    assert(0);
  }
}

void coroutine_yield(scheduler_t *sched) {
  assert(sched != NULL);
  int id = sched->current;
  assert(id >= 0 && id < sched->capacity);
  coroutine_t *co = sched->coroutines[id];
  assert(co != NULL);
  
  char *stack_top = co->stack + co->stack_size;
  char stack_bottom = 0;
  assert(stack_top - &stack_bottom <= co->stack_size);

  co->state = COROUTINE_SUSPENDE;
  sched->current = -1;
  swapcontext(&co->ctx, &sched->ctx);
}

int coroutine_state(scheduler_t *sched, int id) {
  assert(sched != NULL);
  assert(id >= 0 && id < sched->capacity);
  if(sched->coroutines[id] == NULL) {
    return COROUTINE_DEAD;
  }
  return sched->coroutines[id]->state;
}

int coroutine_current(scheduler_t *sched) {
  return sched->current;
}

int coroutine_count(scheduler_t *sched){
  return sched->count;
}

void coroutine_destroy(scheduler_t *sched, int id) {
  if(coroutine_state(sched, id)) {
    _co_delete(sched, id);
    sched->count--;
  }
}
