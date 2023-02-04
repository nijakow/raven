/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2023  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "serializer/serializer.h"

#include "persistence.h"


void persistence_create(struct persistence* persistence, struct raven* raven) {
    persistence->raven = raven;
}

void persistence_destroy(struct persistence* persistence) {

}


static bool persistence_save_raven(struct persistence* persistence, struct serializer* serializer) {
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
