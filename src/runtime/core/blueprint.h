/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#ifndef RAVEN_CORE_BLUEPRINT_H
#define RAVEN_CORE_BLUEPRINT_H

#include "../../defs.h"
#include "base_obj.h"
#include "vars.h"


/*
 * A blueprint works much like a class in other languages.
 *
 * The struct contains a pointer to the file that it is bound to,
 * a pointer to the blueprint it inherits from, a linked list of
 * its direct member functions and a list of its local variables.
 *
 * Blueprints extend `struct base_obj`, thus technically making
 * them first-class objects. However, this was only done to ease
 * garbage collection and is in no way relevant for the language
 * itself.
 */
struct blueprint {
    struct base_obj    _;
    struct raven*      raven;
    char*              virt_path;
    struct blueprint*  parent;
    struct function*   methods;
    struct vars        vars;
};

struct blueprint* blueprint_new(struct raven* raven, const char* virt_path);
void blueprint_mark(struct gc* gc, struct blueprint* blue);
void blueprint_del(struct blueprint* blueprint);

struct object*      blueprint_instantiate(struct blueprint* blueprint, struct raven* raven);
struct object_page* blueprint_instantiate_page(struct blueprint* blueprint);

struct blueprint* blueprint_load_relative(struct blueprint* bp, const char* path);

unsigned int blueprint_get_instance_size(struct blueprint* bp);

bool blueprint_inherit(struct blueprint* blue, struct blueprint* parent);

void blueprint_add_var(struct blueprint* blue,
                       struct type*      type,
                       struct symbol*    name);
void blueprint_add_func(struct blueprint* blue,
                        struct symbol*    name,
                        struct function*  func);

struct function* blueprint_lookup(struct blueprint* blue, struct symbol* msg, unsigned int args, bool allow_private);

struct blueprint* blueprint_recompile(struct blueprint* blue);
struct blueprint* blueprint_find_newest_version(struct blueprint* blue);

bool blueprint_is_soulmate(struct blueprint* blue, struct blueprint* potential_soulmate);
struct blueprint* blueprint_soulmate(struct blueprint* blue, struct blueprint* potential_soulmate);

static inline struct raven* blueprint_raven(struct blueprint* blue) {
    return blue->raven;
}

static inline struct blueprint* blueprint_parent(struct blueprint* blue) {
    return blue->parent;
}

static inline const char* blueprint_virt_path(struct blueprint* blue) {
    return blue->virt_path;
}

static inline struct vars* blueprint_vars(struct blueprint* bp) {
    return &bp->vars;
}

#endif
