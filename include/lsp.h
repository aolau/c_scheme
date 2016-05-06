#ifndef _LSP_H_
#define _LSP_H_

#include <stddef.h>
#include <stdbool.h>

typedef struct lsp_obj lsp_obj;
typedef struct lsp_context lsp_context;

typedef enum lsp_mark_type_ {UNUSED = 1, EXTERNAL, INTERNAL} lsp_mark_type;

lsp_context * lsp_init();
void lsp_shutdown(lsp_context *c);

lsp_obj * lsp_env_create(lsp_obj *names, lsp_obj *values,
                               lsp_context *ctx);
void lsp_context_push_env(lsp_context *c, lsp_obj *e);

long int lsp_obj_as_num(lsp_obj *o);
const char * lsp_obj_as_string(lsp_obj *o);

lsp_obj * lsp_obj_num(long int val, lsp_context *ctx);
lsp_obj * lsp_obj_cons(lsp_obj *car, lsp_obj *cdr,
                             lsp_context *ctx);

void lsp_obj_delete(lsp_obj *o);

int lsp_obj_mark(lsp_obj *o, lsp_mark_type mark);
    
lsp_obj * lsp_read(char *txt, lsp_context *ctx);

char * lsp_print(lsp_obj *o);

lsp_obj * lsp_eval(lsp_obj *expr, lsp_context *ctx);

/* private - TODO: Move to other header? */
lsp_obj * lsp_read_obj(char *txt, char **next, lsp_context *ctx);
char * lsp_print_obj(lsp_obj *obj, char *buf);
lsp_obj * lsp_obj_nil();

char * lsp_eat_space(char *txt);
char lsp_peek(char *txt);
bool lsp_is_digit(char c);

#endif /* _LSP_H_ */
