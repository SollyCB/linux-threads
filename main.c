#include "../solh/sol.h"

#include <sched.h>
#include <linux/sched.h>
#include <sys/mman.h>
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

void entry(struct stack_head *stack)
{
  println("OK");
  stack->join = 1;
  sleep(200);
}

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
        : : : "rax", "rcx", "rsi", "rdi", "r11", "memory"
    );
}

int main() {
  ctx[0].id = 1;
  ctx[1].id = 2;

  u64 stack_size = 1024 * 1024;
  struct stack_head *stack = mmap(NULL, stack_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON|MAP_STACK, -1, 0);
  log_os_error_if(stack == MAP_FAILED, "OI");

  stack = stack + stack_size / sizeof(*stack) - 1;
  stack->entry = entry;
  stack->join = 0;

  newthread(stack);

  _mm_mfence();
  sleep(1);

  println("%u", stack->join);

  return 0;
}
