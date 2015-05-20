/*
 * /obelix/src/script.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <exception.h>
#include <logging.h>
#include <namespace.h>
#include <script.h>
#include <str.h>
#include <stacktrace.h>
#include <thread.h>
#include <wrapper.h>

int script_debug = 0;

static void             _script_init(void) __attribute__((constructor(102)));

/* -- data_t type description structures ---------------------------------- */

static vtable_t _wrapper_vtable_script[] = {
  { .id = FunctionCopy,     .fnc = (void_t) script_copy },
  { .id = FunctionCmp,      .fnc = (void_t) script_cmp },
  { .id = FunctionFree,     .fnc = (void_t) script_free },
  { .id = FunctionToString, .fnc = (void_t) script_tostring },
  { .id = FunctionCall,     .fnc = (void_t) script_execute },
  { .id = FunctionNone,     .fnc = NULL }
};

static vtable_t _wrapper_vtable_bound_method[] = {
  { .id = FunctionCopy,     .fnc = (void_t) bound_method_copy },
  { .id = FunctionCmp,      .fnc = (void_t) bound_method_cmp },
  { .id = FunctionFree,     .fnc = (void_t) bound_method_free },
  { .id = FunctionToString, .fnc = (void_t) bound_method_tostring },
  { .id = FunctionCall,     .fnc = (void_t) bound_method_execute },
  { .id = FunctionNone,     .fnc = NULL }
};

/* ------------------------------------------------------------------------ */

void _script_init(void) {
  logging_register_category("script", &script_debug);
  wrapper_register(Script, "script", _wrapper_vtable_script);
  wrapper_register(BoundMethod, "boundmethod", _wrapper_vtable_bound_method);
}

/* -- Script data functions ----------------------------------------------- */

data_t * data_create_script(script_t *script) {
  return data_create(Script, script);
}

data_t * data_create_closure(closure_t *closure) {
  return data_create(Closure, closure);
}

/* -- S C R I P T  P U B L I C  F U N C T I O N S  ------------------------ */

script_t * _script_set_instructions(script_t *script, list_t *block) {
  if (!block) {
    block = script -> main_block;
  }
  script -> instructions = block;
  return script;
}

script_t * script_create(module_t *mod, script_t *up, char *name) {
  script_t   *ret;
  char       *anon = NULL;

  if (!name) {
    asprintf(&anon, "__anon__%d__", hashptr(ret));
    name = anon;
  }
  if (script_debug) {
    debug("Creating script '%s'", name);
  }
  ret = NEW(script_t);
  
  ret -> main_block = list_create();
  list_set_free(ret -> main_block, (free_t) instruction_free);
  list_set_tostring(ret -> main_block, (tostring_t) instruction_tostring);
  _script_set_instructions(ret, NULL);
  ret -> deferred_blocks = datastack_create("deferred blocks");
  ret -> bookmarks = datastack_create("bookmarks");
  
  ret -> functions = strdata_dict_create();
  ret -> labels = strvoid_dict_create();
  ret -> pending_labels = datastack_create("pending labels");
  ret -> params = NULL;
  ret -> async = 0;
  ret -> current_line = -1;

  if (up) {
    dict_put(up -> functions, strdup(name), data_create_script(ret));
    ret -> up = script_copy(up);
    ret -> mod = mod_copy(up -> mod);
    ret -> name = name_deepcopy(up -> name);
    name_extend(ret -> name, name);
  } else {
    assert(mod);
    ret -> mod = mod_copy(mod);
    ret -> up = NULL;
    ret -> name = name_create(0);
  }
  ret -> fullname = NULL;
  ret -> str = NULL;
  ret -> refs = 1;
  free(anon);
  return ret;
}

script_t * script_copy(script_t *script) {
  if (script) {
    script -> refs++;
  }
  return script;
}

name_t * script_fullname(script_t *script) {
  if (!script -> fullname) {
    script -> fullname = name_deepcopy(script -> mod -> name);
    name_append(script -> fullname, script -> name);
  }
  return script -> fullname;
}

char * script_tostring(script_t *script) {
  if (!script -> str) {
    script -> str = strdup(name_tostring(script_fullname(script)));
  }
  return script -> str;
}

void script_free(script_t *script) {
  if (script && (--script -> refs <= 0)) {
    datastack_free(script -> pending_labels);
    datastack_free(script -> bookmarks);
    datastack_free(script -> deferred_blocks);
    list_free(script -> instructions);
    dict_free(script -> labels);
    array_free(script -> params);
    dict_free(script -> functions);
    script_free(script -> up);
    mod_free(script -> mod);
    name_free(script -> name);
    name_free(script -> fullname);
    free(script -> str);
    free(script);
  }
}

int script_cmp(script_t *s1, script_t *s2) {
  if (s1 && !s2) {
    return 1;
  } else if (!s1 && s2) {
    return -1;
  } else if (!s1 && !s2) {
    return 0;
  } else {
    return name_cmp(script_fullname(s1), script_fullname(s2));
  }
}

unsigned int script_hash(script_t *script) {
  return (script) ? name_hash(script_fullname(script)) : 0L;
}

script_t * script_push_instruction(script_t *script, instruction_t *instruction) {
  listnode_t    *node;
  data_t        *label;
  int            line;
  instruction_t *last;

  last = list_tail(script -> instructions);
  line = (last) ? last -> line : -1;
  if (script -> current_line > line) {
    instruction -> line = script -> current_line;
  }
  list_push(script -> instructions, instruction);
  if (!datastack_empty(script -> pending_labels)) {
    label = datastack_peek(script -> pending_labels);
    instruction_set_label(instruction, label);
    node = list_tail_pointer(script -> instructions);
    while (!datastack_empty(script -> pending_labels)) {
      label = datastack_pop(script -> pending_labels);
      dict_put(script -> labels, strdup(data_charval(label)), node);
      data_free(label);
    }
  }
  return script;
}

script_t * script_start_deferred_block(script_t *script) {
  list_t *block;

  block = list_create();
  list_set_free(block, (free_t) instruction_free);
  list_set_tostring(block, (tostring_t) instruction_tostring);
  _script_set_instructions(script, block);
  return script;
}

script_t * script_end_deferred_block(script_t *script) {
  datastack_push(script -> deferred_blocks, 
                 data_create(Pointer, sizeof(list_t), script -> instructions));
  _script_set_instructions(script, NULL);
  return script;
}

script_t * script_pop_deferred_block(script_t *script) {
  list_t        *block;
  data_t        *data;
  instruction_t *instr;
  
  data = datastack_pop(script -> deferred_blocks);
  block = data_unwrap(data);
  list_join(script -> instructions, block);
  data_free(data);
  return script;
}

script_t * script_bookmark(script_t *script) {
  listnode_t *node = list_tail_pointer(script -> instructions);
  data_t     *data = data_create(Pointer, sizeof(listnode_t), node);
  
  assert(data_unwrap(data) == node);
  datastack_push(script -> bookmarks, data);
  return script;
}

script_t * script_discard_bookmark(script_t *script) {
  data_t        *data = datastack_pop(script -> bookmarks);
  listnode_t    *node = data_unwrap(data);

  datastack_pop(script -> bookmarks);  
  return script;
}

script_t * script_defer_bookmarked_block(script_t *script) {
  listnode_t    *node;
  data_t        *data;
  list_t        *block = script -> instructions;
  instruction_t *instr;
  
  data = datastack_pop(script -> bookmarks);
  node = data_unwrap(data);
  debug("%p -> %p", node, (node) ? node -> list : NULL);
  script_start_deferred_block(script);
  if (node) {
    list_position(node);
    block = list_split(block);
  }
  list_join(script -> instructions, block);
  script_end_deferred_block(script);
  return script;
}

void script_list(script_t *script) {
  instruction_t *instr;
  
  debug("==================================================================");
  debug("Script Listing - %s", script_tostring(script));
  debug("------------------------------------------------------------------");
  debug("%-6s %-11.11s%-15.15s", "Line", "Label", "Instruction");
  debug("------------------------------------------------------------------");
  for (list_start(script -> instructions); list_has_next(script -> instructions); ) {
    instr = (instruction_t *) list_next(script -> instructions);
    debug(instruction_tostring(instr));
  }
  debug("==================================================================");
}

script_t * script_get_toplevel(script_t *script) {
  script_t *ret;

  for (ret = script; ret -> up; ret = ret -> up);
  return ret;
}

data_t * script_execute(script_t *script, array_t *args, dict_t *kwargs) {
  closure_t *closure;
  data_t    *retval;
  
  if (script_debug) {
    debug("script_execute(%s)", script_tostring(script));
  }
  closure = script_create_closure(script, NULL, NULL);
  retval = closure_execute(closure, args, kwargs);
  closure_free(closure);
  if (script_debug) {
    debug("  script_execute returns %s", data_tostring(retval));
  }
  return retval;
}

data_t * script_create_object(script_t *script, array_t *params, dict_t *kwparams) {
  object_t  *retobj;
  data_t    *retval;
  data_t    *dscript;
  data_t    *bm;
  data_t    *self;

  if (script_debug) {
    debug("script_create_object(%s)", script_tostring(script));
  }
  retobj = object_create(dscript = data_create(Script, script));
  if (!script -> up) {
    script -> mod -> obj = object_copy(retobj);
  }
  retobj -> constructing = TRUE;
  retval = bound_method_execute(data_boundmethodval(retobj -> constructor), 
                                params, kwparams);
  retobj -> constructing = FALSE;
  if (!data_is_exception(retval)) {
    retobj -> retval = retval;
    retval = data_create(Object, retobj);
  }
  if (script_debug) {
    debug("  script_create_object returns %s", data_tostring(retval));
  }
  data_free(dscript);
  return retval;
}

bound_method_t * script_bind(script_t *script, object_t *object) {
  bound_method_t *ret = bound_method_create(script, object);
  
  return ret;
}

closure_t * script_create_closure(script_t *script, closure_t *up, data_t *self) {
  return closure_create(script, up, self);
}

/* -- B O U N D  M E T H O D  F U N C T I O N S   ------------------------- */

bound_method_t * bound_method_create(script_t *script, object_t *self) {
  bound_method_t *ret = NEW(bound_method_t);
  
  ret -> script = script_copy(script);
  ret -> self = object_copy(self);
  ret -> closure = NULL;
  ret -> str = NULL;
  ret -> refs = 1;
  return ret;
}

void bound_method_free(bound_method_t *bm) {
  if (bm) {
    bm -> refs--;
    if (bm -> refs <= 0) {
      script_free(bm -> script);
      object_free(bm -> self);
      closure_free(bm -> closure);
      free(bm -> str);
      free(bm);
    }
  }
}

bound_method_t * bound_method_copy(bound_method_t *bm) {
  if (bm) {
    bm -> refs++;
  }
  return bm;
}

int bound_method_cmp(bound_method_t *bm1, bound_method_t *bm2) {
  return (!object_cmp(bm1 -> self, bm2 -> self)) 
    ? script_cmp(bm1 -> script, bm2 -> script)
    : 0;
}

char * bound_method_tostring(bound_method_t *bm) {
  if (bm -> script) {
    asprintf(&bm -> str, "%s (bound)", 
             script_tostring(bm -> script));
  } else {
    bm -> str = strdup("uninitialized");
  }
  return bm -> str;
}

data_t * bound_method_execute(bound_method_t *bm, array_t *params, dict_t *kwparams) {
  closure_t *closure;
  data_t    *self;
  data_t    *ret;

  self = (bm -> self) ? data_create(Object, bm -> self) : NULL;
  closure = closure_create(bm -> script, bm -> closure, self);
  ret = closure_execute(closure, params, kwparams);
  closure_free(closure);
  return ret;  
}

