#include "persistence.h"

void persistence_create(struct persistence* persistence, struct raven* raven) {
    persistence->raven = raven;
}

void persistence_destroy(struct persistence* persistence) {
    
}
