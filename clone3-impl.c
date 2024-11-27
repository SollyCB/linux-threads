#include "../solh/sol.h"

#include <sched.h>
#include <linux/sched.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/syscall.h>
#include <asm/prctl.h>

/*
 * Bro this guy is sick!
 * https://nullprogram.com/blog/2023/03/23/
 */

struct gcc_align(16) stack_head {
  void (*entry)(struct stack_head *);
  u64 join;
};

void entry(struct stack_head maybe_unused *stack)
{
  println("OK");
  stack->join = 1;
  sleep(200);
}

__attribute((naked))
static long newthread(struct clone_args __attribute((unused)) *args,
                      long __attribute((unused)) args_size)
{
  // SYS_clone3 == 435
  __asm volatile (
      "mov $435, %%eax\n"  // SYS_clone3
      "syscall\n"
      "mov %%rsp, %%rdi\n" // entry point argument
      "ret\n"
      : : : "rax", "rcx", "rsi", "rdi", "r11", "memory"
  );
}

int main() {
  u64 stack_size = 1024 * 1024;
  struct stack_head *stack = malloc(stack_size);

  stack = stack + stack_size / sizeof(*stack) - 1;
  stack->entry = entry;
  stack->join = 0;

  struct clone_args maybe_unused args = {};
  args.flags       = CLONE_FILES|CLONE_FS|CLONE_SIGHAND|CLONE_SYSVSEM|CLONE_THREAD|CLONE_VM;
  args.stack       = (u64) stack;
  args.stack_size  = stack_size;
  args.exit_signal = SIGCHLD;

  newthread(&args, sizeof(args));

  _mm_mfence();
  sleep(1);

  println("%u", stack->join);

  return 0;
}
