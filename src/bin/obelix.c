/*
 * obelix.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
 * 
 * This file is part of Obelix.
 *
 * Obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <data.h>
#include <exception.h>
#include <file.h>
#include <lexer.h>
#include <loader.h>
#include <logging.h>
#include <name.h>
#include <namespace.h>
#include <resolve.h>
#include <socket.h>
#include <script.h>

typedef struct _cmdline_args {
  int    argc;
  char **argv;
  char  *grammar;
  char  *debug;
  char  *basepath;
  char  *syspath;
  int    list;
  int    trace;
  int    server;
} cmdline_args_t;

void debug_settings(cmdline_args_t *args) {
  int      debug_all = 0;
  array_t *cats;
  int      ix;
  
  logging_reset();
  if (args -> debug) {
    debug("debug optarg: %s", args -> debug);
    cats = array_split(args -> debug, ",");
    for (ix = 0; ix < array_size(cats); ix++) {
      logging_enable(str_array_get(cats, ix));
    }
  }
}

int run_script(scriptloader_t *loader, name_t *name, array_t *argv) {
  data_t      *ret;
  object_t    *obj;
  module_t    *mod;
  int          retval;
  exception_t *ex;
  int          type;
  
  ret = scriptloader_run(loader, name, argv, NULL);
  if (script_debug) {
    debug("Exiting with exit code %s [%s]", data_tostring(ret), data_typename(ret));
  }
  if (ex = data_as_exception(ret)) {
    if (ex -> code != ErrorExit) {
      error("Error: %s", data_tostring(ret));
      retval = 0 - (int) ex -> code;
    } else {
      retval = data_intval(ex -> throwable);
    }
  } else {
    retval = data_intval(ret);
  }
  data_free(ret);
  return retval;  
}

name_t * build_name(char *scriptname) {
  char   *buf;
  char   *ptr;
  name_t *name;
  
  buf = strdup(scriptname);
  while (ptr = strchr(buf, '/')) {
    *ptr = '.';
  }
  if (ptr = strrchr(buf, '.')) {
    if (!strcmp(ptr, ".obl")) {
      *ptr = 0;
    }
  }
  name = name_split(buf, ".");
  free(buf);
  return name;
}

scriptloader_t * _create_loader(cmdline_args_t *args) {
  array_t        *path;
  scriptloader_t *loader;
  
  path = (args -> basepath) ? array_split(args -> basepath, ":") 
                            : str_array_create(0);
  array_push(path, getcwd(NULL, 0));
  
  loader = scriptloader_create(args -> syspath, path, args -> grammar);
  if (loader) {
    scriptloader_set_option(loader, ObelixOptionList, args -> list);
    scriptloader_set_option(loader, ObelixOptionTrace, args -> trace);
  }
  array_free(path);
  return loader;
}

int run_from_cmdline(cmdline_args_t *args) {
  name_t         *name;
  array_t        *obl_argv;
  int             ix;
  scriptloader_t *loader;
  int             retval = 0;
  
  loader = _create_loader(args);
  if (loader) {
    scriptloader_set_option(loader, ObelixOptionList, args -> list);
    scriptloader_set_option(loader, ObelixOptionTrace, args -> trace);
    
    name = build_name(args -> argv[optind]);
    obl_argv = str_array_create(args -> argc - optind);
    for (ix = optind + 1; ix < args -> argc; ix++) {
      array_push(obl_argv, data_create(String, args -> argv[ix]));
    }
    retval = run_script(loader, name, obl_argv);
    array_free(obl_argv);
    name_free(name);
    scriptloader_free(loader);
  }
  return retval;
}

data_t * _run_script(scriptloader_t *loader, cmdline_args_t *args, char *cmd, socket_t *client) {
  array_t     *line = array_split(cmd, " ");
  char        *script;
  name_t      *name;
  int          ix;
  array_t     *obl_argv;
  data_t      *real_ret;
  data_t      *ret;
  exception_t *ex;
  
  if (array_size(line)) {
    script = str_array_get(line, 0);
    name = build_name(script);
    obl_argv = array_slice(line, 1, array_size(line));
    
    ret = scriptloader_run(loader, name, obl_argv, NULL);
    real_ret = ret;
    if (script_debug) {
      debug("Exiting with exit code %s [%s]", data_tostring(ret), data_typename(ret));
    }
    if ((ex = data_as_exception(ret)) && (ex -> code == ErrorExit)) {
      data_free(ret);
      ret = ex -> throwable;
    }
    file_printf(client -> sockfile, "[%s] %s\n", data_typename(ret), data_tostring(ret));
    data_free(real_ret);
  }
  array_free(line);
}

void * _connection_handler(connection_t *connection) {
  cmdline_args_t *args = (cmdline_args_t *) connection -> context;
  socket_t       *client = (socket_t *) connection -> client;
  array_t        *path;
  name_t         *name;
  scriptloader_t *loader;
  char           *cmd;
  
  loader = _create_loader(args);
  if (loader) {
    file_printf(client -> sockfile, "Obelix 0,1\n");
    do {
      file_printf(client -> sockfile, "READY\n");
      cmd = file_readline(client -> sockfile);
      if (!strncmp(cmd, "RUN ", 4)) {
        _run_script(loader, args, cmd + 4, client);
      }
    } while (0);
  }
  return connection;
}

int start_server(cmdline_args_t *args) {
  socket_t *server;
  
  server = serversocket_create(args -> server);
  socket_listen(server, _connection_handler, args);
  return 0;
}

int main(int argc, char **argv) {
  cmdline_args_t  args;
  int             opt;

  memset(&args, 0, sizeof(cmdline_args_t));
  args.argc = argc;
  args.argv = argv;
  while ((opt = getopt(argc, argv, "s:g:d:p:v:ltS")) != -1) {
    switch (opt) {
      case 's':
        args.syspath = optarg;
        break;
      case 'g':
        args.grammar = optarg;
        break;
      case 'd':
        args.debug = optarg;
        break;
      case 'p':
        args.basepath = optarg;
        break;
      case 'v':
        log_level = atoi(optarg);
        break;
      case 'l':
        args.list = 1;
        break;
      case 't':
        args.trace = 1;
        args.list = 1;
        break;
      case 'S':
        args.server = 14400;
        break;
    }
  }
  if (argc == optind && !args.server) {
    fprintf(stderr, "Usage: obelix <filename> [options ...]\n");
    exit(1);
  }
  debug_settings(&args);
  
  if (args.server) {
    start_server(&args);
  } else {
    return run_from_cmdline(&args);
  }
}

