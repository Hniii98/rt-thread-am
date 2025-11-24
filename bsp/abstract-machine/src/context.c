#include <am.h>
#include <klib.h>
#include <rtthread.h>

#define STACK_SIZE 1024 * 4

// static Context **src = NULL;
// static Context **dst = NULL;

typedef struct {
  void *entry;
  void *parameter;
  void *exit;
} ParamsPack;

typedef struct {
  rt_ubase_t from;
  rt_ubase_t to;
} SwitchPack;




static void __wrapper(void *arg) {
  // Unpack paramter pack.
  ParamsPack *ppack = (ParamsPack *)arg;
  void *param = (void *)ppack->parameter;
  void (*tentry)(void *) = (void (*)(void *))ppack->entry;
  void (*texit)() = (void (*)())ppack->exit;

  // Call function
  tentry(param);
  texit();
}

static Context* ev_handler(Event e, Context *c) {
  switch (e.event) {
    case EVENT_YIELD:
      SwitchPack *spack =(SwitchPack *)rt_thread_self()->user_data;
      if(spack->from){
        // Store current context *.
        *(Context **)spack->from = c;
      }
      // Switch contex
      c = *(Context **)spack->to;
      break;
    case EVENT_IRQ_TIMER:
      // Do nothing for now.  
      break;
    default: printf("Unhandled event ID = %d\n", e.event); assert(0);
  }
 return c;
}

void __am_cte_init() {
  cte_init(ev_handler);
}

void rt_hw_context_switch_to(rt_ubase_t to) {
  // Fetch thread pcb and use it's private data to pass paramter.
  rt_thread_t pcb = rt_thread_self();
  rt_ubase_t *thread_temp = &pcb->user_data;

  // Buffer it't original data.
  rt_ubase_t buffer = *thread_temp;

  // Store switch pack.
  SwitchPack spack = {.from =(rt_ubase_t)NULL, .to = to};
  *thread_temp =(rt_ubase_t)&spack;

  yield();

  // Recover orginal data.
  *thread_temp = buffer;
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
  // Fetch thread pcb and use it's private data to pass paramter.
  rt_thread_t pcb = rt_thread_self();
  rt_ubase_t *thread_temp = &pcb->user_data;

  // Buffer it't original data.
  rt_ubase_t buffer = *thread_temp;

  // Store switch pack.
  SwitchPack spack = {.from = from, .to = to};
  *thread_temp =(rt_ubase_t)&spack;

  yield();

  // Recover orginal data.
  *thread_temp = buffer;
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
  ParamsPack *ppack = (ParamsPack *)top;
  ppack->entry = tentry;
  ppack->parameter = parameter;
  ppack->exit = texit;

  // Construct a context at stack top
  Context *c = kcontext((Area){(void *)top, (void *)buttom}, __wrapper, (void *)ppack);

  return (rt_uint8_t *)c;
}