/*
 * /obelix/src/property.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include "libvm.h"

#include <function.h>
#include <mutex.h>

static inline void _property_init(void);

static property_t * _property_new(property_t *, va_list);
static void _property_free(property_t *);

/* -- data_t type description structures ---------------------------------- */

static vtable_t _vtable_Property[] = {
    { .id = FunctionNew, .fnc = (void_t) _property_new },
    { .id = FunctionFree, .fnc = (void_t) _property_free },
    { .id = FunctionNone, .fnc = NULL }
};

int Property = -1;
int property_debug = 0;

/* ------------------------------------------------------------------------ */

static typedescr_t * _validator_load_nolock(char *, char *);

static data_t *      _validator_required(property_t *, data_t *);
static data_t *      _validator_discard(property_t *, data_t *);

int Validator = -1;
int RequiredValidator = -1;
int TransientValidator = -1;

static dict_t *  _validators;
static mutex_t * _validators_mutex;

static vtable_t _vtable_Validator[] = {
        { .id = FunctionNone,    .fnc = NULL }
};

static vtable_t _vtable_RequiredValidator[] = {
    { .id = FunctionAssign,  .fnc = (void_t) _validator_required },
    { .id = FunctionPersist, .fnc = (void_t) _validator_required },
    { .id = FunctionNone,    .fnc = NULL }
};

static vtable_t _vtable_TransientValidator[] = {
    { .id = FunctionPersist,  .fnc = (void_t) _validator_discard },
    { .id = FunctionRetrieve, .fnc = (void_t) _validator_discard },
    { .id = FunctionNone,     .fnc = NULL }
};

void _property_init(void) {
  logging_register_module(property);


  typedescr_register(Validator, validator_t);
  _validators = dict_create(NULL);
  dict_set_key_type(_validators, &type_str);
  dict_set_data_type(_validators, &type_int);
  _validators_mutex = mutex_create();

  typedescr_register(Property, property_t);
  typedescr_register_with_name(RequiredValidator, "required", validator_t);
  typedescr_register_with_name(TransientValidator, "transient", validator_t);
  validator_register(default_value_register());
  validator_register(transient_register());
  validator_register(private_register());
  validator_register(key_register());
  validator_register(choices_register());
}

/* -- V A L I D A T O R S ------------------------------------------------- */

typedescr_t * validator_register(typedescr_t *def) {
  _property_init();
  mutex_lock(_validators_mutex);
  typedescr_assign_inheritance(typetype(def), Validator);
  debug(property, "Registering validator type '%s' (%d)", typename(def), typetype(def));
  dict_put(_validators, strdup(typename(def)), (void *) ((intptr_t) typetype(def)));
  mutex_unlock(_validators_mutex);
  return def;
}

typedescr_t * _validator_load_nolock(char *code, char *regfnc_name) {
  function_t  *fnc;
  char        *fncname;
  typedescr_t *ret = NULL;
  create_t     regfnc;

  if (regfnc_name) {
    fncname = regfnc_name;
  } else {
    asprintf(&fncname, "%s_register", code);
  }
  debug(property, "Loading validator definition '%s'. regfnc '%s'", code, fncname);
  fnc = function_create(fncname, NULL);
  if (fnc -> fnc) {
    regfnc = (create_t) fnc -> fnc;
    ret = regfnc();
    debug(property, "Validator definition '%s' has type %d", code, typetype(ret));
    validator_register(ret);
  } else {
    error("Registration function '%s' for validator type '%s' cannot be resolved",
        fncname, code);
  }
  function_free(fnc);
  if (fncname != regfnc_name) {
    free(fncname);
  }
  return ret;
}

typedescr_t * validator_load(char *code, char *regfnc_name) {
  typedescr_t *ret = NULL;

  _property_init();
  mutex_lock(_validators_mutex);
  ret = dict_has_key(_validators, code)
        ? validator_get(code)
        : _validator_load_nolock(code, regfnc_name);
  mutex_unlock(_validators_mutex);
  return ret;
}

typedescr_t * validator_get(char *code) {
  typedescr_t *ret;
  int          type;

  _property_init();
  mutex_lock(_validators_mutex);
  if (dict_has_key(_validators, code)) {
    type = (int) (intptr_t) dict_get(_validators, code);
    ret = typedescr_get(type);
  } else {
    ret = _validator_load_nolock(code, NULL);
  }
  mutex_unlock(_validators_mutex);
  return ret;
}

validator_t * validator_create(char *code, property_t *prop) {
  typedescr_t *type;
  validator_t *ret = NULL;

  _property_init();
  type = validator_get(code);
  if (type) {
    debug(property, "Creating validator. code: '%s', type: %d", code, typetype(type));
    ret = (validator_t *) data_create(typetype(type), prop);
  } else {
    error("Attempt to create scanner with unregistered code '%s'", code);
  }
  return ret;
}

data_t * _validator_required(property_t *prop, data_t *value) {
  return (data_notnull(value))
         ? value
         : exception_create(ErrorType,
          "Property '%s' is required", data_tostring(prop));
}

data_t * _validator_discard(property_t *prop, data_t *value) {
  return NULL;
}

/* -- P R O P E R T Y  S T A T I C  F U N C T I O N S  -------------------- */

property_t * _property_new(property_t * property, va_list args) {
  char * name = va_arg(args, char *);
  data_t * class = va_arg(args, data_t *);

  property->_d.str = strdup(name);
  property->_d.free_str = DontFreeData;
  property->class = data_copy(class);
  property->validators = datalist_create(NULL);
  property->key = FALSE;
  property->transient = FALSE;
  property->private = FALSE;
  return property;
}

void _property_free(property_t * property) {
  if (property) {
    data_free(property->class);
  }
}

/* -- P R O P E R T Y  P U B L I C  F U N C T I O N S  -------------------- */

property_t * property_create_of_type(char * name, int type) {
  return property_create_of_class(name, (data_t *) typedescr_get(type));
}

property_t * property_create_of_class(char * name, data_t * class) {
  return (property_t *) data_create(Property, name, class);
}

data_t * property_assign(property_t * prop, data_t * value) {
  data_t * ret = value;

  if (value) {
    if (data_is_typedescr(prop->class)) {
      ret = data_cast(value, typetype(data_as_typedescr(prop->class));
    } else if (data_is_script(prop->class)) {
      if (!script_isa(data_as_script(prop->class), value)) {
        ret = NULL;
      }
    }
    if (!ret) {
      ret = (data_t *) exception_create(ErrorType,
          "Invalid value '%s' for property '%s'",
          data_tostring(value), data_tostring(prop));
    }
  }
  return ret;
}
