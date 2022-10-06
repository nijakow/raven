/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../raven/raven.h"

#include "codewriter.h"
#include "compiler.h"
#include "reader.h"
#include "parser.h"
#include "parsepiler.h"

#include "script.h"

struct function* script_compile(struct raven*   raven,
                                const  char*    source,
                                struct mapping* vars,
                                struct log*     log) {
  t_reader           reader;
  struct parser      parser;
  struct codewriter  codewriter;
  struct compiler    compiler;
  struct function*   result;

  result = NULL;
  reader_create(&reader, source);
  parser_create(&parser, raven, &reader, log);
  codewriter_create(&codewriter, raven);
  compiler_create(&compiler, raven, &codewriter, NULL);
  if (vars != NULL)
    compiler_set_mapping_vars(&compiler, vars);
  if (parsepile_script(&parser, &compiler)) {
    result = compiler_finish(&compiler);
  }
  compiler_destroy(&compiler);
  codewriter_destroy(&codewriter);
  parser_destroy(&parser);
  reader_destroy(&reader);
  return result;
}
