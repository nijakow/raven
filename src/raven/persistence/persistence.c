/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../../runtime/core/objects/mapping.h"
#include "../raven.h"

#include "serializer/serializer.h"

#include "persistence.h"


void persistence_create(struct persistence* persistence, struct raven* raven) {
    persistence->raven = raven;
}

void persistence_destroy(struct persistence* persistence) {

}


static struct symbol* persistence_get_symbol(struct persistence* persistence, const char* name) {
    return raven_find_symbol(persistence->raven, name);
}


static bool persistence_save_raven(struct persistence* persistence, struct serializer* serializer) {
    struct mapping*  toplevel_mapping;

    toplevel_mapping = mapping_new(persistence->raven);

    serializer_write_ptr(serializer, toplevel_mapping);

    return false;
}


bool persistence_save(struct persistence* persistence, const char* path) {
    struct serializer  serializer;
    bool               result;

    serializer_create(&serializer);
    result = persistence_save_raven(persistence, &serializer);
    serializer_destroy(&serializer);

    return result;
}
