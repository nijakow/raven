/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../../../defs.h"
#include "../../../../raven/raven.h"
#include "../../../../util/memory.h"


#include "../../blueprint.h"

#include "object.h"


struct obj_info OBJECT_INFO = {
    .type  = OBJ_TYPE_OBJECT,
    .mark  = (mark_func)  object_mark,
    .del   = (del_func)   object_del,
    .stats = (stats_func) base_obj_stats
};


static void object_unlink(struct object* object) {
    struct object** ptr;

    if (object->parent != NULL) {
        ptr = &object->parent->children;

        while (*ptr != NULL) {
            if (*ptr == object)
                *ptr = object->sibling;
            else
                ptr = &((*ptr)->sibling);
        }

        object->parent  = NULL;
        object->sibling = NULL;
    }
}


struct object* object_new(struct raven* raven, struct blueprint* blueprint) {
    struct object*       object;
    struct object_page*  page;
    struct blueprint*    current_blue;

    object = base_obj_new(raven_objects(raven),
                          &OBJECT_INFO,
                          sizeof(struct object));

    if (object != NULL) {
        object->was_initialized = false;
        object->heartbeat_next  = NULL;
        object->heartbeat_prev  = NULL;
        object->parent          = NULL;
        object->sibling         = NULL;
        object->children        = NULL;
        object->pages           = NULL;
        object->stash           = any_nil();
    }

    /*
     * Create a new page for each blueprint in the inheritance chain.
     */
    for (current_blue = blueprint; current_blue != NULL; current_blue = blueprint_parent(current_blue)) {
        page = blueprint_instantiate_page(current_blue);
        object_add_page(object, page);
    }

    return object;
}

void object_mark(struct gc* gc, struct object* object) {
    struct object_page*  page;

    gc_mark_ptr(gc, object->parent);
    gc_mark_ptr(gc, object->sibling);
    gc_mark_ptr(gc, object->children);
    gc_mark_any(gc, object->stash);

    for (page = object->pages; page != NULL; page = page->next)
        object_page_mark(gc, page);

    base_obj_mark(gc, &object->_);
}

void object_del(struct object* object) {
    if (object->heartbeat_prev != NULL)
        *(object->heartbeat_prev) = object->heartbeat_next;
    if (object->heartbeat_next != NULL)
        object->heartbeat_next->heartbeat_prev = object->heartbeat_prev;

    while (object->children != NULL)
        object_unlink(object->children);
    object_unlink(object);

    while (object->pages != NULL) {
        object_page_del(object->pages);
    }

    base_obj_del(&object->_);
}

void object_add_page(struct object* object, struct object_page* page) {
    object_page_link(page, object);
}

void object_insert_page_before(struct object* object, struct object_page* before, struct object_page* page) {
    object_page_link_before(page, object, before);
}

void object_remove_page(struct object* object, struct object_page* page) {
    object_page_unlink(page);
}

void object_move_to(struct object* object, struct object* target) {
    object_unlink(object);
    if (target != NULL) {
        object->parent   = target;
        object->sibling  = target->children;
        target->children = object;
    }
}

void object_link_heartbeat(struct object* object, struct object** list) {
    if (object->heartbeat_prev == NULL) {
        if (*list != NULL)
            (*list)->heartbeat_prev = &object->heartbeat_next;
        object->heartbeat_prev =  list;
        object->heartbeat_next = *list;
        *list                  =  object;
    }
}

static struct object_page* object_soulmate_page(struct object* object, struct blueprint* bp) {
    struct object_page*  page;

    for (page = object->pages; page != NULL; page = page->next) {
        if (blueprint_is_soulmate(object_page_blueprint(page), bp))
            return page;
    }

    return NULL;
}

static void object_switch_blueprint(struct object* object, struct blueprint* bp_new) {
    struct blueprint*    bp_tmp;
    struct object_page*  old_page;
    struct object_page*  new_page;

    /*
     * TODO: Unlink all pages that are not in the new blueprint.
     *
     * Idea for an approach: Take all pages, move them to an external list
     * and clear the object's page list. Then, for each blueprint in the
     * new inheritance chain, check if there is a page in the external list
     * that is a soulmate of the blueprint. If so, move it to the object's
     * page list. If not, create a new page and add it to the object's page
     * list.
     */

    for (bp_tmp = bp_new; bp_tmp != NULL; bp_tmp = blueprint_parent(bp_tmp)) {
        old_page = object_soulmate_page(object, bp_tmp);
        new_page = blueprint_instantiate_page(bp_tmp);
        object_insert_page_before(object, old_page, new_page);
        object_page_del(old_page);
    }

    // /*
    //  * Resize the object, so that new slots can fit.
    //  */
    // struct blueprint*  bp_tmp;
    // struct blueprint*  soulmate;
    // struct symbol*     name;
    // any*               new_slots;
    // any*               old_slots;
    // unsigned int       new_size;
    // unsigned int       offset;
    // unsigned int       i;
    // unsigned int       i2;

    // new_size  = blueprint_get_instance_size(bp_new);

    // old_slots = object_slots(object);
    // new_slots = memory_alloc(sizeof(any) * new_size);

    // if (new_slots != NULL) {
    //     /*
    //      * Initialize every slot properly.
    //      */
    //     for (i = 0; i < new_size; i++) {
    //         new_slots[i] = any_nil();
    //     }

    //     for (bp_tmp = bp_new; bp_tmp != NULL; bp_tmp = blueprint_parent(bp_tmp)) {
    //         offset   = vars_offset(blueprint_vars(bp_tmp));
    //         soulmate = blueprint_soulmate(object_blueprint(object), bp_tmp);
    //         if (soulmate != NULL) {
    //             for (i = 0; i < vars_count1(blueprint_vars(bp_tmp)); i++) {
    //                 name    = vars_name_for_local_index(blueprint_vars(bp_tmp), i);
    //                 if (vars_find(blueprint_vars(soulmate), name, NULL, &i2))
    //                     new_slots[offset + i] = old_slots[i2];
    //             }
    //         }
    //     }

    //     /*
    //      * Install the new slots.
    //      */
    //     object->slot_count = new_size;
    //     object->slots      = new_slots;

    //     /*
    //      * Release the old slots (if they were allocated).
    //      */
    //     if (old_slots != object->payload) {
    //         memory_free(old_slots);
    //     }
    // }

    // /*
    //  * Do the magic operation!
    //  */
    // object->blue = bp_new;
}

void object_recompile(struct object* object) {
    struct blueprint*  new_bp;

    new_bp = blueprint_recompile(object_blueprint(object));

    if (new_bp != NULL) {
        object_switch_blueprint(object, new_bp);
    }
}


bool object_resolve_func_and_page(struct object* object, struct object_page_and_function* result, struct symbol* message, unsigned int args, bool allow_private) {
    return object_page_lookup_list(object->pages, result, message, args, allow_private);
}
