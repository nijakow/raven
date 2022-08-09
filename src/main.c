/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "defs.h"
#include "raven.h"


static struct raven  THE_RAVEN;

static void raven_signal_handler(int signal) {
  raven_interrupt(&THE_RAVEN);
}

void raven_main(struct raven* raven, int argc, char *argv[]) {
  char*  mudlib_path;

  /*
   * Find the location of the Mudlib.
   * Usually, it is either in `$RAVEN_MUDLIB` or in `argv[1]`.
   */
  if (argc >= 2) {
    mudlib_path = argv[1];
  } else {
    mudlib_path = getenv("RAVEN_MUDLIB");
    if (mudlib_path == NULL)
      mudlib_path = "../lib";
  }

  /*
   * Try to boot up Raven with the specified mudlib.
   * If it worked, enable a port and start the main loop.
   */
  if (raven_boot(raven, mudlib_path)) {
    raven_serve_on(raven, 4242);
    raven_run(raven);
  }
}

int main(int argc, char *argv[]) {
  /*
   * 'Twas brillig, and the slithy toves
   *     Did gyre and gimble in the wabe:
   * All mimsy were the borogoves,
   *     And the mome raths outgrabe.
   *
   *          - Lewis Carroll, Jabberwocky
   */
  raven_create(&THE_RAVEN);
  signal(SIGINT, raven_signal_handler);
  raven_main(&THE_RAVEN, argc, argv);
  raven_destroy(&THE_RAVEN);
  return 0;
}
