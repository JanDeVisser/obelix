/*
 * /obelix/include/native.h - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#ifndef __NATIVE_H__
#define __NATIVE_H__

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef data_t * (*native_t)(char *, array_t *, dict_t *);

typedef struct _native_fnc {
  data_t    _d;
  name_t   *name;
  int       async;
  native_t  native_method;
  array_t  *params;
} native_fnc_t;

/* -- N A T I V E  F N C  P R O T O T Y P E S ----------------------------- */

extern native_fnc_t *   native_fnc_create(char *name, native_t);
extern native_fnc_t *   native_fnc_copy(native_fnc_t *);
extern data_t *         native_fnc_execute(native_fnc_t *, array_t *, dict_t *);
extern int              native_fnc_cmp(native_fnc_t *, native_fnc_t *);
extern char *           native_fnc_tostring(native_fnc_t *);

#define data_is_native(d)      ((d) && data_hastype((d), Native))
#define data_nativeval(d)      (data_is_native((d)) ? ((native_fnc_t *) (d)) : NULL)
#define native_fnc_copy(f)     ((native_fnc_t *) data_copy((data_t *) (f)))
#define native_fnc_free(f)     (data_free((data_t *) (f)))

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __NATIVE_H__ */
