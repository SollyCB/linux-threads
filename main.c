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

struct ctx {
  u32 id;
} ctx_arr[2];

struct ctx* ctx_(void)
{
  u64 p;
  asm("mov %%fs, %0" : "=r" (p) :);
  return (struct ctx*) p;
}

#define ctx ctx_()

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

#define CW 0

#if CW
// @TODO Convert to use clone3, both for fun, and to try to get TLS working!
__attribute((naked))
static long newthread(struct stack_head maybe_unused *stack)
{
    __asm volatile (
        "mov  %%rdi, %%rsi\n"     // arg2 = stack
        "mov  $0x50f00, %%edi\n"  // arg1 = clone flags
        "mov  $56, %%eax\n"       // SYS_clone
        "syscall\n"
        "mov  %%rsp, %%rdi\n"     // entry point argument
        "ret\n"
        : : : "rax", "rcx", "rsi", "rdi", "r11", "memory" // rcx and r11 are clobbered by syscall [1]
    );
    // [1] "after saving the address of the instruction following SYSCALL into RCX).
    // RFLAGS gets stored into R11" (http://www.felixcloutier.com/x86/SYSCALL.html)
}
#else
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
#endif

void handler(int sig)
{
  if (sig == SIGCHLD) {println("Got sig child");}
  else {println("Different signal");}
}

int main() {
  ctx_arr[0].id = 1;
  ctx_arr[1].id = 2;

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

#if CW
  newthread(stack);
#else
  newthread(&args, sizeof(args));
#endif

  _mm_mfence();
  sleep(1);

  println("%u", stack->join);

  return 0;
}
