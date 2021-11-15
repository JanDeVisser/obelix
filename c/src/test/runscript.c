/*
 * src/bin/test/runscript.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
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

#include <data.h>
#include <loader.h>
#include <name.h>
#include <namespace.h>
#include <object.h>
#include <testsuite.h>

extern data_t * run_script(char *);

#define SYSPATH    "../../../share/"
#define USERPATH   "./"

data_t * run_script(char *script) {
  scriptloader_t *loader;
  name_t         *path;
  name_t         *name;
  data_t         *data;
  object_t       *obj;
  module_t       *mod;
  
  name = name_create(1, script);
  path = name_create(1, USERPATH);
  loader = scriptloader_create(SYSPATH, path, NULL);
  data = scriptloader_run(loader, name, NULL, NULL);
  scriptloader_free(loader);
  name_free(path);
  name_free(name);
  return data;
}

