/*
 * /obelix/include/nvp.h - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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


#ifndef __NVP_H__
#define __NVP_H__

#include <data.h>

typedef struct _nvp {
  data_t *name;
  data_t *value;
  int     refs;
  char   *str;
} nvp_t;

#define data_is_nvp(d) ((d) && (data_type((d)) == NVP))
#define data_nvpval(d) ((nvp_t *) (data_is_nvp((d)) ? ((d) -> ptrval) : NULL))

extern nvp_t *      nvp_create(data_t *, data_t *);
extern void         nvp_free(nvp_t *);
extern nvp_t *      nvp_copy(nvp_t *);
extern int          nvp_cmp(nvp_t *, nvp_t *);
extern unsigned int nvp_hash(nvp_t *);
extern char *       nvp_tostring(nvp_t *);

#endif /* __NVP_H__ */
