/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

/*
 * This file is surprisingly short, but it actually contains
 * most of the garbage collector's functionality.
 *
 * The process behind Raven's garbage collection is quite simple:
 * A garbage collector's task is to automatically detect and delete
 * all objects that are not needed by the runtime anymore, thereby
 * reclaiming storage and avoiding memory leaks.
 *
 * To do that, Raven's garbage collector divides all objects in the
 * system into three categories: WHITE, GRAY and BLACK.
 *
 * All objects start out in the WHITE category when they are created.
 * During the garbage collection process, the garbage collector moves
 * every white object it encounters into the GRAY category. This will
 * automatically link the object into a linked list of all currently
 * gray objects. Once this has been done for the most important objects
 * in the system, the collector revisits every GRAY object in this list,
 * turning it BLACK and appending its "children" (i.e. all other objects
 * that the selected gray object is pointing to) to the gray list as
 * well. Any GRAY or BLACK objects that are encountered will be ignored
 * during this process, and not reinserted into the gray list.
 * If the list gets empty, the second phase of the garbage collection
 * starts. During this phase, the collector visits every currently
 * loaded object in the system, and destroys all that are still marked
 * as WHITE. Then, every BLACK object will be set back to WHITE (there
 * are no GRAY objects anymore), and the garbage collector terminates.
 */

#include "../../raven/raven.h"
#include "../core/object_table.h"
#include "gc.h"


void gc_clear_mark_list(struct gc* gc);

/*
 * Create a new GC instance.
 */
void gc_create(struct gc* gc, struct raven* raven) {
    gc->raven     = raven;
    gc->mark_list = NULL;
}

/*
 * Destroy a GC instance.
 */
void gc_destroy(struct gc* gc) {
    gc_clear_mark_list(gc);
}

/*
 * Remove an object from a garbage collector's gray list,
 * and return it.
 */
static struct base_obj* gc_pop(struct gc* gc) {
    struct base_obj*  obj;

    obj           = gc->mark_list;
    gc->mark_list = base_obj_forward(obj);

    return obj;
}

/*
 * Clear a garbage collector's gray list, so that it
 * can take in new objects.
 *
 * Usually, this function shouldn't do anything at all,
 * since the gray list is usually left empty after a
 * previous garbage collection cycle.
 */
void gc_clear_mark_list(struct gc* gc) {
    while (gc->mark_list != NULL)
        base_obj_mark_white(gc_pop(gc));
}

/*
 * Add an object to the gray list (if needed).
 */
void gc_mark_ptr(struct gc* gc, void* ptr) {
    if (ptr != NULL)
        base_obj_dispatch_mark(gc, ptr);
}

/*
 * Add the object stored in an `any` to the gray list.
 */
void gc_mark_any(struct gc* gc, any value) {
    if (any_is_ptr(value))
        gc_mark_ptr(gc, any_to_ptr(value));
}

/*
 * Insert all toplevel objects into the gray list.
 */
void gc_mark_roots(struct gc* gc) {
    raven_mark(gc, gc_raven(gc));
}

/*
 * Perform the marking phase.
 *
 * For as long as there are still objects in the gray list,
 * take the next object out and turn it BLACK. Visit its
 * children and add them into the gray list, if required.
 */
void gc_mark_loop(struct gc* gc) {
    struct base_obj*  obj;

    while (gc->mark_list != NULL) {
        obj = gc_pop(gc);
        base_obj_mark_children(gc, obj);
        base_obj_mark_black(obj);
    }
}

/*
 * Collect all WHITE objects.
 *
 * This is done by getting a double pointer to the object list,
 * checking if the first object in that list is WHITE. If it is,
 * remove it from the list and delete it. Otherwise, move
 * the pointer to the next pointer in the list.
 */
void gc_collect(struct gc* gc) {
    struct base_obj**  ptr;
    struct base_obj*   obj;

    ptr = &raven_objects(gc_raven(gc))->objects;
    while (*ptr != NULL) {
        obj = *ptr;
        if (base_obj_is_marked(obj)) {
            base_obj_mark_white(obj);
            ptr = &(obj->next);
        } else {
            *ptr = obj->next;
            base_obj_dispatch_del(obj);
        }
    }
}

/*
 * This function outlines the garbage collection process
 * at its highest level. There are three main steps:
 *   1) Mark all root objects.
 *   2) Start the marking process, turning all reachable objects BLACK.
 *   3) Collect all WHITE objects. Then, turn every BLACK object back to WHITE.
 */
void gc_run(struct gc* gc) {
    gc_clear_mark_list(gc);
    gc_mark_roots(gc);
    gc_mark_loop(gc);
    gc_collect(gc);
}
