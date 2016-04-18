#include <stdlib.h>
#include <assert.h>

enum scheme_obj_type {SYMBOL, STRING, NUM, CONS};

typedef struct scheme_obj {
    enum scheme_obj_type type;
} scheme_obj;

void * scheme_alloc(size_t size) {
    return malloc(size);
}

void scheme_free(void *mem) {
    free(mem);
}

scheme_obj * scheme_obj_create(int val) {
    scheme_obj * o = scheme_alloc(sizeof(val));
    return o;
}

void scheme_obj_delete(scheme_obj *o) {
    scheme_free(o);
}

scheme_obj * scheme_read(const char *code) {
    return scheme_obj_create(0);
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
    default:
        assert(0);
    }
    return NULL;
}
