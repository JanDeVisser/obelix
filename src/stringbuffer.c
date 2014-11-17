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

static stringbuffer_t * _sb_expand(stringbuffer_t *, int);

stringbuffer_t * _sb_expand(stringbuffer_t *sb, int targetlen) {
  int newsize;
  char *oldbuf;
  stringbuffer_t *ret = sb;

  if (sb -> bufsize < (targetlen + 1)) {
    for (newsize = sb -> bufsize * 2; newsize < (targetlen + 1); newsize *= 2);
    oldbuf = sb -> buffer;
    sb -> buffer = realloc(sb -> buffer, newsize);
    if (sb -> buffer) {
      memset(sb -> buffer + sb -> len, 0, sb -> newsize - sb -> len);
      sb -> bufsize = newsize;
    } else {
      sb -> buffer = oldbuf;
      ret = NULL;
    }
  }
  return ret;
}

stringbuffer_t * sb_create(char *buffer) {
  stringbuffer_t *ret;

  ret = NEW(stringbuffer_t);
  if (ret) {
    ret -> read_fnc = (read_t) sb_read;
    ret -> buffer = buffer;
    ret -> pos = 0;
    ret -> len = strlen(buffer);
    ret -> bufsize = 0;
  }
  return ret;
}

stringbuffer_t * sb_copy_str(char *buffer) {
  stringbuffer_t *ret = NULL;
  char *b = strdup(buffer);

  if (b) {
    ret = sb_create(b);
    ret -> bufsize = strlen(b) + 1;
  }
  return ret;
}

stringbuffer_t * sb_copy(stringbuffer_t *sb) {
  stringbuffer_t *ret = NULL;
  char *b = strdup(sb_str(sb));

  if (b) {
    ret = sb_create(b);
    ret -> bufsize = sb_len(sb);
  }
  return ret;
}

stringbuffer_t * sb_init(int size) {
  stringbuffer_t *ret = NULL;
  char *b = (char *) malloc(size);

  if (b) {
    memset(b, 0, size);
    ret = sb_create(b);
    ret -> bufsize = size;
  }
  return ret;
}

void sb_free(stringbuffer_t *sb) {
  if (sb) {
    if (sb -> bufsize) {
      free(sb -> buffer);
    }
    free(sb);
  }
}

int sb_len(stringbuffer_t *sb) {
  return sb -> len;
}

char * sb_str(stringbuffer_t *sb) {
  return sb -> buffer;
}

int sb_read(stringbuffer_t *sb, char *target, int num) {
  if ((sb -> pos + num) > sb -> len) {
    num = sb -> len - sb -> pos;
  }
  strncpy(target, sb -> buffer + sb -> pos, num);
  sb -> pos = sb -> pos + num;
  return num;
}

stringbuffer_t * sb_append_char(stringbuffer_t *sb, int ch) {
  stringbuffer_t *ret = NULL;

  if (sb -> bufsize && (ch > 0)) {
    if (_sb_expand(sb, sb -> len + 1)) {
      sb -> buffer[sb -> len++] = ch;
      ret = sb;
    }
  }
  return ret;
}

stringbuffer_t * sb_append_str(stringbuffer_t *sb, char *other) {
  stringbuffer_t *ret = NULL;

  if (sb -> bufsize) {
    if (_sb_expand(sb, sb_len(sb) + strlen(other) + 1)) {
      strcat(sb -> buffer, other);
      ret = sb;
    }
  }
  return ret;
}

stringbuffer_t * sb_append(stringbuffer_t *sb, stringbuffer_t *other) {
  stringbuffer_t *ret = NULL;

  if (sb -> bufsize) {
    if (_sb_expand(sb, sb_len(sb) + sb_len(other) + 1)) {
      strcat(sb -> buffer, sb_str(other));
      ret = sb;
    }
  }
  return ret;
}

stringbuffer_t * sb_chop(stringbuffer_t *sb, int num) {
  stringbuffer_t *ret = NULL;

  if (sb -> bufsize) {
    for (; sb -> len && num; num--) {
      sb -> len--;
      sb -> buffer[sb -> len] = 0;
    }
    ret = sb;
  }
  return ret;
}

stringbuffer_t * sb_erase(stringbuffer_t *sb) {
  stringbuffer_t *ret = NULL;

  if (sb -> bufsize) {
    memset(sb -> buffer, 0, sb -> bufsize);
    sb -> len = 0;
    ret = sb;
  }
  return ret;
}

