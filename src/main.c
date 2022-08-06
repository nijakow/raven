#include "defs.h"
#include "raven.h"
#include "core/any.h"
#include "core/blueprint.h"
#include "core/objects/object.h"
#include "vm/fiber.h"
#include "vm/interpreter.h"
#include "vm/scheduler.h"
#include "util/log.h"

int main(int argc, char *argv[]) {
  struct raven       raven;
  struct object*     object;
  struct fiber*      fiber;

  raven_create(&raven);
  if (!raven_boot(&raven, argc < 2 ? "../lib" : argv[1]))
    printf("Unable to boot the driver!\n");
  else {
    object = raven_get_object(&raven, "/secure/master.c");
    if (object != NULL) {
      fiber = scheduler_new_fiber(raven_scheduler(&raven));
      if (fiber != NULL) {
        fiber_push(fiber, any_from_ptr(object));
        fiber_send(fiber, raven_find_symbol(&raven, "main"), 0);
        raven_run(&raven);
      }
    }
  }
  raven_destroy(&raven);

  return 0;
}
