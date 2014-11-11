/*
 * /obelix/src/stringbuffer.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <string.h>

#include <core.h>
#include <stringbuffer.h>

stringbuffer_t * sb_create(char *buffer) {
  stringbuffer_t *ret;

  ret = NEW(stringbuffer_t);
  if (ret) {
    ret -> read_fnc = (read_t) sb_read;
    ret -> buffer = buffer;
    ret -> pos = 0;
    ret -> len = strlen(buffer);
  }
  return ret;
}

void sb_free(stringbuffer_t *sb) {
  if (sb) {
    free(sb);
  }
}

int sb_read(stringbuffer_t *sb, char *target, int num) {
  if ((sb -> pos + num) > sb -> len) {
    num = sb -> len - sb -> pos;
  }
  strncpy(target, sb -> buffer + sb -> pos, num);
  sb -> pos = sb -> pos + num;
  return num;
}
