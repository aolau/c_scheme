#ifndef _SCHEME_H_
#define _SCHEME_H_

typedef struct scheme_obj scheme_obj;


void * scheme_alloc(size_t size);

void scheme_free(void *mem);

scheme_obj * scheme_obj_create(int val);

void scheme_obj_delete(scheme_obj *o);

scheme_obj * scheme_read(const char *code);

char * scheme_print(scheme_obj *o);

scheme_obj * scheme_eval(scheme_obj *expr);

#endif /* _SCHEME_H_ */
