/*
 * obelix.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
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

#include <stdio.h>
#include <expr.h>

static data_t * _add(context_t *ctx, void *data, list_t *params) {
  int sum, val;
  listiterator_t *iter;
  data_t *cur, *ret;

  debug("+");
  sum = 0;
  for(iter = li_create(params); li_has_next(iter); ) {
    cur = (data_t *) li_next(iter);
    data_get(cur, &val);
    sum += val;
  }
  return data_create(Int, &sum);
}

static data_t * _mult(context_t *ctx, void *data, list_t *params) {
  int prod, val;
  listiterator_t *iter;
  data_t *cur, *ret;
  
  debug("*");
  prod = 1;
  for(iter = li_create(params); li_has_next(iter); ) {
    cur = (data_t *) li_next(iter);
    data_get(cur, &val);
    prod *= val;
  }
  return data_create(Int, &prod);
}

int main(int argc, char **argv) {
  expr_t *expr, *e;
  data_t *result;
  int res;
  
  expr = expr_funccall(NULL, _add);
  expr_int_literal(expr, 1);
  expr_int_literal(expr, 2);
  e = expr_funccall(expr, _mult);
  expr_int_literal(e, 2);
  expr_int_literal(e, 3);
  
  result = expr_evaluate(expr);
  data_get(result, &res);
  printf("result: %d\n", res);
  data_free(result);
  expr_free(expr);
}
