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

typedef struct scheme_context {
    scheme_obj *env_top;
} scheme_context;


static scheme_context * scheme_context_create() {
    scheme_context *c = scheme_alloc(sizeof(scheme_context));

    c->env_top = scheme_env_create(NULL, NULL);
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

typedef struct scheme_env {
    scheme_obj *parent_env;
    scheme_obj *names;
    scheme_obj *values;
} scheme_env;

typedef struct scheme_cons {
    scheme_obj *car;
    scheme_obj *cdr;
} scheme_cons;

enum scheme_obj_type {SYMBOL, STRING, NUM, CONS, QUOTE, ENV};

typedef struct scheme_obj {
    enum scheme_obj_type type;
    union {
        const char *str;
        long int num;
        scheme_cons *con;
        scheme_env env;
        scheme_obj *expr;
    } value;
} scheme_obj;

scheme_obj * scheme_obj_alloc() {
    return scheme_alloc(sizeof(scheme_obj));
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
    scheme_obj *o = scheme_alloc(sizeof(scheme_obj));
    o->type = CONS;
    o->value.con = NULL;
    
    return o;
}

bool scheme_obj_is_nil(scheme_obj *o) {
    return o->type == CONS && o->value.con == NULL;
}

scheme_obj * scheme_car(scheme_obj *cons) {
    if (scheme_obj_is_nil(cons))
        return scheme_obj_nil();
    else
        return cons->value.con->car;
}

scheme_obj * scheme_cdr(scheme_obj *cons) {
    if (scheme_obj_is_nil(cons))
        return scheme_obj_nil();
    else
        return cons->value.con->cdr;
}

scheme_obj * scheme_obj_cons(scheme_obj *car, scheme_obj *cdr) {
    scheme_cons *c = scheme_alloc(sizeof(scheme_cons));
    c->car = car;
    c->cdr = cdr;

    scheme_obj *o = scheme_alloc(sizeof(scheme_obj));
    o->type = CONS;
    o->value.con = c;
    
    return o;
}

void scheme_context_set_env(scheme_context *c, scheme_obj *e) {
    c->env_top = e;
}
    
scheme_obj * scheme_env_create(scheme_obj *names, scheme_obj *values) {
    scheme_obj *o = scheme_obj_alloc();

    o->type = ENV;

    o->value.env.parent_env = scheme_obj_nil();
    o->value.env.names = names ? names : scheme_obj_nil();
    o->value.env.values = values ? values : scheme_obj_nil();

    return o;
}

void scheme_env_add(scheme_env *env, scheme_obj *name, scheme_obj *value) {
    env->names = scheme_obj_cons(name, env->names);
    env->values = scheme_obj_cons(value, env->values);
}

scheme_obj * scheme_env_lookup(scheme_obj *env, scheme_obj *name) {
    scheme_obj *names = env->value.env.names;
    scheme_obj *values = env->value.env.values;

    while (! scheme_obj_is_nil(names)) {
        if (scheme_obj_equal(name, scheme_car(names)))
            return scheme_car(values);
        
        names = scheme_cdr(names);
        values = scheme_cdr(values);
    }

    TRACE("Lookup failed for: %s", scheme_obj_as_string(name));
    return scheme_obj_nil();     
}

scheme_obj * scheme_obj_num(long int num) {
    scheme_obj *o = scheme_alloc(sizeof(scheme_obj));
    o->type = NUM;
    o->value.num = num;
    return o;
}

scheme_obj * scheme_obj_string(const char *str) {
    scheme_obj *o = scheme_alloc(sizeof(scheme_obj));
    o->type = STRING;
    o->value.str = str;
    return o;
}

scheme_obj * scheme_obj_symbol(const char *str) {
    scheme_obj *o = scheme_alloc(sizeof(scheme_obj));
    o->type = SYMBOL;
    o->value.str = str;
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

scheme_obj * scheme_read_num(char *txt, char **next) {
    const long int num = strtol(txt, next, 10);
    return scheme_obj_num(num);
}

char * scheme_make_string(const char *data, int len) {
    char *s = scheme_alloc(sizeof(char) * len + 1);
    memcpy(s, data, len);
    s[len] = '\0';
    return s;
}

scheme_obj * scheme_read_symbol(char *txt, char **next) {
    size_t span = strcspn(txt, " )");
    char *s = scheme_make_string(txt, span);

    *next = txt + span;
    
    return scheme_obj_symbol(s);
}

scheme_obj * scheme_read_string(char *txt, char **next) {
    txt++;
    size_t span = strcspn(txt, "\"");
    char *s = scheme_make_string(txt, span);

    *next = txt + span + 1;

    return scheme_obj_string(s);
}

scheme_obj * scheme_read_list_inner(char *txt, char **next) {
    scheme_obj *car = scheme_read_obj(txt, next);
    txt = scheme_eat_space(*next);
    
    scheme_obj *cdr = NULL;
    
    if (scheme_peek(txt) == ')') {
        cdr = scheme_obj_nil();
        *next = txt + 1;
    } else {
        cdr = scheme_read_list_inner(txt, next);
    }

    return scheme_obj_cons(car, cdr);
}

scheme_obj * scheme_read_list(char *txt, char **next) {
    txt++;
    txt = scheme_eat_space(txt);

    scheme_obj *res = NULL;
        
    if (scheme_peek(txt) == ')') {
        res = scheme_obj_nil();
        *next = txt + 1;
    } else {
        res = scheme_read_list_inner(txt, next);
    }
    
    return res;
}


scheme_obj * scheme_read_quote(char *txt, char **next) {
    txt++;
    scheme_obj *expr = scheme_read_obj(txt, next);
    return scheme_obj_quote(expr);
}

scheme_obj * scheme_read_obj(char *txt, char **next) {
    CHECK(*next != NULL);
    
    txt = scheme_eat_space(txt);
    scheme_obj *obj = NULL;

    char next_char = scheme_peek(txt);
    if (next_char == '(') {
        obj = scheme_read_list(txt, next);
    } else if (next_char == '\"') {
        obj = scheme_read_string(txt, next);
    } else if (scheme_is_digit(next_char)) {
        obj = scheme_read_num(txt, next);
    } else if (next_char == '\'') {
        obj = scheme_read_quote(txt, next);
    } else {
        obj = scheme_read_symbol(txt, next);
    }

    return obj;
}

scheme_obj * scheme_read(char *txt) {
    char *next = "";
    return scheme_read_obj(txt, &next);
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

        buf = scheme_print_obj(cur->value.con->car, buf);
        cur = cur->value.con->cdr;
    }
    return buf;
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

scheme_obj * scheme_primitive_mul(scheme_obj *args) {
    scheme_obj *cur = args;
    unsigned int prod = 1;

    while (! scheme_obj_is_nil(cur)) {
        prod = prod * scheme_obj_as_num(scheme_car(cur));
        cur = scheme_cdr(cur);
    }

    return scheme_obj_num(prod);
}

scheme_obj * scheme_primitive_add(scheme_obj *args) {
    scheme_obj *cur = args;
    long int sum = 0;
    
    while(! scheme_obj_is_nil(cur)) {
        sum += scheme_obj_as_num(scheme_car(cur));
        cur = scheme_cdr(cur);
    }

    return scheme_obj_num(sum);
}

scheme_obj * scheme_primitive_sub(scheme_obj *args) {
    long int sum = scheme_obj_as_num(scheme_car(args));
    scheme_obj *cur = scheme_cdr(args);
    
    while(! scheme_obj_is_nil(cur)) {
        sum -= scheme_obj_as_num(scheme_car(cur));
        cur = scheme_cdr(cur);
    }

    return scheme_obj_num(sum);
}

scheme_obj * scheme_fallback_proc(scheme_obj *args) {
    SHOULD_NEVER_BE_HERE;
    return scheme_obj_nil();
}

typedef scheme_obj * (*proc_ptr)(scheme_obj *);

proc_ptr scheme_get_proc(const char *name) {
    if (strcmp(name, "+") == 0)
        return scheme_primitive_add;
    if (strcmp(name, "-") == 0)
        return scheme_primitive_sub;
    if (strcmp(name, "*") == 0)
        return scheme_primitive_mul;
    
    return scheme_fallback_proc;
}

scheme_obj * scheme_apply(scheme_obj *proc, scheme_obj * args) {
    const char *proc_name = scheme_obj_as_string(proc);
    return (*scheme_get_proc(proc_name))(args);
}

scheme_obj * scheme_eval_seq(scheme_obj *seq, scheme_context *ctx) {
    if (scheme_obj_is_nil(seq)) {
        return scheme_obj_nil();
    } else {
        return scheme_obj_cons(scheme_eval(scheme_car(seq), ctx),
                               scheme_eval_seq(scheme_cdr(seq), ctx));
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

scheme_obj * scheme_eval_cons(scheme_obj *o, scheme_context *ctx) {
    scheme_obj *res = NULL;
    const char *op = scheme_obj_as_string(scheme_car(o));
    if (scheme_string_equal(op, "if")) {
        res = scheme_if(scheme_cdr(o), ctx);
    } else if (scheme_string_equal(op, "list")) {
        res = scheme_list(scheme_cdr(o), ctx);
    } else {
        scheme_obj *proc = scheme_eval(scheme_car(o), ctx);
        scheme_obj *args = scheme_eval_seq(scheme_cdr(o), ctx);
        res = scheme_apply(proc, args);
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
    case QUOTE:
        res = scheme_eval_quote(expr, ctx);
        break;
    default:
        SHOULD_NEVER_BE_HERE;
    }
    return res;
}
