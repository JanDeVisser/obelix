/*
 * /obelix/src/stdlib/sys.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <oblconfig.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#elif defined(HAVE_GETVERSIONEX)
#include <windows.h>
#endif /* HAVE_SYS_UTSNAME_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <data.h>
#include <exception.h>
#include <vm.h>
#include <str.h>
#include <math.h>

extern char   **environ;

#ifdef HAVE_GETVERSIONEX
static char *   _windows_release(OSVERSIONINFOEX *);
static char *   _windows_version(OSVERSIONINFOEX *);
static char *   _windows_machine(SYSTEM_INFO *);
#endif /* HAVE_GETVERSIONEX */

extern data_t * _function_getenv(char *, array_t *, dict_t *);
extern data_t * _function_uname(char *, array_t *, dict_t *);

/* ------------------------------------------------------------------------ */

#ifdef HAVE_GETVERSIONEX
typedef struct _winrelease {
  char *name;
  int   major;
  int   minor;
  int   type;
} winrelease_t;

static winrelease_t _releases[] = {
  { .name = "Windows 2000",           .major = 5,  .minor = 0, .type = 0 },
  { .name = "Windows XP",             .major = 5,  .minor = 1, .type = 0 },
  { .name = "Windows XP 64bit",       .major = 5,  .minor = 2, .type = VER_NT_WORKSTATION },
  { .name = "Windows Server 2003",    .major = 5,  .minor = 2, .type = 0 },
  { .name = "Windows Vista",          .major = 6,  .minor = 0, .type = VER_NT_WORKSTATION },
  { .name = "Windows Server 2008",    .major = 6,  .minor = 0, .type = 0 },
  { .name = "Windows Server 2008 R2", .major = 6,  .minor = 1, .type = 0 },
  { .name = "Windows 7",              .major = 6,  .minor = 1, .type = VER_NT_WORKSTATION },
  { .name = "Windows Server 2012",    .major = 6,  .minor = 2, .type = 0 },
  { .name = "Windows 8",              .major = 6,  .minor = 2, .type = VER_NT_WORKSTATION },
  { .name = "Windows Server 2012 R2", .major = 6,  .minor = 3, .type = 0 },
  { .name = "Windows 8.1",            .major = 6,  .minor = 3, .type = VER_NT_WORKSTATION },
  { .name = "Windows Server 2016",    .major = 10, .minor = 0, .type = 0 },
  { .name = "Windows 10",             .major = 10, .minor = 0, .type = VER_NT_WORKSTATION },
  { .name = NULL,                     .major = 0,  .minor = 0, .type = 0 }
};

char * _windows_release(OSVERSIONINFOEX *version) {
  winrelease_t *wr;

  for (wr = _releases; wr -> name; wr++) {
    if ((wr -> major == version -> dwMajorVersion) &&
        (wr -> minor == version -> dwMinorVersion) &&
        ((wr -> type && wr -> type == version -> wProductType) ||
        (!wr -> type))) {
      return wr -> name;
    }
  }
  return "Windows";
}

char * _windows_version(OSVERSIONINFOEX *version) {
  static char  buf[80];
  char        *v;

  v = buf;
  sprintf(v, "%d.%d (Build %d)",
	  version -> dwMajorVersion,
	  version -> dwMinorVersion,
	  version -> dwBuildNumber);
  if (version -> wServicePackMajor) {
    v = v + strlen(v);
    sprintf(v, "SP %d", version -> wServicePackMajor);
    if (version -> wServicePackMinor) {
      v = v + strlen(v);
      sprintf(v, ".%d", version -> wServicePackMinor);
    }
  }
  return buf;
}

char * _windows_machine(SYSTEM_INFO *sysinfo) {
  static char  buf[80];
  char        *m;

  m = buf;
  switch (sysinfo -> wProcessorArchitecture) {
  case PROCESSOR_ARCHITECTURE_AMD64:
    strcpy(m, "AMD64");
    break;
  case PROCESSOR_ARCHITECTURE_ARM:
    strcpy(m, "ARM");
    break;
  case PROCESSOR_ARCHITECTURE_IA64:
    strcpy(m, "IA64");
    break;
  case PROCESSOR_ARCHITECTURE_INTEL:
    strcpy(m, "Intel");
    break;
  default:
    strcpy(m, "Unknown");
    break;
  }
  m = m + strlen(m);
  sprintf(m, " (%d processors)", sysinfo -> dwNumberOfProcessors);
  return buf;
}

#endif /* HAVE_GETVERSIONEX */

static void _object_set(object_t *obj, char *attr, data_t *value) {
  object_set(obj, attr, value);
  data_free(value);
}

/* ------------------------------------------------------------------------ */

data_t * _function_getenv(char *name, array_t *params, dict_t *kwargs) {
  object_t  *obj;
  data_t    *value;
  char     **e;
  char      *n = NULL;
  char      *v;
  int        len = 0;
  int        l;

  obj = object_create(NULL);
  for (e = environ; *e; e++) {
    l = strlen(*e);
    if (l > len) {
      if (n) free(n);
      n = strdup(*e);
      len = l;
    } else {
      strncpy(n, *e, l);
      n[l] = 0;
    }
    v = strchr(n, '=');
    if (v) {
      *v++ = 0;
      value = (data_t *) str_copy_chars(v);
      object_set(obj, n, value);
      data_free(value);
    }
  }
  free(n);
  return (data_t *) obj;
}

data_t * _function_uname(char *name, array_t *params, dict_t *kwargs) {
  object_t         *obj;
  data_t           *ret = NULL;
#ifdef HAVE_UTSNAME_H
  struct utsname    buf;
#lse
  char              hostname[80];
#endif /* HAVE_UTSNAME_H */
#ifdef HAVE_GETVERSIONEX
  OSVERSIONINFOEX   version;
  SYSTEM_INFO       sysinfo;
  char              hostname[80];
#endif /* HAVE_GETVERSIONEX */

#ifdef HAVE_UTSNAME_H
  if (!uname(&buf)) {
    obj = object_create(NULL);
    object_set(obj, "sysname", str_to_data(buf.sysname));
    object_set(obj, "nodename", str_to_data(buf.nodename));
    object_set(obj, "release", str_to_data(buf.release));
    object_set(obj, "version", str_to_data(buf.version));
    object_set(obj, "machine", str_to_data(buf.machine));
#ifdef _GNU_SOURCE
    object_set(obj, "domainname", str_to_data(buf.domainname));
#endif
    ret = (data_t *) obj;
  } else {
    ret = data_exception(ErrorSysError, "Error executing uname(): %s",
			 strerror(errno));
  }
#endif /* HAVE_SYS_UTSNAME_H */

#ifdef HAVE_GETVERSIONEX
  obj = object_create(NULL);
#ifdef HAVE_GETHOSTNAME
  if (gethostname(hostname, 80)) {
    object_set(obj, "sysname", str_to_data(hostname));
    object_set(obj, "nodename", str_to_data(hostname));
  } else {
    ret = data_exception(ErrorSysError, "Error executing gethostname()");
  }
#endif /* HAVE_GETHOSTNAME */
  if (!ret) {
    version.dwOSVersionInfoSize = sizeof(version);
    if (GetVersionEx((OSVERSIONINFO *) &version)) {
      object_set(obj, "release",
		 str_to_data(_windows_release(&version)));
      object_set(obj, "version",
		 str_to_data(_windows_version(&version)));
    } else {
      ret = data_exception(ErrorSysError, "Error executing GetVersionEx()");
    }
  }
#ifdef HAVE_GETSYSTEMINFO
  if (!ret) {
    GetSystemInfo(&sysinfo);
    object_set(obj, "machine",
	       str_to_data(_windows_machine(&sysinfo)));
  }
#endif /* HAVE_GETSYSTEMINFO */
  if (!ret) {
    ret = (data_t *) object_copy(obj);
  }
  object_free(obj);
#endif /* HAVE_GETVERSIONEX */
  return ret;
}

data_t * _function_exit(char *name, array_t *params, dict_t *kwargs) {
  data_t *exit_code;
  data_t *error;

  if (params && array_size(params)) {
    exit_code = data_copy(data_array_get(params, 0));
  } else {
    exit_code = int_to_data(0);
  }

  error = data_exception(ErrorExit,
			 "Exit with code '%s'", data_tostring(exit_code));
  data_as_exception(error) -> throwable = exit_code;
  return error;
}
