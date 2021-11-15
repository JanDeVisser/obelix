/*
 * /obelix/src/virtualmachine/debug.c - Copyright (c) 2020 Jan de Visser <jan@finiandarcy.com>
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

#include "libvm.h"
#include <stdio.h>
#ifdef WITH_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif /* WITH_READLINE */

#ifdef WITH_READLINE
static char * _debug_readstring(char *prompt) {
  return readline(prompt);
}
#else /* WITH_READLINE */
static char * _debug_readstring(char *prompt) {
  char buf[1024];

  printf(prompt);
  return fgets(buf, 1024, stdin);
}
#endif /* WITH_READLINE */

static void _stack_list_visitor(data_t *entry) {
  printf("   . %-40.40s [%-10.10s]\n", data_tostring(entry), data_typename(entry));
}

/*
void _stack_counters_visitor(counter_t *counter) {
  printf("   . %-6.6d", counter -> count);
}

void _stack_bookmarks_visitor(bookmark_t *bookmark) {
  printf("   . %-6.6d", bookmark -> depth);
}
*/

static void _debug_list_stack(debugger_t *debugger) {
  array_visit(debugger -> vm -> stack -> list, (visit_t) _stack_list_visitor);
  
  /*
  if (array_size(stack -> counters) > 0) {
    _debug("---- Counters ---------------------------------------------");
    array_visit(stack -> counters, (visit_t) _stack_counters_visitor);
  }
  if (array_size(stack -> bookmarks) > 0) {
    _debug("---- Bookmarks ---------------------------------------------");
    array_visit(stack -> counters, (visit_t) _stack_bookmarks_visitor);
  }
  */
  printf("\n");
}

static void _debug_print_stacktrace(debugger_t *debugger) {
  stacktrace_t *stacktrace = stacktrace_create();
  
  printf("%s\n", stacktrace_tostring(stacktrace));
}

static void _debug_list(debugger_t *debugger, instruction_t *instr) {
  bytecode_list_and_mark(debugger -> bytecode, instr);
  printf("\n");
}

static void _debug_print_var(debugger_t *debugger, char *cmd) {
  char   *name = strchr(cmd, ' ');
  name_t *path;
  data_t *variable;
  
  if (!name) {
    printf("Error: print <variable>\n");
    return;
  }
  
  path = name_parse(strtrim(name));
  if (path && name_size(path)) {
    variable = data_get(debugger -> scope, path);
    if (variable) {
      printf("%s (%s) = %s\n", name_tostring(path), data_typename(variable), data_tostring(variable));
    } else {
      printf("Error: unknown variable '%s'\n", name_tostring(path));
    }  
  }
}

/* ----------------------------------------------------------------------- */

extern debugger_t * debugger_create(vm_t *vm, data_t *scope) {
  debugger_t *ret = NEW(debugger_t);
  
  ret -> scope = scope;
  ret -> vm = vm;
  ret -> bytecode = vm -> bytecode;
  ret -> status = DebugStatusRun;
  return ret;
}

extern void debugger_start(debugger_t *debugger) {
  if (debugger -> status == DebugStatusSingleStep) {
    printf("Starting '%s'\n", vm_tostring(debugger -> vm));
  }
}

extern debugcmd_t debugger_step_before(debugger_t *debugger, instruction_t *instr) {
  char       *line;
  char       *trimmed;
  debugcmd_t  ret = DebugCmdNone;
  
  if (debugger -> status != DebugStatusSingleStep) {
    return DebugCmdGo;
  }
  
  assert(debugger -> status == DebugStatusSingleStep);
  printf("%s\n", instruction_tostring(instr));
  do {
    line = _debug_readstring("# ");
    if (line) {
      trimmed = strtrim(line);
      switch (*trimmed) {
        case 'T':
        case 't':
          _debug_list_stack(debugger);
          break;
        case 'C':
        case 'c':
          debugger -> status = DebugStatusRunOut;
          // fall through
        case 'S':
        case 's':
          ret = DebugCmdGo;
          break;
        case 'F':
        case 'f':
          _debug_print_stacktrace(debugger);
          break;
        case 'L':
        case 'l':
          _debug_list(debugger, instr);
          break;
        case 'P':
        case 'p':
          _debug_print_var(debugger, trimmed);
          break;
        case 'Q':
        case 'q':
          ret = DebugCmdHalt;
          break;
      }
      free(line);
    }
  } while (ret == DebugCmdNone);
  return ret;
}

extern void debugger_step_after(debugger_t *debugger, instruction_t * instr, data_t *ret) {
  if ((debugger -> status == DebugStatusSingleStep) && ret && (ret != data_null())) {
    printf("  -> %s (%s)\n", data_tostring(ret), data_typename(ret));
  }
}

extern void debugger_exit(debugger_t *debugger, data_t *ret) {
  if (debugger -> status == DebugStatusSingleStep) {
    printf("  '%s' returns %s (%s)\n", 
           vm_tostring(debugger -> vm), 
           data_tostring(ret),
           data_typename(ret));
  }
}

extern void debugger_free(debugger_t *debugger) {
  free(debugger);
}
