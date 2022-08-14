/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../raven.h"
#include "codewriter.h"
#include "compiler.h"
#include "reader.h"
#include "parser.h"
#include "parsepiler.h"

#include "script.h"

struct function* script_compile(struct raven*   raven,
                                const  char*    source,
                                struct mapping* vars) {
  t_reader           reader;
  struct parser      parser;
  struct codewriter  codewriter;
  struct compiler    compiler;
  struct function*   result;

  result = NULL;
  reader = (char*) source;
  parser_create(&parser, raven, &reader, raven_log(raven));
  codewriter_create(&codewriter, raven);
  compiler_create(&compiler, &codewriter, NULL);
  if (vars != NULL)
    compiler_set_mapping_vars(&compiler, vars);
  if (parsepile_script(&parser, &compiler)) {
    result = compiler_finish(&compiler);
  }
  compiler_destroy(&compiler);
  codewriter_destroy(&codewriter);
  parser_destroy(&parser);
  return result;
}
