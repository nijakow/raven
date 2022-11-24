/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_OBJECTS_BASE_OBJ_H
#define RAVEN_OBJECTS_BASE_OBJ_H

/*
 * This file contains all the code for basic objects that can be
 * managed by the garbage collector.
 *
 * In order to create such an object, write a struct that inherits
 * from `base_obj`:
 *
 *    struct my_object {
 *        struct base_obj  _;
 *    };
 *
 * You can then allocate such an object by doing this:
 *
 *    struct obj_info MY_OBJECT_INFO = {
 *      .type = OBJ_TYPE_MYOBJECT,   // in defs.h
 *      .mark = (mark_func) my_object_mark,  // Must be provided by you
 *      .del  = (del_func)  my_object_del    // Same ^^
 *    };
 *
 *    {
 *      struct my_object* obj;
 *
 *      obj = base_obj_new(table, &MY_OBJECT_INFO, sizeof(struct my_object));
 *    }
 *
 * The object will be managed by the VM, deallocation happens automatically.
 * The pointer `table` refers to a `struct object_table`, usually the object
 * table from your current `struct raven` instance. Retrieving it is simple:
 *
 *    table = raven_objects(raven);
 *
 * Your implementations of `mark_func` and `del_func` should look like this:
 *
 *    void my_object_mark(struct gc* gc, struct my_object* obj) {
 *      // TODO: Mark all the slots you need to mark
 *      base_obj_mark(gc, &obj->_);
 *    }
 *
 *    void my_object_del(struct my_object* obj) {
 *      // TODO: Free up all the data you need to free
 *      base_obj_del(&obj->_);
 *    }
 *
 * Everything else will be managed by the VM.
 */

#include "../../defs.h"
#include "object_table.h"

/*
 * Definitions of our virtual function pointer types.
 *
 * This makes it easier to cast them when defining the object info.
 */
typedef void (*mark_func)(struct gc*, void*);
typedef void (*del_func)(void*);
typedef void (*stats_func)(void*, struct obj_stats*);

/*
 * The `struct obj_info` itself.
 *
 * It contains the type tag and the virtual functions.
 */
struct obj_info {
    enum obj_type type;
    mark_func     mark;
    del_func      del;
    stats_func    stats;
};

/*
 * Raven's garbage collector is very simple. Objects are either
 * marked as WHITE, GRAY or BLACK.
 *
 *  - WHITE represents an object that hasn't been reached by the
 *    garbage collector (yet). In the sweep phase of the collection,
 *    all objects marked as WHITE will be deleted.
 *  - GRAY represents objects that have been encountered, but not
 *    fully marked yet.
 *  - BLACK objects have been fully marked.
 */
enum gc_tag {
    GC_TAG_WHITE,
    GC_TAG_GRAY,
    GC_TAG_BLACK
};

/*
 * The actual `struct base_obj`.
 */
struct base_obj {
    /*
     * Some information about the object: Its type and its
     * virtual functions are referenced through this pointer.
     */
    struct obj_info* info;

    /*
     * All objects are linked to each other, this forming the
     * global object list. This pointer is used for that.
     */
    struct base_obj* next;

    /*
     * The following member is used for the garbage collection
     * process. It holds the object's gc color and a pointer to
     * the next object in its gray-list. This is done with tagged
     * pointers.
     *
     * Since the gray-list pointer can be used for multiple
     * other purposes, it's called `forward`.
     */
    uintptr_t forward;
};

void* base_obj_new(struct object_table* table,
                   struct obj_info*     info,
                   size_t               size);
void  base_obj_mark(struct gc* gc, struct base_obj* obj);
void  base_obj_del(void*);
void  base_obj_stats(void*, struct obj_stats* stats);

void  base_obj_dispatch_mark(struct gc* gc, struct base_obj* obj);
void  base_obj_dispatch_del(void*);
void  base_obj_dispatch_stats(void*, struct obj_stats* stats);

void  base_obj_mark_children(struct gc* gc, struct base_obj* obj);

static inline struct obj_info* base_obj_info(struct base_obj* obj) {
    return obj->info;
}

static inline enum obj_type base_obj_type(struct base_obj* obj) {
    return base_obj_info(obj)->type;
}

static inline bool base_obj_is(struct base_obj* obj, enum obj_type type) {
    return obj->info->type == type;
}

static inline struct base_obj* base_obj_forward(struct base_obj* obj) {
    return (struct base_obj*) ((obj->forward) & ~((uintptr_t) 0x03));
}

static inline void base_obj_set_forward(struct base_obj* obj,
                                        struct base_obj* value) {
    obj->forward = ((obj->forward) & ((uintptr_t) 0x03)) | ((uintptr_t) value);
}

static inline enum gc_tag base_obj_get_gc_tag(struct base_obj* obj) {
    return (enum gc_tag) ((obj->forward) & ((uintptr_t) 0x03));
}

static inline void base_obj_set_gc_tag(struct base_obj* obj, enum gc_tag tag) {
    obj->forward = ((obj->forward) & ~((uintptr_t) 0x03)) | ((uintptr_t) tag);
}

static inline bool base_obj_is_white(struct base_obj* obj) {
    return base_obj_get_gc_tag(obj) == GC_TAG_WHITE;
}

static inline bool base_obj_is_gray(struct base_obj* obj) {
    return base_obj_get_gc_tag(obj) == GC_TAG_GRAY;
}

static inline bool base_obj_is_black(struct base_obj* obj) {
    return base_obj_get_gc_tag(obj) == GC_TAG_BLACK;
}

static inline bool base_obj_is_marked(struct base_obj* obj) {
    return !base_obj_is_white(obj);
}

static inline void base_obj_mark_white(struct base_obj* obj) {
    base_obj_set_gc_tag(obj, GC_TAG_WHITE);
}

static inline void base_obj_mark_gray(struct base_obj* obj) {
    base_obj_set_gc_tag(obj, GC_TAG_GRAY);
}

static inline void base_obj_mark_black(struct base_obj* obj) {
    base_obj_set_gc_tag(obj, GC_TAG_BLACK);
}

#endif
