#include <stdio.h>

#include "coroutine.h"

struct args {
	int value;
};

void task(scheduler_t *sched, void *arg) {
  struct args *args = arg;
  printf("task(%d) start: %d\n", coroutine_current(sched), args->value);
  for(int i = 0; i < 3; i++) {
    printf("task(%d) before yield: %d\n", coroutine_current(sched), args->value);
    coroutine_yield(sched);
    printf("task(%d) after yield: %d\n", coroutine_current(sched), args->value);
  }
  printf("task(%d) end\n", coroutine_current(sched));
}

#define NUM_CO 16
 
int main(int argc, char const *argv[]) {
  struct args args_list[NUM_CO];
  int co_list[NUM_CO];

  scheduler_t *sched = scheduler_new(1024 * 1024, NUM_CO);

  for(int i = 0; i < NUM_CO; i++) {
    co_list[i] = coroutine_new(sched, task, &args_list[i]);
  }

  for(int i = 0;; i++) {
    int flag = 1;
    for(int j = 0; j < NUM_CO; j++) {
      if(coroutine_state(sched, co_list[j])) {
        flag = 0;
        args_list[j].value = i;
        coroutine_resume(sched, co_list[j]);
      }
    }
    if(flag) {
      break;
    }
  }

  scheduler_close(sched);

  printf("\nsafe exit.\n");
  return 0;
}
