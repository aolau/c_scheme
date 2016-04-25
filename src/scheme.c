#include "scheme.h"
#include "check.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void * scheme_alloc(size_t size) {
    return malloc(size);
}

static void scheme_free(void *mem) {
    free(mem);
}

typedef struct scheme_env {
    scheme_obj *parent_env;
    scheme_obj *names;
    scheme_obj *values;
} scheme_env;

typedef struct scheme_cons {
    scheme_obj *car;
    scheme_obj *cdr;
} scheme_cons;

enum scheme_obj_type {NIL, SYMBOL, STRING, NUM, CONS, QUOTE, ENV};

#define SCHEME_STRING_SIZE 32

typedef struct scheme_obj {
    enum scheme_obj_type type;
    bool mark;
    union {
        char str[SCHEME_STRING_SIZE];
        long int num;
        scheme_cons con;
        scheme_env env;
        scheme_obj *expr;
    } value;
} scheme_obj;

static scheme_obj static_obj_nil = {.type = NIL};

#define SCHEME_HEAP_SIZE 10000

typedef struct free_obj {
    struct free_obj *next;
} free_obj;


typedef struct scheme_mem {
    scheme_obj heap[SCHEME_HEAP_SIZE];
    free_obj *free_list;
} scheme_mem;

typedef struct scheme_context {
    scheme_obj *env_top;
    scheme_mem mem;
} scheme_context;

void scheme_mem_free(scheme_mem *m, scheme_obj *o) {
    free_obj *fo = (free_obj*)o;
    fo->next = m->free_list;
    m->free_list = fo;
}

scheme_obj * scheme_mem_alloc(scheme_mem *m) {
    free_obj *fo = m->free_list;
    m->free_list = fo->next;

    memset(fo, 0, sizeof(scheme_obj));
    
    return (scheme_obj *)fo;
}

void scheme_obj_set_mark(scheme_obj *o, bool value);
bool scheme_obj_is_marked(scheme_obj *o);

void scheme_mem_mark_all(scheme_mem *m) {
    TRACE("Marking all memory...");
    for (int i = 0; i < SCHEME_HEAP_SIZE; i++) {
        scheme_obj_set_mark(&(m->heap[i]), true);
    }
}

void scheme_mem_collect(scheme_mem *m) {
    CHECK(m->free_list == NULL);

    TRACE("Collecting garbage...");
    int count = 0;
    for (int i = 0; i < SCHEME_HEAP_SIZE; i++) {
        if (scheme_obj_is_marked(&(m->heap[i]))) {
            scheme_mem_free(m, &(m->heap[i]));
            count++;
        }
    }
    TRACE("%d objects freed", count);
}

bool scheme_mem_no_free(scheme_mem *m) {
    return m->free_list == NULL;
}

void scheme_mem_mark_unused(scheme_context *ctx) {
    
}

scheme_obj *scheme_mem_get(scheme_context *ctx) {
    scheme_mem *m = &ctx->mem;
    
    if (scheme_mem_no_free(m)) {
        scheme_mem_mark_unused(ctx);
        scheme_mem_collect(m);
    }

    CHECK(scheme_mem_no_free(m) == false);

    return scheme_mem_alloc(m);
}

void scheme_mem_init(scheme_mem *m) {
    TRACE("Initializing heap...");

    m->free_list = NULL;
    scheme_mem_mark_all(m);
    scheme_mem_collect(m);
}

void scheme_context_push_env(scheme_context *ctx, scheme_obj *env);

static scheme_context * scheme_context_create() {
    scheme_context *c = scheme_alloc(sizeof(scheme_context));
    scheme_mem_init(&c->mem);
    
    scheme_context_push_env(c, NULL);
    return c;
}

static void scheme_context_delete(scheme_context *c) {
    scheme_free(c);
}

scheme_context * scheme_init() {
    return scheme_context_create();
}

void scheme_shutdown(scheme_context *c) {
    scheme_context_delete(c);
}


scheme_obj * scheme_obj_alloc(scheme_context *ctx) {
    return scheme_mem_get(ctx);
}

void scheme_obj_set_mark(scheme_obj *o, bool value) {
    o->mark = value;
}

bool scheme_obj_is_marked(scheme_obj *o) {
    return o->mark;
}

bool scheme_string_equal(const char *s1, const char *s2) {
    return strcmp(s1, s2) == 0;
}

bool scheme_obj_equal(scheme_obj *o1, scheme_obj *o2) {
    if (o1->type != o2->type)
        return false;

    switch (o1->type) {
    case SYMBOL:
    case STRING:
        return scheme_string_equal(o1->value.str, o2->value.str);
    default:
        break;
    }

    return false;
}

scheme_obj * scheme_obj_nil() {
    return &static_obj_nil;
}

bool scheme_obj_is_nil(scheme_obj *o) {
    return o == &static_obj_nil;
}

scheme_obj * scheme_car(scheme_obj *cons) {
    if (scheme_obj_is_nil(cons))
        return scheme_obj_nil();
    else
        return cons->value.con.car;
}

scheme_obj * scheme_cdr(scheme_obj *cons) {
    if (scheme_obj_is_nil(cons))
        return scheme_obj_nil();
    else
        return cons->value.con.cdr;
}

scheme_obj * scheme_obj_cons(scheme_obj *car, scheme_obj *cdr,
                             scheme_context *ctx) {
    scheme_obj *o = scheme_obj_alloc(ctx);

    o->type = CONS;
    o->value.con.car = car;
    o->value.con.cdr = cdr;
    
    return o;
}

scheme_obj * scheme_env_create(scheme_obj *names, scheme_obj *values,
                               scheme_context *ctx) {
    scheme_obj *o = scheme_obj_alloc(ctx);

    o->type = ENV;

    o->value.env.parent_env = scheme_obj_nil(ctx);
    o->value.env.names = names ? names : scheme_obj_nil(ctx);
    o->value.env.values = values ? values : scheme_obj_nil(ctx);

    return o;
}

void scheme_env_add(scheme_env *env, scheme_obj *name, scheme_obj *value,
                    scheme_context *ctx) {
    env->names = scheme_obj_cons(name, env->names, ctx);
    env->values = scheme_obj_cons(value, env->values, ctx);
}

scheme_obj * scheme_env_lookup(scheme_obj *env, scheme_obj *name) {
    if (scheme_obj_is_nil(env)) {
        TRACE("Lookup failed for: %s", scheme_obj_as_string(name));
        return scheme_obj_nil();
    }
     
    scheme_obj *names = scheme_car(env)->value.env.names;
    scheme_obj *values = scheme_car(env)->value.env.values;

    while (! scheme_obj_is_nil(names)) {
        if (scheme_obj_equal(name, scheme_car(names)))
            return scheme_car(values);
        
        names = scheme_cdr(names);
        values = scheme_cdr(values);
    }
    return scheme_env_lookup(scheme_cdr(env), name);
}

scheme_obj * scheme_obj_num(long int num, scheme_context *ctx) {
    scheme_obj *o = scheme_obj_alloc(ctx);
    o->type = NUM;
    o->value.num = num;
    return o;
}

scheme_obj * scheme_obj_string(const char *str, scheme_context *ctx) {
    scheme_obj *o = scheme_obj_alloc(ctx);
    o->type = STRING;
    strncpy(o->value.str, str, SCHEME_STRING_SIZE);
    return o;
}

scheme_obj * scheme_obj_symbol(const char *str, scheme_context *ctx) {
    scheme_obj *o = scheme_obj_alloc(ctx);
    o->type = SYMBOL;
    strncpy(o->value.str, str, SCHEME_STRING_SIZE);
    return o;
}

scheme_obj * scheme_obj_quote(scheme_obj *expr) {
    scheme_obj *o = scheme_alloc(sizeof(scheme_obj));
    o->type = QUOTE;
    o->value.expr = expr;
    return o;
}

long int scheme_obj_as_num(scheme_obj *o) {
    CHECK(o->type == NUM);
    return o->value.num;
}

const char * scheme_obj_as_string(scheme_obj *o) {
    CHECK(o->type == STRING || o->type == SYMBOL);
    return o->value.str;
}

void scheme_obj_delete(scheme_obj *o) {
    scheme_free(o);
}

char * scheme_eat_space(char *txt) {
    char *pos = txt;
    while (*pos == ' ')
        pos++;
    return pos;
}

char scheme_peek(char *txt) {
    return txt[0];
}

bool scheme_is_digit(char c) {
    return c >= 48 && c < 58;
}

scheme_obj * scheme_read_num(char *txt, char **next,
                             scheme_context *ctx) {
    const long int num = strtol(txt, next, 10);
    return scheme_obj_num(num, ctx);
}

char * scheme_make_string(const char *data, int len) {
    char *s = scheme_alloc(sizeof(char) * len + 1);
    memcpy(s, data, len);
    s[len] = '\0';
    return s;
}

scheme_obj * scheme_read_symbol(char *txt, char **next,
                                scheme_context *ctx) {
    size_t span = strcspn(txt, " )");
    char *s = scheme_make_string(txt, span);

    *next = txt + span;
    
    return scheme_obj_symbol(s, ctx);
}

scheme_obj * scheme_read_string(char *txt, char **next,
                                scheme_context *ctx) {
    txt++;
    size_t span = strcspn(txt, "\"");
    char *s = scheme_make_string(txt, span);

    *next = txt + span + 1;

    return scheme_obj_string(s, ctx);
}

scheme_obj * scheme_read_list_inner(char *txt, char **next,
                                    scheme_context *ctx) {
    scheme_obj *car = scheme_read_obj(txt, next, ctx);
    txt = scheme_eat_space(*next);
    
    scheme_obj *cdr = NULL;
    
    if (scheme_peek(txt) == ')') {
        cdr = scheme_obj_nil();
        *next = txt + 1;
    } else {
        cdr = scheme_read_list_inner(txt, next, ctx);
    }

    return scheme_obj_cons(car, cdr, ctx);
}

scheme_obj * scheme_read_list(char *txt, char **next,
                              scheme_context *ctx) {
    txt++;
    txt = scheme_eat_space(txt);

    scheme_obj *res = NULL;
        
    if (scheme_peek(txt) == ')') {
        res = scheme_obj_nil();
        *next = txt + 1;
    } else {
        res = scheme_read_list_inner(txt, next, ctx);
    }
    
    return res;
}


scheme_obj * scheme_read_quote(char *txt, char **next, scheme_context *ctx) {
    txt++;
    scheme_obj *expr = scheme_read_obj(txt, next, ctx);
    return scheme_obj_quote(expr);
}

scheme_obj * scheme_read_obj(char *txt, char **next,
                             scheme_context *ctx) {
    CHECK(*next != NULL);
    
    txt = scheme_eat_space(txt);
    scheme_obj *obj = NULL;

    char next_char = scheme_peek(txt);
    if (next_char == '(') {
        obj = scheme_read_list(txt, next, ctx);
    } else if (next_char == '\"') {
        obj = scheme_read_string(txt, next, ctx);
    } else if (scheme_is_digit(next_char)) {
        obj = scheme_read_num(txt, next, ctx);
    } else if (next_char == '\'') {
        obj = scheme_read_quote(txt, next, ctx);
    } else {
        obj = scheme_read_symbol(txt, next, ctx);
    }

    return obj;
}

scheme_obj * scheme_read(char *txt, scheme_context *ctx) {
    char *next = "";
    return scheme_read_obj(txt, &next, ctx);
}

char * scheme_print_num(scheme_obj *o, char *buf) {
    int written = sprintf(buf, "%ld", o->value.num);
    return buf + written;
}

char * scheme_print_symbol(scheme_obj *o, char *buf) {
    CHECK(o->type == SYMBOL);
    
    const char *str = o->value.str;
    const size_t len = strlen(str);
    memcpy(buf, str, len);

    return buf + len;
}

char * scheme_print_string(scheme_obj *o, char *buf) {
    CHECK(o->type == STRING);

    int len = sprintf(buf, "\"%s\"", o->value.str);
    return buf + len;
}

char * scheme_print_quote(scheme_obj *o, char *buf) {
    sprintf(buf++, "\'");
    return scheme_print_obj(o->value.expr, buf);
}

char * scheme_print_list(scheme_obj *o, char *buf) {
    sprintf(buf, "(");
    buf++;

    scheme_obj *cur = o;
    while (1) {
        if (scheme_obj_is_nil(cur)) {
            sprintf(buf++, ")");
            break;
        } else if (cur != o) {
            sprintf(buf++, " ");
        }

        buf = scheme_print_obj(cur->value.con.car, buf);
        cur = cur->value.con.cdr;
    }
    return buf;
}

char * scheme_print_nil(char *buf) {
    sprintf(buf, "nil");
    return buf + 3;
}

char * scheme_print_obj(scheme_obj *obj, char *buf) {
    char *next = buf;
    
    switch (obj->type) {
    case STRING:
        next = scheme_print_string(obj, buf);
        break;
    case NUM:
        next = scheme_print_num(obj, buf);
        break;
    case CONS:
        next = scheme_print_list(obj, buf);
        break;
    case NIL:
        next = scheme_print_nil(buf);
        break;
    case SYMBOL:
        next = scheme_print_symbol(obj, buf);
        break;
    case QUOTE:
        next = scheme_print_quote(obj, buf);
        break;
    default:
        SHOULD_NEVER_BE_HERE;
    }
    return next;
}

char * scheme_print(scheme_obj *obj) {
    static char buf[256];
    char * next = scheme_print_obj(obj, buf);
    *next = '\0';
    return buf;
}

scheme_obj * scheme_eval_quote(scheme_obj *o, scheme_context *ctx) {
    return scheme_eval(o->value.expr, ctx);
}

scheme_obj * scheme_primitive_mul(scheme_obj *args, scheme_context *ctx) {
    scheme_obj *cur = args;
    unsigned int prod = 1;

    while (! scheme_obj_is_nil(cur)) {
        prod = prod * scheme_obj_as_num(scheme_car(cur));
        cur = scheme_cdr(cur);
    }

    return scheme_obj_num(prod, ctx);
}

scheme_obj * scheme_primitive_add(scheme_obj *args, scheme_context *ctx) {
    scheme_obj *cur = args;
    long int sum = 0;
    
    while(! scheme_obj_is_nil(cur)) {
        sum += scheme_obj_as_num(scheme_car(cur));
        cur = scheme_cdr(cur);
    }

    return scheme_obj_num(sum, ctx);
}

scheme_obj * scheme_primitive_sub(scheme_obj *args, scheme_context *ctx) {
    long int sum = scheme_obj_as_num(scheme_car(args));
    scheme_obj *cur = scheme_cdr(args);
    
    while(! scheme_obj_is_nil(cur)) {
        sum -= scheme_obj_as_num(scheme_car(cur));
        cur = scheme_cdr(cur);
    }

    return scheme_obj_num(sum, ctx);
}

scheme_obj * scheme_fallback_proc(scheme_obj *args, scheme_context *ctx) {
    SHOULD_NEVER_BE_HERE;
    return scheme_obj_nil();
}

typedef scheme_obj * (*proc_ptr)(scheme_obj *, scheme_context *);

proc_ptr scheme_get_proc(const char *name) {
    if (strcmp(name, "+") == 0)
        return scheme_primitive_add;
    if (strcmp(name, "-") == 0)
        return scheme_primitive_sub;
    if (strcmp(name, "*") == 0)
        return scheme_primitive_mul;
    
    return scheme_fallback_proc;
}

scheme_obj * scheme_apply(scheme_obj *proc, scheme_obj * args,
                          scheme_context *ctx) {
    const char *proc_name = scheme_obj_as_string(proc);
    return (*scheme_get_proc(proc_name))(args, ctx);
}

scheme_obj * scheme_eval_seq(scheme_obj *seq, scheme_context *ctx) {
    if (scheme_obj_is_nil(seq)) {
        return scheme_obj_nil();
    } else {
        return scheme_obj_cons(scheme_eval(scheme_car(seq), ctx),
                               scheme_eval_seq(scheme_cdr(seq), ctx),
                               ctx);
    }
}

bool scheme_is_true(scheme_obj *value) {
    return ! scheme_obj_is_nil(value);
}

scheme_obj * scheme_if(scheme_obj *args,
                       scheme_context *ctx) {
    scheme_obj *pred = scheme_car(args);
    scheme_obj *then_clause = scheme_car(scheme_cdr(args));
    scheme_obj *else_clause = scheme_car(scheme_cdr(scheme_cdr(args)));
    
    if (scheme_is_true(scheme_eval(pred, ctx)))
        return scheme_eval(then_clause, ctx);
    else
        return scheme_eval(else_clause, ctx);
}

scheme_obj * scheme_list(scheme_obj *objs, scheme_context *ctx) {
    return scheme_eval_seq(objs, ctx);
}

void scheme_eval_bindings(scheme_obj *bindings, scheme_context *ctx) {
    scheme_obj *cur = bindings;
    while (! scheme_obj_is_nil(cur)) {
        scheme_obj *name = scheme_car(scheme_car(cur));
        scheme_obj *value = scheme_eval(
            scheme_car(scheme_cdr(scheme_car(cur))), ctx);
        scheme_env_add(&(scheme_car(ctx->env_top)->value.env),
                       name, value, ctx);

        cur = scheme_cdr(cur);
    }
}


void scheme_context_push_env(scheme_context *ctx, scheme_obj *env) {
    ctx->env_top = scheme_obj_cons(env ? env :
                                   scheme_env_create(NULL, NULL, ctx),
                                   ctx->env_top,
                                   ctx);
}

void scheme_context_pop_env(scheme_context *ctx) {
    ctx->env_top = scheme_cdr(ctx->env_top);
}

scheme_obj * scheme_let(scheme_obj *args, scheme_context *ctx) {
    scheme_obj *bindings = scheme_car(args);
    scheme_obj *body = scheme_cdr(args);

    scheme_context_push_env(ctx, NULL);
    scheme_eval_bindings(bindings, ctx);

    scheme_obj *cur = body;
    scheme_obj *res = NULL;
    while (! scheme_obj_is_nil(cur)) {
        res = scheme_eval(scheme_car(cur), ctx);
        cur = scheme_cdr(cur);
    }
    scheme_context_pop_env(ctx);
    return res;
}

scheme_obj * scheme_eval_cons(scheme_obj *o, scheme_context *ctx) {
    scheme_obj *res = NULL;
    const char *op = scheme_obj_as_string(scheme_car(o));
    if (scheme_string_equal(op, "if")) {
        res = scheme_if(scheme_cdr(o), ctx);
    } else if (scheme_string_equal(op, "list")) {
        res = scheme_list(scheme_cdr(o), ctx);
    } else if (scheme_string_equal(op, "let")) {
        res = scheme_let(scheme_cdr(o), ctx);
    } else {
        scheme_obj *proc = scheme_eval(scheme_car(o), ctx);
        scheme_obj *args = scheme_eval_seq(scheme_cdr(o), ctx);
        res = scheme_apply(proc, args, ctx);
    }
    return res;
}

scheme_obj * scheme_eval_symbol(scheme_obj *name, scheme_context *ctx) {
    scheme_obj *value = scheme_env_lookup(ctx->env_top, name);
    if (scheme_obj_is_nil(value))
        return name; /* assume primitive proc for now */
    return value;
}

scheme_obj * scheme_eval(scheme_obj *expr, scheme_context *ctx) {
    scheme_obj *res = scheme_obj_nil();
    
    switch (expr->type) {
    case SYMBOL:
        res = scheme_eval_symbol(expr, ctx);
        break;
    case STRING:
    case NUM:
        res = expr;
        break;
    case CONS:
        if (! scheme_obj_is_nil(expr))
            res = scheme_eval_cons(expr, ctx);
        break;
    case NIL:
        res = scheme_obj_nil();
        break;
    case QUOTE:
        res = scheme_eval_quote(expr, ctx);
        break;
    default:
        SHOULD_NEVER_BE_HERE;
    }
    return res;
}
