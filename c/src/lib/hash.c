/*
 * core.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

 /*
  * A super fast hash, by Paul Hsieh.
  * http://www.azillionmonkeys.com/qed/hash.html
  *
  * Licensed under the LPGL 2.1
  */

#include "libcore.h"
#ifndef HAVE_STDINT_H
#include <pstdint.h>
#endif /* !HAVE_STDINT_H */

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

#define SuperFastHash hash

unsigned int SuperFastHash(const void *voiddata, size_t len) {
  char     *data = (char *) voiddata;
  uint32_t  hash = len, tmp;
  int       rem;

  if (len <= 0 || !data) {
    return 0;
  }

  rem = len & 3;
  len >>= 2;

  /* Main loop */
  for (;len > 0; len--) {
    hash  += get16bits(data);
    tmp    = (get16bits(data + 2) << 11) ^ hash;
    hash   = (hash << 16) ^ tmp;
    data  += 2 * sizeof(uint16_t);
    hash  += hash >> 11;
  }

  /* Handle end cases */
  switch (rem) {
    case 3:
      hash += get16bits(data);
      hash ^= hash << 16;
      hash ^= ((signed char) data[sizeof(uint16_t)]) << 18;
      hash += hash >> 11;
      break;
    case 2:
      hash += get16bits(data);
      hash ^= hash << 11;
      hash += hash >> 17;
      break;
    case 1:
      hash += (signed char) *data;
      hash ^= hash << 10;
      hash += hash >> 1;
      break;
  }

  /* Force "avalanching" of final 127 bits */
  hash ^= hash << 3;
  hash += hash >> 5;
  hash ^= hash << 4;
  hash += hash >> 17;
  hash ^= hash << 25;
  hash += hash >> 6;

  return (unsigned int) hash;
}

unsigned int OldSlowHash(const void *buf, size_t size) {
  int            hash = 5381;
  size_t         i;
  int            c;
  unsigned char *data = (unsigned char *) buf;

  for (i = 0; i < size; i++) {
    c = *data++;
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return hash;
}

unsigned int hashptr(const void *ptr) {
  return hash(&ptr, sizeof(void *));
}

unsigned int hashlong(long val) {
  // return hash(&val, sizeof(long));
  return (unsigned int) val;
}

unsigned int hashdouble(double val) {
  return hash(&val, sizeof(double));
}

unsigned int hashblend(unsigned int h1, unsigned int h2) {
  return (h1 << 1) + h1 + h2;
}
