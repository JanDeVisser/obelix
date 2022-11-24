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

typedef struct _$enum_value {
  int32_t value;
  char* text;
} $enum_value;

typedef struct _$token {
    char const* file_name;
    int line_start;
    int column_start;
    int line_end;
    int column_end;
} $token;

#ifndef STRING_IMPL
typedef void* string;
#endif

extern void $fatal($token, char const*);

extern $enum_value $get_enum_value($enum_value[], int32_t);

extern void exit(int status);
extern void * malloc(size_t size);

extern int64_t fsize(int fd);

extern string str_view_for(char const*);
extern string str_allocate(char const*);
extern string str_adopt(char*);
extern string str_copy(string);
extern void str_free(string);
extern string str_concat(string, string);
extern string str_multiply(string, uint32_t);
extern char * str_data(string);
extern uint32_t str_length(string);
extern int str_compare(string, string);
extern string to_string_s(int64_t, int);
extern string to_string_u(uint64_t, int);



extern int $fputs(int, string);
extern int $puts(string);
extern int $eputs(string);
extern int putln(string);
extern int putln_s(int64_t);
extern int putln_u(uint64_t);
extern int putint(int64_t);
extern int putint(int64_t);

extern string cstr_to_string(char *);
extern int cputs(char *);
extern int cputln(int8_t *);

#endif /* __OBELIX_H__ */
