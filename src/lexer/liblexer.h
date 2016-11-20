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

#ifndef obllexer_EXPORTS
#define OBLLEXER_IMPEXP extern
#define OBL_STATIC
#endif /* !obllexer_EXPORTS */

#include <ctype.h>
#include <stdio.h>

#include <config.h>
#include <core.h>
#include <lexer.h>
#include <nvp.h>

#define FunctionMatch           FunctionUsr1
#define FunctionMatch2          FunctionUsr2
#define FunctionGetConfig       FunctionUsr3
#define FunctionDump            FunctionUsr4
#define FunctionDestroyScanner  FunctionUsr5
#define FunctionReconfigScanner FunctionUsr6

#endif /* __LIBLEXER_H__ */
