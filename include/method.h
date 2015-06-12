/*
 * method. - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __METHOD_H__
#define	__METHOD_H__

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct _method {
  data_t         _d;
  methoddescr_t *method;
  data_t        *self;
} mth_t;

extern mth_t *       mth_create(methoddescr_t *, data_t *);
extern mth_t *       mth_copy(mth_t *);
extern data_t *      mth_call(mth_t *, array_t *, dict_t *);
extern unsigned int  mth_hash(mth_t *);
extern int           mth_cmp(mth_t *, mth_t *);

extern int Method;

#define data_is_method(d)   ((d) && (data_hastype((d), method)))
#define data_as_method(d)   ((method_t *) (data_is_method((d)) ? (d) : NULL))
#define method_free(o)      (data_free((data_t *) (o)))
#define method_tostring(o)  (data_tostring((data_t *) (o)))
#define method_copy(o)      ((data_t *) data_copy((data_t *) (o)))

#ifdef	__cplusplus
}
#endif

#endif /* __METHOD_H__ */

