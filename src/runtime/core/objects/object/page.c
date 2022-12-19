/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../../../defs.h"
#include "../../../../util/memory.h"
#include "../../../gc/gc.h"
#include "../../blueprint.h"
#include "../../vars.h"

#include "object.h"

#include "page.h"

void object_page_create(struct object_page* page, unsigned int slot_count, struct blueprint* blue) {
    unsigned int  index;

    page->object      = NULL;
    page->next        = NULL;
    page->blue        = blue;
    page->slot_count  = slot_count;
    page->slots       = page->payload;

    for (index = 0; index < slot_count; index++)
        page->slots[index] = any_nil();
}

void object_page_destroy(struct object_page* page) {
    object_page_unlink(page);
}

struct object_page* object_page_new(struct blueprint* blue) {
    struct object_page* page;
    unsigned int        slot_count;

    slot_count = blueprint_get_instance_size(blue);

    page = memory_alloc(sizeof(struct object_page) + slot_count * sizeof(any));

    if (page != NULL) {
        object_page_create(page, slot_count, blue);
    }

    return page;
}

void object_page_mark(struct gc* gc, struct object_page* page) {
    unsigned int  index;

    gc_mark_ptr(gc, page->blue);
    for (index = 0; index < page->slot_count; index++)
        gc_mark_any(gc, page->slots[index]);
}

void object_page_del(struct object_page* page) {
    object_page_destroy(page);
    memory_free(page);
}

void object_page_link(struct object_page* page, struct object* object) {
    struct object_page**  list_head;

    page->object = object;
    
    for (list_head = &object->pages;
        *list_head != NULL;
         list_head = &(*list_head)->next) {
        /*
         * Do nothing.
         */
    }

    *list_head = page;
}

void object_page_link_before(struct object_page* page, struct object* object, struct object_page* before) {
    struct object_page**  list_head;

    page->object = object;
    
    for (list_head = &object->pages;
        *list_head != before;
         list_head = &(*list_head)->next) {
        /*
         * Do nothing.
         */
    }

    page->next = before;
    *list_head = page;
}

void object_page_unlink(struct object_page* page) {
    struct object_page**  list_head;

    if (page->object != NULL) {

        for (list_head = &page->object->pages;
            *list_head != page;
            list_head = &(*list_head)->next) {
            /*
            * Do nothing.
            */
        }

        *list_head = page->next;

        page->object = NULL;
    }
}

bool object_page_lookup(struct object_page* page, struct object_page_and_function* result, struct symbol* message, unsigned int args, bool is_public) {
    struct function*  function;

    function = blueprint_lookup(page->blue, message, args, is_public);

    if (function != NULL) {
        if (result != NULL) {
            result->page     = page;
            result->function = function;
        }
        return true;
    } else {
        return false;
    }
}

bool object_page_lookup_list(struct object_page* page, struct object_page_and_function* result, struct symbol* message, unsigned int args, bool is_public) {
    if (object_page_lookup(page, result, message, args, is_public))
        return true;
    else if (page->next != NULL)
        return object_page_lookup_list(page->next, result, message, args, is_public);
    else
        return false;
}

void object_page_transfer_vars(struct object_page* page, struct object_page* other) {
    struct blueprint*  blue;
    struct blueprint*  other_blue;
    struct symbol*     var_name;
    unsigned int       index;
    unsigned int       other_index;

    blue       = object_page_blueprint(page);
    other_blue = object_page_blueprint(other);

    for (index = 0; index < vars_count(blueprint_vars(blue)); index++) {
        var_name = vars_name_for_local_index(blueprint_vars(blue), index);
        for (other_index = 0; other_index < vars_count(blueprint_vars(other_blue)); other_index++) {
            if (vars_find(blueprint_vars(other_blue), var_name, NULL, &other_index)) {
                page->slots[index] = other->slots[other_index];
                break;
            }
        }
    }
}
