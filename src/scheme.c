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

struct cons;

typedef struct cons {
    scheme_obj *car;
    struct cons *cdr;
} cons;

typedef struct lambda {
    
} lambda;

enum scheme_obj_type {SYMBOL, STRING, NUM, CONS, LAMBDA};

typedef struct scheme_obj {
    enum scheme_obj_type type;
    union {
        const char *str;
        long int num;
        cons con;
    } value;
} scheme_obj;

long int scheme_obj_as_num(scheme_obj *o) {
    assert(o->type == NUM);
    return o->value.num;
}

scheme_obj * scheme_obj_num(long int num) {
    scheme_obj *o = scheme_alloc(sizeof(scheme_obj));
    o->type = NUM;
    o->value.num = num;
    return o;
}

const char * scheme_obj_as_string(scheme_obj *o) {
    assert(o->type == STRING || o->type == SYMBOL);
    return o->value.str;
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

scheme_obj * scheme_read_num(char *txt, char **next_char) {
    const long int num = strtol(txt, next_char, 10);
    return scheme_obj_num(num);
}

char * scheme_make_string(const char *data, int len) {
    char *s = scheme_alloc(sizeof(char) * len + 1);
    memcpy(s, data, len);
    s[len] = '\0';
    return s;
}

scheme_obj * scheme_read_symbol(char *txt, char **next_char) {
    size_t span = strcspn(txt, " )");
    char *s = scheme_make_string(txt, span);

    *next_char = txt + span;
    
    return scheme_obj_symbol(s);
}

scheme_obj * scheme_read_string(char *txt, char **next_char) {
    txt = txt + 1;
    size_t span = strcspn(txt, "\"");
    char *s = scheme_make_string(txt, span);

    *next_char = txt + span;

    return scheme_obj_string(s);
}

scheme_obj * scheme_read(char *txt) {
    txt = scheme_eat_space(txt);
    scheme_obj *obj = NULL;
    char *next = "";
    
    char next_char = scheme_peek(txt);
    if (next_char == '(') {
        /* read list */
    } else if (next_char == '\"') {
        obj = scheme_read_string(txt, &next);
    } else if (scheme_is_digit(next_char)) {
        obj = scheme_read_num(txt, &next);
    } else {
        obj = scheme_read_symbol(txt, &next);
    }

    txt = next;
    return obj;
}

char * scheme_print_num(scheme_obj *o, char *buf) {
    int written = sprintf(buf, "%ld", o->value.num);
    return buf + written;
}

char * scheme_print_to_buf(scheme_obj *obj, char *buf) {
    char *next = buf;
    
    switch (obj->type) {
    case SYMBOL:
        break;
    case STRING:
        break;
    case NUM:
        next = scheme_print_num(obj, buf);
        break;
    case CONS:
        break;
    default:
        assert(0);
    }
    return next;
}

char * scheme_print(scheme_obj *obj) {
    static char buf[256];
    char * next = scheme_print_to_buf(obj, buf);
    *next = '\0';
    return buf;
}

scheme_obj * scheme_eval(scheme_obj *expr) {
    switch (expr->type) {
    case SYMBOL:
        break;
    case STRING:
        break;
    case NUM:
        break;
    case CONS:
        break;
    default:
        assert(0);
    }
    return NULL;
}

