#include "lsp.h"
#include "check.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void * lsp_alloc(size_t size) {
    return malloc(size);
}

static void lsp_free(void *mem) {
    free(mem);
}

typedef struct lsp_env {
    lsp_obj *names;
    lsp_obj *values;
} lsp_env;

typedef struct lsp_cons {
    lsp_obj *car;
    lsp_obj *cdr;
} lsp_cons;

enum lsp_obj_type {FREELIST, NIL, SYMBOL, STRING, NUM, CONS,
                      QUOTE, ENV, LAMBDA,
                      OBJ_TYPE_MAX_};

const char * obj_type_to_str(int t) {
    static char *str[] = {
        "FREELIST",
        "NIL",
        "SYMBOL",
        "STRING",
        "NUM",
        "CONS",
        "QUOTE",
        "ENV",
        "LAMBDA",
        "UNDEFINED"
    };

    return str[t];
}

typedef struct lsp_lambda {
    lsp_obj *args;
    lsp_obj *body;
} lsp_lambda;

#define LSP_STRING_SIZE 32

typedef struct lsp_obj {
    enum lsp_obj_type type;
    lsp_mark_type mark;
    lsp_obj *next;
    union {
        char str[LSP_STRING_SIZE];
        long int num;
        lsp_cons con;
        lsp_env env;
        lsp_lambda lambda;
        lsp_obj *expr;
    } value;
} lsp_obj;

static lsp_obj static_obj_nil = {.type = NIL};

lsp_obj * lsp_obj_nil() {
    return &static_obj_nil;
}

bool lsp_obj_is_nil(lsp_obj *o) {
    return o == &static_obj_nil;
}

lsp_obj * lsp_car(lsp_obj *cons) {
    if (lsp_obj_is_nil(cons))
        return lsp_obj_nil();
    else
        return cons->value.con.car;
}

lsp_obj * lsp_cdr(lsp_obj *cons) {
    if (lsp_obj_is_nil(cons))
        return lsp_obj_nil();
    else
        return cons->value.con.cdr;
}

#define LSP_HEAP_SIZE 100000

typedef struct lsp_mem {
    lsp_obj heap[LSP_HEAP_SIZE];
    lsp_obj *free_list;
} lsp_mem;

typedef struct lsp_context {
    lsp_obj *env_top;
    lsp_mem mem;
} lsp_context;

void lsp_mem_free(lsp_mem *m, lsp_obj *o) {
    memset(o, 0, sizeof(lsp_obj));
    
    o->next = m->free_list;
    m->free_list = o;
}

lsp_obj * lsp_mem_alloc(lsp_mem *m) {
    lsp_obj *o = m->free_list;
    m->free_list = o->next;
    
    return o;
}


bool lsp_obj_is_unused(lsp_obj *o);
bool lsp_obj_is_internal(lsp_obj *o);
void lsp_obj_set_mark(lsp_obj *o, lsp_mark_type value);

void lsp_mem_unmark_all_(lsp_mem *m, bool force) {
    TRACE("Unmarking all memory...");
    for (int i = 0; i < LSP_HEAP_SIZE; i++) {
        if (force || (! lsp_obj_is_internal(&m->heap[i])))
            lsp_obj_set_mark(&(m->heap[i]), UNUSED);
    }
}

void lsp_mem_force_unmark_all(lsp_mem *m) {
    lsp_mem_unmark_all_(m, true);
}

void lsp_mem_unmark_all(lsp_mem *m) {
    lsp_mem_unmark_all_(m, false);
}

void lsp_mem_collect(lsp_mem *m) {
    CHECK(m->free_list == NULL);

    TRACE("Collecting garbage...");
    int count = 0;
    for (int i = 0; i < LSP_HEAP_SIZE; i++) {
        if (lsp_obj_is_unused(&(m->heap[i]))) {
            lsp_mem_free(m, &(m->heap[i]));
            count++;
        }
    }
    TRACE("%d objects freed", count);
}

bool lsp_mem_no_free(lsp_mem *m) {
    return m->free_list == NULL;
}

int lsp_obj_mark(lsp_obj *o, lsp_mark_type mark) {
    int marked = 0;
    if (! lsp_obj_is_nil(o)) {
        lsp_obj_set_mark(o, mark);
        marked++;
    }

    switch (o->type) {
    case STRING:
    case NUM:
    case SYMBOL:
    case NIL:
        break;
    case CONS:
        marked += lsp_obj_mark(lsp_car(o), mark);
        marked += lsp_obj_mark(lsp_cdr(o), mark);
        break;
    case ENV:
        marked += lsp_obj_mark(o->value.env.names, mark);
        marked += lsp_obj_mark(o->value.env.values, mark);
        break;
    case QUOTE:
        marked += lsp_obj_mark(o->value.expr, mark);
        break;
    case LAMBDA:
        marked += lsp_obj_mark(o->value.lambda.args, mark);
        marked += lsp_obj_mark(o->value.lambda.body, mark);
        break;
    default:
        SHOULD_NEVER_BE_HERE;
    }
    return marked;
}

int lsp_mem_mark_used(lsp_context *ctx) {
    lsp_obj *env = ctx->env_top;
    int marked = lsp_obj_mark(env, EXTERNAL);

    TRACE("%d objects in use", marked);
}

lsp_obj *lsp_mem_get(lsp_context *ctx) {
    lsp_mem *m = &ctx->mem;
    
    if (lsp_mem_no_free(m)) {
        TRACE_NL;
        lsp_mem_unmark_all(m);
        lsp_mem_mark_used(ctx);
        lsp_mem_collect(m);
    }

    CHECK(lsp_mem_no_free(m) == false);

    return lsp_mem_alloc(m);
}

void lsp_mem_init(lsp_mem *m) {
    TRACE("Initializing heap...");

    m->free_list = NULL;
    lsp_mem_force_unmark_all(m);
    lsp_mem_collect(m);
}

void lsp_context_push_env(lsp_context *ctx, lsp_obj *env);

static lsp_context * lsp_context_create() {
    lsp_context *c = lsp_alloc(sizeof(lsp_context));
    lsp_mem_init(&c->mem);

    c->env_top = lsp_obj_nil();
    return c;
}

static void lsp_context_delete(lsp_context *c) {
    lsp_free(c);
}

lsp_context * lsp_init() {
    lsp_context *c = lsp_context_create();
    lsp_context_push_env(c, lsp_env_create(
                                lsp_read("(+ - * t nil)", c),
                                lsp_read("(+ - * t ())", c),
                                c));
    return c;
}

void lsp_mem_show_leaks(lsp_mem *m) {
    int type_counts[OBJ_TYPE_MAX_] = {0};
    int malformed = 0;
    for (int i = 0; i < LSP_HEAP_SIZE; i++) {
        lsp_obj *o = &m->heap[i];
        if (o->type >= OBJ_TYPE_MAX_ || o->type < 0) {
            malformed++;
        } else if (! lsp_obj_is_unused(o)) {
            type_counts[o->type]++;
        }
    }

    TRACE("GC stats");
    TRACE("Leaks:");
    for (int i = 1; i < OBJ_TYPE_MAX_; i++) {
        TRACE("%s = %d", obj_type_to_str(i), type_counts[i]);
    }
    TRACE_NL;
    TRACE("Malformed = %d", malformed);
    TRACE_NL;
}

void lsp_mem_shutdown(lsp_context *c) {
    lsp_mem *m = &c->mem;
    lsp_obj_mark(c->env_top, UNUSED);
    lsp_mem_show_leaks(m);
}

void lsp_shutdown(lsp_context *c) {
    lsp_mem_shutdown(c);
    lsp_context_delete(c);
}

lsp_obj * lsp_obj_alloc(lsp_context *ctx) {
    lsp_obj * o = lsp_mem_get(ctx);
    lsp_obj_set_mark(o, INTERNAL);
    return o;
}

void lsp_obj_set_mark(lsp_obj *o, lsp_mark_type value) {
    o->mark = value;
}

bool lsp_obj_is_internal(lsp_obj *o) {
    return o->mark == INTERNAL;
}

bool lsp_obj_is_unused(lsp_obj *o) {
    return o->mark == UNUSED;
}

bool lsp_string_equal(const char *s1, const char *s2) {
    return strcmp(s1, s2) == 0;
}

bool lsp_num_equal(long int n1, long int n2) {
    return n1 == n2;
}

bool lsp_obj_equal(lsp_obj *o1, lsp_obj *o2) {
    if (o1->type != o2->type)
        return false;

    switch (o1->type) {
    case SYMBOL:
    case STRING:
        return lsp_string_equal(o1->value.str, o2->value.str);
    case NUM:
        return lsp_num_equal(o1->value.num, o2->value.num);
    default:
        break;
    }

    return false;
}

lsp_obj * lsp_obj_copy(lsp_obj *o, lsp_context *ctx) {
    if (lsp_obj_is_nil(o)) {
        return lsp_obj_nil();
    }

    lsp_obj *copy = lsp_obj_alloc(ctx);

    copy->type = o->type;
    copy->value = o->value;
    
    switch (o->type) {
    case NUM:
    case STRING:
    case SYMBOL:
        break;
    case CONS:
        copy->value.con.car = lsp_obj_copy(lsp_car(o), ctx);
        copy->value.con.cdr = lsp_obj_copy(lsp_cdr(o), ctx);
        break;
    case QUOTE:
        copy->value.expr = lsp_obj_copy(o->value.expr, ctx);
        break;
    case LAMBDA:
        copy->value.lambda.args = lsp_obj_copy(o->value.lambda.args, ctx);
        copy->value.lambda.body = lsp_obj_copy(o->value.lambda.body, ctx);
        break;
    case ENV:
    default:
        SHOULD_NEVER_BE_HERE;
    }
    return copy;
}

lsp_obj * lsp_obj_cons(lsp_obj *car, lsp_obj *cdr,
                             lsp_context *ctx) {
    lsp_obj *o = lsp_obj_alloc(ctx);

    o->type = CONS;
    o->value.con.car = car;
    o->value.con.cdr = cdr;
    
    return o;
}

lsp_obj * lsp_env_create(lsp_obj *names, lsp_obj *values,
                               lsp_context *ctx) {
    lsp_obj *o = lsp_obj_alloc(ctx);

    o->type = ENV;

    o->value.env.names = names ? names : lsp_obj_nil();
    o->value.env.values = values ? values : lsp_obj_nil();

    return o;
}

void lsp_env_add(lsp_env *env, lsp_obj *name, lsp_obj *value,
                    lsp_context *ctx) {
    env->names = lsp_obj_cons(name, env->names, ctx);
    env->values = lsp_obj_cons(value, env->values, ctx);
}

lsp_obj * lsp_env_lookup(lsp_obj *env, lsp_obj *name,
                               lsp_context *ctx) {
    if (lsp_obj_is_nil(env)) {
        TRACE("Lookup failed for: %s", lsp_obj_as_string(name));
        return lsp_obj_nil();
    }
     
    lsp_obj *names = lsp_car(env)->value.env.names;
    lsp_obj *values = lsp_car(env)->value.env.values;

    while (! lsp_obj_is_nil(names)) {
        if (lsp_obj_equal(name, lsp_car(names)))
            return lsp_obj_copy(lsp_car(values), ctx);
        
        names = lsp_cdr(names);
        values = lsp_cdr(values);
    }
    return lsp_env_lookup(lsp_cdr(env), name, ctx);
}

lsp_obj * lsp_obj_num(long int num, lsp_context *ctx) {
    lsp_obj *o = lsp_obj_alloc(ctx);
    o->type = NUM;
    o->value.num = num;
    return o;
}

lsp_obj * lsp_obj_string(const char *str, lsp_context *ctx) {
    lsp_obj *o = lsp_obj_alloc(ctx);
    o->type = STRING;
    strncpy(o->value.str, str, LSP_STRING_SIZE);
    return o;
}

lsp_obj * lsp_obj_symbol(const char *str, lsp_context *ctx) {
    lsp_obj *o = lsp_obj_alloc(ctx);
    o->type = SYMBOL;
    strncpy(o->value.str, str, LSP_STRING_SIZE);
    return o;
}

lsp_obj * lsp_obj_quote(lsp_obj *expr, lsp_context *ctx) {
    lsp_obj *o = lsp_obj_alloc(ctx);
    o->type = QUOTE;
    o->value.expr = expr;
    return o;
}

long int lsp_obj_as_num(lsp_obj *o) {
    CHECK(o->type == NUM);
    return o->value.num;
}

const char * lsp_obj_as_string(lsp_obj *o) {
    CHECK(o->type == STRING || o->type == SYMBOL);
    return o->value.str;
}

void lsp_obj_delete(lsp_obj *o) {
    lsp_free(o);
}

char * lsp_eat_space(char *txt) {
    char *pos = txt;
    while (*pos == ' ')
        pos++;
    return pos;
}

lsp_obj *lsp_truth(bool value, lsp_context *ctx) {
    if (value)
        return lsp_obj_symbol("t", ctx);
    else
        return lsp_obj_nil();
}

char lsp_peek(char *txt) {
    return txt[0];
}

bool lsp_is_digit(char c) {
    return c >= 48 && c < 58;
}

lsp_obj * lsp_read_num(char *txt, char **next,
                             lsp_context *ctx) {
    const long int num = strtol(txt, next, 10);
    return lsp_obj_num(num, ctx);
}

char * lsp_make_string(const char *data, int len) {
    char *s = lsp_alloc(sizeof(char) * len + 1);
    memcpy(s, data, len);
    s[len] = '\0';
    return s;
}

lsp_obj * lsp_read_symbol(char *txt, char **next,
                                lsp_context *ctx) {
    size_t span = strcspn(txt, " )");

    /* FIXME: Memory leak */
    char *s = lsp_make_string(txt, span);

    *next = txt + span;
    
    lsp_obj *o = lsp_obj_symbol(s, ctx);
    lsp_free(s);
    return o;
}

lsp_obj * lsp_read_string(char *txt, char **next,
                                lsp_context *ctx) {
    txt++;
    size_t span = strcspn(txt, "\"");
    char *s = lsp_make_string(txt, span);

    *next = txt + span + 1;

    lsp_obj *o = lsp_obj_string(s, ctx);
    free(s);
    return o;
}

lsp_obj * lsp_read_list_inner(char *txt, char **next,
                                    lsp_context *ctx) {
    lsp_obj *car = lsp_read_obj(txt, next, ctx);
    txt = lsp_eat_space(*next);
    
    lsp_obj *cdr = NULL;
    
    if (lsp_peek(txt) == ')') {
        cdr = lsp_obj_nil();
        *next = txt + 1;
    } else {
        cdr = lsp_read_list_inner(txt, next, ctx);
    }

    return lsp_obj_cons(car, cdr, ctx);
}

lsp_obj * lsp_read_list(char *txt, char **next,
                              lsp_context *ctx) {
    txt++;
    txt = lsp_eat_space(txt);

    lsp_obj *res = NULL;
        
    if (lsp_peek(txt) == ')') {
        res = lsp_obj_nil();
        *next = txt + 1;
    } else {
        res = lsp_read_list_inner(txt, next, ctx);
    }
    
    return res;
}


lsp_obj * lsp_read_quote(char *txt, char **next, lsp_context *ctx) {
    txt++;
    lsp_obj *expr = lsp_read_obj(txt, next, ctx);
    return lsp_obj_quote(expr, ctx);
}

lsp_obj * lsp_read_obj(char *txt, char **next,
                             lsp_context *ctx) {
    CHECK(*next != NULL);
    
    txt = lsp_eat_space(txt);
    lsp_obj *obj = NULL;

    char next_char = lsp_peek(txt);
    if (next_char == '(') {
        obj = lsp_read_list(txt, next, ctx);
    } else if (next_char == '\"') {
        obj = lsp_read_string(txt, next, ctx);
    } else if (lsp_is_digit(next_char)) {
        obj = lsp_read_num(txt, next, ctx);
    } else if (next_char == '\'') {
        obj = lsp_read_quote(txt, next, ctx);
    } else {
        obj = lsp_read_symbol(txt, next, ctx);
    }

    return obj;
}

lsp_obj * lsp_read(char *txt, lsp_context *ctx) {
    char *next = "";
    return lsp_read_obj(txt, &next, ctx);
}

char * lsp_print_num(lsp_obj *o, char *buf) {
    int written = sprintf(buf, "%ld", o->value.num);
    return buf + written;
}

char * lsp_print_symbol(lsp_obj *o, char *buf) {
    CHECK(o->type == SYMBOL);
    
    const char *str = o->value.str;
    const size_t len = strlen(str);
    memcpy(buf, str, len);

    return buf + len;
}

char * lsp_print_string(lsp_obj *o, char *buf) {
    CHECK(o->type == STRING);

    int len = sprintf(buf, "\"%s\"", o->value.str);
    return buf + len;
}

char * lsp_print_quote(lsp_obj *o, char *buf) {
    sprintf(buf++, "\'");
    return lsp_print_obj(o->value.expr, buf);
}

char * lsp_print_list(lsp_obj *o, char *buf) {
    sprintf(buf, "(");
    buf++;

    lsp_obj *cur = o;
    while (1) {
        if (lsp_obj_is_nil(cur)) {
            sprintf(buf++, ")");
            break;
        } else if (cur != o) {
            sprintf(buf++, " ");
        }

        if (cur->type != CONS) {
            buf += sprintf(buf, ". ");
            buf = lsp_print_obj(cur, buf);
            cur = lsp_obj_nil();
        } else {
            buf = lsp_print_obj(cur->value.con.car, buf);
            cur = cur->value.con.cdr;
        }
    }
    return buf;
}

char * lsp_print_nil(char *buf) {
    sprintf(buf, "nil");
    return buf + 3;
}

char * lsp_print_lambda(lsp_obj *o, char *buf) {
    return buf + sprintf(buf, "lambda");
}

char * lsp_print_obj(lsp_obj *obj, char *buf) {
    char *next = buf;
    
    switch (obj->type) {
    case STRING:
        next = lsp_print_string(obj, buf);
        break;
    case NUM:
        next = lsp_print_num(obj, buf);
        break;
    case CONS:
        next = lsp_print_list(obj, buf);
        break;
    case NIL:
        next = lsp_print_nil(buf);
        break;
    case SYMBOL:
        next = lsp_print_symbol(obj, buf);
        break;
    case QUOTE:
        next = lsp_print_quote(obj, buf);
        break;
    case LAMBDA:
        next = lsp_print_lambda(obj, buf);
        break;
    default:
        SHOULD_NEVER_BE_HERE;
    }
    return next;
}

char * lsp_print(lsp_obj *obj) {
    static char buf[256];
    char * next = lsp_print_obj(obj, buf);
    *next = '\0';
    return buf;
}


lsp_obj * lsp_eval_quote(lsp_obj *o, lsp_context *ctx) {
    return lsp_obj_copy(o->value.expr, ctx);
}

lsp_obj * lsp_primitive_mul(lsp_obj *args, lsp_context *ctx) {
    lsp_obj *cur = args;
    unsigned int prod = 1;

    while (! lsp_obj_is_nil(cur)) {
        prod = prod * lsp_obj_as_num(lsp_car(cur));
        cur = lsp_cdr(cur);
    }

    return lsp_obj_num(prod, ctx);
}

lsp_obj * lsp_primitive_add(lsp_obj *args, lsp_context *ctx) {
    lsp_obj *cur = args;
    long int sum = 0;
    
    while(! lsp_obj_is_nil(cur)) {
        sum += lsp_obj_as_num(lsp_car(cur));
        cur = lsp_cdr(cur);
    }

    return lsp_obj_num(sum, ctx);
}

lsp_obj * lsp_primitive_sub(lsp_obj *args, lsp_context *ctx) {
    long int sum = lsp_obj_as_num(lsp_car(args));
    lsp_obj *cur = lsp_cdr(args);
    
    while(! lsp_obj_is_nil(cur)) {
        sum -= lsp_obj_as_num(lsp_car(cur));
        cur = lsp_cdr(cur);
    }

    return lsp_obj_num(sum, ctx);
}

lsp_obj * lsp_fallback_proc(lsp_obj *args, lsp_context *ctx) {
    SHOULD_NEVER_BE_HERE;
    return lsp_obj_nil();
}

typedef lsp_obj * (*proc_ptr)(lsp_obj *, lsp_context *);

proc_ptr lsp_get_proc(const char *name) {
    if (strcmp(name, "+") == 0)
        return lsp_primitive_add;
    if (strcmp(name, "-") == 0)
        return lsp_primitive_sub;
    if (strcmp(name, "*") == 0)
        return lsp_primitive_mul;
    
    return lsp_fallback_proc;
}


lsp_obj * lsp_eval_seq(lsp_obj *seq, lsp_context *ctx) {
    if (lsp_obj_is_nil(seq)) {
        return lsp_obj_nil();
    } else {
        return lsp_obj_cons(lsp_eval(lsp_car(seq), ctx),
                               lsp_eval_seq(lsp_cdr(seq), ctx),
                               ctx);
    }
}

bool lsp_is_true(lsp_obj *value) {
    return ! lsp_obj_is_nil(value);
}

lsp_obj * lsp_if(lsp_obj *args,
                       lsp_context *ctx) {
    lsp_obj *pred = lsp_eval(lsp_car(args), ctx);
    
    lsp_obj *then_clause = lsp_car(lsp_cdr(args));
    lsp_obj *else_clause = lsp_car(lsp_cdr(lsp_cdr(args)));

    lsp_obj *res = lsp_obj_nil();
    
    if (lsp_is_true(pred))
        res = lsp_eval(then_clause, ctx);
    else
        res = lsp_eval(else_clause, ctx);

    lsp_obj_mark(pred, UNUSED);
    return res;
}

lsp_obj * lsp_list(lsp_obj *objs, lsp_context *ctx) {
    return lsp_eval_seq(objs, ctx);
}

void lsp_eval_bindings(lsp_obj *bindings, lsp_context *ctx) {
    lsp_obj *cur = bindings;
    while (! lsp_obj_is_nil(cur)) {
        lsp_obj *name = lsp_car(lsp_car(cur));
        lsp_obj *value = lsp_eval(
            lsp_car(lsp_cdr(lsp_car(cur))), ctx);
        lsp_env_add(&(lsp_car(ctx->env_top)->value.env),
                       name, value, ctx);

        cur = lsp_cdr(cur);
    }
}


void lsp_context_push_env(lsp_context *ctx, lsp_obj *env) {
    ctx->env_top = lsp_obj_cons((env ?
                                    env :
                                    lsp_env_create(NULL, NULL, ctx)),
                                   ctx->env_top,
                                   ctx);
}

void lsp_context_pop_env(lsp_context *ctx) {
    lsp_obj *env = ctx->env_top;
    lsp_obj_mark(env, UNUSED);
    ctx->env_top = lsp_cdr(env);
}

lsp_obj * lsp_eval_body(lsp_obj *b, lsp_context *ctx) {
    lsp_obj *cur = b;
    lsp_obj *res = NULL;
    while (! lsp_obj_is_nil(cur)) {
        res = lsp_eval(lsp_car(cur), ctx);
        cur = lsp_cdr(cur);
    }
    return res;
}

lsp_obj * lsp_apply(lsp_obj *proc, lsp_obj * args,
                          lsp_context *ctx) {
    lsp_obj *res = NULL;
    
    if (proc->type == LAMBDA) {
        lsp_context_push_env(ctx,
                                lsp_env_create(proc->value.lambda.args,
                                                  args,
                                                  ctx));
        res = lsp_eval_body(proc->value.lambda.body, ctx);
        lsp_context_pop_env(ctx);
    } else {
        const char *proc_name = lsp_obj_as_string(proc);
        res =  (*lsp_get_proc(proc_name))(args, ctx);
    }
    return res;
}

lsp_obj * lsp_let(lsp_obj *args, lsp_context *ctx) {
    lsp_obj *bindings = lsp_car(args);
    lsp_obj *body = lsp_cdr(args);

    lsp_context_push_env(ctx, NULL);
    lsp_eval_bindings(bindings, ctx);

    lsp_obj *res = lsp_eval_body(body, ctx);

    lsp_context_pop_env(ctx);
    return res;
}

lsp_obj * lsp_set(lsp_obj *name, lsp_obj *value,
                        lsp_context *ctx) {
    lsp_env_add(&lsp_car(ctx->env_top)->value.env, name, value, ctx);
    return value;
}

lsp_obj * lsp_obj_lambda(lsp_obj *o, lsp_context *ctx) {
    lsp_obj *args = lsp_obj_copy(lsp_car(o), ctx);
    lsp_obj *body = lsp_obj_copy(lsp_cdr(o), ctx);

    lsp_obj *l = lsp_obj_alloc(ctx);
    l->type = LAMBDA;
    l->value.lambda.args = args;
    l->value.lambda.body = body;

    return l;
}

lsp_obj * lsp_defun(lsp_obj *o, lsp_context *ctx) {
    lsp_obj *name = lsp_obj_copy(lsp_car(o), ctx);
    lsp_obj *proc = lsp_obj_lambda(lsp_cdr(o), ctx);

    lsp_set(name, proc, ctx);
    return name;
}

#define MAX_FILE_SIZE 10000
char * lsp_load_file(lsp_obj *args, lsp_context *ctx) {
    const char *file_name = lsp_obj_as_string(lsp_car(args));
    static char buf[MAX_FILE_SIZE];

    FILE *fp = fopen(file_name, "r");
    if (fp != NULL) {
        char * cur = buf;
        while (1) {
            size_t len = fread(cur, sizeof(char), 1, fp);

            if (len == 0) {
                *cur = '\0';
                return buf;
            }
            
            if (*cur != '\n' && *cur != '\t') {
                cur++;
            }
        }
    }
    return NULL;
}

lsp_obj * lsp_eval_cons(lsp_obj *o, lsp_context *ctx) {
    lsp_obj *res = NULL;
    const char *op = lsp_obj_as_string(lsp_car(o));
    lsp_obj *args = lsp_cdr(o);
    
    if (lsp_string_equal(op, "if")) {
        res = lsp_if(lsp_cdr(o), ctx);
    } else if (lsp_string_equal(op, "list")) {
        res = lsp_list(lsp_cdr(o), ctx);
    } else if (lsp_string_equal(op, "let")) {
        res = lsp_let(lsp_cdr(o), ctx);
    } else if (lsp_string_equal(op, "set")) {
        lsp_obj *args = lsp_cdr(o);
        lsp_obj *name = lsp_eval(lsp_car(args), ctx);
        lsp_obj *value = lsp_eval(lsp_car(lsp_cdr(args)), ctx);
        res = lsp_set(name, value, ctx);
    } else if (lsp_string_equal(op, "lambda")) {
        res = lsp_obj_lambda(args, ctx);
    } else if (lsp_string_equal(op, "defun")) {
        res = lsp_defun(lsp_cdr(o), ctx);
    } else if (lsp_string_equal(op, "progn")) {
        res = lsp_eval_body(lsp_cdr(o), ctx);
    } else if (lsp_string_equal(op, "cons")) {
        lsp_obj *car = lsp_eval(lsp_car(args), ctx);
        lsp_obj *cdr = lsp_eval(lsp_car(lsp_cdr(args)), ctx);
        res = lsp_obj_cons(car, cdr, ctx);
    } else if (lsp_string_equal(op, "car")) {
        lsp_obj *e = lsp_eval(lsp_car(args), ctx);
        res = lsp_car(e);
        e->value.con.car = lsp_obj_nil();
        lsp_obj_mark(e, UNUSED);
    } else if (lsp_string_equal(op, "cdr")) {
        lsp_obj *e = lsp_eval(lsp_car(args), ctx);
        res = lsp_cdr(e);
        e->value.con.cdr = lsp_obj_nil();
        lsp_obj_mark(e, UNUSED);
    } else if (lsp_string_equal(op, "equal")) {
        lsp_obj *a = lsp_eval(lsp_car(args), ctx);
        lsp_obj *b = lsp_eval(lsp_car(lsp_cdr(args)), ctx);
        res = lsp_truth(lsp_obj_equal(a, b), ctx);
        lsp_obj_mark(a, UNUSED);
        lsp_obj_mark(b, UNUSED);
    } else if (lsp_string_equal(op, "load")) {
        char *code = lsp_load_file(args, ctx);
        lsp_obj *ro = lsp_read(code, ctx);
        lsp_obj *eo = lsp_eval(ro, ctx);
        lsp_obj_mark(ro, UNUSED);
        res = eo;
    } else {
        lsp_obj *proc = lsp_eval(lsp_car(o), ctx);
        lsp_obj *args = lsp_eval_seq(lsp_cdr(o), ctx);
        res = lsp_apply(proc, args, ctx);
        lsp_obj_mark(proc, UNUSED);
        lsp_obj_mark(args, UNUSED);
    }
    return res;
}

lsp_obj * lsp_eval_symbol(lsp_obj *name, lsp_context *ctx) {
    lsp_obj *value = lsp_env_lookup(ctx->env_top, name, ctx);
    return value;
}

lsp_obj * lsp_eval(lsp_obj *expr, lsp_context *ctx) {
    lsp_obj *res = lsp_obj_nil();
    
    switch (expr->type) {
    case SYMBOL:
        res = lsp_eval_symbol(expr, ctx);
        break;
    case STRING:
    case NUM:
        res = expr;
        break;
    case CONS:
        if (! lsp_obj_is_nil(expr))
            res = lsp_eval_cons(expr, ctx);
        break;
    case NIL:
        res = lsp_obj_nil();
        break;
    case QUOTE:
        res = lsp_eval_quote(expr, ctx);
        break;
    default:
        SHOULD_NEVER_BE_HERE;
    }
    return res;
}
