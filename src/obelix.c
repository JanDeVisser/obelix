#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



typedef struct _module {
  dict_t *names;
  proc_t *code;
} module_t;

extern module_t * mod_parse(int);
extern value_t * mod_run(module_t);


int main(int argc, char **argv) {
  int fno;
  module_t *mod;
  int ret = -1;
  value_t retval;

  fno = (argc) ? open_file(argv[0]) : 0;
  mod = mod_parse(fno);
  if (mod) {
    retval = mod_run(mod);
    ret = val_toint(retval);
  }
  return ret;
}
