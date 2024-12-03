#include "../solh/sol.h"

#include <sched.h>
#include <linux/sched.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <asm/prctl.h>

struct gcc_align(16) context {
  const char *name;
} ctx_arr[2];

u32 thread_count = 0;

#define ctx ((struct context*)readgs())

struct gcc_align(16) stack_head {
  void (*entry)(struct stack_head *);
  u32 thread_id;
};

int fn3(void) {println("fn3, name = %s", ctx->name); sleep(1);      ; return 1;}
int fn2(void) {println("fn2, name = %s", ctx->name); sleep(1); fn3(); return 1;}
int fn1(void) {println("fn1, name = %s", ctx->name); sleep(1); fn2(); return 1;}
int fn0(void) {println("fn0, name = %s", ctx->name); sleep(1); fn1(); return 1;}

void entry(struct stack_head maybe_unused *stack)
{
  writegs(&ctx_arr[stack->thread_id]);
  println("thread started! context address is %uh", ctx);
  fn0();
  sleep(200);
}

naked_fn static long newthread(struct clone_args maybe_unused *cl, u64 maybe_unused size)
{
  // Bro this guy is sick! https://nullprogram.com/blog/2023/03/23/
  __asm volatile (
    "mov  $435, %%eax\n"       // SYS_clone3
    "syscall\n"
    "mov  %%rsp, %%rdi\n"     // entry fn's argument
    "ret\n"
    : : : "rax", "rcx", "rsi", "rdi", "r11", "memory" // rcx and r11 are clobbered by syscall [2]
  );
  // [1] CLONE_FILES|CLONE_FS|CLONE_SIGHAND|CLONE_SYSVSEM|CLONE_THREAD|CLONE_VM
  // [2] "after saving the address of the instruction following SYSCALL into RCX).
  // RFLAGS gets stored into R11" (http://www.felixcloutier.com/x86/SYSCALL.html)
}

int main() {

  writegs(&ctx_arr[0]);

  ctx_arr[0].name = "main";
  ctx_arr[1].name = "worker";

  u32 flags = CLONE_FILES|CLONE_FS|CLONE_SIGHAND|CLONE_SYSVSEM|CLONE_THREAD|CLONE_VM;

  u64 stack_size = 1024 * 1024;
  void *stack = malloc(stack_size);

  stack_size -= sizeof(struct stack_head);
  struct stack_head *sh = stack + stack_size;
  sh->entry     = entry;
  sh->thread_id = ++thread_count;

  struct clone_args ca = {};
  ca.flags      = flags;
  ca.stack_size = stack_size;
  ca.stack      = (u64)stack;

  newthread(&ca, sizeof(ca));

  fn0();

  println("main context address is %uh", ctx)

  _mm_mfence();
  sleep(1);

  return 0;
}
