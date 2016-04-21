#include "scheme.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

static void * scheme_alloc(size_t size) {
    return malloc(size);
}

static void scheme_free(void *mem) {
    free(mem);
}

typedef struct scheme_context {

} scheme_context;

static scheme_context * scheme_context_create() {
    scheme_context *c = scheme_alloc(sizeof(scheme_context));
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

struct scheme_cons;

typedef struct scheme_cons {
    scheme_obj *car;
    scheme_obj *cdr;
} scheme_cons;

typedef struct lambda {
    
} lambda;

enum scheme_obj_type {SYMBOL, STRING, NUM, CONS, QUOTE};

typedef struct scheme_obj {
    enum scheme_obj_type type;
    union {
        const char *str;
        long int num;
        scheme_cons *con;
        scheme_obj *expr;
    } value;
} scheme_obj;

scheme_obj * scheme_obj_nil() {
    scheme_obj *o = scheme_alloc(sizeof(scheme_obj));
    o->type = CONS;
    o->value.con = NULL;
    
    return o;
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
    assert(o->type == NUM);
    return o->value.num;
}

const char * scheme_obj_as_string(scheme_obj *o) {
    assert(o->type == STRING || o->type == SYMBOL);
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
    assert(*next != NULL);
    
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
    assert(o->type == SYMBOL);
    
    const char *str = o->value.str;
    const size_t len = strlen(str);
    memcpy(buf, str, len);

    return buf + len;
}

char * scheme_print_string(scheme_obj *o, char *buf) {
    assert(o->type == STRING);

    int len = sprintf(buf, "\"%s\"", o->value.str);
    return buf + len;
}

char * scheme_print_quote(scheme_obj *o, char *buf) {
    sprintf(buf++, "\'");
    return scheme_print_obj(o->value.expr, buf);
}

bool scheme_obj_is_nil(scheme_obj *o) {
    return o->type == CONS && o->value.con == NULL;
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
        assert(0);
    }
    return next;
}

char * scheme_print(scheme_obj *obj) {
    static char buf[256];
    char * next = scheme_print_obj(obj, buf);
    *next = '\0';
    return buf;
}

scheme_obj * scheme_eval_quote(scheme_obj *o) {
    return scheme_eval(o->value.expr);
}

scheme_obj * scheme_eval(scheme_obj *expr) {
    scheme_obj *res = scheme_obj_nil();
    
    switch (expr->type) {
    case SYMBOL:
    case STRING:
    case NUM:
        res = expr;
        break;
    case CONS:
        break;
    case QUOTE:
        res = scheme_eval_quote(expr);
        break;
    default:
        assert(0);
    }
    return res;
}
