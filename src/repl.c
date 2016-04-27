#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "scheme.h"

int main() {
    printf("-- Welcome to lsp! --\n\n");
    
    scheme_context *ctx = scheme_init();
    
    bool run = true;
    
    while (run) {
        char * line = readline("\nlsp> ");
        add_history(line);
        
        scheme_obj *o = scheme_read(line, ctx);
        scheme_obj *eo = scheme_eval(o, ctx);

        char *res = scheme_print(eo);
        printf(": %s\n", res);
        
        scheme_obj_mark(o, UNUSED);
        scheme_obj_mark(eo, UNUSED);

        if (strcmp(res, "quit") == 0) {
            run = false;
        }
    }
    
    scheme_shutdown(ctx);
    return 1;
}
