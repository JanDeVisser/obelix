/*
 * libparser.h - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#ifndef __LIBLEXER_H__
#define __LIBLEXER_H__

#include <oblconfig.h>
#define OBLLEXER_IMPEXP __DLL_EXPORT__

#include <ctype.h>
#include <stdio.h>

#include <core.h>
#include <lexer.h>
#include <nvp.h>

#define FunctionMatch           FunctionUsr1
#define FunctionMatch2          FunctionUsr2
#define FunctionGetConfig       FunctionUsr3
#define FunctionDump            FunctionUsr4
#define FunctionDestroyScanner  FunctionUsr5
#define FunctionReconfigScanner FunctionUsr6

OBLLEXER_IMPEXP typedescr_t * comment_register(void);
OBLLEXER_IMPEXP typedescr_t * identifier_register(void);
OBLLEXER_IMPEXP typedescr_t * keyword_register(void);
OBLLEXER_IMPEXP typedescr_t * number_register(void);
OBLLEXER_IMPEXP typedescr_t * position_register(void);
OBLLEXER_IMPEXP typedescr_t * qstring_register(void);
OBLLEXER_IMPEXP typedescr_t * whitespace_register(void);

#endif /* __LIBLEXER_H__ */
