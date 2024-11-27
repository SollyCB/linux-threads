#include "../solh/sol.h"

#include <sched.h>
#include <linux/sched.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <asm/prctl.h>

#include <immintrin.h>

struct gcc_align(16) context {
  u64 id;
} ctx_arr[2];

#define ctx ((struct context*)readfs())

struct gcc_align(16) stack_head {
  void (*entry)(struct stack_head *);
  u64 join;
};

void entry(struct stack_head maybe_unused *stack)
{
  println("thread started!");
  println("thread context is %uh", ctx);
  stack->join = 1;
  sleep(200);
}

// @TODO Convert to use clone3, both for fun, and to try to get TLS working!
// Update - sent an email to Chris Wellons to try to see why clone3 will not work,
// but for now clone is working fine, with TLS!
naked_fn static long newthread(struct stack_head maybe_unused *stack,
                               struct context maybe_unused *tctx)
{
  // Bro this guy is sick! https://nullprogram.com/blog/2023/03/23/
  __asm volatile (
    "mov  %%rsi, %%r8\n"      // arg5 = tls
    "mov  %%rdi, %%rsi\n"     // arg2 = stack
    "mov  $0xd0f00, %%edi\n"  // arg1 = clone flags [1]
    "mov  $56, %%eax\n"       // SYS_clone
    "syscall\n"
    "mov  %%rsp, %%rdi\n"     // entry point argument
    "ret\n"
    : : : "rax", "rcx", "rsi", "rdi", "r11", "memory" // rcx and r11 are clobbered by syscall [2]
  );
  // [1] CLONE_FILES|CLONE_FS|CLONE_SIGHAND|CLONE_SYSVSEM|CLONE_THREAD|CLONE_VM
  // [2] "after saving the address of the instruction following SYSCALL into RCX).
  // RFLAGS gets stored into R11" (http://www.felixcloutier.com/x86/SYSCALL.html)
}

void fn(void)
{
  ctx_arr[0].id = 1;
  ctx_arr[1].id = 2;

  u64 stack_size = 1024 * 1024;
  struct stack_head *stack = malloc(stack_size);

  stack = stack + stack_size / sizeof(*stack) - 1;
  stack->entry = entry;
  stack->join = 0;

  newthread(stack, &ctx_arr[1]);

  _mm_mfence();
  sleep(1);

  println("main context is %uh", ctx)
}

int main() {
  u64 fs = readfs();

  writefs(&ctx_arr[0]);
  fn();
  println("out");

  writefs(fs); // without this, gcc reports stack smash
  return 0;
}
