#ifndef __DICT_H__
#define __DICT_H__

typedef struct _dict {
  cmp_t comp;
  list_t *entries;
} dict_t;

extern dict_t * dict_create(cmp_t);
extern void * dict_put(dict_t, void *, void *);
extern void * dict_get(dict_t, void *);
extern int dict_size(dict_t);

#endif /* __DICT_H__ */
