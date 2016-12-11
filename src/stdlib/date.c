/*
 * /obelix/src/stdlib/date.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

#include <stdio.h>
#include <time.h>

#include <data.h>
#include <exception.h>
#include <str.h>

typedef enum _datetime_flag {
  DatetimeFlagCopy = -3,
  DatetimeFlagDateTime = -2,
  DatetimeFlagTimeT = -1,
  DatetimeFlagParse = 0,
  DatetimeFlagHM = 2,
  DatetimeFlagHMS = 3,
  DatetimeFlagYMD = 4,
} datetime_flag_t;

typedef struct _datetime {
  data_t     _d;
  time_t     dt;
  int        tm_set;
  struct tm  tm;
} datetime_t;

__DLL_EXPORT__ data_t * time_create(int, int, int);
__DLL_EXPORT__ data_t * date_create(int, int, int);
__DLL_EXPORT__ data_t * datetime_create(datetime_t *, datetime_t *);

static void             _date_init(void);
__DLL_EXPORT__ data_t * _function_date_init(char *, array_t *, dict_t *);

static datetime_t * _timebase_new(datetime_t *, va_list);
static data_t *     _timebase_resolve(datetime_t *, char *);
static int          _timebase_intval(datetime_t *);
static data_t *     _timebase_cast(datetime_t *, int);
static struct tm *  _timebase_tm(datetime_t *);
static datetime_t * _timebase_assign(datetime_t *);

static vtable_t _vtable_Timebase[] = {
  { .id = FunctionNew,         .fnc = (void_t) _timebase_new },
  { .id = FunctionResolve,     .fnc = (void_t) _timebase_resolve },
  { .id = FunctionIntValue,    .fnc = (void_t) _timebase_intval },
  { .id = FunctionCast,        .fnc = (void_t) _timebase_cast },
  { .id = FunctionNone,        .fnc = NULL }
};

static data_t *         _timeofday_new(datetime_t *, va_list);
static char *           _timeofday_tostring(datetime_t *);
static data_t *         _timeofday_parse(char *);
static data_t *         _timeofday_resolve(datetime_t *, char *);
static data_t *         _timeofday_set(datetime_t *, char *, data_t *);
static data_t *         _function_time(data_t *, char *, array_t *, dict_t *);

static vtable_t _vtable_Time[] = {
  { .id = FunctionNew,         .fnc = (void_t) _timeofday_new },
  { .id = FunctionAllocString, .fnc = (void_t) _timeofday_tostring },
  { .id = FunctionParse,       .fnc = (void_t) _timeofday_parse },
  { .id = FunctionResolve,     .fnc = (void_t) _timeofday_resolve },
  { .id = FunctionSet,         .fnc = (void_t) _timeofday_set },
  { .id = FunctionNone,        .fnc = NULL }
};

static methoddescr_t _methods_Time[] = {
  { .type = Any,    .name = "time",      .method = (method_t) _function_time,  .argtypes = { Any, Int, Int },           .minargs = 0, .varargs = 1, .maxargs = 3 },
  { .type = Any,    .name = "timeofday", .method = (method_t) _function_time,  .argtypes = { Any, Any, Any },           .minargs = 0, .varargs = 0, .maxargs = 0 },
  { .type = NoType, .name = NULL,        .method = NULL,                       .argtypes = { NoType, NoType, NoType },  .minargs = 0, .varargs = 0 }
};

static data_t *     _date_new(datetime_t *, va_list);
static char *       _date_tostring(datetime_t *);
static data_t *     _date_parse(char *);
static data_t *     _date_resolve(datetime_t *, char *);
static data_t *     _date_set(datetime_t *, char *, data_t *);

static data_t *     _function_date(data_t *, char *, array_t *, dict_t *);

static vtable_t _vtable_Date[] = {
  { .id = FunctionNew,      .fnc = (void_t) _date_new },
  { .id = FunctionToString, .fnc = (void_t) _date_tostring },
  { .id = FunctionParse,    .fnc = (void_t) _date_parse },
  { .id = FunctionResolve,  .fnc = (void_t) _date_resolve },
  { .id = FunctionSet,      .fnc = (void_t) _date_set },
  { .id = FunctionNone,     .fnc = NULL }
};

static methoddescr_t _methods_Date[] = {
  { .type = Any,    .name = "date",  .method = (method_t) _function_date,  .argtypes = { Any, Int, Int },          .minargs = 0, .varargs = 1, .maxargs = 3 },
  { .type = Any,    .name = "today", .method = (method_t) _function_date,  .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0, .maxargs = 0 },
  { .type = NoType, .name = NULL,    .method = NULL,                       .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

static data_t *     _datetime_new(datetime_t *, va_list);
static char *       _datetime_tostring(datetime_t *);
static data_t *     _datetime_parse(char *);

static data_t *     _function_datetime(data_t *, char *, array_t *, dict_t *);

static vtable_t _vtable_Datetime[] = {
  { .id = FunctionNew,      .fnc = (void_t) _datetime_new },
  { .id = FunctionToString, .fnc = (void_t) _datetime_tostring },
  { .id = FunctionParse,    .fnc = (void_t) _datetime_parse },
  { .id = FunctionNone,     .fnc = NULL }
};

static methoddescr_t _methods_Datetime[] = {
  { .type = Any,    .name = "datetime", .method = (method_t) _function_datetime, .argtypes = { Any, Any, Int },          .minargs = 0, .varargs = 1, .maxargs = 6 },
  { .type = Any,    .name = "now",      .method = (method_t) _function_datetime, .argtypes = { Any, Any, Any },          .minargs = 0, .varargs = 0, .maxargs = 0 },
  { .type = NoType, .name = NULL,       .method = NULL,                          .argtypes = { NoType, NoType, NoType }, .minargs = 0, .varargs = 0 }
};

int Timebase = -1;
int Datetime = -1;
int Date = -1;
int Time = -1;
int date_debug = 0;

/* ------------------------------------------------------------------------ */

void _date_init(void) {
  if (Datetime < 1) {
    logging_register_module(date);
    typedescr_register(Timebase, datetime_t);
    typedescr_register_with_methods(Time, datetime_t);
    typedescr_assign_inheritance(Time, Timebase);
    typedescr_register_with_methods(Date, datetime_t);
    typedescr_assign_inheritance(Date, Timebase);
    typedescr_register_with_methods(Datetime, datetime_t);
    typedescr_assign_inheritance(Datetime, Time);
    typedescr_assign_inheritance(Datetime, Date);
  }
  assert(Datetime);
}

/* -- T I M E B A S E ------------------------------------------------ */

datetime_t * _timebase_new(datetime_t *timebase, va_list args) {
  int         flag = va_arg(args, int);
  datetime_t *datetime;

  timebase -> dt = (time_t) 0;
  timebase -> tm_set = 0;
  memset(&timebase -> tm, 0, sizeof(struct tm));
  switch (flag) {
    case DatetimeFlagCopy:
      datetime = va_arg(args, datetime_t *);
      timebase -> dt = datetime -> dt;
      break;
    case DatetimeFlagTimeT:
      timebase -> dt = va_arg(args, time_t);
      break;
  }
  if (flag == DatetimeFlagTimeT) {
  }
  return timebase;
}

data_t * _timebase_resolve(datetime_t *timebase, char *name) {
  data_t    *ret = NULL;

  if (!timebase -> tm_set) {
    if (!gmtime_r(&timebase -> dt, &timebase -> tm)) {
      return data_exception(ErrorInternalError, "Could not convert time");
    }
    timebase -> tm_set = 1;
  }
  if (!strcmp(name, "seconds_since_epoch")) {
    ret = int_to_data((long) timebase -> dt);
  }
  return ret;
}

int _timebase_intval(datetime_t *datetime) {
  return (int) datetime -> dt;
}

data_t * _timebase_cast(datetime_t *timebase, int totype) {
  if (totype == Int) {
    return int_to_data(timebase -> dt);
  } else if (totype == Bool) {
    return data_false();
  } if (typedescr_is(typedescr_get(totype), Timebase)) {
    return data_create(totype, DatetimeFlagCopy, timebase);
  } else {
    return NULL;
  }
}

struct tm * _timebase_tm(datetime_t *timebase) {
  if (!timebase -> tm_set) {
    if (!gmtime_r(&timebase -> dt, &timebase -> tm)) {
      return NULL;
    }
    timebase -> tm_set = 1;
  }
  return &timebase -> tm;
}

datetime_t * _timebase_assign(datetime_t *datetime) {
  time_t t;

  if ((t = timegm(&datetime -> tm)) != -1) {
    datetime -> dt = t;
    datetime -> tm_set = 1;
  } else {
    memset(&datetime -> tm, 0, sizeof(struct tm));
    datetime -> tm_set = 0;
    datetime = NULL;
  }
  return datetime;
}


/* -- T I M E -------------------------------------------------------- */

data_t * _timeofday_new(datetime_t *timeofday, va_list args) {
  int        flag = va_arg(args, int);
  char      *s;
  int        hour = 0, min = 0, sec = 0;
  data_t    *ret = (data_t *) timeofday;

  timeofday -> tm_set = 0;
  memset(&timeofday -> tm, 0, sizeof(struct tm));
  switch (flag) {
    case DatetimeFlagParse:
      if (data_type(timeofday) == Time) {
        ret = _timeofday_parse((s = va_arg(args, char *)));
        if (!ret) {
          ret = data_exception(ErrorParameterValue,
            "Cannot parse time value '%s'", s);
        }
      }
      break;
    case DatetimeFlagHMS:
    case DatetimeFlagHM:
      hour = va_arg(args, int);
      min = va_arg(args, int);
      if (flag == DatetimeFlagHMS) {
        sec = va_arg(args, int);
      }
      if ((hour < 0) || (hour > 23)) {
        ret = data_exception(ErrorParameterValue, "Invalid 'hour' value %d", hour);
      } else if ((min < 0) || (min > 59)) {
        ret = data_exception(ErrorParameterValue, "Invalid 'minute' value %d", min);
      } else if ((sec < 0) || (sec > 59)) {
        ret = data_exception(ErrorParameterValue, "Invalid 'second' value %d", sec);
      } else {
        timeofday -> tm.tm_hour = hour;
        timeofday -> tm.tm_min = min;
        timeofday -> tm.tm_sec = sec;
        timeofday -> tm_set = 1;
        if (!_timebase_assign(timeofday)) {
          ret = data_exception(ErrorParameterValue,
                  "Could not convert '%d:%d:%d' to a time value", hour, min, sec);
        }
      }
      break;
  }
  return ret;
}

char * _timeofday_tostring(datetime_t *time) {
  char      *buf = NULL;

  if (!time -> tm_set) {
    if (!gmtime_r(&time -> dt, &time -> tm)) {
      return NULL;
    }
    time -> tm_set = 1;
  }
  asprintf(&buf, "%d:%d:%d",
    time -> tm.tm_hour, time -> tm.tm_min, time -> tm.tm_sec);
  return buf;
}

data_t * _timeofday_parse(char *time) {
  char    *copy = NULL;
  str_t   *s = NULL;
  data_t  *ret = NULL;
  array_t *split = NULL;
  int      sz;
  long     l;
  int      hour = 0, min = 0, sec = 0;

  if (!time || !*time) {
    return NULL;
  }
  copy = strdup(time);
  time = strtrim(copy);
  if (!*time) {
    free(copy);
    return NULL;
  }

  /* Now we can be sure there is at least one string in the split. */
  s = str_wrap(time);
  split = str_split(s, ":");
  sz = array_size(split);
  if (sz == 1) {
    if (!strtoint(data_tostring(array_get(split, 0)), &l)) {
      ret = data_create(Time, DatetimeFlagTimeT, (time_t) l);
    }
  } else if (sz <= 3) {
    if (!strtoint(data_tostring(array_get(split, 0)), &l)) {
      hour = l;
      if (!strtoint(data_tostring(array_get(split, 1)), &l)) {
        min = l;
        if ((sz == 3) && !strtoint(data_tostring(array_get(split, 2)), &l)) {
          sec = l;
        }
        ret = data_create(Time, DatetimeFlagHMS, hour, min, sec);
      }
    }
  }
  array_free(split);
  str_free(s);
  free(copy);
  return ret;
}

data_t * _timeofday_resolve(datetime_t *datetime, char *name) {
  data_t    *ret = NULL;

  if (!datetime -> tm_set) {
    if (!gmtime_r(&datetime -> dt, &datetime -> tm)) {
      return data_exception(ErrorInternalError, "Could not convert time");
    }
    datetime -> tm_set = 1;
  }
  if (!strcmp(name, "hour")) {
    ret = int_to_data(datetime -> tm.tm_hour);
  } else if (!strcmp(name, "minute")) {
    ret = int_to_data(datetime -> tm.tm_min);
  } else if (!strcmp(name, "second")) {
    ret = int_to_data(datetime -> tm.tm_sec);
  }
  return ret;
}

data_t * _timeofday_set(datetime_t *datetime, char *name, data_t *value) {
  int     must_update = 0;
  int     val;
  data_t *ret = NULL;

  if (!datetime -> tm_set) {
    if (!gmtime_r(&datetime -> dt, &datetime -> tm)) {
      return data_exception(ErrorInternalError, "Could not convert time");
    }
    datetime -> tm_set = 1;
  }

  val = data_intval(value);
  if (!strcmp(name, "hour")) {
    if ((val < 0) || (val > 23)) {
      ret = data_exception(ErrorParameterValue, "Invalid 'hour' value %d", val);
    } else {
      datetime -> tm.tm_hour = val;
      must_update = 1;
    }
  } else if (!strcmp(name, "minute")) {
    if ((val < 0) || (val > 59)) {
      ret = data_exception(ErrorParameterValue, "Invalid 'minute' value %d", val);
    } else {
      datetime -> tm.tm_min = val;
      must_update = 1;
    }
  } else if (!strcmp(name, "second")) {
    if ((val < 0) || (val > 59)) {
      ret = data_exception(ErrorParameterValue, "Invalid 'second' value %d", val);
    } else {
      datetime -> tm.tm_sec = val;
      must_update = 1;
    }
  }
  if (must_update) {
    if (!(ret = (data_t *) _timebase_assign(datetime))) {
      ret = data_exception(ErrorParameterValue,
              "Invalid '%s' value %d", name, val);
    }
  }
  return ret;
}

data_t * _function_date_init(char *name, array_t *args, dict_t *kwargs) {
  (void) name;
  (void) args;
  (void) kwargs;

  _date_init();
  return data_true();
}

data_t * _function_time(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  int     sz;
  data_t *d;
  int_t  *t, *min, *sec;

  (void) self;
  (void) name;
  (void) kwargs;

  if (!args || !(sz = array_size(args))) {
    return data_create(Time, -1, time(NULL));
  }
  d = data_array_get(args, 0);
  t = (int_t *) data_cast(d, Int);
  if (sz == 1) {
    return (t)
      ? data_create(Time, DatetimeFlagTimeT, (time_t) data_intval((data_t *) t))
      : data_parse(Time, data_tostring(d));
  }
  if (t) {
    min = (int_t *) data_array_get(args, 1);
    if (sz == 3) {
      sec = (int_t *) data_array_get(args, 2);
    }
    return time_create(data_intval((data_t *) t), data_intval(min), data_intval(sec));
  } else {
    return data_exception(ErrorParameterValue,
      "Cannot create time from value '%s'", data_tostring(t));
  }
}

data_t * time_create(int hour, int min, int sec) {
  _date_init();
  return (data_t *) data_create(Time, DatetimeFlagHMS, hour, min, sec);
}

/* -- D A T E ------------------------------------------------------------- */

data_t * _date_new(datetime_t *date, va_list args) {
  int     flag = va_arg(args, int);
  char   *s;
  int     day, month, year;
  data_t *ret = (data_t *) date;
  time_t  t;

  switch (flag) {
    case DatetimeFlagParse:
      if (data_type(date) == Date) {
        if (!(ret = _date_parse((s = va_arg(args, char *))))) {
          ret = data_exception(ErrorParameterValue,
            "Cannot parse date value '%s'", s);
        }
      }
      break;
    case DatetimeFlagYMD:
      year = va_arg(args, int);
      month = va_arg(args, int);
      day = va_arg(args, int);
      if ((month < 1) || (month > 12)) {
        ret = data_exception(ErrorParameterValue, "Invalid 'month' value %d", month);
      } else if ((day < 1) || (day > 31)) {
        ret = data_exception(ErrorParameterValue, "Invalid 'day' value %d", day);
      } else {
        date -> tm.tm_year = year - 1900;
        date -> tm.tm_mon = month - 1;
        date -> tm.tm_mday = day;
        if ((t = timegm(&date -> tm)) >= 0) {
          date -> dt = t;
          date -> tm_set = 1;
        } else {
          memset(&date -> tm, 0, sizeof(struct tm));
          date -> tm_set = 1;
          ret = data_exception(ErrorParameterValue,
                  "Could not convert '%d-%d-%d' to a time value", year, month, day);
        }
      }
      break;
  }
  return ret;
}

char * _date_tostring(datetime_t *time) {
  char      *buf = NULL;

  if (!time -> tm_set) {
    if (!gmtime_r(&time -> dt, &time -> tm)) {
      return NULL;
    }
    time -> tm_set = 1;
  }
  asprintf(&buf, "%4d-%2d-%2d",
    time -> tm.tm_year + 1970, time -> tm.tm_mon + 1, time -> tm.tm_mday);
  return buf;
}

data_t * _date_parse(char *date) {
  char    *copy = NULL;
  str_t   *s = NULL;
  data_t  *ret = NULL;
  array_t *split = NULL;
  int      sz;
  long     l;
  int      year = 0, month = 0, day = 0;

  if (!date || !*date) {
    return NULL;
  }
  copy = strdup(date);
  date = strtrim(copy);
  if (!*date) {
    free(copy);
    return NULL;
  }

  /* Now we can be sure there is at least one string in the split. */
  s = str_wrap(date);
  split = str_split(s, "-");
  sz = array_size(split);
  if (sz == 3) {
    if (!strtoint(data_tostring(array_get(split, 0)), &l)) {
      year = l;
      if (!strtoint(data_tostring(array_get(split, 1)), &l)) {
        month = l;
        if ((sz == 3) && !strtoint(data_tostring(array_get(split, 2)), &l)) {
          day = l;
          ret = data_create(Date, DatetimeFlagYMD, year, month, day);
        }
      }
    }
  }
  array_free(split);
  str_free(s);
  free(copy);
  return (data_t *) ret;
}

data_t * _date_resolve(datetime_t *datetime, char *name) {
  data_t    *ret = NULL;

  if (!datetime -> tm_set) {
    if (!gmtime_r(&datetime -> dt, &datetime -> tm)) {
      return data_exception(ErrorInternalError, "Could not convert time");
    }
    datetime -> tm_set = 1;
  }
  if (!strcmp(name, "year")) {
    ret = int_to_data(datetime -> tm.tm_year + 1900);
  } else if (!strcmp(name, "month")) {
    ret = int_to_data(datetime -> tm.tm_mon + 1);
  } else if (!strcmp(name, "day")) {
    ret = int_to_data(datetime -> tm.tm_mday);
  } else if (!strcmp(name, "day_of_week")) {
    ret = int_to_data(datetime -> tm.tm_wday + 1);
  } else if (!strcmp(name, "day_of_year")) {
    ret = int_to_data(datetime -> tm.tm_yday + 1);
  } else if (!strcmp(name, "seconds_since_epoch")) {
    ret = int_to_data((long) datetime -> dt);
  }
  return ret;
}

data_t * _date_set(datetime_t *datetime, char *name, data_t *value) {
  int     must_update = 0;
  int     val;
  data_t *ret = NULL;

  if (!datetime -> tm_set) {
    if (!gmtime_r(&datetime -> dt, &datetime -> tm)) {
      return data_exception(ErrorInternalError, "Could not convert time");
    }
    datetime -> tm_set = 1;
  }

  val = data_intval(value);
  if (!strcmp(name, "year")) {
    datetime -> tm.tm_year = val - 1900;
    must_update = 1;
  } else if (!strcmp(name, "month")) {
    if ((val < 1) || (val > 12)) {
      ret = data_exception(ErrorParameterValue, "Invalid 'month' value %d", val);
    } else {
      datetime -> tm.tm_mon = val - 1;
      must_update = 1;
    }
  } else if (!strcmp(name, "day")) {
    if ((val < 1) || (val > 31)) {
      ret = data_exception(ErrorParameterValue, "Invalid 'day' value %d", val);
    } else {
      datetime -> tm.tm_mday = val;
      must_update = 1;
    }
  }
  if (must_update) {
    datetime -> dt = timegm(&datetime -> tm);
    ret = (data_t *) datetime;
  }
  return ret;
}

data_t * _function_date(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  int     sz;
  data_t *d;
  int_t  *t, *month, *day;
  data_t *ret;

  (void) self;
  (void) name;
  (void) kwargs;

  if (!args || !(sz = array_size(args))) {
    return data_create(Date, DatetimeFlagTimeT, time(NULL));
  }
  d = data_array_get(args, 0);
  t = (int_t *) data_cast(d, Int);
  if (t) {
    if (sz == 1) {
      return data_create(Date, DatetimeFlagTimeT, (time_t) data_intval(t));
    } else {
      month = (int_t *) data_array_get(args, 1);
      day = (int_t *) data_array_get(args, 2);
      return data_create(
        Date, DatetimeFlagYMD,
        data_intval(t), data_intval(month), data_intval(day));
    }
  } else if (sz == 1) {
    if (!(ret = data_parse(Date, data_tostring(d)))) {
      ret = data_exception(ErrorParameterValue,
        "Cannot create date from value '%s'", data_tostring(d));
    }
  }
  return ret;
}

data_t * date_create(int year, int month, int day) {
  _date_init();
  return data_create(Date, DatetimeFlagYMD, year, month, day);
}

/* -- D A T E T I M E ----------------------------------------------------- */

data_t * _datetime_new(datetime_t *datetime, va_list args) {
  int         flag = va_arg(args, int);
  datetime_t *d;
  datetime_t *t;
  struct tm  *tm;
  data_t     *ret = (data_t *) datetime;
  char       *s;

  switch (flag) {
    case DatetimeFlagParse:
      if (!(ret = _datetime_parse((s = va_arg(args, char *))))) {
        ret = data_exception(ErrorParameterValue,
          "Cannot parse datetime value '%s'", s);
      }
      break;
    case DatetimeFlagDateTime:
      d = va_arg(args, datetime_t *);
      t = va_arg(args, datetime_t *);
      tm = _timebase_tm(d);
      datetime -> tm.tm_year = tm -> tm_year;
      datetime -> tm.tm_mon = tm -> tm_mon;
      datetime -> tm.tm_mday = tm -> tm_mday;
      tm = _timebase_tm(t);
      datetime -> tm.tm_hour = tm -> tm_hour;
      datetime -> tm.tm_min = tm -> tm_min;
      datetime -> tm.tm_sec = tm -> tm_sec;
      if (!_timebase_assign(datetime)) {
        ret = data_exception(ErrorParameterValue,
                "Could not convert '%s' '%s' to a datetime value",
                data_tostring(d), data_tostring(t));
      }
      break;
  }
  return ret;
}

data_t * _datetime_parse(char *datetime) {
  char       *copy = NULL;
  str_t      *s = NULL;
  data_t     *ret = NULL;
  array_t    *split = NULL;
  datetime_t *d = NULL;
  datetime_t *t = NULL;

  if (!datetime || !*datetime) {
    return NULL;
  }
  copy = strdup(datetime);
  datetime = strtrim(copy);
  if (!*datetime) {
    free(copy);
    return NULL;
  }

  s = str_wrap(datetime);
  split = str_split(s, " ");
  if ((array_size(split) == 2) &&
      (d = (datetime_t *) data_parse(Date, data_tostring(array_get(split, 0)))) &&
      (t = (datetime_t *) data_parse(Time, data_tostring(array_get(split, 1))))) {
    ret = (data_t *) datetime_create(d, t);
  }
  data_free((data_t *) d);
  data_free((data_t *) t);
  array_free(split);
  str_free(s);
  free(copy);
  return (data_t *) ret;
}

char * _datetime_tostring(datetime_t *datetime) {
  char *d;
  char *t;
  char *buf;

  asprintf(&buf, "%s %s",
    (d = _date_tostring(datetime)), (t = _datetime_tostring(datetime)));
  free(d);
  free(t);
  return buf;
}

data_t * _function_datetime(data_t *self, char *name, array_t *args, dict_t *kwargs) {
  int         sz;
  data_t     *arg;
  data_t     *data;
  int_t      *i1 = NULL, *i2 = NULL, *i3 = NULL;
  data_t     *ret = NULL;
  datetime_t *d = NULL;
  datetime_t *t = NULL;

  (void) self;
  (void) name;
  (void) kwargs;

  if (!args || !(sz = array_size(args))) {
    return data_create(Datetime, DatetimeFlagTimeT, time(NULL));
  }
  arg = data_array_get(args, 0);
  i1 = (int_t *) data_cast(arg, Int);
  if (i1) {
    switch (sz) {
      case 1:
        ret = data_create(Datetime, DatetimeFlagTimeT, (time_t) data_intval(i1));
        break;
      case 2:
        arg = data_array_get(args, 1);
        i2 = (int_t *) data_cast(arg, Int);
        if (!i2) {
          ret = data_exception(ErrorParameterValue,
            "Cannot create datetime from value '%s'", data_tostring(arg));
        } else {
          ret = datetime_create(
            (d = (datetime_t *) data_create(
              Date, DatetimeFlagTimeT, (time_t) data_intval(i1))),
            (t = (datetime_t *) data_create(
              Time, DatetimeFlagTimeT, (time_t) data_intval(i2))));
        }
        break;
      case 6:
        arg = data_array_get(args, 1);
        i2 = (int_t *) data_cast(arg, Int);
        if (!i2) {
          ret = data_exception(ErrorParameterValue,
            "Cannot create datetime from value '%s'", data_tostring(arg));
        }
        if (!ret) {
          arg = data_array_get(args, 2);
          i3 = (int_t *) data_cast(arg, Int);
          data = date_create(data_intval(i1), data_intval(i2), data_intval(i3));
          if (data_is_exception(data)) {
            ret = data;
          } else {
            d = (datetime_t *) data;
          }
        }
        if (!ret) {
          data_free((data_t *) i1);
          data_free((data_t *) i2);
          data_free((data_t *) i3);
          arg = data_array_get(args, 3);
          i1 = (int_t *) data_cast(arg, Int);
          if (!i1) {
            ret = data_exception(ErrorParameterValue,
              "Cannot create datetime from value '%s'", data_tostring(arg));
          }
        }
        if (!ret) {
          arg = data_array_get(args, 4);
          i2 = (int_t *) data_cast(arg, Int);
          if (!i2) {
            ret = data_exception(ErrorParameterValue,
              "Cannot create datetime from value '%s'", data_tostring(arg));
          }
        }
        if (!ret) {
          arg = data_array_get(args, 5);
          i3 = (int_t *) data_cast(arg, Int);
        }
        if (!ret) {
          data = time_create(data_intval(i1), data_intval(i2), data_intval(i3));
          if (data_is_exception(data)) {
            ret = data;
          } else {
            t = (datetime_t *) data;
          }
        }
        if (!ret) {
          ret = datetime_create(d, t);
        }
        break;
      default:
        ret = data_exception(ErrorArgCount,
          "Cannot create datetime from %d arguments", sz);
        break;
    } /* switch (sz) */
  } else { /* i1, i.e. args[1] is an int value */
    switch (sz) {
      case 1:
        if (!(ret = _datetime_parse(data_tostring(arg)))) {
          ret = data_exception(ErrorParameterValue,
            "Cannot create time from value '%s'", data_tostring(arg));
        }
        break;
      case 2:
        if (!(d = (datetime_t *) _date_parse(data_tostring(arg)))) {
          ret = data_exception(ErrorParameterValue,
            "Cannot create date from value '%s'", data_tostring(arg));
        }
        if (!ret) {
          if (!(t = (datetime_t *) _timeofday_parse(data_tostring(data_array_get(args, 1))))) {
            ret = data_exception(ErrorParameterValue,
              "Cannot create time from value '%s'",
              data_tostring(data_array_get(args, 1)));
          }
        }
        break;
      default:
        ret = data_exception(ErrorArgCount,
          "Cannot create datetime from %d arguments", sz);
        break;
    }
  }
  data_free((data_t *) i1);
  data_free((data_t *) i2);
  data_free((data_t *) i3);
  data_free((data_t *) d);
  data_free((data_t *) t);
  return ret;
}
