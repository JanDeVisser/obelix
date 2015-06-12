/*
 * /obelix/include/fsentry.h - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#ifndef __FSENTRY_H__
#define __FSENTRY_H__

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _fsentry {
  data_t       _d;
  char        *name;
  struct stat  statbuf;
  int          exists;
} fsentry_t;

extern fsentry_t *  fsentry_create(char *);
extern fsentry_t *  fsentry_getentry(fsentry_t *, char *name);
extern unsigned int fsentry_hash(fsentry_t *);
extern int          fsentry_cmp(fsentry_t *, fsentry_t *);
extern int          fsentry_exists(fsentry_t *);
extern int          fsentry_isfile(fsentry_t *);
extern int          fsentry_isdir(fsentry_t *);
extern int          fsentry_canread(fsentry_t *);
extern int          fsentry_canwrite(fsentry_t *);
extern int          fsentry_canexecute(fsentry_t *);
extern list_t *     fsentry_getentries(fsentry_t *);
extern file_t *     fsentry_open(fsentry_t *);
  
extern int          FSEntry;

#define data_is_fsentry(d)  ((d) && (data_hastype((d), FSEntry)))
#define data_as_fsentry(d)  ((fsentry_t *) (data_is_fsentry((d)) ? ((fsentry_t *) (d)) : NULL))
#define fsentry_free(f)     (data_free((data_t *) (f)))
#define fsentry_tostring(f) (data_tostring((data_t *) (f)))
#define fsentry_copy(f)     ((fsentry_t *) data_copy((data_t *) (f)))

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __FSENTRY_H__ */
