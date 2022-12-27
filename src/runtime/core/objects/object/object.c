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

struct object_page* object_soulmate_page(struct object* object, struct blueprint* bp) {
    struct object_page*  page;

    for (page = object->pages; page != NULL; page = page->next) {
        if (blueprint_is_soulmate(object_page_blueprint(page), bp))
            return page;
    }

    return NULL;
}

static void object_switch_blueprint(struct object* object, struct blueprint* bp_new) {
    struct blueprint*     bp_tmp;
    struct object_page*   external_list;
    struct object_page**  iter;
    struct object_page*   page;
    bool                  found;

    /*
     * If the object already has the new blueprint, do nothing.
     */
    if (object_blueprint(object) == bp_new)
        return;

    /*
     * Take all pages, move them to an external list
     * and clear the object's page list.
     */
    external_list = object->pages;
    object->pages = NULL;

    /*
     * Iterate over the inheritance chain of the new blueprint.
     */
    for (bp_tmp = bp_new; bp_tmp != NULL; bp_tmp = blueprint_parent(bp_tmp)) {
        /*
         * Reset the found flag.
         */
        found = false;

        /*
         * Iterate over the list of pages. If there is a page that is a
         * soulmate of the current blueprint, move it back to the object's page
         * list. Otherwise, create a new page and add it to the object's page
         * list.
         */
        for (iter = &external_list; *iter != NULL; iter = &((*iter)->next)) {
            /*
             * If the page is a soulmate of the current blueprint, move it
             * back to the object's page list.
             */
            if (object_page_blueprint(*iter) == bp_tmp) {
                page = *iter;
                *iter = page->next;
                page->next = NULL;
                object_add_page(object, page);
                found = true;
                break;
            } else if (blueprint_is_soulmate(object_page_blueprint(*iter), bp_tmp)) {
                page = blueprint_instantiate_page(bp_tmp);
                object_page_transfer_vars(page, *iter);
                object_add_page(object, page);
                found = true;
                break;
            }
        }

        /*
         * If no page was found, create a new one.
         */
        if (!found) {
            page = blueprint_instantiate_page(bp_tmp);
            object_add_page(object, page);
        }
    }

    /*
     * Delete all remaining pages.
     */
    while (external_list != NULL) {
        page = external_list;
        external_list = page->next;
        page->object  = NULL;  /* The variable was not updated, so we do it here */
        object_page_del(page);
    }

    /*
     * TODO: "Zap" all stack frames that are associated with the old pages.
     *       This avoids the problem of having a stack frame that points to
     *       a page that is no longer part of the object.
     */
}

void object_recompile(struct object* object) {
    struct blueprint*  new_bp;

    new_bp = blueprint_recompile(object_blueprint(object));

    if (new_bp != NULL) {
        object_switch_blueprint(object, new_bp);
    }
}

void object_refresh(struct object* object) {
    struct blueprint*  new_bp;

    new_bp = blueprint_find_newest_version(object_blueprint(object));

    if (new_bp != NULL) {
        object_switch_blueprint(object, new_bp);
    }
}


bool object_resolve_func_and_page(struct object* object, struct object_page_and_function* result, struct symbol* message, unsigned int args, bool allow_private) {
    return object_page_lookup_list(object->pages, result, message, args, allow_private);
}
