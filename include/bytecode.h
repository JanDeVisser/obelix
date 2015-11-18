/*
 * /obelix/include/bytecode.h - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#ifndef __BYTECODE_H__
#define __BYTECODE_H__

#include <libparser.h>
#include <data.h>
#include <datastack.h>
#include <dict.h>
#include <instruction.h>
#include <list.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _bytecode {
  data_t       _d;
  data_t      *owner;
  list_t      *instructions;
  list_t      *main_block;
  datastack_t *deferred_blocks;
  datastack_t *bookmarks;
  datastack_t *pending_labels;
  dict_t      *labels;
  int          current_line;
} bytecode_t;

OBLPARSER_IMPEXP int Bytecode;
OBLPARSER_IMPEXP int bytecode_debug;

OBLPARSER_IMPEXP bytecode_t * bytecode_create(data_t *owner);
OBLPARSER_IMPEXP void         bytecode_free(bytecode_t *);
OBLPARSER_IMPEXP char *       bytecode_tostring(bytecode_t *);
OBLPARSER_IMPEXP bytecode_t * bytecode_push_instruction(bytecode_t *, data_t *);
OBLPARSER_IMPEXP bytecode_t * bytecode_start_deferred_block(bytecode_t *);
OBLPARSER_IMPEXP bytecode_t * bytecode_end_deferred_block(bytecode_t *);
OBLPARSER_IMPEXP bytecode_t * bytecode_pop_deferred_block(bytecode_t *);
OBLPARSER_IMPEXP bytecode_t * bytecode_bookmark(bytecode_t *);
OBLPARSER_IMPEXP bytecode_t * bytecode_discard_bookmark(bytecode_t *);
OBLPARSER_IMPEXP bytecode_t * bytecode_defer_bookmarked_block(bytecode_t *);
OBLPARSER_IMPEXP void         bytecode_list(bytecode_t *);

#define data_is_bytecode(d)   ((d) && data_hastype((d), Bytecode))
#define data_as_bytecode(d)   ((bytecode_t *) (data_is_bytecode((d)) ? (d) : NULL))
#define bytecode_copy(bc)     ((bytecode_t *) data_copy((data_t *) (bc)))
#define bytecode_free(bc)     (data_free((data_t *) (bc)))
#define bytecode_tostring(bc) (data_tostring((data_t *) (bc)))

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __BYTECODE_H__ */
