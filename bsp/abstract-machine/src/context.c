#include <am.h>
#include <klib.h>
#include <rtthread.h>

#define STACK_SIZE 1024 * 4

static Context **src = NULL;
static Context **dst = NULL;

typedef struct {
  void *entry;
  void *parameter;
  void *exit;
} Pack;

static void __wrapper(void *arg) {
  // Unpack parameters.
  Pack *unpack = (Pack *)arg;
  void *param = (void *)unpack->parameter;
  void (*tentry)(void *) = (void (*)(void *))unpack->entry;
  void (*texit)() = (void (*)())unpack->exit;

  // Call function
  tentry(param);
  texit();
}

static Context* ev_handler(Event e, Context *c) {
  switch (e.event) {
    case EVENT_YIELD:
      if(src) *src = c;
      c = *dst;
      break;
    default: printf("Unhandled event ID = %d\n", e.event); assert(0);
  }
 return c;
}

void __am_cte_init() {
  cte_init(ev_handler);
}

void rt_hw_context_switch_to(rt_ubase_t to) {
  src = NULL;
  dst = (Context **)to;
  yield();
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
  src = (Context **)from;
  dst = (Context **)to;
  yield();
}

void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread) {
  assert(0);
}

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
  // Alignment for stack buttom (High position).
  uintptr_t buttom =(uintptr_t)stack_addr & (~(sizeof(uintptr_t) - 1)); 

  // Allocate memory space for runtime stack.
  uintptr_t top = buttom - STACK_SIZE;

  // Pack parameters.
  Pack *pack = (Pack *)top;
  pack->entry = tentry;
  pack->parameter = parameter;
  pack->exit = texit;
  
  // Construct a context at stack top
  Context *c = kcontext((Area){(void *)top, (void *)buttom}, __wrapper, (void *)pack);

  return (rt_uint8_t *)c;
}