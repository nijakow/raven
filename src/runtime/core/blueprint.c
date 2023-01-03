/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../raven/raven.h"
#include "../../platform/fs/fs.h"
#include "../../util/memory.h"

#include "objects/object/object.h"
#include "objects/function.h"

#include "blueprint.h"

struct obj_info BLUEPRINT_INFO = {
    .type  = OBJ_TYPE_BLUEPRINT,
    .mark  = (mark_func)  blueprint_mark,
    .del   = (del_func)   blueprint_del,
    .stats = (stats_func) base_obj_stats
};

static void blueprint_create(struct blueprint* blue, const char* virt_path) {
    blue->virt_path = memory_strdup(virt_path);
    blue->parent    = NULL;
    blue->methods   = NULL;
    vars_create(&blue->vars);
}

static void blueprint_destroy(struct blueprint* blue) {
    while (blue->methods != NULL)
        function_unlink(blue->methods);
    vars_destroy(&blue->vars);
    memory_free(blue->virt_path);
}

struct blueprint* blueprint_new(struct raven* raven, const char* virt_path) {
    struct blueprint* blueprint;

    blueprint = base_obj_new(raven_objects(raven),
                             &BLUEPRINT_INFO,
                             sizeof(struct blueprint));

    if (blueprint != NULL) {
        blueprint->raven = raven;
        blueprint_create(blueprint, virt_path);
    }

    return blueprint;
}

void blueprint_del(struct blueprint* blueprint) {
    blueprint_destroy(blueprint);
    base_obj_del(&blueprint->_);
}

void blueprint_mark(struct gc* gc, struct blueprint* blue) {
    struct function*  func;

    if (blue == NULL) return;

    for (func = blue->methods; func != NULL; func = func->next_method)
        gc_mark_ptr(gc, func);

    vars_mark(gc, &blue->vars);
    gc_mark_ptr(gc, blue->parent);

    base_obj_mark(gc, &blue->_);
}

struct object* blueprint_instantiate(struct blueprint* blueprint,
                                     struct raven*     raven) {
    return object_new(raven, blueprint);
}

struct object_page* blueprint_instantiate_page(struct blueprint* blueprint) {
    return object_page_new(blueprint);
}

unsigned int blueprint_get_instance_size(struct blueprint* bp) {
    return vars_count(blueprint_vars(bp));
}

bool blueprint_inherit(struct blueprint* blue, struct blueprint* parent) {
    if (blue->parent != NULL)
        return false;
    else {
        blue->parent = parent;
        return true;
    }
}

void blueprint_add_var(struct blueprint* blue,
                       struct type*      type,
                       struct symbol*    name) {
    vars_add(blueprint_vars(blue), type, name);
}

void blueprint_add_func(struct blueprint* blue,
                        struct symbol*    name,
                        struct function*  func) {
    function_in_blueprint(func, blue, name);
}

static bool raven_modifier_is_hidden(enum raven_modifier mod) {
    return (mod == RAVEN_MODIFIER_PRIVATE) || (mod == RAVEN_MODIFIER_PROTECTED);
}

struct function* blueprint_lookup(struct blueprint* blue, struct symbol* msg, unsigned int args, bool allow_private) {
    struct function*  function;

    function = blue->methods;

    while (function != NULL) {
        if (function->name == msg
            && (!raven_modifier_is_hidden(function->modifier) || allow_private)
            && function_takes_args(function, args))
            return function;
        function = function->next_method;
    }

    return NULL;
}

struct blueprint* blueprint_load_relative(struct blueprint* bp,
                                          const  char*      path) {
    // TODO, FIXME, XXX!
    return NULL;
}

struct blueprint* blueprint_recompile(struct blueprint* blue) {
    // TODO, FIXME, XXX!
    return NULL;
}

struct blueprint* blueprint_find_newest_version(struct blueprint* blue) {
    // TODO, FIXME, XXX!
    return NULL;
}

bool blueprint_is_soulmate(struct blueprint* blue, struct blueprint* potential_soulmate) {
    // TODO, FIXME, XXX!
    return false;
}

struct blueprint* blueprint_soulmate(struct blueprint* blue, struct blueprint* potential_soulmate) {
    while (blue != NULL) {
        if (blueprint_is_soulmate(blue, potential_soulmate))
            return blue;
        blue = blueprint_parent(blue);
    }
    return NULL;
}
