#include "frame.h"

void frame_mark(struct gc* gc, struct frame* frame) {
  gc_mark_ptr(gc, frame->function);
}
