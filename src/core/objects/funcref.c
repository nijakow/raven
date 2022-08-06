#include "../../vm/fiber.h"
#include "../../vm/interpreter.h"

#include "funcref.h"

struct obj_info FUNCREF_INFO = {
  .type = OBJ_TYPE_FUNCREF,
  .mark = funcref_mark,
  .del  = funcref_del
};


struct funcref* funcref_new(struct raven*  raven,
                            any            receiver,
                            struct symbol* msg) {
  struct funcref*  funcref;

  funcref = base_obj_new(raven, &FUNCREF_INFO, sizeof(struct funcref));

  if (funcref != NULL) {
    funcref->receiver = receiver;
    funcref->message  = msg;
  }

  return funcref;
}

void funcref_mark(struct gc* gc, void* funcref) {
  struct funcref* ref;

  ref = funcref;
  gc_mark_any(gc, ref->receiver);
  gc_mark_ptr(gc, ref->message);
  base_obj_mark(gc, &ref->_);
}

void funcref_del(void* funcref) {
  struct funcref* ref;

  ref = funcref;
  base_obj_del(&ref->_);
}


void funcref_enter(struct funcref* funcref,
                   struct fiber*   fiber,
                   any*            args,
                   unsigned int    arg_count) {
  unsigned int  i;

  fiber_push(fiber, funcref->receiver);
  for (i = 0; i < arg_count; i++)
    fiber_push(fiber, args[i]);
  fiber_send(fiber, funcref->message, arg_count);
}
