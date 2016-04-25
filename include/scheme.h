#ifndef _SCHEME_H_
#define _SCHEME_H_

#include <stddef.h>
#include <stdbool.h>

typedef struct scheme_obj scheme_obj;
typedef struct scheme_context scheme_context;

scheme_context * scheme_init();
void scheme_shutdown(scheme_context *c);

scheme_obj * scheme_env_create(scheme_obj *names, scheme_obj *values,
                               scheme_context *ctx);
void scheme_context_push_env(scheme_context *c, scheme_obj *e);

long int scheme_obj_as_num(scheme_obj *o);
const char * scheme_obj_as_string(scheme_obj *o);

scheme_obj * scheme_obj_num(long int val);
scheme_obj * scheme_obj_cons(scheme_obj *car, scheme_obj *cdr,
                             scheme_context *ctx);

void scheme_obj_delete(scheme_obj *o);

scheme_obj * scheme_read(char *txt, scheme_context *ctx);

char * scheme_print(scheme_obj *o);

scheme_obj * scheme_eval(scheme_obj *expr, scheme_context *ctx);

/* private - TODO: Move to other header? */
scheme_obj * scheme_read_obj(char *txt, char **next, scheme_context *ctx);
char * scheme_print_obj(scheme_obj *obj, char *buf);
scheme_obj * scheme_obj_nil();

char * scheme_eat_space(char *txt);
char scheme_peek(char *txt);
bool scheme_is_digit(char c);

#endif /* _SCHEME_H_ */
