/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef __OBELIX_H__
#define __OBELIX_H__

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
extern int64_t fsize(int fd);

extern int fputs(int, string);
extern int puts(string);
extern int eputs(string);
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
