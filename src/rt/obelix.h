/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __OBELIX_H__
#define __OBELIX_H__

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

extern int* __error(void);
#define _stdlib_errno (*__error())

typedef struct _string {
  uint32_t length;
  uint8_t* data;
} string;

typedef struct _enum_value {
  int32_t value;
  char* text;
} $enum_value;

typedef struct _token {
    char const* file_name;
    int line_start;
    int column_start;
    int line_end;
    int column_end;
} token;

extern void obelix_fatal(token, char const*);

extern $enum_value $get_enum_value($enum_value[], int32_t);

extern void exit(int status);
extern void * malloc(size_t size);

extern int64_t fsize(int fd);

extern int obl_fputs(int, string);
extern int obl_puts(string);
extern int obl_eputs(string);
extern int putln(string);
extern int putln_s(int64_t);
extern int putln_u(uint64_t);
extern int putint(int64_t);
extern int putint(int64_t);

extern string cstr_to_string(char *);
extern int cputs(char *);
extern int cputln(int8_t *);

extern string string_alloc(char *);
extern string to_string_s(int64_t, int);
extern string to_string_u(uint64_t, int);
extern string string_concat(string, string);

#endif /* __OBELIX_H__ */
