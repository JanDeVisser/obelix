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
  data_t  _d;
  data_t *name;
  data_t *value;
} nvp_t;

extern void                 nvp_init(void);
extern nvp_t *      nvp_create(data_t *, data_t *);
extern nvp_t *      nvp_parse(char *);
extern int          nvp_cmp(nvp_t *, nvp_t *);
extern unsigned int nvp_hash(nvp_t *);

extern int NVP;

#define data_is_nvp(d)   ((d) && (data_hastype((d), NVP)))
#define data_as_nvp(d)   ((nvp_t *) (data_is_nvp((d)) ? (d) : NULL))
#define nvp_free(o)      (data_free((data_t *) (o)))
#define nvp_tostring(o)  (data_tostring((data_t *) (o)))
#define nvp_copy(o)      ((data_t *) data_copy((data_t *) (o)))

#endif /* __NVP_H__ */
