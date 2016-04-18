#ifndef _SCHEME_H_
#define _SCHEME_H_

#include <stddef.h>
#include <stdbool.h>

typedef struct scheme_obj scheme_obj;
typedef struct scheme_context scheme_context;

scheme_context * scheme_init();
void scheme_shutdown(scheme_context *c);

double scheme_obj_as_num(scheme_obj *o);

scheme_obj * scheme_obj_num(double val);

void scheme_obj_delete(scheme_obj *o);


scheme_obj * scheme_read(const char *code);

char * scheme_print(scheme_obj *o);

scheme_obj * scheme_eval(scheme_obj *expr);

/* private - TODO: Move to other header? */
const char * scheme_eat_space(const char *txt);
char scheme_peek(const char *txt);
bool scheme_is_digit(char c);

#endif /* _SCHEME_H_ */
