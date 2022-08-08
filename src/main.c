/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "defs.h"
#include "raven.h"


void raven_main(struct raven* raven, int argc, char *argv[]) {
  if (raven_boot(raven, argc < 2 ? "../lib" : argv[1]))
    raven_run(raven);
}

int main(int argc, char *argv[]) {
  struct raven  raven;

  raven_create(&raven);
  raven_main(&raven, argc, argv);
  raven_destroy(&raven);

  return 0;
}
