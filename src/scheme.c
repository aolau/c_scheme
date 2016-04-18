 #include "scheme.h"

#include <stdlib.h>
#include <assert.h>

void * scheme_alloc(size_t size) {
    return malloc(size);
}

void scheme_free(void *mem) {
    free(mem);
}

typedef struct scheme_context {

} scheme_context;

scheme_context * scheme_context_create() {
    scheme_context *c = scheme_alloc(sizeof(scheme_context));
    return c;
}

void scheme_context_delete(scheme_context *c) {
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
        char * str;
        double num;
        cons con;
    } value;
} scheme_obj;

double scheme_obj_as_num(scheme_obj *o) {
    assert(o->type == NUM);
    
    return o->value.num;
}

scheme_obj * scheme_obj_num(double num) {
    scheme_obj * o = scheme_alloc(sizeof(scheme_obj));
    o->type = NUM;
    o->value.num = num;
    return o;
}

void scheme_obj_delete(scheme_obj *o) {
    scheme_free(o);
}

scheme_obj * scheme_read(const char *code) {
    return scheme_obj_num(0.0);
}

char * scheme_print(scheme_obj *o) {
    return "";
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
    case LAMBDA:
        break;
    default:
        assert(0);
    }
    return NULL;
}

