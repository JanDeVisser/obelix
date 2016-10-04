/*
 * obelix/src/lexer/test/tlexer.h - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

#include <lexa.h>
#include <testsuite.h>

#ifndef __TLEXER_H__
#define __TLEXER_H__

extern lexa_t *lexa;

extern lexa_t * setup(void);
extern lexa_t * setup_with_scanners(void);
extern void     teardown();

extern void     _setup_with_scanners(void);
extern void     _teardown();

extern void     create_lexa(void);
extern void     create_number(void);
extern void     create_qstring(void);
extern void     create_comment(void);
extern void     create_keyword(void);

#endif /* __TLEXER_H__ */
