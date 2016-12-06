/*
 * /obelix/test/resolve.c - bufright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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
 * You should have received a buf of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <resolve.h>

typedef void *   (*helloworld_t)(char *);

#define OBL_DIR_EQ        OBL_DIR "="
#define OBL_DIR_EQ_LEN    (strlen(OBL_DIR_EQ))

static void set_obldir(char *argv0) {
#ifdef HAVE_PUTENV
  char *ptr;
  char  buf[MAX_PATH + OBL_DIR_EQ_LEN + 1];
  char *path = buf + OBL_DIR_EQ_LEN;

  strcpy(buf, OBL_DIR_EQ);
  strncpy(path, argv0, MAX_PATH);
  path[MAX_PATH] = 0;
  ptr = strrchr(path, '/');
  if (!ptr) {
    ptr = strrchr(path, '\\');
  }
  if (ptr) {
    *ptr = 0;
    putenv(buf);
  }
#endif /* HAVE_PUTENV */
}

int main(int argc, char *argv[]) {
  helloworld_t  hw;
  void         *test;

  set_obldir(argv[0]);
  resolve_library("libtestlib.so");
  hw = (helloworld_t) resolve_function("testlib_helloworld");
  test = hw("test");
  printf("-> '%s'\n", (char *) test);
  return 0;
}
