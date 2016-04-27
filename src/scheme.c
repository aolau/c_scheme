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
    scheme_obj *names;
    scheme_obj *values;
} scheme_env;

typedef struct scheme_cons {
    scheme_obj *car;
    scheme_obj *cdr;
} scheme_cons;

enum scheme_obj_type {FREELIST, NIL, SYMBOL, STRING, NUM, CONS, QUOTE, ENV,
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
        "UNDEFINED"
    };

    return str[t];
}

#define SCHEME_STRING_SIZE 32

typedef struct scheme_obj {
    enum scheme_obj_type type;
    scheme_mark_type mark;
    scheme_obj *next;
    union {
        char str[SCHEME_STRING_SIZE];
        long int num;
        scheme_cons con;
        scheme_env env;
        scheme_obj *expr;
    } value;
} scheme_obj;

static scheme_obj static_obj_nil = {.type = NIL};

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

#define SCHEME_HEAP_SIZE 10000

typedef struct scheme_mem {
    scheme_obj heap[SCHEME_HEAP_SIZE];
    scheme_obj *free_list;
} scheme_mem;

typedef struct scheme_context {
    scheme_obj *env_top;
    scheme_mem mem;
} scheme_context;

void scheme_mem_free(scheme_mem *m, scheme_obj *o) {
    memset(o, 0, sizeof(scheme_obj));
    
    o->next = m->free_list;
    m->free_list = o;
}

scheme_obj * scheme_mem_alloc(scheme_mem *m) {
    scheme_obj *o = m->free_list;
    m->free_list = o->next;
    
    return o;
}


bool scheme_obj_is_unused(scheme_obj *o);
bool scheme_obj_is_internal(scheme_obj *o);
void scheme_obj_set_mark(scheme_obj *o, scheme_mark_type value);

void scheme_mem_unmark_all_(scheme_mem *m, bool force) {
    TRACE("Unmarking all memory...");
    for (int i = 0; i < SCHEME_HEAP_SIZE; i++) {
        if (force || (! scheme_obj_is_internal(&m->heap[i])))
            scheme_obj_set_mark(&(m->heap[i]), UNUSED);
    }
}

void scheme_mem_force_unmark_all(scheme_mem *m) {
    scheme_mem_unmark_all_(m, true);
}

void scheme_mem_unmark_all(scheme_mem *m) {
    scheme_mem_unmark_all_(m, false);
}

void scheme_mem_collect(scheme_mem *m) {
    CHECK(m->free_list == NULL);

    TRACE("Collecting garbage...");
    int count = 0;
    for (int i = 0; i < SCHEME_HEAP_SIZE; i++) {
        if (scheme_obj_is_unused(&(m->heap[i]))) {
            scheme_mem_free(m, &(m->heap[i]));
            count++;
        }
    }
    TRACE("%d objects freed", count);
}

bool scheme_mem_no_free(scheme_mem *m) {
    return m->free_list == NULL;
}

int scheme_obj_mark(scheme_obj *o, scheme_mark_type mark) {
    int marked = 0;
    if (! scheme_obj_is_nil(o)) {
        scheme_obj_set_mark(o, mark);
        marked++;
    }

    switch (o->type) {
    case STRING:
    case NUM:
    case SYMBOL:
    case NIL:
        break;
    case CONS:
        marked += scheme_obj_mark(scheme_car(o), mark);
        marked += scheme_obj_mark(scheme_cdr(o), mark);
        break;
    case ENV:
        marked += scheme_obj_mark(o->value.env.names, mark);
        marked += scheme_obj_mark(o->value.env.values, mark);
        break;
    case QUOTE:
        marked += scheme_obj_mark(o->value.expr, mark);
        break;
    default:
        SHOULD_NEVER_BE_HERE;
    }
    return marked;
}

int scheme_mem_mark_used(scheme_context *ctx) {
    scheme_obj *env = ctx->env_top;
    int marked = scheme_obj_mark(env, EXTERNAL);

    TRACE("%d objects in use", marked);
}

scheme_obj *scheme_mem_get(scheme_context *ctx) {
    scheme_mem *m = &ctx->mem;
    
    if (scheme_mem_no_free(m)) {
        TRACE_NL;
        scheme_mem_unmark_all(m);
        scheme_mem_mark_used(ctx);
        scheme_mem_collect(m);
    }

    CHECK(scheme_mem_no_free(m) == false);

    return scheme_mem_alloc(m);
}

void scheme_mem_init(scheme_mem *m) {
    TRACE("Initializing heap...");

    m->free_list = NULL;
    scheme_mem_force_unmark_all(m);
    scheme_mem_collect(m);
}

void scheme_context_push_env(scheme_context *ctx, scheme_obj *env);

static scheme_context * scheme_context_create() {
    scheme_context *c = scheme_alloc(sizeof(scheme_context));
    scheme_mem_init(&c->mem);

    c->env_top = scheme_obj_nil();
    scheme_context_push_env(c, NULL);
    return c;
}

static void scheme_context_delete(scheme_context *c) {
    scheme_free(c);
}

scheme_context * scheme_init() {
    return scheme_context_create();
}

void scheme_mem_show_leaks(scheme_mem *m) {
    int type_counts[OBJ_TYPE_MAX_] = {0};
    int malformed = 0;
    for (int i = 0; i < SCHEME_HEAP_SIZE; i++) {
        scheme_obj *o = &m->heap[i];
        if (o->type >= OBJ_TYPE_MAX_ || o->type < 0) {
            malformed++;
        } else if (! scheme_obj_is_unused(o)) {
            type_counts[o->type]++;
        }
    }

    TRACE("GC stats");
    TRACE("Leaks:");
    for (int i = 1; i < OBJ_TYPE_MAX_; i++) {
        TRACE("%s = %d", obj_type_to_str(i), type_counts[i]);
    }
    TRACE("Malformed = %d", malformed);
    TRACE_NL;
}

void scheme_mem_shutdown(scheme_context *c) {
    scheme_mem *m = &c->mem;
    scheme_obj_mark(c->env_top, UNUSED);
    scheme_mem_show_leaks(m);
}

void scheme_shutdown(scheme_context *c) {
    scheme_mem_shutdown(c);
    scheme_context_delete(c);
}

scheme_obj * scheme_obj_alloc(scheme_context *ctx) {
    scheme_obj * o = scheme_mem_get(ctx);
    scheme_obj_set_mark(o, INTERNAL);
    return o;
}

void scheme_obj_set_mark(scheme_obj *o, scheme_mark_type value) {
    o->mark = value;
}

bool scheme_obj_is_internal(scheme_obj *o) {
    return o->mark == INTERNAL;
}

bool scheme_obj_is_unused(scheme_obj *o) {
    return o->mark == UNUSED;
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

    o->value.env.names = names ? names : scheme_obj_nil();
    o->value.env.values = values ? values : scheme_obj_nil();

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

scheme_obj * scheme_obj_quote(scheme_obj *expr, scheme_context *ctx) {
    scheme_obj *o = scheme_obj_alloc(ctx);
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

    /* FIXME: Memory leak */
    char *s = scheme_make_string(txt, span);

    *next = txt + span;
    
    scheme_obj *o = scheme_obj_symbol(s, ctx);
    scheme_free(s);
    return o;
}

scheme_obj * scheme_read_string(char *txt, char **next,
                                scheme_context *ctx) {
    txt++;
    size_t span = strcspn(txt, "\"");
    char *s = scheme_make_string(txt, span);

    *next = txt + span + 1;

    scheme_obj *o = scheme_obj_string(s, ctx);
    free(s);
    return o;
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
    return scheme_obj_quote(expr, ctx);
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
    scheme_obj *pred = scheme_eval(scheme_car(args), ctx);
    
    scheme_obj *then_clause = scheme_car(scheme_cdr(args));
    scheme_obj *else_clause = scheme_car(scheme_cdr(scheme_cdr(args)));

    scheme_obj *res = scheme_obj_nil();
    
    if (scheme_is_true(pred))
        res = scheme_eval(then_clause, ctx);
    else
        res = scheme_eval(else_clause, ctx);

    scheme_obj_mark(pred, UNUSED);
    return res;
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
    ctx->env_top = scheme_obj_cons((env ?
                                    env :
                                    scheme_env_create(NULL, NULL, ctx)),
                                   ctx->env_top,
                                   ctx);
}

void scheme_context_pop_env(scheme_context *ctx) {
    scheme_obj *env = ctx->env_top;
    scheme_obj_mark(env, UNUSED);
    ctx->env_top = scheme_cdr(env);
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
        scheme_obj_mark(proc, UNUSED);
        scheme_obj_mark(args, UNUSED);
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
