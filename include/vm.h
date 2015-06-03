/*
 * /obelix/include/vm.h - Copyright (c) 2015 Jan de Visser <jan@de-visser.net>
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

#ifndef __VM_H__
#define __VM_H__

#include <stdarg.h>

#include <data.h>
#include <datastack.h>
#include <list.h>
#include <nvp.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */
  
#define NUM_STASHES     8

typedef struct _vm {
  data_t       data;
  data_t      *stashes[NUM_STASHES];
  bytecode_t  *bytecode;
  datastack_t *stack;
  datastack_t *contexts;
} vm_t;

extern vm_t *   vm_create(bytecode_t *);
extern data_t * vm_pop(vm_t *);
extern data_t * vm_peek(vm_t *);
extern data_t * vm_push(vm_t *, data_t *);
extern data_t * vm_stash(vm_t *, unsigned int, data_t *);
extern data_t * vm_unstash(vm_t *, unsigned int);
extern nvp_t *  vm_push_context(vm_t *, char *, data_t *);
extern nvp_t *  vm_peek_context(vm_t *);
extern nvp_t *  vm_pop_context(vm_t *);
extern data_t * vm_execute(vm_t *, data_t *);

extern int VM;

#define data_is_vm(d)   ((d) && data_hastype((d), VM))
#define data_vmval(d)   (data_is_vm((d)) ? ((vm_t *) ((d) -> ptrval)) : NULL)
#define vm_copy(vm)     ((closure_t *) data_copy((vm)))
#define vm_free(vm)     (data_free((data_t *) (vm)))
#define vm_tostring(vm) (data_tostring((data_t *) (vm)))

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_H__ */
