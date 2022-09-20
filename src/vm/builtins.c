/*
 * This file is part of the Raven MUD.
 * Copyright (c) 2022  Eric Nijakowski
 *
 * See README and LICENSE for further information.
 */

#include "../defs.h"

#include "../raven/raven.h"
#include "../core/objects/object.h"
#include "../core/objects/array.h"
#include "../core/objects/mapping.h"
#include "../core/objects/string.h"
#include "../core/objects/function.h"
#include "../core/objects/funcref.h"
#include "../core/objects/connection.h"
#include "../core/blueprint.h"
#include "../fs/file.h"
#include "../lang/script.h"
#include "../util/log.h"
#include "../util/wrap.h"

#include "frame.h"
#include "fiber.h"
#include "interpreter.h"

#include "builtins.h"

#define arg_error(fiber) fiber_crash_msg(fiber, "Argument error in builtin!")

void print_object(struct log* log, any object) {
  if (any_is_int(object)) {
    log_printf(log, "%d", any_to_int(object));
  } else if (any_is_obj(object, OBJ_TYPE_STRING)) {
    log_printf(log, "%s", string_contents(any_to_ptr(object)));
  } else if (any_is_obj(object, OBJ_TYPE_OBJECT)) {
    log_printf(log, "object %p", any_to_ptr(object));
  } else if (any_is_obj(object, OBJ_TYPE_ARRAY)) {
    log_printf(log, "array %p (size %u)",
               any_to_ptr(object),
               array_size(any_to_ptr(object)));
  } else if (any_is_obj(object, OBJ_TYPE_FUNCREF)) {
    log_printf(log, "funcref %p", any_to_ptr(object));
  } else if (any_is_obj(object, OBJ_TYPE_FUNCTION)) {
    log_printf(log, "function %p", any_to_ptr(object));
  } else if (any_is_obj(object, OBJ_TYPE_SYMBOL)) {
    log_printf(log, "symbol #'%s'", ((struct symbol*) any_to_ptr(object))->name);
  } else if (any_is_nil(object)) {
    log_printf(log, "(nil)");
  } else if (any_is_ptr(object)) {
    log_printf(log, "%p", any_to_ptr(object));
  } else if (any_is_char(object)) {
    log_printf(log, "%c", any_to_char(object));
  } else {
    log_printf(log, "???");
  }
}

void builtin_throw(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1)
    arg_error(fiber);
  else {
    fiber_throw(fiber, arg[0]);
  }
}

void builtin_sleep(struct fiber* fiber, any* arg, unsigned int args) {
  raven_time_t  current_time;

  if (args != 1 || !any_is_int(arg[0]))
    arg_error(fiber);
  else {
    current_time = raven_time(fiber_raven(fiber));
    fiber_sleep_until(fiber, current_time + any_to_int(arg[0]));
  }
}

void builtin_fork(struct fiber* fiber, any* arg, unsigned int args) {
  if (args == 0 || !any_is_obj(arg[0], OBJ_TYPE_FUNCREF))
    arg_error(fiber);
  else {
    raven_call_out_func(fiber_raven(fiber),
                        any_to_ptr(arg[0]),
                        arg,
                        args);
    fiber_set_accu(fiber, any_nil());
  }
}

void builtin_this_connection(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 0)
    arg_error(fiber);
  else {
    if (fiber_connection(fiber) == NULL)
      fiber_set_accu(fiber, any_nil());
    else
      fiber_set_accu(fiber, any_from_ptr(fiber_connection(fiber)));
  }
}

void builtin_connection_player(struct fiber* fiber, any* arg, unsigned int args) {
  struct connection*  connection;
  struct fiber*       c_fiber;

  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_CONNECTION))
    arg_error(fiber);
  else {
    fiber_set_accu(fiber, any_nil());

    connection = any_to_ptr(arg[0]);
    c_fiber    = connection_fiber(connection);

    if (c_fiber != NULL) {
      fiber_set_accu(fiber, fiber_vars(c_fiber)->this_player);
    }
  }
}

void builtin_print(struct fiber* fiber, any* arg, unsigned int args) {
  for (unsigned int x = 0; x < args; x++) {
    print_object(raven_log(fiber_raven(fiber)), arg[x]);
  }
}

static void builtin_write_to_hlp(struct fiber*      fiber,
                                 any*               arg,
                                 unsigned int       args,
                                 struct connection* connection) {
  char buf[1024*8];

  if (connection != NULL) {
    for (unsigned int x = 0; x < args; x++) {
      if (any_is_int(arg[x])) {
        snprintf(buf, sizeof(buf), "%d", any_to_int(arg[x]));
      } else if (any_is_obj(arg[x], OBJ_TYPE_STRING)) {
        snprintf(buf, sizeof(buf), "%s", string_contents(any_to_ptr(arg[x])));
      } else if (any_is_nil(arg[x])) {
        snprintf(buf, sizeof(buf), "(nil)");
      } else if (any_is_ptr(arg[x])) {
        snprintf(buf, sizeof(buf), "%p", any_to_ptr(arg[x]));
      } else if (any_is_char(arg[x])) {
        snprintf(buf, sizeof(buf), "%c", any_to_char(arg[x]));
      } else {
        snprintf(buf, sizeof(buf), "???");
      }
      connection_output_str(connection, buf);
    }
  }
}

void builtin_write(struct fiber* fiber, any* arg, unsigned int args) {
  builtin_write_to_hlp(fiber, arg, args, fiber_connection(fiber));
}

void builtin_write_to(struct fiber* fiber, any* arg, unsigned int args) {
  if (args < 1 || !any_is_obj(arg[0], OBJ_TYPE_CONNECTION))
    arg_error(fiber);
  else {
    builtin_write_to_hlp(fiber, arg + 1, args - 1, any_to_ptr(arg[0]));
  }
}

void builtin_input_line(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 0)
    arg_error(fiber);
  else {
    fiber_wait_for_input(fiber);
  }
}

void builtin_close(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_CONNECTION))
    arg_error(fiber);
  else {
    connection_close(any_to_ptr(arg[0]));
  }
}

void builtin_the(struct fiber* fiber, any* arg, unsigned int args) {
  any               self;
  struct object*    obj;
  struct blueprint* blue;
  struct file*      file;
  struct file*      file2;
  struct file*      file3;

  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_STRING))
    arg_error(fiber);
  else {
    fiber_set_accu(fiber, any_nil());
    self = frame_self(fiber_top(fiber));
    blue = any_get_blueprint(self);
    if (blue != NULL) {
      file = blueprint_file(blue);
      if (file != NULL) {
        file2 = file_parent(file);
        if (file2 == NULL) file2 = file;
        file3 = file_resolve_flex(file2, string_contents(any_to_ptr(arg[0])));
        if (file3 == NULL)
          fiber_set_accu(fiber, any_nil());
        else {
          obj = file_get_object(file3);
          fiber_set_accu(fiber, (obj == NULL) ? any_nil() : any_from_ptr(obj));
        }
      }
    }
  }
}

void builtin_initializedp(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_OBJECT))
    arg_error(fiber);
  else {
    fiber_set_accu(fiber,
                   any_from_int(object_was_initialized(any_to_ptr(arg[0]))));
  }
}

void builtin_initialize(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_OBJECT))
    arg_error(fiber);
  else {
    object_set_initialized(any_to_ptr(arg[0]));
  }
}

void builtin_arrayp(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1)
    arg_error(fiber);
  else {
    fiber_set_accu(fiber, any_from_int(any_is_obj(arg[0], OBJ_TYPE_ARRAY)));
  }
}

void builtin_stringp(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1)
    arg_error(fiber);
  else {
    fiber_set_accu(fiber, any_from_int(any_is_obj(arg[0], OBJ_TYPE_STRING)));
  }
}

void builtin_objectp(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1)
    arg_error(fiber);
  else {
    fiber_set_accu(fiber, any_from_int(any_is_obj(arg[0], OBJ_TYPE_OBJECT)));
  }
}

void builtin_functionp(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1)
    arg_error(fiber);
  else {
    fiber_set_accu(fiber, any_from_int(any_is_obj(arg[0], OBJ_TYPE_FUNCREF)));
  }
}

void builtin_nil_proxy(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1)
    arg_error(fiber);
  else {
    raven_vars(fiber_raven(fiber))->nil_proxy = arg[0];
  }
}

void builtin_string_proxy(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1)
    arg_error(fiber);
  else {
    raven_vars(fiber_raven(fiber))->string_proxy = arg[0];
  }
}

void builtin_array_proxy(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1)
    arg_error(fiber);
  else {
    raven_vars(fiber_raven(fiber))->array_proxy = arg[0];
  }
}

void builtin_mapping_proxy(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1)
    arg_error(fiber);
  else {
    raven_vars(fiber_raven(fiber))->mapping_proxy = arg[0];
  }
}

void builtin_symbol_proxy(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1)
    arg_error(fiber);
  else {
    raven_vars(fiber_raven(fiber))->symbol_proxy = arg[0];
  }
}

void builtin_connect_func(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_FUNCREF))
    arg_error(fiber);
  else {
    raven_vars(fiber_raven(fiber))->connect_func = any_to_ptr(arg[0]);
  }
}

void builtin_disconnect_func(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_FUNCREF))
    arg_error(fiber);
  else {
    raven_vars(fiber_raven(fiber))->disconnect_func = any_to_ptr(arg[0]);
  }
}

void builtin_clone_object(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_OBJECT))
    arg_error(fiber);
  else {
    fiber_set_accu(fiber, any_from_ptr(object_clone(fiber_raven(fiber),
                                                    any_to_ptr(arg[0]))));
  }
}

void builtin_object_move(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 2 || !any_is_obj(arg[0], OBJ_TYPE_OBJECT))
    arg_error(fiber);
  else {
    if (any_is_obj(arg[1], OBJ_TYPE_OBJECT))
      object_move_to(any_to_ptr(arg[0]), any_to_ptr(arg[1]));
    else if (any_is_nil(arg[1]))
      object_move_to(any_to_ptr(arg[0]), NULL);
    else
      arg_error(fiber);
    fiber_set_accu(fiber, arg[0]);
  }
}

void builtin_object_parent(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_OBJECT))
    arg_error(fiber);
  else {
    if (object_parent(any_to_ptr(arg[0])) == NULL)
      fiber_set_accu(fiber, any_nil());
    else
      fiber_set_accu(fiber, any_from_ptr(object_parent(any_to_ptr(arg[0]))));
  }
}

void builtin_object_sibling(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_OBJECT))
    arg_error(fiber);
  else {
    if (object_sibling(any_to_ptr(arg[0])) == NULL)
      fiber_set_accu(fiber, any_nil());
    else
      fiber_set_accu(fiber, any_from_ptr(object_sibling(any_to_ptr(arg[0]))));
  }
}

void builtin_object_children(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_OBJECT))
    arg_error(fiber);
  else {
    if (object_children(any_to_ptr(arg[0])) == NULL)
      fiber_set_accu(fiber, any_nil());
    else
      fiber_set_accu(fiber, any_from_ptr(object_children(any_to_ptr(arg[0]))));
  }
}

void builtin_object_path(struct fiber* fiber, any* arg, unsigned int args) {
  struct blueprint*     blue;
  struct file*          file;
  struct string*        string;
  struct stringbuilder  sb;

  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_OBJECT))
    arg_error(fiber);
  else {
    fiber_set_accu(fiber, any_nil());
    blue = any_get_blueprint(arg[0]);
    if (blue != NULL) {
      file = blueprint_file(blue);
      if (file != NULL) {
        stringbuilder_create(&sb);
        file_write_path(file, &sb);
        string = string_new_from_stringbuilder(fiber_raven(fiber), &sb);
        stringbuilder_destroy(&sb);
        fiber_set_accu(fiber, any_from_ptr(string));
      }
    }
  }
}

void builtin_object_set_hb(struct fiber* fiber, any* arg, unsigned int args) {
  struct object**  heartbeats;

  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_OBJECT))
    arg_error(fiber);
  else {
    heartbeats = object_table_heartbeats(raven_objects(fiber_raven(fiber)));
    object_link_heartbeat(any_to_ptr(arg[0]), heartbeats);
    fiber_set_accu(fiber, arg[0]);
  }
}

void builtin_object_fst_hb(struct fiber* fiber, any* arg, unsigned int args) {
  struct object*  first;

  if (args != 0)
    arg_error(fiber);
  else {
    first = *object_table_heartbeats(raven_objects(fiber_raven(fiber)));
    if (first == NULL)
      fiber_set_accu(fiber, any_nil());
    else
      fiber_set_accu(fiber, any_from_ptr(first));
  }
}

void builtin_object_next_hb(struct fiber* fiber, any* arg, unsigned int args) {
  struct object*  object;

  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_OBJECT))
    arg_error(fiber);
  else {
    object = any_to_ptr(arg[0]);
    if (object_next_heartbeat(object) == NULL)
      fiber_set_accu(fiber, any_nil());
    else
      fiber_set_accu(fiber, any_from_ptr(object_next_heartbeat(object)));
  }
}

void builtin_substr(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 3
   || !any_is_obj(arg[0], OBJ_TYPE_STRING)
   || !any_is_int(arg[1])
   || !any_is_int(arg[2]))
    arg_error(fiber);
  else {
    fiber_set_accu(fiber,
                   any_from_ptr(string_substr(any_to_ptr(arg[0]),
                                              any_to_int(arg[1]),
                                              any_to_int(arg[2]),
                                              fiber_raven(fiber))));
  }
}

void builtin_mkarray(struct fiber* fiber, any* arg, unsigned int args) {
  struct array* array;

  if (args != 1 || !any_is_int(arg[0]))
    arg_error(fiber);
  else {
    array = array_new(fiber_raven(fiber), any_to_int(arg[0]));
    fiber_set_accu(fiber, any_from_ptr(array));
  }
}

void builtin_append(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 2 || !any_is_obj(arg[0], OBJ_TYPE_ARRAY))
    arg_error(fiber);
  else {
    array_append(any_to_ptr(arg[0]), arg[1]);
  }
}

void builtin_keys(struct fiber* fiber, any* arg, unsigned int args) {
  struct array* array;

  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_MAPPING))
    arg_error(fiber);
  else {
    array = mapping_keys(any_to_ptr(arg[0]), fiber_raven(fiber));
    if (array == NULL)
      fiber_set_accu(fiber, any_nil());
    else
      fiber_set_accu(fiber, any_from_ptr(array));
  }
}

void builtin_isspace(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 1 || !any_is_char(arg[0]))
    arg_error(fiber);
  else {
    fiber_set_accu(fiber, any_from_int(isspace(any_to_char(arg[0]))));
  }
}

void builtin_wrap(struct fiber* fiber, any* arg, unsigned int args) {
  struct string*        string;
  struct stringbuilder  sb;
  char*                 wrapped;

  if (args != 2
      || !any_is_obj(arg[0], OBJ_TYPE_STRING)
      || !any_is_int(arg[1]))
    arg_error(fiber);
  else {
    stringbuilder_create(&sb);
    string_wrap_into(string_contents(any_to_ptr(arg[0])),
                     any_to_int(arg[1]),
                     &sb);
    stringbuilder_get(&sb, &wrapped);
    stringbuilder_destroy(&sb);
    string = string_new(fiber_raven(fiber), wrapped);
    free(wrapped);
    fiber_set_accu(fiber, any_from_ptr(string));
  }
}

void builtin_implements(struct fiber* fiber, any* arg, unsigned int args) {
  bool result;

  if (args != 2 || !any_is_obj(arg[1], OBJ_TYPE_SYMBOL))
    arg_error(fiber);
  else {
    result = (any_resolve_func(arg[0], any_to_ptr(arg[1])) != NULL);
    fiber_set_accu(fiber, any_from_int(result));
  }
}

void builtin_call(struct fiber* fiber, any* arg, unsigned int args) {
  struct funcref*  funcref;
  struct function* function;
  unsigned int     index;
  any              fargs[args - 1];

  if (args < 1) {
    arg_error(fiber);
  } else if (args >= 2 && any_is_obj(arg[0], OBJ_TYPE_FUNCTION)) {
    function = any_to_ptr(arg[0]);
    for (index = 1; index < args; index++)
      fiber_push(fiber, arg[index]);
    fiber_push_frame(fiber, function, args - 2);
  } else if (any_is_obj(arg[0], OBJ_TYPE_FUNCREF)) {
    funcref = any_to_ptr(arg[0]);
    for (index = 0; index < args - 1; index++)
      fargs[index] = arg[index + 1];
    funcref_enter(funcref, fiber, fargs, args - 1);
  } else {
    arg_error(fiber);
  }
}

void builtin_this_player(struct fiber* fiber, any* arg, unsigned int args) {
  if (args == 0)
    fiber_set_accu(fiber, fiber_vars(fiber)->this_player);
  else if (args == 1)
    fiber_vars(fiber)->this_player = arg[0];
  else
    arg_error(fiber);
}

void builtin_ls(struct fiber* fiber, any* arg, unsigned int args) {
  struct array*   files;
  struct string*  string;
  struct string*  name;
  const char*     path;
  struct file*    file;
  struct file*    child;

  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_STRING))
    arg_error(fiber);
  else {
    files  = array_new(fiber_raven(fiber), 0);
    string = any_to_ptr(arg[0]);
    path   = string_contents(string);
    file   = filesystem_resolve(raven_fs(fiber_raven(fiber)), path);
    if (file != NULL) {
      for (child = file_children(file);
           child != NULL;
           child = file_sibling(child)) {
        name = string_new(fiber_raven(fiber), file_name(child));
        array_append(files, any_from_ptr(name));
      }
    }
    fiber_set_accu(fiber, any_from_ptr(files));
  }
}

void builtin_resolve(struct fiber* fiber, any* arg, unsigned int args) {
  struct filesystem*  filesystem;
  struct file*        file;
  struct string*      result_str;
  const char*         base_path;
  const char*         direction;
        char*         result_path;

  if (args != 2
      || !any_is_obj(arg[0], OBJ_TYPE_STRING)
      || !any_is_obj(arg[1], OBJ_TYPE_STRING))
    arg_error(fiber);
  else {
    filesystem = raven_fs(fiber_raven(fiber));
    base_path  = string_contents(any_to_ptr(arg[0]));
    direction  = string_contents(any_to_ptr(arg[1]));
    file       = filesystem_resolve(filesystem, base_path);

    if (file == NULL)
      fiber_set_accu(fiber, any_nil());
    else {
      file        = file_resolve(file, direction);
      if (file == NULL)
        fiber_set_accu(fiber, any_nil());
      else {
        /*
         * TODO, FIXME, XXX: result_path could be NULL!
         */
        result_path = file_path(file);
        result_str  = string_new(fiber_raven(fiber), result_path);
        free(result_path);
        fiber_set_accu(fiber, any_from_ptr(result_str));
      }
    }
  }
}

void builtin_cat(struct fiber* fiber, any* arg, unsigned int args) {
  struct filesystem*    filesystem;
  struct file*          file;
  const char*           path;
  struct string*        result;
  struct stringbuilder  sb;

  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_STRING))
    arg_error(fiber);
  else {
    filesystem = raven_fs(fiber_raven(fiber));
    path       = string_contents(any_to_ptr(arg[0]));
    file       = filesystem_resolve(filesystem, path);

    if (file == NULL)
      fiber_set_accu(fiber, any_nil());
    else {
      stringbuilder_create(&sb);
      file_cat(file, &sb);
      result = string_new_from_stringbuilder(fiber_raven(fiber), &sb);
      stringbuilder_destroy(&sb);
      fiber_set_accu(fiber, any_from_ptr(result));
    }
  }
}

void builtin_cc(struct fiber* fiber, any* arg, unsigned int args) {
  struct filesystem*  filesystem;
  struct file*        file;
  const  char*        path;

  if (args != 1 || !any_is_obj(arg[0], OBJ_TYPE_STRING))
    arg_error(fiber);
  else {
    filesystem = raven_fs(fiber_raven(fiber));
    path       = string_contents(any_to_ptr(arg[0]));
    file       = filesystem_resolve(filesystem, path);

    if (file == NULL)
      fiber_set_accu(fiber, any_from_int(1));
    else {
      if (file_recompile(file, raven_log(fiber_raven(fiber)))) {
        fiber_set_accu(fiber, any_nil()); /* Success! */
      } else {
        fiber_set_accu(fiber, any_from_int(2)); /* Failure! */
      }
    }
  }
}

void builtin_cc_script(struct fiber* fiber, any* arg, unsigned int args) {
  struct function*  function;

  if (args != 2 || !any_is_obj(arg[0], OBJ_TYPE_STRING)
                || !any_is_obj(arg[1], OBJ_TYPE_MAPPING))
    arg_error(fiber);
  else {
    function = script_compile(fiber_raven(fiber),
                              string_contents(any_to_ptr(arg[0])),
                              any_to_ptr(arg[1]));
    if (function == NULL)
      fiber_set_accu(fiber, any_nil());
    else
      fiber_set_accu(fiber, any_from_ptr(function));
  }
}

void builtin_gc(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 0)
    arg_error(fiber);
  else {
    raven_gc(fiber_raven(fiber));
    fiber_set_accu(fiber, any_nil());
  }
}

void builtin_disassemble(struct fiber* fiber, any* arg, unsigned int args) {
  struct blueprint*  blueprint;
  struct function*   function;

  if (args != 2 || !any_is_obj(arg[1], OBJ_TYPE_SYMBOL))
    arg_error(fiber);
  else {
    blueprint = any_get_blueprint(arg[0]);
    if (blueprint != NULL) {
      function = blueprint_lookup(blueprint, any_to_ptr(arg[1]));
      if (function != NULL) {
        function_disassemble(function, raven_log(fiber_raven(fiber)));
      }
    }
    /* TODO: Return a string */
  }
}

void builtin_random(struct fiber* fiber, any* arg, unsigned int args) {
  if (args != 0)
    arg_error(fiber);
  else {
    fiber_set_accu(fiber, any_from_int(rand()));
  }
}
