/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef OBELIX_CRT_H
#define OBELIX_CRT_H

#include <stdint.h>

typedef struct _string {
  uint32_t length;
  uint8_t* data;
} string;

extern void exit(int status);
extern void * malloc(size_t size);

extern int open(const char *path, int oflag, ...);
extern int close(int fildes);
extern ssize_t write(int fildes, const void *buf, size_t nbyte);
extern ssize_t read(int fildes, void *buf, size_t nbyte);

extern int fputs(int, string);
extern int puts(string);
extern int eputs(string);
extern int putln(string);

extern string cstr_to_string(char const*);
extern int cputs(char const*);
extern int cputln(char const*);

extern string string_alloc(char const *);
extern string to_string_s(int64_t, int);
extern string to_string_u(uint64_t, int);
extern string string_concat(string, string);

#endif // OBELIX_CRT_H
